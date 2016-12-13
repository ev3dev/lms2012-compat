/*
 * LEGO® MINDSTORMS EV3
 *
 * Copyright (C) 2010-2013 The LEGO Group
 * Copyright (C) 2016 David Lechner <david@lechnology.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

/*! \page SoundLibrary Sound Library
 *
 *- \subpage  SoundLibraryDescription
 *- \subpage  SoundLibraryCodes
 */

/*! \page SoundLibraryDescription Description
 *
 *
 */

/*! \page SoundLibraryCodes Byte Code Summary
 *
 *
 */

#include "lms2012.h"
#include "c_sound.h"

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdbool.h>

#include <asoundlib.h>
#include <linux/input.h>

#ifdef    DEBUG_C_SOUND
#define   DEBUG
#endif

#define DEFAULT_SOUND_CARD          "default"
#define SOUND_FILE_BUFFER_SIZE      128

#define SCALE_VOLUME(vol,min,max)   ((vol) * ((max) - (min)) / 100 + (min))

typedef enum {
    SOUND_STOPPED,
    SOUND_FILE_PLAYING,
    SOUND_FILE_LOOPING,
    SOUND_TONE_PLAYING,
} SOUND_STATES;

typedef struct {
    bool no_sound;
    int hSoundFile;
    int event_fd;
    snd_pcm_t *pcm;
    snd_mixer_t *mixer;
    snd_mixer_elem_t *pcm_mixer_elem;
    snd_mixer_elem_t *tone_mixer_elem;
    long save_pcm_volume;
    long save_tone_volume;

    DATA8 SoundOwner;
    SOUND_STATES cSoundState;
    ULONG ToneStartTime;
    UWORD ToneDuration;
    UWORD SoundFileFormat;
    UWORD SoundDataLength;
    UWORD SoundSampleRate;
    UWORD SoundPlayMode;
    SWORD ValPrev;
    SWORD Index;
    SWORD Step;
    size_t BytesToWrite;
    char PathBuffer[MAX_FILENAME_SIZE];
    UBYTE SoundData[SOUND_FILE_BUFFER_SIZE];
} SOUND_GLOBALS;

SOUND_GLOBALS SoundInstance;

// TODO: this function is duplicated from cUiButtonOpenFile - it can be shared
static int cSoundOpenEventFile(void)
{
    struct udev_enumerate *enumerate;
    struct udev_list_entry *list;
    int file = -1;

    enumerate = udev_enumerate_new(VMInstance.udev);
    udev_enumerate_add_match_subsystem(enumerate, "input");
    udev_enumerate_add_match_sysname(enumerate, "input*");
    // We are looking for a device that has SND_BELL and SND_TONE
    udev_enumerate_add_match_property(enumerate, "SND", "6");
    udev_enumerate_scan_devices(enumerate);
    list = udev_enumerate_get_list_entry(enumerate);
    if (list == NULL) {
        fprintf(stderr, "Failed to get sound input device\n");
    } else {
        // just taking the first match in the list
        const char *path = udev_list_entry_get_name(list);
        struct udev_device *input_device;

        input_device = udev_device_new_from_syspath(VMInstance.udev, path);
        udev_enumerate_unref(enumerate);
        enumerate = udev_enumerate_new(VMInstance.udev);
        udev_enumerate_add_match_subsystem(enumerate, "input");
        udev_enumerate_add_match_sysname(enumerate, "event*");
        udev_enumerate_add_match_parent(enumerate, input_device);
        udev_enumerate_scan_devices(enumerate);
        list = udev_enumerate_get_list_entry(enumerate);
        if (list == NULL) {
            fprintf(stderr, "Failed to get sound event device\n");
        } else {
            // again, there should only be one match
            struct udev_device *event_device;

            path = udev_list_entry_get_name(list);
            event_device = udev_device_new_from_syspath(VMInstance.udev, path);

            path = udev_device_get_devnode(event_device);
            file = open(path, O_WRONLY);
            if (file == -1) {
                fprintf(stderr, "Could not open %s: %s\n", path, strerror(errno));
            }

            udev_device_unref(event_device);
        }
        udev_device_unref(input_device);
    }
    udev_enumerate_unref(enumerate);

    return file;
}

/*
 * cSoundPlayTone:
 *
 * Plays a tone using the Linux input event device.
 *
 * Specifying a frequency of 0 stops tone
 *
 * returns -1 if there was an error
 */
static int cSoundPlayTone(DATA16 frequency)
{
    struct input_event event = {
        .time   = { 0 },
        .type   = EV_SND,
        .code   = SND_TONE,
        .value  = frequency,
    };

    return write(SoundInstance.event_fd, &event, sizeof(event));
}

/*
 * cSoundIsTonePlaying:
 *
 * returns a non-zero value if a tone is playing
 */
static int cSoundIsTonePlaying(void)
{
    unsigned char status = 0;

    ioctl(SoundInstance.event_fd, EVIOCGSND(sizeof(status)), &status);

    return status;
}

static void cSoundGetMixer()
{
    snd_mixer_t *mixer;
    snd_mixer_elem_t *element;
    const char *name;
    int err;
    int card = -1;

    err = snd_card_next(&card);
    if (err < 0) {
        fprintf(stderr, "Failed to get sound card: %s\n", snd_strerror(err));
        return;
    }

    if (card == -1) {
        fprintf(stderr, "No sound card available\n");
        SoundInstance.no_sound = TRUE;
        return;
    }

    err = snd_mixer_open(&mixer, 0);
    if (err <  0) {
        fprintf(stderr, "Failed to open mixer: %s\n", snd_strerror(err));
        return;
    }
    err = snd_mixer_attach(mixer, DEFAULT_SOUND_CARD);
    if (err < 0) {
        fprintf(stderr, "Failed to attach mixer: %s\n", snd_strerror(err));
        goto err1;
    }
    err = snd_mixer_selem_register(mixer, NULL, NULL);
    if (err < 0) {
        fprintf(stderr, "Failed to register mixer: %s\n", snd_strerror(err));
        goto err1;
    }
    err = snd_mixer_load(mixer);
    if (err < 0) {
        fprintf(stderr, "Failed to load mixer: %s\n", snd_strerror(err));
        goto err1;
    }
    for (element = snd_mixer_first_elem(mixer); element != NULL;
         element = snd_mixer_elem_next(element))
    {
        name = snd_mixer_selem_get_name(element);
        if (strcmp(name, "PCM") == 0) {
            SoundInstance.pcm_mixer_elem = element;
            snd_mixer_selem_get_playback_volume(element, SND_MIXER_SCHN_MONO,
                                                &SoundInstance.save_pcm_volume);
        } else if (strcmp(name, "Beep") == 0) {
            SoundInstance.tone_mixer_elem = element;
            snd_mixer_selem_get_playback_volume(element, SND_MIXER_SCHN_MONO,
                                                &SoundInstance.save_tone_volume);
        }
    }

    if (!SoundInstance.pcm_mixer_elem) {
        fprintf(stderr, "Could not find 'PCM' volume control\n");
    }
    if (!SoundInstance.tone_mixer_elem) {
        fprintf(stderr, "Could not find 'Beep' volume control\n");
    }

    if (SoundInstance.pcm_mixer_elem || SoundInstance.tone_mixer_elem) {
        // We only need to hang on to the mixer if we found one of the elements
        // that we were looking for.
        SoundInstance.mixer = mixer;
        return;
    }

err1:
    snd_mixer_free(mixer);
}

static snd_pcm_t *cSoundGetPcm(void)
{
    snd_pcm_t *pcm;
    snd_pcm_format_t format;
    int err;

    if (SoundInstance.no_sound) {
        return NULL;
    }

    err = snd_pcm_open(&pcm, DEFAULT_SOUND_CARD, SND_PCM_STREAM_PLAYBACK, 0);
    if (err < 0) {
        fprintf(stderr, "Failed to open PCM playback: %s\n", snd_strerror(err));
        return NULL;
    }

    format = (SoundInstance.SoundFileFormat == SOUND_FILEFORMAT_ADPCM)
        ? SND_PCM_FORMAT_IMA_ADPCM : SND_PCM_FORMAT_U8;
    err = snd_pcm_set_params(pcm, format, SND_PCM_ACCESS_RW_INTERLEAVED,
                             1, SoundInstance.SoundSampleRate, 1, 500000);
    if (err < 0) {
        fprintf(stderr, "Failed to set PCM hwparams: %s\n", snd_strerror(err));
        goto err1;
    }

    return pcm;

err1:
    snd_pcm_close(pcm);

    return NULL;
}

RESULT cSoundInit(void)
{
    SoundInstance.event_fd = cSoundOpenEventFile();
    cSoundGetMixer();
    SoundInstance.hSoundFile = -1;

    return OK;
}

RESULT cSoundOpen(void)
{
    return OK;
}

RESULT cSoundClose(void)
{
    SoundInstance.cSoundState = SOUND_STOPPED;

    cSoundPlayTone(0);

    if (SoundInstance.pcm) {
        snd_pcm_close(SoundInstance.pcm);
        SoundInstance.pcm = NULL;
    }

    if (SoundInstance.hSoundFile >= 0) {
        close(SoundInstance.hSoundFile);
        SoundInstance.hSoundFile = -1;
    }

    return OK;
}

RESULT cSoundUpdate(void)
{
    int     BytesRead;
    UWORD   BytesToRead;
    // UBYTE   BytesWritten = 0;
    RESULT  Result = FAIL;

    switch(SoundInstance.cSoundState) {
    case SOUND_STOPPED:
        // Do nothing
        break;

    case SOUND_FILE_LOOPING:
    case SOUND_FILE_PLAYING:
        if (SoundInstance.BytesToWrite > 0) {
            if (SoundInstance.BytesToWrite > SOUND_FILE_BUFFER_SIZE) {
                BytesToRead = SOUND_FILE_BUFFER_SIZE;
            } else {
                BytesToRead = SoundInstance.BytesToWrite;
            }

            if (SoundInstance.hSoundFile >= 0) {
                int frames;

                // TODO: convert bytes to frames and vice versa in case of ADPCM

                frames = snd_pcm_avail(SoundInstance.pcm);
                if (frames < 0) {
                    snd_pcm_recover(SoundInstance.pcm, frames, 1);
                    break;
                }
                if (frames <= BytesToRead) {
                    // if there not enough room, wait for the next loop
                    Result = OK;
                    break;
                }

                BytesRead = read(SoundInstance.hSoundFile,
                                 SoundInstance.SoundData, BytesToRead);
                frames = snd_pcm_writei(SoundInstance.pcm,
                                        SoundInstance.SoundData,
                                        BytesRead);
                if (frames < 0) {
                    frames = snd_pcm_recover(SoundInstance.pcm, frames, 0);
                }
                if (frames < 0) {
                    fprintf(stderr, "Failed to write sound data to PCM: %s\n",
                            snd_strerror(frames));
                    cSoundClose();
                    // Result != OK
                    break;
                } else {
                    SoundInstance.BytesToWrite -= BytesRead;
                }
            }
        } else {
            // We have finished writing the file, so if we are looping, start
            // over, otherwise wait for the sound to finish playing.

            if (SoundInstance.cSoundState == SOUND_FILE_LOOPING) {
                lseek(SoundInstance.hSoundFile, 8, SEEK_SET);
                SoundInstance.BytesToWrite = SoundInstance.SoundDataLength;
            } else {
                snd_pcm_state_t state = snd_pcm_state(SoundInstance.pcm);

                if (state != SND_PCM_STATE_RUNNING) {
                    cSoundClose();
                }
            }
        }

        Result = OK;

        break;

    case SOUND_TONE_PLAYING:
        if (cSoundIsTonePlaying()) {
            ULONG elapsed = VMInstance.NewTime - SoundInstance.ToneStartTime;

            if (elapsed >= SoundInstance.ToneDuration) {
                // stop the tone
                cSoundPlayTone(0);
            } else {
                // keep playing
                break;
            }
        }
        SoundInstance.cSoundState = SOUND_STOPPED;
        Result = OK;

        break;
    }

    return Result;
}

RESULT cSoundExit(void)
{
    // restore the system volume levels

    if (SoundInstance.pcm_mixer_elem) {
        snd_mixer_selem_set_playback_volume_all(SoundInstance.pcm_mixer_elem,
                                                SoundInstance.save_pcm_volume);
    }
    if (SoundInstance.tone_mixer_elem) {
        snd_mixer_selem_set_playback_volume_all(SoundInstance.tone_mixer_elem,
                                                SoundInstance.save_tone_volume);
    }
    if (SoundInstance.mixer) {
        snd_mixer_free(SoundInstance.mixer);
    }

    return OK;
}

//******* BYTE CODE SNIPPETS **************************************************

/*! \page cSound Sound
 *  <hr size="1"/>
 *  <b>     opSOUND ()  </b>
 *
 *- Memory file entry\n
 *- Dispatch status unchanged
 *
 *  \param  (DATA8)   CMD     - Specific command \ref soundsubcode
 *
 *  - CMD = BREAK\n
 *
 *\n
 *  - CMD = TONE
 *    -  \param  (DATA8)    VOLUME    - Volume [0..100]\n
 *    -  \param  (DATA16)   FREQUENCY - Frequency [Hz]\n
 *    -  \param  (DATA16)   DURATION  - Duration [mS]\n
 *
 *\n
 *  - CMD = PLAY
 *    -  \param  (DATA8)    VOLUME    - Volume [0..100]\n
 *    -  \param  (DATA8)    NAME      - First character in filename (character string)\n
 *
 *\n
 *  - CMD = REPEAT
 *    -  \param  (DATA8)    VOLUME    - Volume [0..100]\n
 *    -  \param  (DATA8)    NAME      - First character in filename (character string)\n
 *
 *\n
 *
 */
/*! \brief  opSOUND byte code
 *
 */
void cSoundEntry(void)
{
    int     Cmd;
    UWORD   Volume;
    UWORD   Frequency;
    UWORD   Duration;
    DATA8   *pFileName;
    char    PathName[MAX_FILENAME_SIZE];
    UBYTE   Tmp1;
    UBYTE   Tmp2;
    UBYTE   Loop = FALSE;

    Cmd = *(DATA8*)PrimParPointer();

    switch(Cmd) {
    case scTONE:
        cSoundClose();

        SoundInstance.SoundOwner = CallingObjectId();

        Volume = *(DATA8*)PrimParPointer();
        Frequency = *(DATA16*)PrimParPointer();
        Duration = *(DATA16*)PrimParPointer();

        if (SoundInstance.tone_mixer_elem) {
            long min, max;

            snd_mixer_selem_get_playback_volume_range(SoundInstance.tone_mixer_elem,
                                                      &min, &max);
            snd_mixer_selem_set_playback_volume_all(SoundInstance.tone_mixer_elem,
                                                    SCALE_VOLUME(Volume, min, max));
        }

        SoundInstance.ToneStartTime = VMInstance.NewTime;
        SoundInstance.ToneDuration = Duration;
        if (cSoundPlayTone(Frequency) != -1) {
            SoundInstance.cSoundState = SOUND_TONE_PLAYING;
        }

        break;

    case scBREAK:
        cSoundClose();

        break;

    case scREPEAT:
        Loop = TRUE;
        // Fall through
    case scPLAY:
        cSoundClose();

        SoundInstance.SoundOwner = CallingObjectId();

        Volume = *(DATA8*)PrimParPointer();
        pFileName = (DATA8*)PrimParPointer();

        if (SoundInstance.pcm_mixer_elem) {
            long min, max;

            snd_mixer_selem_get_playback_volume_range(SoundInstance.pcm_mixer_elem,
                                                      &min, &max);
            snd_mixer_selem_set_playback_volume_all(SoundInstance.pcm_mixer_elem,
                                                    SCALE_VOLUME(Volume, min, max));
        }

        if (pFileName != NULL) {
            // Get Path and concatenate

            PathName[0] = 0;
            if (pFileName[0] != '.') {
                GetResourcePath(PathName, MAX_FILENAME_SIZE);
                sprintf(SoundInstance.PathBuffer, "%s%s.rsf", (char*)PathName,
                        (char*)pFileName);
            } else {
                sprintf(SoundInstance.PathBuffer, "%s.rsf", (char*)pFileName);
            }

            // Open SoundFile

            SoundInstance.hSoundFile = open(SoundInstance.PathBuffer, O_RDONLY);

            if (SoundInstance.hSoundFile >= 0) {
                // BIG Endianess

                read(SoundInstance.hSoundFile, &Tmp1, 1);
                read(SoundInstance.hSoundFile, &Tmp2, 1);
                SoundInstance.SoundFileFormat = (UWORD)Tmp1 << 8 | (UWORD)Tmp2;

                read(SoundInstance.hSoundFile, &Tmp1, 1);
                read(SoundInstance.hSoundFile, &Tmp2, 1);
                SoundInstance.SoundDataLength = (UWORD)Tmp1 << 8 | (UWORD)Tmp2;

                read(SoundInstance.hSoundFile, &Tmp1, 1);
                read(SoundInstance.hSoundFile, &Tmp2, 1);
                SoundInstance.SoundSampleRate = (UWORD)Tmp1 << 8 | (UWORD)Tmp2;

                read(SoundInstance.hSoundFile, &Tmp1, 1);
                read(SoundInstance.hSoundFile, &Tmp2, 1);
                SoundInstance.SoundPlayMode = (UWORD)Tmp1 << 8 | (UWORD)Tmp2;

                SoundInstance.pcm = cSoundGetPcm();
                if (SoundInstance.pcm) {
                    SoundInstance.cSoundState = Loop ? SOUND_FILE_LOOPING
                                                     : SOUND_FILE_PLAYING;
                    SoundInstance.BytesToWrite = SoundInstance.SoundDataLength;
                }
            } else {
                fprintf(stderr, "Failed to open sound file '%s': %s\n",
                        SoundInstance.PathBuffer, strerror(errno));
            }
        } else {
            fprintf(stderr, "Sound file name was NULL\n");
        }
        break;
    }
}

/*! \page   cSound
 *  <hr size="1"/>
 *  <b>     opSOUND_TEST (BUSY) </b>
 *
 *- Test if sound busy (playing file or tone\n
 *- Dispatch status unchanged
 *
 *  \return  (DATA8)   BUSY    - Sound busy flag (0 = ready, 1 = busy)
 *
 */
/*! \brief  opSOUND_TEST byte code
 *
 */

void cSoundTest(void)
{
    DATA8 busy = 0;

    if (SoundInstance.pcm || cSoundIsTonePlaying()) {
        busy = 1;
    }

    *(DATA8*)PrimParPointer() = busy;
}

/*! \page   cSound
 *  <hr size="1"/>
 *  <b>     opSOUND_READY () </b>
 *
 *- Wait for sound ready (wait until sound finished)\n
 *- Dispatch status can change to BUSYBREAK
 *
 */
/*! \brief  opSOUND_READY byte code
 *
 */

void cSoundReady(void)
{
    IP      TmpIp;
    DSPSTAT DspStat = NOBREAK;

    TmpIp = GetObjectIp();

    if (SoundInstance.pcm || cSoundIsTonePlaying()) {
        // Rewind IP and set status
        DspStat = BUSYBREAK; // break the interpreter and waits busy
        SetDispatchStatus(DspStat);
        SetObjectIp(TmpIp - 1);
    }
}
