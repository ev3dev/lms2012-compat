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


/*! \page BluetoothPart Bluetooth
 *
 *- \subpage  InternalBtFunctionality
 *
 */


/*! \page InternalBtFunctionality Bluetooth description
 *
 *  <hr size="1"/>
 *
  \verbatim

   Support of up to 7 connection - either 7 outgoing or 1 incoming connection.

   Scatter-net is not supported. If connected from outside (by bluetooth) then
   the  connection is closed if an outgoing connection is made from the brick.

   Pin agent is used to handle pairing.

   DBUS is used to handle connection creation.

   Sockets are used to communicate with the remote devices.


   Bluetooth buffer setup
   ----------------------

   RX side:

   Bluetooth socket
        |
         --> c_bt RxBuf - Fragmented async. data bytes
                 |
                  --> c_bt Msg buffer  - Collected as complete LEGO protocol messages
                             |
                              --> c_com rx buffer - Transferred to c_com level for interpretation


   Tx side:

   c_com tx buffer
         |
          --> c_bt WriteBuf Buffer
                      |
                       -->  Bluetooth socket





   In mode2:
   ---------

   RX:

   Bluetooth socket
         |
          --> Mode2Buf - Fragmented data bytes (c_bt.c)
                  |
                   --> Mode2InBuf - Fragmented data bytes (c_i2c.c)
                            |
                             --> Transfer to Mode2 decoding
                                    |
                                     --> READBUF (return bytes from mode2 decoding) -> for mode1 decoding
                                    |
                                    |
                                     --> WriteBuf (return bytes from mode2 decoding) -> for tx to remote mode2 device


   TX:

   Mode2WriteBuf   (c_bt.c)
       |
        --> Transfer to mode2 decoding  (data read in c_i2c.c)
               |
                --> WriteBuf (return bytes from mode2 decoding) -> for tx  (c_bt.c)
                                            |
                                             -->  Bluetooth socket



   CONNECTION MANAGEMENT
   ---------------------


     CONNECTING:

     Connecting brick:                                        Remote brick:

     Connect to brick (Issued from byte codes)     |
     - Set busy flag                               |
     - Disable page inquiry                        |
     - Open socket                           --->  |  --->    EVT_CONN_REQUEST
                                                   |          - Issue remote name request
                                                   |
     Optional pin/passkey exchange (agent)   <-->  |  <-->    optional pin/passkey exchange (agent)
                                                   |
     EVT_CONN_COMPLETE                       <---  |  --->    EVT_CONN_COMPLETE
     - Disable page inquiry                        |          - Disable page inquiry        (Cannot be connected to more than one, as a slave)
     - Update Device list                          |          - Insert all info in dev list (Except connected)
     - Update Search list                          |
                                                   |
     Socket write ready (Remote socket open) <---  |  --->    Success on accept listen socket (Socket gives remote address)
     - NoOfConnDevices++                           |          - Set slave mode
     - Update Search list to connected             |          - NoOfConnDevices++
     - Update Device list to connected             |          - Update Device list to connected
                                                   |          - Update Search list to connected
                                                   |


     DISCONNECTING:

     Disconnecting brick:                          |          Remote brick:
                                                   |
     Disconnect  (Issued from byte codes)          |
     - Close bluetooth socket                --->  |  --->    Socket indicates remote socket closed
                                                   |          - Close socket
                                                   |
                                                   |
     EVT_DISCONN_COMPLETE                    <---  |  --->    EVT_DISCONN_COMPLETE
     - Update Search list to disconnected          |          - Update Search list to disconnected
     - Update Device list to disconnected          |          - Update Device list to disconnected
     - NoofConnDevices--                           |          - NoofConnDevices--
     - If NoofConnDevices = 0 -> set idle mode     |          - If NoofConnDevices = 0 -> set idle mode

  \endverbatim
  */

#include <glib.h>
#include <glib/gstdio.h>
#include <gio/gunixfdlist.h>

#include <fcntl.h>
#include <string.h>

#include "lms2012.h"
#include "c_com.h"
#include "c_bt.h"
#include "c_i2c.h"
#include "bluez.h"

#ifdef DEBUG_C_BT
#define DEBUG
#endif

#define C_BT_AGENT_PATH         "/org/ev3dev/lms2012/bluez/agent"
#define C_BT_SPP_PROFILE_PATH   "/org/ev3dev/lms2012/bluez/spp_profile"
#define NONVOL_BT_DATA          "settings/nonvolbt"
#define MAX_DEV_TABLE_ENTRIES   30
#define MAX_BT_NAME_SIZE        248

#define SPP_UUID                "00001101-0000-1000-8000-00805f9b34fb"
#define SVC_UUID                "00000000-deca-fade-deca-deafdecacaff"

enum {
    I_AM_IN_IDLE    = 0,
    I_AM_MASTER     = 1,
    I_AM_SLAVE      = 2,
    I_AM_SCANNING   = 3,
    STOP_SCANNING   = 4,
    TURN_ON         = 5,
    TURN_OFF        = 6,
    RESTART         = 7,
    BLUETOOTH_OFF   = 8
};

enum {
    MSG_BUF_EMPTY   = 0,
    MSG_BUF_LEN     = 1,
    MSG_BUF_BODY    = 2,
    MSG_BUF_FULL    = 3
};

// Defines related to Channels
enum {
    CH_CONNECTING,
    CH_FREE,
    CH_CONNECTED
};

/* Constants related to Decode mode */
enum {
    MODE1,      // normal
    MODE2,      // iPhone/iPad/iPod
};

// Communication sockets
typedef struct {
    SLONG     Socket;
    // bdaddr_t  Addr;
    struct    timeval     Cmdtv;
    fd_set    Cmdfds;
} BTSOCKET;

typedef struct {
    WRITEBUF  WriteBuf;
    READBUF   ReadBuf;
    MSGBUF    MsgBuf;
    BTSOCKET  BtSocket;
    UBYTE     Status;
} BTCHANNEL;

// This is serialized. Don't change unless absolutely necessary.
typedef struct {
    UBYTE       DecodeMode;
    char        BundleID[MAX_BUNDLE_ID_SIZE];
    char        BundleSeedID[MAX_BUNDLE_SEED_ID_SIZE];
} NONVOLBT;

typedef struct {
  UBYTE     ChNo;
}OUTGOING;

typedef struct {
    BTCHANNEL     BtCh[NO_OF_BT_CHS];   // Communication sockets
    READBUF       Mode2Buf;
    WRITEBUF      Mode2WriteBuf;

    OUTGOING      OutGoing;
    char          Adr[13];
    UBYTE         SearchIndex;
    UBYTE         NoOfFoundDev;
    UBYTE         PageState;
    UBYTE         NoOfConnDevs;

    SLONG         State;
    SLONG         OldState;
    ULONG         Delay;
    NONVOLBT      NonVol;
    COM_EVENT     Events;
    UBYTE         DecodeMode;
    char          BtName[vmBRICKNAMESIZE];

    gboolean ConnectBusy;
    gboolean DisconnectBusy;
    gboolean Fail;

    GDBusObjectManager      *object_manager;
    BluezAdapter1           *adapter;
    BluezAgentManager1      *agent_manager;
    BluezAgent1             *agent;
    BluezProfileManager1    *profile_manager;
    BluezProfile1           *spp_profile;
    BluezDevice1            *incoming_device;

    GList *unpaired_list;   // list of unpaired devices
    GList *paired_list;     // list of paired devices
    GList *connected_list;  // list of connected devices

    GIOChannel *channel0;
    guint channel0_source_id;

    GDBusMethodInvocation *request_pin_invocation;
    GDBusMethodInvocation *request_confirmation_invocation;
} BT_GLOBALS;

static BT_GLOBALS BtInstance;


static void BtCloseBtSocket(SLONG *pBtSocket)
{
  if (MIN_HANDLE <= *pBtSocket)
  {
    close(*pBtSocket);
    *pBtSocket = -1;
  }
}

static void BtCloseCh(UBYTE ChIndex)
{

  if (CH_CONNECTING == BtInstance.BtCh[ChIndex].Status)
  {
    if ((0 == BtInstance.NoOfConnDevs) && ((I_AM_MASTER == BtInstance.State) || (I_AM_SLAVE == BtInstance.State)))
	  {
	    // BtSetup(I_AM_IN_IDLE);
	  }
  }
  else
  {
    if (CH_CONNECTED == BtInstance.BtCh[ChIndex].Status)
    {
      if (0 < BtInstance.NoOfConnDevs)
      {
        BtInstance.NoOfConnDevs--;
        if (0 == BtInstance.NoOfConnDevs)
        {
          // BtSetup(I_AM_IN_IDLE);
        }
      }
      else
      {
        if ((I_AM_MASTER == BtInstance.State) || (I_AM_SLAVE == BtInstance.State))
        {
          //Ensure going back to idle if only pairing (no application running on remote device)
          // BtSetup(I_AM_IN_IDLE);
        }
      }
    }
  }
  BtInstance.BtCh[ChIndex].Status = CH_FREE;
  BtCloseBtSocket(&(BtInstance.BtCh[ChIndex].BtSocket.Socket));
}

static void BtDisconnectAll(void)
{
    GList *device = BtInstance.connected_list;
    UBYTE Tmp;

    for (; device != NULL; device = g_list_next(device)) {
        bluez_device1_call_disconnect(BLUEZ_DEVICE1(device->data), NULL, NULL, NULL);
    }

    for (Tmp = 0; Tmp < NO_OF_BT_CHS; Tmp++) {
        if (CH_FREE != BtInstance.BtCh[Tmp].Status) {
            BtCloseCh(Tmp);
        }
    }
}

/**
 * @brief               Start scanning for bluetooth devices.
 *
 * @return              OK on success, otherwise FAIL.
 */
UBYTE BtStartScan(void)
{
    GError *error = NULL;

    if (!BtInstance.adapter) {
        return FAIL;
    }

    if (!bluez_adapter1_call_start_discovery_sync(BtInstance.adapter, NULL, &error)) {
        g_warning("Failed to start bluetooth scan: %s", error->message);
        g_clear_error(&error);
        return FAIL;
    }

    return OK;
}

/**
 * @brief               Stop scanning for bluetooth devices.
 *
 * @return              OK on success, otherwise FAIL.
 */
UBYTE BtStopScan(void)
{
    GError *error = NULL;

    if (!BtInstance.adapter) {
        return FAIL;
    }

    if (!bluez_adapter1_call_stop_discovery_sync(BtInstance.adapter, NULL, &error)) {
        g_warning("Failed to stop bluetooth scan: %s", error->message);
        g_clear_error(&error);
        return FAIL;
    }

    return OK;
}

static UWORD cBtRead(DATA8 ch, UBYTE *pBuf, UWORD Length)
{
    MSGBUF *pMsgBuf;
    UWORD RtnLen = 0;

    pMsgBuf = &BtInstance.BtCh[ch].MsgBuf;

    if (MSG_BUF_FULL == pMsgBuf->Status) {
#ifdef DEBUG
        printf("MSG_BUF_FULL on Bt Channel %d, number of bytes = %d\n",
               ch, pMsgBuf->InPtr);
#endif
        memcpy(pBuf, pMsgBuf->Buf, (pMsgBuf->InPtr));
        RtnLen           =  pMsgBuf->InPtr;
        pMsgBuf->Status  =  MSG_BUF_EMPTY;
    }

    return RtnLen;
}

UWORD cBtReadCh0(UBYTE *pBuf, UWORD Length)
{
    return cBtRead(0, pBuf, Length);
}

UWORD cBtReadCh1(UBYTE *pBuf, UWORD Length)
{
    return cBtRead(1, pBuf, Length);
}

UWORD cBtReadCh2(UBYTE *pBuf, UWORD Length)
{
    return cBtRead(2, pBuf, Length);
}

UWORD cBtReadCh3(UBYTE *pBuf, UWORD Length)
{
    return cBtRead(3, pBuf, Length);
}

UWORD cBtReadCh4(UBYTE *pBuf, UWORD Length)
{
    return cBtRead(4, pBuf, Length);
}

UWORD cBtReadCh5(UBYTE *pBuf, UWORD Length)
{
    return cBtRead(5, pBuf, Length);
}

UWORD cBtReadCh6(UBYTE *pBuf, UWORD Length)
{
    return cBtRead(6, pBuf, Length);
}

UWORD cBtReadCh7(UBYTE *pBuf, UWORD Length)
{
    return cBtRead(7, pBuf, Length);
}

void      DecodeMode2(void)
{
  UWORD   BytesAccepted;
  SLONG   AvailBytes;

  // Buffer is dedicated to mode2 only
  // Only one bluetooth connection is valid at a time
  AvailBytes = (BtInstance.Mode2Buf.InPtr - BtInstance.Mode2Buf.OutPtr);             /* How many bytes is ready to be read */
  if (AvailBytes)
  {
    BytesAccepted = DataToMode2Decoding(&(BtInstance.Mode2Buf.Buf[0]), AvailBytes);  /* Transfer bytes to mode2 decoding   */

    if (BytesAccepted == AvailBytes)
    {
      BtInstance.Mode2Buf.OutPtr = 0;
      BtInstance.Mode2Buf.InPtr  = 0;
      BtInstance.Mode2Buf.Status = READ_BUF_EMPTY;
    }
    else
    {
      BtInstance.Mode2Buf.OutPtr += BytesAccepted;
    }
  }
}


void      DecodeMode1(UBYTE BufNo)
{
  SLONG   AvailBytes;
  READBUF *pReadBuf;
  MSGBUF  *pMsgBuf;

  #ifdef  DEBUG
    SLONG   Test;
  #endif

  /* 1. Check if there is more data to interpret */
  /* 2. Check the status of the active buffer    */
  pReadBuf = &(BtInstance.BtCh[BufNo].ReadBuf); // Source buffer
  pMsgBuf  = &(BtInstance.BtCh[BufNo].MsgBuf);  // Destination Buffer

  AvailBytes = (pReadBuf->InPtr - pReadBuf->OutPtr);          /* How many bytes is ready to be read */
  
  #ifdef DEBUG
    printf("\nDecode mode 1: Avail bytes = %d MsgBuf status = %d\n",AvailBytes,pMsgBuf->Status);
  #endif

  switch(pMsgBuf->Status)
  {
    case MSG_BUF_EMPTY:
    {
      // Message buffer is empty
      pMsgBuf->InPtr   =  0;

      if(TRUE == pMsgBuf->LargeMsg)
      {
        pMsgBuf->Status = MSG_BUF_BODY;
      }
      else
      {
        if (2 <= AvailBytes)
        {
          memcpy(&(pMsgBuf->Buf[pMsgBuf->InPtr]), &(pReadBuf->Buf[pReadBuf->OutPtr]), 2);
          pMsgBuf->InPtr     += 2;
          pReadBuf->OutPtr   += 2;
          AvailBytes         -= 2;

          pMsgBuf->RemMsgLen  = (int)(pMsgBuf->Buf[0]) + ((int)(pMsgBuf->Buf[1]) * 256);
          pMsgBuf->MsgLen     = pMsgBuf->RemMsgLen;

          if (0 != pMsgBuf->RemMsgLen)
          {

            if (pMsgBuf->RemMsgLen <= AvailBytes)
            {
              // Rest of message is received move it to the message buffer
              memcpy(&(pMsgBuf->Buf[pMsgBuf->InPtr]), &(pReadBuf->Buf[pReadBuf->OutPtr]), (pMsgBuf->RemMsgLen));

              AvailBytes       -=  (pMsgBuf->RemMsgLen);
              pReadBuf->OutPtr +=  (pMsgBuf->RemMsgLen);
              pMsgBuf->InPtr   +=  (pMsgBuf->RemMsgLen);
              pMsgBuf->Status   =  MSG_BUF_FULL;

              #ifdef DEBUG
                printf(" Message is received from MSG_BUF_EMPTY: ");
                for (Test = 0; Test < ((pMsgBuf->MsgLen) + 2); Test++)
                {
                  printf("%02X ", pMsgBuf->Buf[Test]);
                }
                printf("\n");
              #endif

              if (0 == AvailBytes)
              {
                // Read buffer is empty
                pReadBuf->OutPtr = 0;
                pReadBuf->InPtr  = 0;
                pReadBuf->Status = READ_BUF_EMPTY;
              }
            }
            else
            {
              // Still some bytes needed to be received
              // So Read buffer is emptied
              memcpy(&(pMsgBuf->Buf[pMsgBuf->InPtr]), &(pReadBuf->Buf[pReadBuf->OutPtr]), AvailBytes);

              pMsgBuf->Status     = MSG_BUF_BODY;
              pMsgBuf->RemMsgLen -= AvailBytes;
              pMsgBuf->InPtr     += AvailBytes;

              pReadBuf->OutPtr    = 0;
              pReadBuf->InPtr     = 0;
              pReadBuf->Status    = READ_BUF_EMPTY;
            }
          }
          else
          {
            if (0 == AvailBytes)
            {
              // Read buffer is empty
              pReadBuf->OutPtr = 0;
              pReadBuf->InPtr  = 0;
              pReadBuf->Status = READ_BUF_EMPTY;
            }
          }
        }
        else
        {
          // Only one byte has been received - first byte of length info
          memcpy(&(pMsgBuf->Buf[pMsgBuf->InPtr]), &(pReadBuf->Buf[pReadBuf->OutPtr]), 1);
          pReadBuf->OutPtr++;
          pMsgBuf->InPtr++;

          pMsgBuf->RemMsgLen = (int)(pMsgBuf->Buf[0]);
          pMsgBuf->Status    = MSG_BUF_LEN;

          pReadBuf->OutPtr = 0;
          pReadBuf->InPtr  = 0;
          pReadBuf->Status = READ_BUF_EMPTY;
        }
      }
    }
    break;

    case MSG_BUF_LEN:
    {
      // Read the last length bytes
      memcpy(&(pMsgBuf->Buf[pMsgBuf->InPtr]), &(pReadBuf->Buf[pReadBuf->OutPtr]), 1);
      pMsgBuf->InPtr++;
      pReadBuf->OutPtr++;
      AvailBytes--;

      pMsgBuf->RemMsgLen = (int)(pMsgBuf->Buf[0]) + ((int)(pMsgBuf->Buf[1]) * 256);
      pMsgBuf->MsgLen    = pMsgBuf->RemMsgLen;

      if (0 != pMsgBuf->RemMsgLen)
      {

        if ((pMsgBuf->RemMsgLen) <= AvailBytes)
        {
          // rest of message is received move it to the message buffer
          memcpy(&(pMsgBuf->Buf[pMsgBuf->InPtr]), &(pReadBuf->Buf[pReadBuf->OutPtr]), (pMsgBuf->RemMsgLen));

          AvailBytes       -= (pMsgBuf->RemMsgLen);
          pReadBuf->OutPtr += (pMsgBuf->RemMsgLen);
          pMsgBuf->InPtr   += (pMsgBuf->RemMsgLen);
          pMsgBuf->Status   = MSG_BUF_FULL;

          #ifdef DEBUG
            printf(" Message is received from MSG_BUF_EMPTY: ");
            for (Test = 0; Test < ((pMsgBuf->MsgLen) + 2); Test++)
            {
              printf("%02X ", pMsgBuf->Buf[Test]);
            }
            printf("\n");
          #endif

          if (0 == AvailBytes)
          {
            // Read buffer is empty
            pReadBuf->OutPtr = 0;
            pReadBuf->InPtr  = 0;
            pReadBuf->Status = READ_BUF_EMPTY;
          }
        }
        else
        {
          // Still some bytes needed to be received
          // So receive buffer is emptied
          memcpy(&(pMsgBuf->Buf[pMsgBuf->InPtr]), &(pReadBuf->Buf[pReadBuf->OutPtr]), AvailBytes);

          pMsgBuf->Status     = MSG_BUF_BODY;
          pMsgBuf->RemMsgLen -= AvailBytes;
          pMsgBuf->InPtr     += AvailBytes;

          pReadBuf->OutPtr    = 0;
          pReadBuf->InPtr     = 0;
          pReadBuf->Status    = READ_BUF_EMPTY;
        }
      }
      else
      {
        if (0 == AvailBytes)
        {
          // Read buffer is empty
          pReadBuf->OutPtr = 0;
          pReadBuf->InPtr  = 0;
          pReadBuf->Status = READ_BUF_EMPTY;

          pMsgBuf->Status  = MSG_BUF_EMPTY;
        }
      }
    }
    break;

    case MSG_BUF_BODY:
    {
      ULONG  BufFree;

      BufFree = (sizeof(pMsgBuf->Buf) - (pMsgBuf->InPtr));
      if (BufFree < (pMsgBuf->RemMsgLen))
      {

        pMsgBuf->LargeMsg = TRUE;

        //This is large message
        if (BufFree >= AvailBytes)
        {
          //The available bytes can be included in this buffer
          memcpy(&(pMsgBuf->Buf[pMsgBuf->InPtr]), &(pReadBuf->Buf[pReadBuf->OutPtr]), AvailBytes);

          //Buffer is still not full
          pMsgBuf->Status     = MSG_BUF_BODY;
          pMsgBuf->RemMsgLen -= AvailBytes;
          pMsgBuf->InPtr     += AvailBytes;

          //Readbuffer has been completely emptied
          pReadBuf->OutPtr    = 0;
          pReadBuf->InPtr     = 0;
          pReadBuf->Status    = READ_BUF_EMPTY;
        }
        else
        {
          //The available bytes cannot all be included in the buffer
          memcpy(&(pMsgBuf->Buf[pMsgBuf->InPtr]), &(pReadBuf->Buf[pReadBuf->OutPtr]), BufFree);
          pReadBuf->OutPtr   +=  BufFree;
          pMsgBuf->InPtr     +=  BufFree;
          pMsgBuf->RemMsgLen -=  BufFree;
          pMsgBuf->Status     =  MSG_BUF_FULL;
        }
      }
      else
      {
        pMsgBuf->LargeMsg = FALSE;

        if ((pMsgBuf->RemMsgLen) <= AvailBytes)
        {
          // rest of message is received move it to the message buffer
          memcpy(&(pMsgBuf->Buf[pMsgBuf->InPtr]), &(pReadBuf->Buf[pReadBuf->OutPtr]), (pMsgBuf->RemMsgLen));

          AvailBytes       -= (pMsgBuf->RemMsgLen);
          pReadBuf->OutPtr += (pMsgBuf->RemMsgLen);
          pMsgBuf->InPtr   += (pMsgBuf->RemMsgLen);
          pMsgBuf->Status   = MSG_BUF_FULL;

          #ifdef DEBUG
            printf(" Message is received from MSG_BUF_EMPTY: ");
            for (Test = 0; Test < ((pMsgBuf->MsgLen) + 2); Test++)
            {
              printf("%02X ", pMsgBuf->Buf[Test]);
            }
            printf("\n");
          #endif

          if (0 == AvailBytes)
          {
            // Read buffer is empty
            pReadBuf->OutPtr = 0;
            pReadBuf->InPtr  = 0;
            pReadBuf->Status = READ_BUF_EMPTY;
          }
        }
        else
        {
          // Still some bytes needed to be received
          // So receive buffer is emptied
          memcpy(&(pMsgBuf->Buf[pMsgBuf->InPtr]), &(pReadBuf->Buf[pReadBuf->OutPtr]), AvailBytes);

          pMsgBuf->Status     = MSG_BUF_BODY;
          pMsgBuf->RemMsgLen -= AvailBytes;
          pMsgBuf->InPtr     += AvailBytes;

          pReadBuf->OutPtr    = 0;
          pReadBuf->InPtr     = 0;
          pReadBuf->Status    = READ_BUF_EMPTY;
        }
      }
    }
    break;

    case MSG_BUF_FULL:
    {
    }
    break;

    default:
    {
    }
    break;
  }
}

static void DecodeBtStream(UBYTE BufNo)
{
    if (BtInstance.NonVol.DecodeMode == MODE1) {
        DecodeMode1(BufNo);
    } else {
        DecodeMode2();
    }
}

static void BtClose(void)
{
  BtDisconnectAll();

  // if (MIN_HANDLE <= BtInstance.HciSocket.Socket)
  // {
  //   ioctl(BtInstance.HciSocket.Socket, HCIDEVDOWN, 0);
  //   hci_close_dev(BtInstance.HciSocket.Socket);
  //   BtInstance.HciSocket.Socket = -1;
  // }

  I2cStop();
}

static void BtTxMsgs(void)
{
    WRITEBUF  *pWriteBuf;
    UWORD     ByteCnt;
    guint     BytesWritten;

    if (!BtInstance.channel0) {
        return;
    }

    pWriteBuf = &BtInstance.BtCh[0].WriteBuf;

    ByteCnt = pWriteBuf->InPtr - pWriteBuf->OutPtr;

    if (!ByteCnt) {
        return;
    }

    g_io_channel_write_chars(BtInstance.channel0,
                             (gchar *)&pWriteBuf->Buf[pWriteBuf->OutPtr],
                             ByteCnt, &BytesWritten, NULL);
    if (BytesWritten > 0) {
        pWriteBuf->OutPtr += BytesWritten;

#ifdef DEBUG
        printf("transmitted Bytes to send %d, Bytes written = %d\n",
               ByteCnt, BytesWritten);
        printf(" errno = %d\n", errno);
#endif

        if (pWriteBuf->OutPtr == pWriteBuf->InPtr) {
            // All bytes has been written - clear the buffer
            pWriteBuf->InPtr  = 0;
            pWriteBuf->OutPtr = 0;
        }
    }
    g_io_channel_flush(BtInstance.channel0, NULL);
}

void BtUpdate(void)
{
    BtTxMsgs();
}

/**
 * @brief           Sets the mode2 state.
 *
 * @param Mode2     The new state.
 *
 * @return          OK on success, otherwise FAIL
 */
RESULT BtSetMode2(UBYTE Mode2)
{
    // TODO: Need an iDevice to test mode2

    return FAIL;
}

/**
 * @brief           Gets the mode2 state.
 *
 * @param pMode2    Pointer to hold the result.
 *
 * @return          OK.
 */
RESULT BtGetMode2(UBYTE *pMode2)
{
    *pMode2 = BtInstance.NonVol.DecodeMode;

    return OK;
}

/**
 * @brief           Set the power state of the bluetooth adapter.
 *
 * @param on        TRUE to turn on or FALSE to turn off.
 *
 * @return          OK on success, otherwise FAIL.
 */
RESULT BtSetOnOff(UBYTE on)
{
    if (!BtInstance.adapter) {
        return FAIL;
    }

    // TODO: need to use connman for power, not bluez
    bluez_adapter1_set_powered(BtInstance.adapter, on);

    // hack to make UI happy since setting property is async
    g_dbus_proxy_set_cached_property(G_DBUS_PROXY(BtInstance.adapter),
                                     "Powered",
                                     g_variant_new_boolean(on));

    return OK;
}

/**
 * @brief           Get the power state of the bluetooth adapter.
 *
 * @param on        Pointer to hold the result. Set to TRUE if powered on,
 *                  otherwise FALSE.
 *
 * @return          OK.
 */
RESULT BtGetOnOff(UBYTE *On)
{
    RESULT Result = OK;

    if (BtInstance.adapter && bluez_adapter1_get_powered(BtInstance.adapter)) {
        *On = TRUE;
    } else {
        *On = FALSE;
    }

    return Result;
}

/**
 * @brief           Turn on/off bluetooth visibility.
 *
 * @param on        Set to TRUE to make the adapter visible.
 *
 * @return          OK.
 */
RESULT BtSetVisibility(UBYTE on)
{
    if (BtInstance.adapter) {

        bluez_adapter1_set_discoverable(BtInstance.adapter, on);

        // hack to make UI happy since setting property is async
        g_dbus_proxy_set_cached_property(G_DBUS_PROXY(BtInstance.adapter),
                                         "Discoverable",
                                         g_variant_new_boolean(on));
    }

    return OK;
}

/**
 * @brief           Check if Bluetooth is visible to other devices.
 *
 * @return          TRUE if visible, FALSE otherwise.
 */
UBYTE BtGetVisibility(void)
{
    if (!BtInstance.adapter) {
        return FALSE;
    }

    return bluez_adapter1_get_discoverable(BtInstance.adapter);
}

static gint compare_bluez_device1_name(BluezDevice1 *device,
                                       const gchar *name)
{
    return g_strcmp0(bluez_device1_get_name(device), name);
}

static void device_connect_callback(GObject *source_object,
                                    GAsyncResult *res,
                                    gpointer user_data)
{
    GError *error = NULL;

    BtInstance.ConnectBusy = FALSE;

    if (!bluez_device1_call_connect_finish(BLUEZ_DEVICE1(source_object), res, &error)) {
        g_warning("Failed to connect bluetooth device: %s", error->message);
        g_error_free(error);
        BtInstance.Fail = TRUE;
        return;
    }

    BtInstance.connected_list = g_list_append(BtInstance.connected_list,
                                              source_object);
}

static void device_pair_callback(GObject *source_object,
                                 GAsyncResult *res,
                                 gpointer user_data)
{
    GError *error = NULL;

    if (!bluez_device1_call_pair_finish(BLUEZ_DEVICE1(source_object), res, &error)) {
        g_warning("Failed to pair bluetooth device: %s", error->message);
        g_error_free(error);
        BtInstance.ConnectBusy = FALSE;
        BtInstance.Fail = TRUE;
        return;
    }

    BtInstance.paired_list = g_list_append(BtInstance.paired_list, source_object);
    BtInstance.unpaired_list = g_list_remove(BtInstance.unpaired_list, source_object);

    bluez_device1_call_connect_profile(BLUEZ_DEVICE1(source_object), SPP_UUID,
                                       NULL, device_connect_callback, NULL);
}

/**
 * @brief               Connect to bluetooth device.
 *
 * This will return FAIL if a connection is already in progress or the device
 * is not found.
 *
 * @param pName         The name of the device.
 *
 * @return              OK on success, otherwise FAIL.
 */
RESULT cBtConnect(const char *pName)
{
    GList *device;

    BtInstance.Fail = FALSE;

    if (BtInstance.ConnectBusy) {
        BtInstance.Fail = TRUE;
        return FAIL;
    }

    device = g_list_find_custom(BtInstance.unpaired_list, pName,
                                (GCompareFunc)compare_bluez_device1_name);

    if (device) {
        // First have to pair the device. The callback will then chain up to
        // bluez_device1_call_connect().
        bluez_device1_call_pair(BLUEZ_DEVICE1(device->data), NULL,
                                device_pair_callback, NULL);
        BtInstance.ConnectBusy = TRUE;

        return OK;
    }

    // device has not been paired yet
    device = g_list_find_custom(BtInstance.paired_list, pName,
                                (GCompareFunc)compare_bluez_device1_name);
    if (!device) {
        g_warning("Could not find bluetooth device '%s' for connecting", pName);
        BtInstance.Fail = TRUE;
        return FAIL;
    }

    bluez_device1_call_connect_profile(BLUEZ_DEVICE1(device->data), SPP_UUID,
                                       NULL, device_connect_callback, NULL);
    BtInstance.ConnectBusy = TRUE;

    return OK;
}

UBYTE cBtDiscChNo(UBYTE ChNo)
{
  UBYTE   RtnVal;
  // UBYTE   TmpCnt;

  RtnVal = FALSE;
  // for(TmpCnt = 0; TmpCnt < MAX_DEV_TABLE_ENTRIES; TmpCnt++)
  // {
  //   if ((TRUE == BtInstance.NonVol.DevList[TmpCnt].Connected) && (ChNo == BtInstance.NonVol.DevList[TmpCnt].ChNo))
  //   {
  //     cBtDiscDevIndex(TmpCnt);
  //     TmpCnt = MAX_DEV_TABLE_ENTRIES;
  //     RtnVal = TRUE;
  //   }
  // }
  return(RtnVal);
}

static void device_disconnect_callback(GObject *source_object,
                                       GAsyncResult *res,
                                       gpointer user_data)
{
    GError *error = NULL;

    BtInstance.DisconnectBusy = FALSE;

    if (!bluez_device1_call_disconnect_finish(BLUEZ_DEVICE1(source_object), res, &error)) {
        g_warning("Failed to disconnect bluetooth device: %s", error->message);
        g_error_free(error);
        BtInstance.Fail = TRUE;
        return;
    }

    BtInstance.connected_list = g_list_remove(BtInstance.connected_list, source_object);
}

/**
 * @brief               Disconnect a bluetooth device.
 *
 * @param pName         The name of the device to disconnect.
 *
 * @result              OK on success, otherwise FAIL.
 */
RESULT cBtDisconnect(const char *pName)
{
    GList *device;

    BtInstance.Fail = FALSE;

    device = g_list_find_custom(BtInstance.paired_list, pName,
                                (GCompareFunc)compare_bluez_device1_name);
    if (!device) {
        device = g_list_find_custom(BtInstance.unpaired_list, pName,
                                    (GCompareFunc)compare_bluez_device1_name);
    }
    if (!device) {
        g_warning("Could not find bluetooth device '%s' for disconnecting", pName);
        BtInstance.Fail = TRUE;
        return FAIL;
    }

    bluez_device1_call_disconnect(BLUEZ_DEVICE1(device->data), NULL,
                                  device_disconnect_callback, NULL);

    return OK;
}

UBYTE     cBtI2cBufReady(void)
{
  UBYTE   RtnVal;

  RtnVal = 1;
  if(0 != BtInstance.BtCh[0].WriteBuf.InPtr)
  {
    RtnVal = 0;
  }
  return(RtnVal);
}


UWORD     cBtI2cToBtBuf(UBYTE *pBuf, UWORD Size)
{
  if(0 == BtInstance.BtCh[0].WriteBuf.InPtr)
  {
    memcpy(BtInstance.BtCh[0].WriteBuf.Buf, pBuf, Size);
    BtInstance.BtCh[0].WriteBuf.InPtr = Size;
  }
  return(Size);
}

static UWORD cBtDevWriteBuf(UBYTE index, UBYTE *pBuf, UWORD Size)
{
    if (BtInstance.BtCh[index].WriteBuf.InPtr == 0) {
        memcpy(BtInstance.BtCh[index].WriteBuf.Buf, pBuf, Size);
        BtInstance.BtCh[index].WriteBuf.InPtr = Size;
    } else {
        Size = 0;
    }

    return Size;
}

UWORD cBtDevWriteBuf0(UBYTE *pBuf, UWORD Size)
{
    if (MODE2 == BtInstance.NonVol.DecodeMode) {
        if (0 == BtInstance.Mode2WriteBuf.InPtr) {
            memcpy(BtInstance.Mode2WriteBuf.Buf, pBuf, Size);
            BtInstance.Mode2WriteBuf.InPtr = Size;
        } else {
            Size = 0;
        }
    } else {
        Size = cBtDevWriteBuf(0, pBuf, Size);
    }

    return Size;
}

UWORD cBtDevWriteBuf1(UBYTE *pBuf, UWORD Size)
{
    return cBtDevWriteBuf(1, pBuf, Size);
}

UWORD cBtDevWriteBuf2(UBYTE *pBuf, UWORD Size)
{
    return cBtDevWriteBuf(2, pBuf, Size);
}

UWORD cBtDevWriteBuf3(UBYTE *pBuf, UWORD Size)
{
    return cBtDevWriteBuf(3, pBuf, Size);
}

UWORD cBtDevWriteBuf4(UBYTE *pBuf, UWORD Size)
{
    return cBtDevWriteBuf(4, pBuf, Size);
}

UWORD cBtDevWriteBuf5(UBYTE *pBuf, UWORD Size)
{
    return cBtDevWriteBuf(5, pBuf, Size);
}

UWORD cBtDevWriteBuf6(UBYTE *pBuf, UWORD Size)
{
    return cBtDevWriteBuf(6, pBuf, Size);
}

UWORD cBtDevWriteBuf7(UBYTE *pBuf, UWORD Size)
{
    return cBtDevWriteBuf(7, pBuf, Size);
}

/**
 * @brief               Gets the bluetooth instance state.
 *
 * @return              OK if bluetooth is idle, BUSY if connect/disconnect
 *                      is in progress or FAIL if there was an error.
 */
RESULT cBtGetHciBusyFlag(void)
{
    if (BtInstance.Fail) {
        return FAIL;
    }

    if (BtInstance.ConnectBusy || BtInstance.DisconnectBusy) {
        return BUSY;
    }

    return OK;
}

/**
 * @brief           Convert bluetooth class to LMS icons.
 *
 * @param icon      The suggested icon name or NULL.
 * @param class     The bluetooth class of device.
 *
 * @return          The icon type.
 */
static BTTYPE cBtGetBtType(const gchar *icon, guint class)
{
    // Try the icon name first
    if (g_strcmp0(icon, "computer") == 0) {
        return BTTYPE_PC;
    }
    if (g_strcmp0(icon, "phone") == 0) {
        return BTTYPE_PHONE;
    }

    // Then fall back to device class
    switch (class & 0xFFFF) {
    case 0x0804:
        // Toy Robot
        return BTTYPE_BRICK;
    default:
        switch ((class >> 8) & 0xFF) {
        case 0x01:
            // Computer
            return BTTYPE_PC;
        case 0x02:
            // Phone
            return BTTYPE_PHONE;
        default:
            return BTTYPE_UNKNOWN;
        }
    }
}

/**
 * @brief               Gets the number of connected devices.
 *
 * @return              The number of devices.
 */
UBYTE cBtGetNoOfConnListEntries(void)
{
    return g_list_length(BtInstance.connected_list);
}

/**
 * @brief           Gets an entry from the list of connected devices
 *
 * @param Item      The index of the item in the list.
 * @param pName     Preallocated char array to hold the name of the device.
 * @param Length    The length of pName
 * @param pType     Pointer to hold the type of device (icon index)
 *
 * @return          OK on success, otherwise FAIL.
 */
RESULT cBtGetConnListEntry(UBYTE Item, char *pName, SBYTE Length, UBYTE *pType)
{
    BluezDevice1 *device;

    device = g_list_nth_data(BtInstance.connected_list, Item);
    if (!device) {
        return FAIL;
    }

    snprintf(pName, Length, "%s", bluez_device1_get_name(device));
    *pType = cBtGetBtType(bluez_device1_get_icon(device),
                          bluez_device1_get_class(device));

    return OK;
}

/**
 * @brief               Get the length of the list of paired devices.
 *
 * @return              The list length.
 */
UBYTE cBtGetNoOfDevListEntries(void)
{
    return g_list_length(BtInstance.paired_list);
}

/**
 * @brief               Get info about a paired bluetooth device.
 *
 * @param Item          Index of the item in the paired device list.
 * @param pConnected    Pointer to hold the connected state.
 * @param pType         Pointer to hold the device type (icon).
 * @param pName         Preallocated char array to hold the name of the device.
 * @param Length        The length of pName.
 *
 * @return              OK on success, otherwise FAIL (index out of range).
 */
RESULT cBtGetDevListEntry(UBYTE Item, SBYTE *pConnected, SBYTE *pType,
                          char *pName, SBYTE Length)
{
    BluezDevice1 *device;

    device = g_list_nth_data(BtInstance.paired_list, Item);
    if (!device) {
        return FAIL;
    }

    snprintf(pName, Length, "%s", bluez_device1_get_name(device));
    *pConnected = bluez_device1_get_connected(device);
    *pType = cBtGetBtType(bluez_device1_get_icon(device),
                          bluez_device1_get_class(device));

    return OK;
}

/**
 * @brief           Get the length of the search device list.
 *
 * @return          The length.
 */
UBYTE cBtGetNoOfSearchListEntries(void)
{
    return g_list_length(BtInstance.unpaired_list);
}

/**
 * @brief               Get information from an item in a search list entry.
 *
 * @param Item          The index of the item in the list.
 * @param pConnected    Pointer to hold the connected state.
 * @param pType         Pointer to hold the device type (icon)
 * @param pPaired       Pointer to hold the paired state.
 * @param pName         Preallocated char array to hold the name.
 * @param Length        The length of pName
 *
 * @ return             OK on success, otherwise FAIL (index out of range).
 */
RESULT cBtGetSearchListEntry(UBYTE Item, SBYTE *pConnected, SBYTE *pType,
                             SBYTE *pPaired, char *pName, SBYTE Length)
{
    BluezDevice1 *device = g_list_nth_data(BtInstance.unpaired_list, Item);

    if (!device) {
        return FAIL;
    }

    snprintf((char*)pName, Length, "%s", bluez_device1_get_name(device));
    *pConnected = bluez_device1_get_connected(device);
    *pPaired = bluez_device1_get_paired(device);
    *pType = cBtGetBtType(bluez_device1_get_icon(device),
                          bluez_device1_get_class(device));

    return OK;
}

/**
 * @brief               Remove a bluetooth device.
 *
 * @param pName         The name of the device to remove.
 *
 * @return              OK on success, otherwise FAIL.
 */
RESULT cBtRemoveItem(const char *pName)
{
    GList *item;
    BluezDevice1 *device;
    GError *error = NULL;

    BtInstance.Fail = FALSE;

    if (!BtInstance.adapter) {
        BtInstance.Fail = TRUE;
        return FAIL;
    }

    item = g_list_find_custom(BtInstance.paired_list, pName,
                              (GCompareFunc)compare_bluez_device1_name);
    if (!item) {
        item = g_list_find_custom(BtInstance.unpaired_list, pName,
                                  (GCompareFunc)compare_bluez_device1_name);
    }

    if (!item) {
        BtInstance.Fail = TRUE;
        return FAIL;
    }

    device = item->data;
    if (!bluez_adapter1_call_remove_device_sync(BtInstance.adapter,
        g_dbus_proxy_get_object_path (G_DBUS_PROXY(device)),
        NULL, &error))
    {
        g_warning("Failed to remove bluetooth device: %s", error->message);
        g_error_free(error);
        BtInstance.Fail = TRUE;
        return FAIL;
    }

    // this will trigger object-removed signals that will actually remove
    // the device from the various lists.

    return OK;
}

/**
 * @brief               Gets the status of the bluetooth adapter.
 *
 * Used for the indicator icon. See TopLineBtIconMap in c_ui.c.
 *
 * @return              Flags indicating the status.
 */
UBYTE cBtGetStatus(void)
{
    UBYTE status = 0;

    if (BtInstance.adapter) {
        if (bluez_adapter1_get_powered(BtInstance.adapter)) {
            status |= 0x01;
        }
        if (bluez_adapter1_get_discoverable(BtInstance.adapter)) {
            status |= 0x02;
        }
        if (BtInstance.NoOfConnDevs > 0) {
            status |= 0x04;
        }
    }

    return status;
}

void      cBtGetId(UBYTE *pId, UBYTE Length)
{
  strncpy((char*)pId, &(BtInstance.Adr[0]), Length);
}

/**
 * @brief               Set the name of the brick
 *
 * @param pName         The new name
 * @param Length        The length of pName
 *
 * @return              OK on success, otherwise FAIL
 */
RESULT cBtSetName(const char *pName, UBYTE Length)
{
    // TODO: set hostname and then send SIGHUP to bluetoothd to reload

    return FAIL;
}

UBYTE cBtGetChNo(UBYTE *pName, UBYTE *pChNos)
{
    // TODO: implement more channels

    return 0;
}

/**
 * @brief               Gets any pending requests.
 *
 * @return              EVENT_BT_PIN or EVENT_BT_REQ_CONF or EVENT_NONE.
 */
COM_EVENT cBtGetEvent(void)
{
    COM_EVENT Evt;

    Evt = BtInstance.Events;
    BtInstance.Events = EVENT_NONE;

    return Evt;
}

/**
 * @brief               Get info about an incoming bluetooth connection request.
 *
 * @param pName         Preallocated char array to hold the name.
 * @param pType         Pointer to hold the device type (icon).
 * @param Length        Length of pName.
 */
void cBtGetIncoming(char *pName, UBYTE *pType, UBYTE Length)
{
    if (BtInstance.incoming_device) {
        snprintf(pName, Length, "%s", bluez_device1_get_name(BtInstance.incoming_device));
        *pType = cBtGetBtType(bluez_device1_get_icon(BtInstance.incoming_device),
                              bluez_device1_get_class(BtInstance.incoming_device));
    }
}

/**
 * @brief               Complete a pin code agent request.
 *
 * @param pPin          The pin code.
 *
 * @return              FAIL if there was not a pending request, otherwise OK.
 */
RESULT cBtSetPin(const char *pPin)
{
    if (!BtInstance.request_pin_invocation) {
        // there is not a pending request
        return FAIL;
    }

    bluez_agent1_complete_request_pin_code(BtInstance.agent,
        BtInstance.request_pin_invocation, pPin);

    BtInstance.request_pin_invocation = NULL;

    return OK;
}

/**
 * @brief               Complete a request to confirm a passkey
 *
 * @param Accept        TRUE to accept, FALSE to reject
 *
 * @return              FAIL if there was not a pending request, otherwise OK.
 */
RESULT cBtSetPasskey(UBYTE Accept)
{
    if (!BtInstance.request_confirmation_invocation) {
        // there is not a pending request
        return FAIL;
    }

    if (Accept) {
        bluez_agent1_complete_request_confirmation(BtInstance.agent,
            BtInstance.request_confirmation_invocation);
    } else {
        g_dbus_method_invocation_return_dbus_error(
            BtInstance.request_confirmation_invocation,
            "org.bluez.Error.Rejected",
            "User rejected passkey");
    }

    BtInstance.request_confirmation_invocation = NULL;

    return OK;
}

void    cBtSetTrustedDev(UBYTE *pBtAddr, UBYTE *pPin, UBYTE PinSize)
{
  // BtInstance.TrustedDev.PinLen = PinSize;
  // snprintf((BtInstance.TrustedDev.Pin), 7, "%s", pPin);
  // cBtStrNoColonToBa(pBtAddr, &(BtInstance.TrustedDev.Adr));
}

/**
 * @brief           Sets the bundle id.
 *
 * @param pId       The new id.
 *
 * @return          TRUE on success, otherwise FALSE (id was too long).
 */
gboolean cBtSetBundleId(const gchar *pId)
{
    if (MAX_BUNDLE_ID_SIZE > strlen(pId)) {
        snprintf(BtInstance.NonVol.BundleID, MAX_BUNDLE_ID_SIZE, "%s", pId);
        return TRUE;
    }

    return FALSE;
}

/**
 * @brief           Sets the seed id.
 *
 * @param pSeedId   The new id.
 *
 * @return          TRUE on success, otherwise FALSE (id was too long).
 */
gboolean cBtSetBundleSeedId(const gchar *pSeedId)
{
    if (MAX_BUNDLE_SEED_ID_SIZE > strlen(pSeedId)) {
        snprintf(BtInstance.NonVol.BundleSeedID, MAX_BUNDLE_SEED_ID_SIZE,
                 "%s", pSeedId);
        return TRUE;
    }

    return FALSE;
}

static gint compare_proxy_object_path(GDBusProxy *proxy, gchar *path)
{
    return g_strcmp0(g_dbus_proxy_get_object_path(proxy), path);
}

static gboolean handle_request_pin_code(BluezAgent1 *object,
                                        GDBusMethodInvocation *invocation,
                                        const gchar *arg_device)
{
    GList *device;

    BtInstance.Events = EVENT_BT_PIN;
    BtInstance.request_pin_invocation = g_object_ref(invocation);

    device = g_list_find_custom(BtInstance.unpaired_list, arg_device,
                                (GCompareFunc)compare_proxy_object_path);
    BtInstance.incoming_device = device ? device->data : NULL;

    return TRUE;
}

static gboolean handle_display_pin_code(BluezAgent1 *object,
                                        GDBusMethodInvocation *invocation,
                                        const gchar *arg_device,
                                        const gchar *arg_pin_code)
{
    g_warning("Unhanded bluetooth agent request: handle_display_pin_code");

    return FALSE;
}

static gboolean handle_request_passkey(BluezAgent1 *object,
                                       GDBusMethodInvocation *invocation,
                                       const gchar *arg_device)
{
    g_warning("Unhanded bluetooth agent request: handle_request_passkey");

    return FALSE;
}

static gboolean handle_display_passkey(BluezAgent1 *object,
                                       GDBusMethodInvocation *invocation,
                                       const gchar *arg_device,
                                       guint arg_passkey,
                                       guint16 arg_entered)
{
    g_warning("Unhanded bluetooth agent request: handle_display_passkey");

    return FALSE;
}

static gboolean handle_request_confirmation(BluezAgent1 *object,
                                            GDBusMethodInvocation *invocation,
                                            const gchar *arg_device,
                                            guint arg_passkey)
{
    GList *device;

    BtInstance.Events = EVENT_BT_REQ_CONF;
    BtInstance.request_confirmation_invocation = g_object_ref(invocation);

    device = g_list_find_custom(BtInstance.unpaired_list, arg_device,
                                (GCompareFunc)compare_proxy_object_path);
    BtInstance.incoming_device = device ? device->data : NULL;

    return TRUE;
}

static gboolean handle_cancel(BluezAgent1 *object,
                              GDBusMethodInvocation *invocation)
{
    if (BtInstance.request_pin_invocation) {
        g_dbus_method_invocation_return_dbus_error(
            BtInstance.request_pin_invocation,
            "org.bluez.Error.Canceled",
            "Canceled by remote device");
        BtInstance.request_pin_invocation = NULL;
    }
    if (BtInstance.request_confirmation_invocation) {
        g_dbus_method_invocation_return_dbus_error(
            BtInstance.request_confirmation_invocation,
            "org.bluez.Error.Canceled",
            "Canceled by remote device");
        BtInstance.request_confirmation_invocation = NULL;
    }

    bluez_agent1_complete_cancel(object, invocation);

    return TRUE;
}

static gboolean handle_release(BluezAgent1 *object,
                               GDBusMethodInvocation *invocation)
{
    if (BtInstance.request_pin_invocation) {
        g_object_unref(BtInstance.request_pin_invocation);
        BtInstance.request_pin_invocation = NULL;
    }
    if (BtInstance.request_confirmation_invocation) {
        g_object_unref(BtInstance.request_confirmation_invocation);
        BtInstance.request_confirmation_invocation = NULL;
    }

    bluez_agent1_complete_release(object, invocation);

    return TRUE;
}

static void request_default_agent_callback(GObject *source_object,
                                           GAsyncResult *res,
                                           gpointer user_data)
{
    BluezAgentManager1 *agent_manager = BLUEZ_AGENT_MANAGER1(source_object);
    GError *error = NULL;

    if (!bluez_agent_manager1_call_request_default_agent_finish(agent_manager,
                                                                res,
                                                                &error))
    {
        g_warning("Failed to request default agent: %s", error->message);
        g_error_free(error);
        return;
    }
}

static void register_agent_callback(GObject *source_object,
                                    GAsyncResult *res,
                                    gpointer user_data)
{
    BluezAgentManager1 *agent_manager = BLUEZ_AGENT_MANAGER1(source_object);
    const char *agent_path = user_data;
    GError *error = NULL;

    if (!bluez_agent_manager1_call_register_agent_finish(agent_manager,
                                                         res,
                                                         &error))
    {
        g_warning("Failed to register bluetooth agent: %s", error->message);
        g_error_free(error);
        return;
    }

    bluez_agent_manager1_call_request_default_agent(agent_manager,
                                                    agent_path,
                                                    NULL,
                                                    request_default_agent_callback,
                                                    NULL);
}

static void unregister_agent_callback(GObject *source_object,
                                      GAsyncResult *res,
                                      gpointer user_data)
{
    BluezAgentManager1 *agent_manager = BLUEZ_AGENT_MANAGER1(source_object);
    GError *error = NULL;

    if (!bluez_agent_manager1_call_unregister_agent_finish(agent_manager,
                                                           res,
                                                           &error))
    {
        g_warning("Failed to unregister bluetooth agent: %s", error->message);
        g_error_free(error);
    }
}

static void regitser_profile_callback(GObject *source_object,
                                      GAsyncResult *res,
                                      gpointer user_data)
{
    GError *error = NULL;

    if (!bluez_profile_manager1_call_register_profile_finish(
                    BLUEZ_PROFILE_MANAGER1(source_object), res, &error))
    {
        g_warning("Failed to register SPP profile: %s", error->message);
        g_error_free(error);
        return;
    }
}

static void on_device_paired(BluezDevice1 *device)
{
    // if pairing was successful, we trust the device
    bluez_device1_set_trusted(device, TRUE);
}

static void on_interface_added(GDBusObjectManager *manager,
                               GDBusObject *object,
                               GDBusInterface *interface,
                               gpointer user_data)
{
    if (BLUEZ_IS_ADAPTER1_PROXY(interface)) {
        if (BtInstance.adapter) {
            g_warning("Multiple bluetooth adapters are not supported.");
        } else {
            BtInstance.adapter = BLUEZ_ADAPTER1(g_object_ref(interface));
            // only discoverable when bluetooth UI is shown
            bluez_adapter1_set_discoverable(BtInstance.adapter, FALSE);
        }
    }
    else if (BLUEZ_IS_AGENT_MANAGER1_PROXY(interface)) {
        BtInstance.agent_manager = BLUEZ_AGENT_MANAGER1(g_object_ref(interface));
        bluez_agent_manager1_call_register_agent(BtInstance.agent_manager,
                                                 C_BT_AGENT_PATH,
                                                 "KeyboardDisplay",
                                                 NULL,
                                                 register_agent_callback,
                                                 C_BT_AGENT_PATH);
    }
    else if (BLUEZ_IS_DEVICE1(interface)) {
        BluezDevice1 *device = BLUEZ_DEVICE1(interface);

        if (bluez_device1_get_paired(device)) {
            g_signal_connect(device, "notify::paired",
                             G_CALLBACK(on_device_paired), NULL);
            on_device_paired(device);
            BtInstance.paired_list =
                g_list_append(BtInstance.paired_list, g_object_ref(interface));
            if (bluez_device1_get_connected(device)) {
                BtInstance.connected_list =
                    g_list_append(BtInstance.connected_list, interface);
            }
        } else {
            BtInstance.unpaired_list =
                g_list_append(BtInstance.unpaired_list, g_object_ref(interface));
        }

    }
    else if (BLUEZ_IS_PROFILE_MANAGER1_PROXY(interface)) {
        GVariantDict options;

        g_variant_dict_init(&options, NULL);
        g_variant_dict_insert(&options, "Name", "s", "Wireless EV3");
        g_variant_dict_insert(&options, "Role", "s", "server");
        g_variant_dict_insert(&options, "Channel", "q", "1");
        g_variant_dict_insert(&options, "RequireAuthentication", "b", TRUE);
        g_variant_dict_insert(&options, "RequireAuthorization", "b", TRUE);

        BtInstance.profile_manager = BLUEZ_PROFILE_MANAGER1(g_object_ref(interface));
        bluez_profile_manager1_call_register_profile(BtInstance.profile_manager,
            g_dbus_interface_skeleton_get_object_path(
                        G_DBUS_INTERFACE_SKELETON(BtInstance.spp_profile)),
            SPP_UUID, g_variant_dict_end(&options), NULL,
            regitser_profile_callback, NULL);
    }
}

static void on_interface_removed(GDBusObjectManager *manager,
                                 GDBusObject *object,
                                 GDBusInterface *interface,
                                 gpointer user_data)
{
    if (G_DBUS_INTERFACE(BtInstance.adapter) == interface) {
        BtInstance.adapter = NULL;
        g_object_unref(interface);
    }
    else if (G_DBUS_INTERFACE(BtInstance.agent_manager) == interface) {
        BtInstance.agent_manager = NULL;
        g_object_unref(interface);
    }
    else if (BLUEZ_IS_DEVICE1(interface)) {
        BtInstance.unpaired_list = g_list_remove(BtInstance.unpaired_list, interface);
        BtInstance.paired_list = g_list_remove(BtInstance.paired_list, interface);
        BtInstance.connected_list = g_list_remove(BtInstance.connected_list, interface);
        g_object_unref(interface);
    }
    else if (G_DBUS_INTERFACE(BtInstance.profile_manager) == interface) {
        BtInstance.profile_manager = NULL;
        g_object_unref(interface);
    }
}

static void on_object_added(GDBusObjectManager *manager,
                            GDBusObject *object,
                            gpointer user_data)
{
    GList *ifaces, *iface;

    ifaces = g_dbus_object_get_interfaces(object);
    for (iface = ifaces; iface != NULL; iface = g_list_next(iface)) {
        on_interface_added(manager, object, G_DBUS_INTERFACE(iface->data), NULL);
    }
    g_list_free_full(ifaces, g_object_unref);
}

static void on_object_removed(GDBusObjectManager *manager,
                              GDBusObject *object,
                              gpointer user_data)
{
    GList *ifaces, *iface;

    ifaces = g_dbus_object_get_interfaces(object);
    for (iface = ifaces; iface != NULL; iface = g_list_next(iface)) {
        on_interface_removed(manager, object, G_DBUS_INTERFACE(iface->data), NULL);
    }
    g_list_free_full(ifaces, g_object_unref);
}

static GType get_proxy_type(GDBusObjectManagerClient *manager,
                            const gchar *object_path,
                            const gchar *interface_name,
                            gpointer user_data)
{
    if (!interface_name) {
        return G_TYPE_DBUS_OBJECT_PROXY;
    }
    if (g_strcmp0(interface_name, "org.bluez.Adapter1") == 0) {
        return BLUEZ_TYPE_ADAPTER1_PROXY;
    }
    if (g_strcmp0(interface_name, "org.bluez.AgentManager1") == 0) {
        return BLUEZ_TYPE_AGENT_MANAGER1_PROXY;
    }
    if (g_strcmp0(interface_name, "org.bluez.Device1") == 0) {
        return BLUEZ_TYPE_DEVICE1_PROXY;
    }
    if (g_strcmp0(interface_name, "org.bluez.ProfileManager1") == 0) {
        return BLUEZ_TYPE_PROFILE_MANAGER1_PROXY;
    }

    return G_TYPE_DBUS_PROXY;
}

static void object_manager_new_callback(GObject *source_object,
                                        GAsyncResult *res,
                                        gpointer user_data)
{
    GError *error = NULL;
    GList *objects, *obj;

    BtInstance.object_manager =
        g_dbus_object_manager_client_new_for_bus_finish(res, &error);
    if (!BtInstance.object_manager) {
        g_warning("Failed to create object manager: %s", error->message);
        g_clear_error(&error);
        return;
    }

    g_signal_connect(BtInstance.object_manager, "interface-added",
                     G_CALLBACK(on_interface_added), NULL);
    g_signal_connect(BtInstance.object_manager, "interface-removed",
                     G_CALLBACK(on_interface_removed), NULL);
    g_signal_connect(BtInstance.object_manager, "object-added",
                     G_CALLBACK(on_object_added), NULL);
    g_signal_connect(BtInstance.object_manager, "object-removed",
                     G_CALLBACK(on_object_removed), NULL);

    objects = g_dbus_object_manager_get_objects(BtInstance.object_manager);
    for (obj = objects; obj != NULL; obj = g_list_next(obj)) {
        on_object_added(BtInstance.object_manager, G_DBUS_OBJECT(obj->data), NULL);
    }
    g_list_free_full(objects, g_object_unref);
}

static void cBtAgentInit(void)
{
    GDBusConnection *connection;
    GError *error = NULL;

    BtInstance.agent = bluez_agent1_skeleton_new();
    g_signal_connect(BtInstance.agent, "handle-request-pin-code",
                     G_CALLBACK(handle_request_pin_code), NULL);
    g_signal_connect(BtInstance.agent, "handle-display-pin-code",
                     G_CALLBACK(handle_display_pin_code), NULL);
    g_signal_connect(BtInstance.agent, "handle-request-passkey",
                     G_CALLBACK(handle_request_passkey), NULL);
    g_signal_connect(BtInstance.agent, "handle-display-passkey",
                     G_CALLBACK(handle_display_passkey), NULL);
    g_signal_connect(BtInstance.agent, "handle-request-confirmation",
                     G_CALLBACK(handle_request_confirmation), NULL);
    g_signal_connect(BtInstance.agent, "handle-cancel",
                     G_CALLBACK(handle_cancel), NULL);
    g_signal_connect(BtInstance.agent, "handle-release",
                     G_CALLBACK(handle_release), NULL);

    connection = g_bus_get_sync(G_BUS_TYPE_SYSTEM, NULL, &error);
    if (!connection) {
        g_warning("Failed to get system bus: %s", error->message);
        g_error_free(error);
        return;
    }

    if (!g_dbus_interface_skeleton_export(G_DBUS_INTERFACE_SKELETON(BtInstance.agent),
                                          connection,
                                          C_BT_AGENT_PATH,
                                          &error))
    {
        g_warning("Failed export bluetooth agent: %s", error->message);
        g_error_free(error);
        return;
    }

    g_object_unref(connection);
}

static void cBtAgentExit(void)
{
    g_dbus_interface_skeleton_unexport(G_DBUS_INTERFACE_SKELETON(BtInstance.agent));
    g_object_unref(BtInstance.agent);
    BtInstance.agent = NULL;
}

static gboolean handle_spp_in(GIOChannel *source,
                              GIOCondition condition,
                              gpointer data)
{
    READBUF *pReadBuf;
    guint BytesRead;

    if (BtInstance.NonVol.DecodeMode == MODE1) {
        // BtCh = 0 is the slave port
        pReadBuf = &BtInstance.BtCh[0].ReadBuf;
    } else {
        // dedicated buffer used for Mode2
        pReadBuf = &BtInstance.Mode2Buf;
    }

    // Check if it is possible to read more byte from the BT stream
    if (READ_BUF_EMPTY  ==  pReadBuf->Status) {
        g_io_channel_read_chars(BtInstance.channel0, (gchar *)pReadBuf->Buf,
                                sizeof(pReadBuf->Buf), &BytesRead, NULL);
        if (BytesRead > 0) {
#ifdef DEBUG
            int Cnt;
            printf("\nData received on BT in Slave mode");
            for (Cnt = 0; Cnt < BytesRead; Cnt++) {
                printf("\n Rx byte nr %02d = %02X",Cnt,pReadBuf->Buf[Cnt]);
            }
            printf("\n");
#endif
            pReadBuf->OutPtr = 0;
            pReadBuf->InPtr  = BytesRead;
            pReadBuf->Status = READ_BUF_FULL;

            DecodeBtStream(0);
        }
    } else {
        DecodeBtStream(0);
    }

    return G_SOURCE_CONTINUE;
}

static gboolean on_spp_new_connection(BluezProfile1 *object,
                                      GDBusMethodInvocation *invocation,
                                      GUnixFDList *fd_list,
                                      const gchar *arg_device,
                                      GVariant *arg_fd,
                                      GVariant *arg_fd_properties)
{
    if (BtInstance.channel0) {
        g_dbus_method_invocation_return_dbus_error(invocation,
                                                   "org.bluez.Error.Rejected",
                                                   "Only one connection allowed");
    } else {
        GIOFlags flags;
        gint real_fd;

        // Thanks: http://www.spinics.net/lists/linux-bluetooth/msg64681.html
        real_fd = g_unix_fd_list_get(fd_list, g_variant_get_handle(arg_fd), NULL);

        BtInstance.channel0 = g_io_channel_unix_new(real_fd);
        // remove encoding - this is binary data
        g_io_channel_set_encoding(BtInstance.channel0, NULL, NULL);
        g_io_channel_set_close_on_unref(BtInstance.channel0, TRUE);
        flags = g_io_channel_get_flags(BtInstance.channel0);
        g_io_channel_set_flags(BtInstance.channel0, flags | G_IO_FLAG_NONBLOCK, NULL);
        BtInstance.channel0_source_id = g_io_add_watch(BtInstance.channel0,
                                                       G_IO_IN | G_IO_PRI,
                                                       handle_spp_in,
                                                       NULL);
    }

    bluez_profile1_complete_new_connection(object, invocation, fd_list);

    return TRUE;
}

static gboolean on_spp_request_disconnection(BluezProfile1 *object,
                                             GDBusMethodInvocation *invocation,
                                             const gchar *arg_device)
{
    if (BtInstance.channel0) {
        g_source_remove(BtInstance.channel0_source_id);
        g_io_channel_unref(BtInstance.channel0);
        BtInstance.channel0 = NULL;
    }

    bluez_profile1_complete_request_disconnection(object, invocation);

    return TRUE;
}

static gboolean on_spp_release(BluezProfile1 *object,
                               GDBusMethodInvocation *invocation)
{
    if (BtInstance.channel0) {
        g_source_remove(BtInstance.channel0_source_id);
        g_io_channel_unref(BtInstance.channel0);
        BtInstance.channel0 = NULL;
    }

    bluez_profile1_complete_release(object, invocation);

    return TRUE;
}

static void cBtSppProfileInit(void)
{
    GDBusConnection *connection;
    GError *error = NULL;

    BtInstance.spp_profile = bluez_profile1_skeleton_new();
    g_signal_connect(BtInstance.spp_profile, "handle-new-connection",
                     G_CALLBACK(on_spp_new_connection), NULL);
    g_signal_connect(BtInstance.spp_profile, "handle-request-disconnection",
                     G_CALLBACK(on_spp_request_disconnection), NULL);
    g_signal_connect(BtInstance.spp_profile, "handle-release",
                     G_CALLBACK(on_spp_release), NULL);

    connection = g_bus_get_sync(G_BUS_TYPE_SYSTEM, NULL, &error);
    if (!connection) {
        g_warning("Failed to get system bus: %s", error->message);
        g_error_free(error);
        return;
    }

    if (!g_dbus_interface_skeleton_export(G_DBUS_INTERFACE_SKELETON(BtInstance.spp_profile),
                                          connection,
                                          C_BT_SPP_PROFILE_PATH,
                                          &error))
    {
        g_warning("Failed export bluetooth SPP profile: %s", error->message);
        g_error_free(error);
        return;
    }

    g_object_unref(connection);
}

static void cBtSppProfileExit(void)
{
    g_dbus_interface_skeleton_unexport(G_DBUS_INTERFACE_SKELETON(BtInstance.spp_profile));
    g_object_unref(BtInstance.spp_profile);
    BtInstance.spp_profile = NULL;
}

void BtInit(const char* pName)
{
    int     File;
    FILE   *FSer;

    // BtInstance.OnOffSeqCnt = 0;

    // Load all persistent variables
    FSer = fopen("./settings/BTser","r");
    if (FSer != NULL) {
        fgets(BtInstance.Adr, 13, FSer);
        fclose (FSer);
    } else {
        //Error - Fill with zeros
        strcpy(BtInstance.Adr, "000000000000");
    }

    File = open(NONVOL_BT_DATA,O_RDONLY);
    if (File >= MIN_HANDLE) {
        // Reads both Visible and ON/OFF status
        if (read(File,(UBYTE*)&BtInstance.NonVol, sizeof(BtInstance.NonVol)) != sizeof(BtInstance.NonVol)) {
            // TODO: handle error
        }
        close (File);
    } else {
        // Default settings
        BtInstance.NonVol.DecodeMode = MODE1;

        snprintf(BtInstance.NonVol.BundleID, MAX_BUNDLE_ID_SIZE, "%s", LEGO_BUNDLE_ID);
        snprintf(BtInstance.NonVol.BundleSeedID, MAX_BUNDLE_SEED_ID_SIZE, "%s", LEGO_BUNDLE_SEED_ID);
    }

    BtInstance.Events = EVENT_NONE;
    // BtInstance.TrustedDev.Status  =  FALSE;

    BtInstance.BtName[0] = 0;
    snprintf(BtInstance.BtName, vmBRICKNAMESIZE, "%s", pName);
    // bacpy(&(BtInstance.TrustedDev.Adr), BDADDR_ANY);   // BDADDR_ANY = all zeros!

    I2cInit(&BtInstance.BtCh[0].ReadBuf, &BtInstance.Mode2WriteBuf,
            BtInstance.NonVol.BundleID, BtInstance.NonVol.BundleSeedID);

    cBtAgentInit();
    cBtSppProfileInit();

    g_dbus_object_manager_client_new_for_bus(G_BUS_TYPE_SYSTEM,
                                             G_DBUS_OBJECT_MANAGER_CLIENT_FLAGS_NONE,
                                             "org.bluez", "/",
                                             get_proxy_type, NULL, NULL,
                                             NULL, /* cancellable */
                                             object_manager_new_callback, NULL);
}

void BtExit(void)
{
    int File;

    if (BtInstance.object_manager) {
        GList *objects, *obj;

        if (BtInstance.agent_manager) {
            bluez_agent_manager1_call_unregister_agent(BtInstance.agent_manager,
                g_dbus_interface_skeleton_get_object_path(
                    G_DBUS_INTERFACE_SKELETON(BtInstance.agent)),
                NULL, unregister_agent_callback, NULL);
        }

        objects = g_dbus_object_manager_get_objects(BtInstance.object_manager);
        for (obj = objects; obj != NULL; obj = g_list_next(obj)) {
            on_object_removed(BtInstance.object_manager,
                              G_DBUS_OBJECT(obj->data), NULL);
        }
        g_list_free_full(objects, g_object_unref);
        g_object_unref(BtInstance.object_manager);
        BtInstance.object_manager = NULL;
    }

    cBtSppProfileExit();
    cBtAgentExit();
    I2cExit();
    BtClose();

    File = open(NONVOL_BT_DATA, O_CREAT | O_WRONLY | O_TRUNC, SYSPERMISSIONS);
    if (File >= MIN_HANDLE) {
        write(File,(UBYTE*)&BtInstance.NonVol, sizeof(BtInstance.NonVol));
        close (File);
    }
}
