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

#ifndef LED_H_
#define LED_H_

#include "lms2012.h"

typedef enum {
    USER_LED_LEFT   = 0x01,
    USER_LED_RIGHT  = 0x02,
    USER_LED_RED    = 0x04,
    USER_LED_GREEN  = 0x08,
} cUiLedFlags;

int cUiLedOpenTriggerFile(const char *name);
unsigned int cUiLedCreateUserLed(const char *name, cUiLedFlags flags);
void cUiLedSetState(LEDPATTERN State);

#endif /* LED_H_ */
