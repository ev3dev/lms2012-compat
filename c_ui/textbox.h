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

#ifndef TEXTBOX_H_
#define TEXTBOX_H_

#include "lms2012.h"

void cUiTextboxReadLine(DATA8 *pText, DATA32 Size, DATA8 Del, DATA8 Lng,
                        DATA16 Line, DATA8 *pLine, DATA8 *pFont);
void cUiTextboxAppendLine(DATA8 *pText, DATA32 Size, DATA8 Del, DATA8 *pLine,
                          DATA8 Font);
RESULT cUiTextbox(DATA16 X, DATA16 Y, DATA16 X1, DATA16 Y1, DATA8 *pText,
                  DATA32 Size, DATA8 Del, DATA16 *pLine);

#endif /* TEXTBOX_H_ */

