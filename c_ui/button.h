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

#ifndef BUTTON_H_
#define BUTTON_H_

#include "lms2012.h"

int cUiButtonOpenFile(void);
void cUiButtonClearAll(void);
void cUiUpdateButtons(DATA16 Time);
DATA8 cUiButtonTestShortPress(DATA8 Button);
DATA8 cUiButtonGetShortPress(DATA8 Button);
DATA8 cUiButtonTestLongPress(DATA8 Button);
DATA16 cUiButtonTestHorz(void);
DATA16 cUiButtonGetHorz(void);
DATA16 cUiButtonGetVert(void);

#endif /* BUTTON_H_ */
