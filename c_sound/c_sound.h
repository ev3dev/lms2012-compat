/*
 * LEGO® MINDSTORMS EV3
 *
 * Copyright (C) 2010-2013 The LEGO Group
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

#ifndef C_SOUND_H_
#define C_SOUND_H_

#include  "lms2012.h"
#include  <stdio.h>
#include  <string.h>
#include  <sys/stat.h>

#define STEP_SIZE_TABLE_ENTRIES 89
#define INDEX_TABLE_ENTRIES     16
typedef enum {
  SOUND_STOPPED,
  SOUND_SETUP_FILE,
  SOUND_FILE_PLAYING,
  SOUND_FILE_LOOPING,
  SOUND_TONE_PLAYING,
  SOUND_TONE_LOOPING
} SOUND_STATES;

// Percentage to SoundLevel -
// Adjust the percentage, if non-linear SPL response is needed

#define SND_LEVEL_1   13  // 13% (12.5)
#define SND_LEVEL_2   25  // 25%
#define SND_LEVEL_3   38  // 38% (37.5)
#define SND_LEVEL_4   50  // 50%
#define SND_LEVEL_5   63  // 63% (62.5)
#define SND_LEVEL_6   75  // 75%
#define SND_LEVEL_7   88  // 88% (87.5)

#define TONE_LEVEL_1    8  //  8%
#define TONE_LEVEL_2   16  // 16%
#define TONE_LEVEL_3   24  // 24%
#define TONE_LEVEL_4   32  // 32%
#define TONE_LEVEL_5   40  // 40%
#define TONE_LEVEL_6   48  // 48%
#define TONE_LEVEL_7   56  // 56%
#define TONE_LEVEL_8   64  // 64%
#define TONE_LEVEL_9   72  // 72%
#define TONE_LEVEL_10  80  // 80%
#define TONE_LEVEL_11  88  // 88%
#define TONE_LEVEL_12  96  // 96%

#define FILEFORMAT_RAW_SOUND      0x0100
#define FILEFORMAT_ADPCM_SOUND    0x0101
#define SOUND_MODE_ONCE           0x00
#define SOUND_LOOP                0x01
#define SOUND_ADPCM_INIT_VALPREV  0x7F
#define SOUND_ADPCM_INIT_INDEX    20

typedef struct
{
  //*****************************************************************************
  // Sound Global variables
  //*****************************************************************************

  int     SoundDriverDescriptor;
  int     hSoundFile;

  DATA8   SoundOwner;
  SOUND_STATES  cSoundState;
  SOUND   Sound;
  SOUND	 	*pSound;
  UWORD	  BytesLeft;
  UWORD	  SoundFileFormat;
  UWORD	  SoundDataLength;
  UWORD	  SoundSampleRate;
  UWORD	  SoundPlayMode;
  UWORD   SoundFileLength;
  SWORD   ValPrev;
  SWORD   Index;
  SWORD   Step;
  UBYTE   BytesToWrite;
  char    PathBuffer[MAX_FILENAME_SIZE];
  struct  stat FileStatus;
  UBYTE   SoundData[SOUND_FILE_BUFFER_SIZE + 1]; // Add up for CMD
} SOUND_GLOBALS;

extern SOUND_GLOBALS SoundInstance;

RESULT cSoundInit(void);
RESULT cSoundOpen(void);
RESULT cSoundUpdate(void);
RESULT cSoundClose(void);
RESULT cSoundExit(void);
void cSoundEntry(void);
void cSoundReady(void);
void cSoundTest(void);

#endif /* C_SOUND_H_ */
