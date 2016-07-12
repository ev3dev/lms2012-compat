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

#ifndef POWER_H_
#define POWER_H_

#include "lms2012.h"

// Used when no battery is present
#define DEFAULT_BATTERY_VOLTAGE 9.0

void cUiPowerOpenBatteryFiles(void);
void cUiPowerUpdateCnt(void);
void cUiPowerCheckVoltage(void);
void cUiCheckPower(UWORD Time);
void cUiPowerCheckTemperature(void);
void cUiPowerInitTemperature(void);
void cUiPowerExitTemperature(void);

#endif /* POWER_H_ */

