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

#ifndef C_WIFI_H_
#define C_WIFI_H_
#define _GNU_SOURCE
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <netdb.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/un.h>
#include <errno.h>
#include <bytecodes.h>

#include "lms2012.h"

#define WIFI_PERSISTENT_PATH    vmSETTINGS_DIR          // FileSys guidance ;-)
#define WIFI_PERSISTENT_FILENAME  "WiFiConnections.dat" // Persistent storage for KNOWN connections

#define MAC_ADDRESS_LENGTH    18  // xx:xx:xx:xx:xx:xx + /0x00
#define FREQUENCY_LENGTH      5
#define SIGNAL_LEVEL_LENGTH   4
#define SECURITY_LENGTH       129 //33
#define FRIENDLY_NAME_LENGTH  33

#define PSK_LENGTH            65  // 32 bytes = 256 bit + /0x00
#define KEY_MGMT_LENGTH       33
#define PAIRWISE_LENGTH       33
#define GROUP_LENGTH          33
#define PROTO_LENGTH          33

#define WIFI_INIT_TIMEOUT     10  //60
#define WIFI_INIT_DELAY       10

#define BROADCAST_IP_LOW  "255"   // "192.168.0.255"
#define BROADCAST_PORT    3015    // UDP
#define TCP_PORT 5555
#define BEACON_TIME 5             // 5 sec's between BEACONs

#define TIME_FOR_WIFI_DONGLE_CHECK 10

#define BLUETOOTH_SER_LENGTH  13  // No "white" space separators
#define BRICK_HOSTNAME_LENGTH     (NAME_LENGTH + 1)

// WiFi AP flags "capabilities" and state
typedef enum {
    VISIBLE   = 0x01,
    CONNECTED = 0x02,
    WPA2      = 0x04,
    KNOWN     = 0x08,
    UNKNOWN   = 0x80,
} WIFI_STATE_FLAGS;

// Common Network stuff
// --------------------
RESULT cWiFiGetIpAddr(char* IpAddress);
RESULT cWiFiGetMyMacAddr(char* MacAddress);
RESULT cWiFiTechnologyPresent(void);

// TCP functions
// -------------
UWORD cWiFiWriteTcp(UBYTE* Buffer, UWORD Length);
UWORD cWiFiReadTcp(UBYTE* Buffer, UWORD Length);

// WPA and AP stuff
// ----------------
void cWiFiMoveUpInList(int Index);
void cWiFiMoveDownInList(int Index);
RESULT cWiFiGetName(char *ApName, int Index, char Length);
RESULT cWiFiSetName(char *ApName, int Index);
RESULT cWiFiGetIndexFromName(const char *Name, UBYTE *Index);
void cWiFiSetEncryptToWpa2(int Index);
void cWiFiSetEncryptToNone(int Index);
ENCRYPT cWifiGetEncrypt(int Index);
void cWiFiDeleteAsKnown(int LocalIndex);
WIFI_STATE_FLAGS cWiFiGetFlags(int Index);
RESULT cWiFiConnectToAp(int Index);
RESULT cWiFiMakePsk(char *ApSsid, char *PassPhrase, int Index);
DATA16 cWifiGetListState(void);
int cWiFiGetApListSize(void);
RESULT cWiFiScanForAPs(void);
RESULT cWiFiGetOnStatus(void);

// Common Control
// --------------
RESULT    cWiFiGetStatus(void);
RESULT    cWiFiTurnOn(void);        // TURN ON
RESULT    cWiFiTurnOff(void);       // TURN OFF
RESULT    cWiFiExit(void);
RESULT    cWiFiInit(void);

#endif /* C_WIFI_H_ */
