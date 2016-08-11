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

#ifndef C_BT_H_
#define C_BT_H_


#include  "lms2012.h"

#define   MAX_BUNDLE_ID_SIZE            35
#define   MAX_BUNDLE_SEED_ID_SIZE       11

enum
{
  BT_SLAVE_CH0,
  BT_HOST_CH0,
  BT_HOST_CH1,
  BT_HOST_CH2,
  BT_HOST_CH3,
  BT_HOST_CH4,
  BT_HOST_CH5,
  BT_HOST_CH6,
  NO_OF_BT_CHS
};
#define   NUMBER_OF_ATTACHED_SLAVES     (NO_OF_BT_CHS - BT_HOST_CH0)


// Defines related to Device List
enum
{
  DEV_EMPTY       = 0x00,
  DEV_KNOWN       = 0x01,
};

enum
{
  READ_BUF_EMPTY,
  READ_BUF_FULL
};

// Buffer to read into from the socket
typedef struct
{
  UBYTE Buf[1024];
  UWORD InPtr;
  UWORD OutPtr;
  UWORD Status;
}READBUF;


// Buffer to write into from the socket
typedef struct
{
  UBYTE Buf[1024];
  UWORD InPtr;
  UWORD OutPtr;
}WRITEBUF;


// Buffer to fill complete message into from READBUF
// only one Messages can fit into this buffer
typedef struct
{
  UBYTE Buf[1024];
  UWORD InPtr;
  UWORD MsgLen;
  UWORD RemMsgLen;
  UWORD Status;
  UBYTE LargeMsg;
}MSGBUF;

void      BtInit(const char *pName);
void      BtExit(void);
void      BtUpdate(void);
RESULT    cBtConnect(const char *pName);
RESULT    cBtDisconnect(const char *pName);
UBYTE     cBtDiscChNo(UBYTE ChNo);

UWORD     cBtReadCh0(UBYTE *pBuf, UWORD Length);
UWORD     cBtReadCh1(UBYTE *pBuf, UWORD Length);
UWORD     cBtReadCh2(UBYTE *pBuf, UWORD Length);
UWORD     cBtReadCh3(UBYTE *pBuf, UWORD Length);
UWORD     cBtReadCh4(UBYTE *pBuf, UWORD Length);
UWORD     cBtReadCh5(UBYTE *pBuf, UWORD Length);
UWORD     cBtReadCh6(UBYTE *pBuf, UWORD Length);
UWORD     cBtReadCh7(UBYTE *pBuf, UWORD Length);

UWORD     cBtDevWriteBuf0(UBYTE *pBuf, UWORD Size);
UWORD     cBtDevWriteBuf1(UBYTE *pBuf, UWORD Size);
UWORD     cBtDevWriteBuf2(UBYTE *pBuf, UWORD Size);
UWORD     cBtDevWriteBuf3(UBYTE *pBuf, UWORD Size);
UWORD     cBtDevWriteBuf4(UBYTE *pBuf, UWORD Size);
UWORD     cBtDevWriteBuf5(UBYTE *pBuf, UWORD Size);
UWORD     cBtDevWriteBuf6(UBYTE *pBuf, UWORD Size);
UWORD     cBtDevWriteBuf7(UBYTE *pBuf, UWORD Size);

UBYTE     cBtI2cBufReady(void);
UWORD     cBtI2cToBtBuf(UBYTE *pBuf, UWORD Size);

// Generic Bluetooth commands
RESULT    BtSetVisibility(UBYTE State);
UBYTE     BtGetVisibility(void);
RESULT    BtSetOnOff(UBYTE On);
RESULT    BtGetOnOff(UBYTE *On);
RESULT    BtSetMode2(UBYTE Mode2);
RESULT    BtGetMode2(UBYTE *pMode2);
UBYTE     BtStartScan(void);
UBYTE     BtStopScan(void);
UBYTE     cBtGetNoOfConnListEntries(void);
RESULT    cBtGetConnListEntry(UBYTE Item, char *pName, SBYTE Length, UBYTE* pType);
UBYTE     cBtGetNoOfDevListEntries(void);
RESULT    cBtGetDevListEntry(UBYTE Item, SBYTE *pConnected, SBYTE *pType, char *pName, SBYTE Length);
RESULT    cBtRemoveItem(const char *pName);
UBYTE     cBtGetNoOfSearchListEntries(void);
RESULT    cBtGetSearchListEntry(UBYTE Item, SBYTE *pConnected, SBYTE *pType, SBYTE *pPaired, char *pName, SBYTE Length);
RESULT    cBtGetHciBusyFlag(void);
void      DecodeMode1(UBYTE BufNo);

UBYTE     cBtGetStatus(void);
void      cBtGetId(UBYTE *pId, UBYTE Length);
RESULT    cBtSetName(const char *pName, UBYTE Length);
UBYTE     cBtGetChNo(UBYTE *pName, UBYTE *pChNo);
void      cBtGetIncoming(char *pName, UBYTE *pCod, UBYTE Len);
COM_EVENT cBtGetEvent(void);
RESULT    cBtSetPin(const char *pPin);
RESULT    cBtSetPasskey(UBYTE Accept);

void      cBtSetTrustedDev(UBYTE *pBtAddr, UBYTE *pPin, UBYTE PinSize);

int       cBtSetBundleId(const char *pId);
int       cBtSetBundleSeedId(const char *pSeedId);


#endif /* C_BT_H_ */
