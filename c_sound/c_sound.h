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

#define SOUND_FILEFORMAT_RAW      0x0100
#define SOUND_FILEFORMAT_ADPCM    0x0101

RESULT cSoundInit(void);
RESULT cSoundOpen(void);
RESULT cSoundUpdate(void);
RESULT cSoundClose(void);
RESULT cSoundExit(void);
void cSoundEntry(void);
void cSoundReady(void);
void cSoundTest(void);

#endif /* C_SOUND_H_ */
