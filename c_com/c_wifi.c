/*
 * LEGO® MINDSTORMS EV3
 *
 * Copyright (C) 2010-2013 The LEGO Group
 * Copyright (C) 2016 David Lechner <david@lechnolgy.com>
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

/*! \page WiFiLibrary WiFi Library
 *
 *- \subpage  WiFiLibraryDescription
 *- \subpage  WiFiLibraryCodes
 */


/*! \page WiFiLibraryDescription Description
 *
 *
 */


/*! \page WiFiLibraryCodes Byte Code Summary
 *
 *
 */

#include <gio/gio.h>

#include "c_wifi.h"
#include "connman.h"

#ifdef DEBUG_WIFI
#define pr_dbg(f, ...) printf(f, ##__VA_ARGS__)
#else
#define pr_dbg(f, ...) while (0) { }
#endif

typedef struct {
    char mac_address[MAC_ADDRESS_LENGTH];     // as it tells
    char frequency[FREQUENCY_LENGTH];         // additional info - not used
    char signal_level[SIGNAL_LEVEL_LENGTH];   // -
    char security[SECURITY_LENGTH];           // Only WPA2 or NONE
    char friendly_name[FRIENDLY_NAME_LENGTH]; // The name, the user will see aka SSID
    char key_management[KEY_MGMT_LENGTH];
    char pre_shared_key[PSK_LENGTH];          // Preshared Key (Encryption)
    char pairwise_ciphers[PAIRWISE_LENGTH];
    char group_ciphers[GROUP_LENGTH];
    char proto[PROTO_LENGTH];
    WIFI_STATE_FLAGS ap_flags;                // Holds the capabilities etc. of the AP
} aps;

static char BtSerialNo[13];              // Storage for the BlueTooth Serial Number
static char BrickName[NAME_LENGTH + 1];  // BrickName for discovery and/or friendly info
static char MyBroadCastAdr[16];          // Broadcast address (the MASK included)

static struct timeval TimerStartVal, TimerCurrentVal;

static unsigned int TimeOut = 0;

static char Buffer[1024];

static uint TcpReadBufPointer = 0;

static WIFI_STATE WiFiConnectionState = WIFI_NOT_INITIATED;
static WIFI_INIT_STATE InitState = NOT_INIT;
// UDP
static int UdpSocketDescriptor = 0;
static int UdpTxCount;
static struct sockaddr_in ServerAddr;
static int BroadCast = 1;
static BEACON_MODE BeaconTx = NO_TX;
static TCP_STATES TcpState = TCP_DOWN;
static RESULT WiFiStatus = OK;
static int TcpConnectionSocket = 0;
static struct sockaddr_in servaddr;
static int TCPListenServer = 0;
static UWORD TcpTotalLength = 0;
static UWORD TcpRestLen = 0;
static UBYTE TcpReadState = TCP_IDLE;

static ConnmanManager *connman_manager = NULL;
static ConnmanTechnology *ethernet_technology = NULL;
static ConnmanTechnology *wifi_technology = NULL;
static GList *service_list = NULL;
static DATA16 service_list_state = 1;

// ******************************************************************************

// Start the Timer
static void cWiFiStartTimer(void)
{
    gettimeofday(&TimerStartVal, NULL);
}

// Get Elapsed time in seconds
static int cWiFiCheckTimer(void)
{
    // Get actual time and calculate elapsed time
    gettimeofday(&TimerCurrentVal, NULL);

    return (int)(TimerCurrentVal.tv_sec - TimerStartVal.tv_sec);
}

/**
 * @brief           Attempt to move a service up in the service list.
 *
 * ConnMan does it's own ordering of services, so don't expect this to have a
 * visible effect.
 *
 * @param Index     The index of the service to move.
 */
void cWiFiMoveUpInList(int Index)
{
    ConnmanService *service;
    GDBusProxy *previous;
    GError *error = NULL;

    service = g_list_nth_data(service_list, Index);
    previous = G_DBUS_PROXY(g_list_nth_data(service_list, Index - 1));

    g_return_if_fail(service != NULL);
    g_return_if_fail(previous != NULL);

    if (!connman_service_call_move_before_sync(service,
        g_dbus_proxy_get_object_path(previous), NULL, &error))
    {
        g_printerr("Failed to move up: %s\n", error->message);
        g_error_free(error);
    }
}

/**
 * @brief           Attempt to move a service down in the service list.
 *
 * ConnMan does it's own ordering of services, so don't expect this to have
 * a visible effect.
 *
 * @param Index     The index of the service to move.
 */
void cWiFiMoveDownInList(int Index)
{
    ConnmanService *service;
    GDBusProxy *next;
    GError *error = NULL;

    service = g_list_nth_data(service_list, Index);
    next = G_DBUS_PROXY(g_list_nth_data(service_list, Index + 1));

    g_return_if_fail(service != NULL);
    g_return_if_fail(next != NULL);

    if (!connman_service_call_move_before_sync(service,
        g_dbus_proxy_get_object_path(next), NULL, &error))
    {
        g_printerr("Failed to move down: %s\n", error->message);
        g_error_free(error);
    }
}

void cWiFiSetEncryptToWpa2(int Index)
{
    // TODO: Connman does not allow us to select the security.
}

void cWiFiSetEncryptToNone(int Index)
{
    // TODO: Connman does not allow us to select the security.
}

/**
 * @brief           Remove the specified service from the list of known services
 *
 * @param Index     The index of the service in the service list.
 */
void cWiFiDeleteAsKnown(int Index)
{
    ConnmanService *service = g_list_nth_data(service_list, Index);
    GError *error = NULL;

    if (service && !connman_service_call_remove_sync(service, NULL, &error)) {
        g_printerr("Failed to remove service: %s\n", error->message);
        g_error_free(error);
    }
}

/**
 * @brief               Get the IP address of the first service.
 *
 * If there are no services, IpAddress is set to "??".
 *
 * @param IpAddress     Preallocated character array.
 * @return              OK if there is a valid IP address.
 */
RESULT cWiFiGetIpAddr(char* IpAddress)
{
    ConnmanService *service;
    RESULT Result = FAIL;

    service = g_list_nth_data(service_list, 0);
    if (service) {
        GVariant *ipv4;
        GVariant *address;

        ipv4 = connman_service_get_ipv4(service);
        address = g_variant_lookup_value(ipv4, "Address", NULL);

        sprintf(IpAddress, "%s", g_variant_get_string(address, NULL));
        Result = OK;

        g_variant_unref(address);
        // ipv4 does not need to be unrefed
    } else {
        strcpy(IpAddress, "??");
    }

    return Result;
}

/**
 * @brief               Get the MAC address of the first service.
 *
 * If there are no services, MacAddress is set to "??".
 *
 * @param MacAddress    Preallocated character array.
 * @return              OK if there is a valid MAC address or FAIL if there is
 *                      no connection.
 */
RESULT cWiFiGetMyMacAddr(char* MacAddress)
{
    RESULT Result = FAIL;

    if (service_list) {
        GVariant *enet;
        GVariant *address;

        enet = connman_service_get_ethernet(CONNMAN_SERVICE(service_list->data));
        address = g_variant_lookup_value(enet, "Address", NULL);

        sprintf(MacAddress, "%s", g_variant_get_string(address, NULL));
        Result = OK;

        g_variant_unref(address);
        // enet does not need to be unrefed
    } else {
        strcpy(MacAddress, "??");
    }

    return Result;
}

/**
 * @brief Checks if WiFi is present on the system
 *
 * Also returns OK if ethernet (wired) technology is present.
 *
 * @return OK if present, otherwise FAIL
 */
RESULT cWiFiTechnologyPresent(void)
{
    return (wifi_technology || ethernet_technology) ? OK : FAIL;
}

/**
 * @brief           Get the name of a service (WiFi AP).
 *
 * Sets ApName to "None" if there is not a service at that index.
 *
 * @param ApName    Preallocated char array to store the name.
 * @param Index     The index of the service.
 * @param Length    The length of ApName.
 * @return          OK if there was a service at the specified index.
 */
RESULT cWiFiGetName(char *ApName, int Index, char Length)
{
    ConnmanService *service;
    RESULT Result = FAIL;

    service = g_list_nth_data(service_list, Index);
    if (service) {
        snprintf(ApName, Length, "%s", connman_service_get_name(service));
        Result = OK;
    } else {
        strcpy(ApName, "None");
    }

    return Result;
}

RESULT cWiFiSetName(char *ApName, int Index)
{
    RESULT Result = FAIL;

    // TODO: This needs to be made to work with ConnMan agent.

    return Result;
}

/**
 * @brief           Gets flags about service properties.
 *
 * @param Index     The index in the service list to check.
 * @return          The flags.
 */
WIFI_STATE_FLAGS cWiFiGetFlags(int Index)
{
    ConnmanService *service;
    WIFI_STATE_FLAGS flags = VISIBLE; // ConnMan does list services that are not visible

    service = g_list_nth_data(service_list, Index);
    if (service) {
        const gchar *state;
        const gchar *const *security;

        state = connman_service_get_state(service);
        if (g_strcmp0(state, "ready") == 0 || g_strcmp0(state, "online") == 0) {
            flags |= CONNECTED;
        }
        security = connman_service_get_security(service);
        for (; *security; security++) {
            if (g_strcmp0(*security, "psk") == 0) {
                flags |= WPA2;
            }
        }
        if (connman_service_get_favorite(service)) {
            flags |= KNOWN;
        }
    }

    return flags;
}

/**
 * @brief           Connect to the service at the specified index.
 *
 * @param Index     Index of the service in the service_list.
 */
RESULT cWiFiConnectToAp(int Index)
{
    ConnmanService *proxy;
    GError *error = NULL;
    RESULT Result = FAIL;

    WiFiStatus = BUSY;
    pr_dbg("cWiFiConnectToAp(int Index = %d)\n", Index);

    proxy = g_list_nth_data(service_list, Index);
    g_return_if_fail(proxy != NULL);

    // TODO: need to make this async so that we can handle agent request for passphrase
    if (connman_service_call_connect_sync(proxy, NULL, &error)) {
        Result = OK;
        pr_dbg("cWiFiMakeConnectionToAp(Index = %d) == OK)\n", Index);
        WiFiConnectionState = UDP_NOT_INITIATED;
        WiFiStatus = OK;
    } else {
        WiFiConnectionState = READY_FOR_AP_SEARCH;
        WiFiStatus = FAIL;
        g_printerr("Connect failed: %s\n", error->message);
    }

    return Result;
}

// Make the pre-shared key from
// Supplied SSID and PassPhrase
// And store it in ApTable[Index]
RESULT cWiFiMakePsk(char *ApSsid, char *PassPhrase, int Index)
{
    RESULT Return = FAIL;

    // TODO: How does this work with ConnMan?

    return Return;
}

/**
 * @brief           Gets the index of a service for a given name
 *
 * @param Name      The name to search for.
 * @param Index     Pointer to store the index if found.
 * @return          OK if a match was found, otherwise FAIL
 */
RESULT cWiFiGetIndexFromName(char *Name, UBYTE *Index)
{
    GList *item;
    RESULT Result = FAIL;

    *Index = 0;
    for (item = service_list; item; item = g_list_next(item)) {
        ConnmanService *service = item->data;
        const char *service_name = connman_service_get_name(service);

        if (g_strcmp0(service_name, Name) == 0) {
            Result = OK;
            break;
        }
        (*Index)++;
    }

    return Result;
}

/**
 * @brief           Scan for new access points
 *
 * If WiFi is not present or turned off, this does nothing (returns FAIL).
 *
 * @return          OK if scan was successful, otherwise FAIL.
 */
RESULT cWiFiScanForAPs()
{
    RESULT Result = FAIL;
    GError *error = NULL;

    pr_dbg("cWiFiScanForAPs\n");

    if (wifi_technology && connman_technology_get_powered(wifi_technology)) {
        if (connman_technology_call_scan_sync(wifi_technology, NULL, &error)) {
            Result = OK;
        } else {
            g_printerr("WiFi scan failed: %s\n", error->message);
            g_error_free(error);
        }
    }

    return Result;
}

/**
 * @brief           Gets a value that changes each time the service list changes.
 *
 * @return          The value.
 */
DATA16 cWifiGetListState(void)
{
    return service_list_state;
}

/**
 * @brief           Get the size of the service list.
 *
 * @return          The size.
 */
int cWiFiGetApListSize(void)
{
    return g_list_length(service_list);
}

RESULT cWiFiGetStatus(void)
{
    pr_dbg("WiFiStatus => GetResult = %d\n", WiFiStatus);

    return WiFiStatus;
}

static void cWiFiUdpClientClose(void)
{
    WiFiStatus = FAIL;                          // Async announcement of FAIL
    WiFiConnectionState = WIFI_INITIATED;
    BeaconTx = NO_TX;                           // Disable Beacon
    if (close(UdpSocketDescriptor) == 0) {
        WiFiStatus = OK;                          // Socket kill
    }
}

static RESULT cWiFiTcpClose(void)
{
    int res;
    UBYTE buffer[128];
    RESULT Result = OK;
    struct linger so_linger;

    WiFiStatus = BUSY;

    so_linger.l_onoff = TRUE;
    so_linger.l_linger = 0;
    setsockopt(TcpConnectionSocket, SOL_SOCKET, SO_LINGER, &so_linger,
               sizeof so_linger);

    if (shutdown(TcpConnectionSocket, 2) < 0) {
        do {
            pr_dbg("In the do_while\n");
            res = read(TcpConnectionSocket, buffer, 100);
            if (res < 0 ) {
                break;
            }
        } while (res != 0);

        pr_dbg("\nError calling Tcp shutdown()\n");
    }

    TcpConnectionSocket = 0;
    WiFiStatus = OK;  // We are at a known state :-)
    WiFiConnectionState = UDP_VISIBLE;

    return Result;
}

static RESULT cWiFiInitTcpServer()
{
  RESULT Result = OK;
  int Temp, SetOn;

  WiFiStatus = FAIL;
  /*  Create a listening socket IPv4, TCP and only single protocol */
  pr_dbg("Start of cWiFiInitTcpServer()...TCPListenServer = %d \n", TCPListenServer);

  if ( TCPListenServer == 0)   // close(TCPListenServer);
  {
    pr_dbg("TCPListenServer == 0 in cWiFiInitTcpServer()...\n");

	  if ( (TCPListenServer  = socket(AF_INET, SOCK_STREAM, 0)) < 0 )
	  {
		  pr_dbg("\nError creating listening socket in cWiFiInitTcpServer()...\n");

	    return Result;  // Bail out with a SOFT error in WiFiStatus
    }

	  /*  Reset the socket address structure    *
	   * and fill in the relevant data members  */

	  memset(&servaddr, 0, sizeof(servaddr));
	  servaddr.sin_family      = AF_INET;           // IPv4
	  servaddr.sin_addr.s_addr = htonl(INADDR_ANY); // Any address
	  servaddr.sin_port        = htons(TCP_PORT);   // The TCP port no. E.g. 5555

	  Temp = fcntl(TCPListenServer, F_GETFL, 0);

	  fcntl(TCPListenServer, F_SETFL, Temp | O_NONBLOCK); // Make the socket NON_BLOCKING

	  /*  Allow reuse of the socket ADDRESS            *
	   *   and PORT. Client disconnecting/reconnecting */

	  SetOn = 1;  // Set On to ON ;-)
	  Temp = setsockopt( TCPListenServer, SOL_SOCKET, SO_REUSEADDR, &SetOn, sizeof(SetOn));

	  /*  Bind the socket address to the      *
	   *   listening socket, and call listen() */

	  if ( bind(TCPListenServer, (struct sockaddr *) &servaddr, sizeof(servaddr)) < 0 )
	  {
         pr_dbg("\nError calling bind()\n");
         pr_dbg("errno = %d\n", errno);

		return Result;  // Bail out with a SOFT error in WiFiStatus
	  }
  } // End of the "DO A SINGLE TIME"

	  if ( listen(TCPListenServer, 1) < 0 )
	  {
		  pr_dbg("\nError calling listen()\n");

		return Result;  // Bail out with a SOFT error in WiFiStatus
	  }

	  // Else show debug text below.....
	  	pr_dbg("\nWAITING for a CLIENT.......\n");

  WiFiStatus = OK;
  return OK;  // Create socket, bind and listen succeeded
              // Or reuse
}

static RESULT cWiFiWaitForTcpConnection(void)
{
  uint size = sizeof(servaddr);
  RESULT Result = BUSY;

  WiFiStatus = OK;  // We're waiting in a "positive" way ;-)

  usleep(1000);	    // Just to be sure some cycles ... :-)

  if((TcpConnectionSocket = accept(TCPListenServer, (struct sockaddr *)&servaddr, &size) ) < 0)
  {
      pr_dbg("\nError calling accept() - returns: %d\r", TcpConnectionSocket);
  }
  else
  {
    Result = OK;
    WiFiStatus = OK;
    TcpReadState = TCP_WAIT_ON_START;


    pr_dbg("\nConnected.... :-) %s : %d\n", inet_ntoa(servaddr.sin_addr), ntohs(servaddr.sin_port));
  }
  pr_dbg("\nNow we're waiting for input TCP ...\n");

  return Result;
}

UWORD cWiFiWriteTcp(UBYTE* Buffer, UWORD Length)
{
    uint DataWritten = 0;                 // Nothing written (BUSY)
    struct timeval WriteTimeVal;          // Always called from COM, so FAIL == also 0
    fd_set  WriteFdSet;

    if (Length > 0) {
#ifdef DEBUG_WIFI
        printf("\ncWiFiWriteTcp Length: %d\n", Length);
        // Code below used for "hunting" packets of correct length
        // but with length bytes set to "0000" and payload all zeroed

        if ((Buffer[0] == 0) && (Buffer[1] == 0)) {
            int i;
            printf("\ncERROR in first 2 entries - WiFiWriteTcp Length: %d\n", Length);
            for (i = 0; i < Length; i++) {
                printf("\nFAIL!!! Buffer[%d] = 0x%x\n", i, Buffer[i]);
            }
        }
#endif

        if (WiFiConnectionState == TCP_CONNECTED) {
            WriteTimeVal.tv_sec = 0;  // NON blocking test use
            WriteTimeVal.tv_usec = 0; // I.e. NO timeout
            FD_ZERO(&WriteFdSet);
            FD_SET(TcpConnectionSocket, &WriteFdSet);

            select(TcpConnectionSocket + 1, NULL, &WriteFdSet, NULL, &WriteTimeVal);
            if (FD_ISSET(TcpConnectionSocket, &WriteFdSet)) {
                // We can Write
                DataWritten = write(TcpConnectionSocket, Buffer, Length);
#ifdef DEBUG_WIFI
                if (DataWritten != Length) {
                    // DataWritten = Data sent, zero = socket busy or -1 = FAIL
                    printf("\nDataWritten = %d, Length = %d\n", DataWritten, Length);
                }
#endif
            }
        }
    }

    return DataWritten;
}

static RESULT cWiFiResetTcp(void)
{
    RESULT Result = FAIL;
    pr_dbg("\nRESET - client disconnected!\n");

    Result = cWiFiTcpClose();

    return Result;
}

UWORD cWiFiReadTcp(UBYTE* Buffer, UWORD Length)
{
    int DataRead = 0; // Nothing read also sent if NOT initiated
                      // COM always polls!!
    struct timeval ReadTimeVal;
    fd_set  ReadFdSet;

    if (WiFiConnectionState == TCP_CONNECTED) {
        ReadTimeVal.tv_sec = 0;  // NON blocking test use
        ReadTimeVal.tv_usec = 0; // I.e. NO timeout
        FD_ZERO(&ReadFdSet);
        FD_SET(TcpConnectionSocket, &ReadFdSet);

        select(TcpConnectionSocket + 1, &ReadFdSet, NULL, NULL, &ReadTimeVal);
        if (FD_ISSET(TcpConnectionSocket, &ReadFdSet)) {
            pr_dbg("\nTcpReadState = %d\n", TcpReadState);

            switch (TcpReadState) {
            case TCP_IDLE:
                // Do Nothing
                break;

            case TCP_WAIT_ON_START:
                pr_dbg("TCP_WAIT_ON_START:\n");

                DataRead = read(TcpConnectionSocket, Buffer, 100); // Fixed TEXT

#ifdef DEBUG_WIFI
                printf("\nDataRead = %d, Buffer = \n", DataRead);
                if (DataRead > 0) {
                    int ii;

                    for (ii = 0; ii < DataRead; ii++) {
                        printf("0x%x, ", Buffer[ii]);
                    }
                } else {
                    printf("DataRead shows FAIL: %d", DataRead);
                }
                printf("\n");
#endif

                if (DataRead == 0) {
                    // We've a disconnect
                    cWiFiResetTcp();
                    break;
                }

                if (strstr((char*)Buffer, "ET /target?sn=") > 0) {
                    pr_dbg("\nTCP_WAIT_ON_START and  ET /target?sn= found :-)"
                           " DataRead = %d, Length = %d, Buffer = %s\n",
                           DataRead, Length, Buffer);

                    // A match found => UNLOCK
                    // Say OK back
                    cWiFiWriteTcp((UBYTE*)"Accept:EV340\r\n\r\n", 16);
                    TcpReadState = TCP_WAIT_ON_LENGTH;
                }

                DataRead = 0; // No COM-module activity yet
                break;

            case TCP_WAIT_ON_LENGTH:
                // We can should read the length of the message
                // The packets can be split from the client
                // I.e. Length bytes (2) can be send as a subset
                // the Sequence can also arrive as a single pair of bytes
                // and the finally the payload will be received

                // Begin on new buffer :-)
                TcpReadBufPointer = 0;
                pr_dbg("TCP_WAIT_ON_LENGTH:\n");
                DataRead = read(TcpConnectionSocket, Buffer, 2);
                if(DataRead == 0) {
                    // We've a disconnect
                    cWiFiResetTcp();
                    break;
                }

                TcpRestLen = (UWORD)(Buffer[0] + Buffer[1] * 256);
                TcpTotalLength = (UWORD)(TcpRestLen + 2);
                if(TcpTotalLength > Length) {
                    TcpReadState = TCP_WAIT_ON_FIRST_CHUNK;
                } else {
                    TcpReadState = TCP_WAIT_ON_ONLY_CHUNK;
                }

                TcpReadBufPointer += DataRead;	// Position in ReadBuffer adjust
                DataRead = 0;                   // Signal NO data yet

                pr_dbg("\n*************** NEW TX *************\n");
                pr_dbg("TCP_WAIT_ON_LENGTH TcpRestLen = %d, Length = %d\n",
                       TcpRestLen, Length);

                break;

            case TCP_WAIT_ON_ONLY_CHUNK:
                pr_dbg("TCP_WAIT_ON_ONLY_CHUNK: BufferStart = %d\n",
                       TcpReadBufPointer);

                DataRead = read(TcpConnectionSocket, &(Buffer[TcpReadBufPointer]),
                                TcpRestLen);

                pr_dbg("DataRead = %d\n",DataRead);
                pr_dbg("BufferPointer = %p\n", &(Buffer[TcpReadBufPointer]));

                if(DataRead == 0) {
                // We've a disconnect
                    cWiFiResetTcp();
                    break;
                }

                TcpReadBufPointer += DataRead;

                if(TcpRestLen == DataRead) {
                    DataRead = TcpTotalLength; // Total count read
                    TcpReadState = TCP_WAIT_ON_LENGTH;
                } else {
                    TcpRestLen -= DataRead; // Still some bytes in this only chunk
                    DataRead = 0;           // No COMM job yet
                }

#ifdef DEBUG_WIFI
                int i;

                for (i = 0; i < TcpTotalLength; i++) {
                    printf("ReadBuffer[%d] = 0x%x\n", i, Buffer[i]);
                }
#endif
                pr_dbg("TcpRestLen = %d, DataRead incl. 2 = %d, Length = %d\n",
                       TcpRestLen, DataRead, Length);

                break;

            case TCP_WAIT_ON_FIRST_CHUNK:
                pr_dbg("TCP_WAIT_ON_FIRST_CHUNK:\n");

                DataRead = read(TcpConnectionSocket, &(Buffer[TcpReadBufPointer]),
                                (Length - 2));
                if(DataRead == 0) {
                    // We've a disconnect
                    cWiFiResetTcp();
                    break;
                }
                pr_dbg("DataRead = %d\n", DataRead);

                TcpRestLen -= DataRead;
                TcpReadState = TCP_WAIT_COLLECT_BYTES;
                DataRead += 2;
                pr_dbg("\nTCP_WAIT_ON_FIRST_CHUNK TcpRestLen = %d, DataRead incl."
                       " 2 = %d, Length = %d\n", TcpRestLen, DataRead, Length);

                break;

            case TCP_WAIT_COLLECT_BYTES:
                pr_dbg("TCP_WAIT_COLLECT_BYTES:\n");

                TcpReadBufPointer = 0;
                if(TcpRestLen < Length) {
                    DataRead = read(TcpConnectionSocket, &(Buffer[TcpReadBufPointer]),
                                    TcpRestLen);
                } else {
                    DataRead = read(TcpConnectionSocket, &(Buffer[TcpReadBufPointer]),
                                    Length);
                }
                pr_dbg("DataRead = %d\n", DataRead);

                if(DataRead == 0) {
                    // We've a disconnect
                    cWiFiResetTcp();
                    break;
                }

                TcpRestLen -= DataRead;
                if(TcpRestLen == 0) {
                    TcpReadState = TCP_WAIT_ON_LENGTH;
                }
                pr_dbg("\nTCP_WAIT_COLLECT_BYTES TcpRestLen = %d, DataRead incl."
                       " 2 = %d, Length = %d\n", TcpRestLen, DataRead, Length);

                break;

            default:
                // Should never go here....
                TcpReadState = TCP_IDLE;
                break;
            }
        }
    }

    return DataRead;
}

static void cWiFiSetBtSerialNo(void)
{
    FILE *File;

    // Get the file-based BT SerialNo
    File = fopen("./settings/BTser", "r");
    if (File) {
        fgets(BtSerialNo, BLUETOOTH_SER_LENGTH, File);
        fclose(File);
    }
}

static void cWiFiSetBrickName(void)
{
    FILE *File;

    // Get the file-based BrickName
    File = fopen("./settings/BrickName", "r");
    if (File) {
        fgets(BrickName, BRICK_HOSTNAME_LENGTH, File);
        fclose(File);
    }
}

static RESULT cWiFiTransmitBeacon(void)
{
    RESULT Result = FAIL;

    ServerAddr.sin_port = htons(BROADCAST_PORT);
    ServerAddr.sin_addr.s_addr = inet_addr(MyBroadCastAdr);
    pr_dbg("\nUDP BROADCAST to port %d, address %s\n", ntohs(ServerAddr.sin_port),
           inet_ntoa(ServerAddr.sin_addr));

    cWiFiSetBtSerialNo(); // Be sure to have updated data :-)
    cWiFiSetBrickName();  // -
    sprintf(Buffer,"Serial-Number: %s\r\nPort: %d\r\nName: %s\r\nProtocol: EV3\r\n",
            BtSerialNo, TCP_PORT, BrickName);

    UdpTxCount =  sendto(UdpSocketDescriptor, Buffer, strlen(Buffer), 0,
                         (struct sockaddr *)&ServerAddr, sizeof(ServerAddr));
    if(UdpTxCount < 0) {
        pr_dbg("\nUDP SendTo ERROR : %d\n", UdpTxCount);

        cWiFiUdpClientClose();  // Kill the auto-beacon stuff
    } else {
        pr_dbg("\nUDP Client - SendTo() is OK! UdpTxCount = %d\n", UdpTxCount);
        pr_dbg("\nWaiting on a reply from UDP server...zzz - Send UNICAST only to me :-)\n");

        Result = OK;
    }

    return Result;
}

static RESULT cWiFiInitUdpConnection(void)
{
    RESULT Result = FAIL;
    int Temp;

    WiFiConnectionState = INIT_UDP_CONNECTION;

    /* Get a socket descriptor for UDP client (Beacon) */
    if ((UdpSocketDescriptor = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        pr_dbg("\nUDP Client - socket() error\n");
    } else {
        pr_dbg("\nUDP Client - socket() is OK!\n");
        pr_dbg("\nBroadCast Adr from Ifconfig = %s\n", MyBroadCastAdr);
        ServerAddr.sin_family = AF_INET;
        ServerAddr.sin_port = htons(BROADCAST_PORT);
        ServerAddr.sin_addr.s_addr = inet_addr(MyBroadCastAdr);
        if (ServerAddr.sin_addr.s_addr == (unsigned long)INADDR_NONE) {
            pr_dbg("\nHOST addr == INADDR_NONE :-( \n");
        } else {
            if (setsockopt(UdpSocketDescriptor, SOL_SOCKET, SO_BROADCAST,
                           &BroadCast, sizeof(BroadCast)) < 0)
            {
                pr_dbg("\nCould not setsockopt SO_BROADCAST\n");
            } else {
                Temp = fcntl(UdpSocketDescriptor, F_GETFL, 0);
                // Make the socket NON_BLOCKING
                fcntl(UdpSocketDescriptor, F_SETFL, Temp | O_NONBLOCK);
                pr_dbg("\nSet SOCKET NON-BLOCKING :-)...\n");

                Result = OK;
            }
        }
        pr_dbg("\nINIT UDP ServerAddr.sin_port = %d, ServerAddr.sin_addr.s_addr = %s\n",
               ntohs(ServerAddr.sin_port), inet_ntoa(ServerAddr.sin_addr));
    }

    return Result;
}

void cWiFiControl(void)
{
    // struct stat st;
    // char Command[128];

    // Do we have to TX the beacons?
    if (BeaconTx == TX_BEACON) {
        // If is it time for it?
        if (cWiFiCheckTimer() >= BEACON_TIME) {
            // Did we manage to TX one?
            if (cWiFiTransmitBeacon() != OK) {
                // Error handling - TODO: Should be user friendly
            } else {
                pr_dbg("\nOK beacon TX\n");
                // Reset for another timing (Beacon)
                cWiFiStartTimer();
            }
        }
    }

    switch (WiFiConnectionState) {
    case WIFI_NOT_INITIATED:
        // NOTHING INIT'ed
        // Idle
        break;

    case WIFI_INIT:
        // Do the time consumption stuff
        switch (InitState) {
        case NOT_INIT:
            // Start the Wpa_Supplicant in BackGround using
            // a very "thin" .conf file
            cWiFiStartTimer();
            pr_dbg("\nWIFI_INIT, NOT_INIT before system... %d\n", WiFiStatus);
            // cWiFiStartWpaSupplicant("/etc/wpa_supplicant.conf", LogicalIfName);
            //system("./wpa_supplicant -Dwext -iwlan<X> -c/etc/wpa_supplicant.conf -B");
            InitState = LOAD_SUPPLICANT;
            break;

        case LOAD_SUPPLICANT:
            TimeOut = cWiFiCheckTimer();
            if (TimeOut < WIFI_INIT_TIMEOUT) {
                // strcpy(Command, "/var/run/wpa_supplicant/");
                // strcat(Command, LogicalIfName);
                // if (stat(Command, &st) == 0) {
                    // pr_dbg("\nWIFI_INIT, LOAD_SUPPLICANT => STAT OK %d\n", WiFiStatus);
                    // // Ensure (help) Interface to become ready
                    // strcpy(Command, "ifconfig ");
                    // strcat(Command, LogicalIfName);
                    // strcat(Command, " down > /dev/null");

                    // system(Command);

                    // strcpy(Command, "ifconfig ");
                    // strcat(Command, LogicalIfName);
                    // strcat(Command, " up > /dev/null");

                    // system(Command);

                //     InitState = WAIT_ON_INTERFACE;
                // }
                //else keep waiting
            } else {
                pr_dbg("\nWIFI_INIT, LOAD_SUPPLICANT => Timed out\n");
                WiFiStatus = FAIL;
                // We're sleeping until user select ON
                WiFiConnectionState = WIFI_NOT_INITIATED;
                InitState = NOT_INIT;
            }
            break;

        case WAIT_ON_INTERFACE:
            // Wait for the Control stuff to be ready

            // Get "handle" to Control Interface
            // strcpy(Command, "/var/run/wpa_supplicant/");
            // strcat(Command, LogicalIfName);

            // if ((ctrl_conn = wpa_ctrl_open(Command)) > 0) {
            //     if (cWiFiWpaPing() == 0) {
            //         pr_dbg("\nWIFI_INIT, WAIT_ON_INTERFACE => Ping OK %d\n", WiFiStatus);
            //         cWiFiPopulateKnownApList();
            //         WiFiStatus = OK;
            //         WiFiConnectionState = WIFI_INITIATED;
            //         InitState = DONE;
            //     } else {
            //         pr_dbg("\nWIFI_INIT, WAIT_ON_INTERFACE => PING U/S\n");
            //         WiFiStatus = FAIL;
            //         cWiFiExit();
            //     }
            // }
            break;
        case DONE:
            break;
        }
        break;

    case WIFI_INITIATED:
        // Temporary state - WiFi lower Stuff turned ON
        pr_dbg("\nWIFI_INITIATED %d\n", WiFiStatus);
        WiFiConnectionState = READY_FOR_AP_SEARCH;
        pr_dbg("\nREADY for search -> %d\n", WiFiStatus);
        break;

    case READY_FOR_AP_SEARCH:
        // We can select SEARCH i.e. Press Connections on the U.I.
        // We have the H/W stack up and running
        break;

    case SEARCH_APS:
        // Polite wait
        pr_dbg("\nSEARCH_APS:\n");
        break;

    case SEARCH_PENDING:
        // Wait some time for things to show up...
        pr_dbg("cWiFiCheckTimer() = %d\r", cWiFiCheckTimer());
        // Give some time for the stuff to show up
        // Get Elapsed time in seconds
        if (20 <= cWiFiCheckTimer()) {
            // Getting the list and update the visible list
            // cWiFiStoreActualApList();
        }
        pr_dbg("\nSEARCH_PENDING:\n");
        break;

    case AP_LIST_UPDATED:
        // Relaxed state until connection wanted
      break;

    case AP_CONNECTING:
        // First connecting to the selected AP
        break;

    case WIFI_CONNECTED_TO_AP:
        // We have an active AP connection
        // Then get a valid IP address via DHCP
        break;

    case UDP_NOT_INITIATED:
        // We have an valid IP address
        // Initiated, connected and ready for UDP
        // I.e. ready for starting Beacons
        pr_dbg("\nHer er UDP_NOT_INITIATED\n");

        WiFiConnectionState = INIT_UDP_CONNECTION;
        break;

    case INIT_UDP_CONNECTION:
        WiFiStatus = BUSY;                    // We're still waiting
        memset(Buffer, 0x00, sizeof(Buffer)); // Reset TX buffer
        pr_dbg("\nLige foer cWiFiInitUdpConnection()\n");

        if (cWiFiInitUdpConnection() == OK) {
            pr_dbg("\nUDP connection READY @ INIT_UDP_CONNECTION\n");
            // Did we manage to TX one?
            if (cWiFiTransmitBeacon() == OK) {
                WiFiConnectionState = UDP_FIRST_TX;
                BeaconTx = TX_BEACON;             // Enable Beacon
                cWiFiStartTimer();                // Start timing (Beacon)
                WiFiStatus = OK;
            } else {
                // TODO: Some ERROR handling where to go - should be user friendly
            }
        } else {
            pr_dbg("\nUDP connection FAILed @ INIT_UDP_CONNECTION\n");
            WiFiStatus = FAIL;
            WiFiConnectionState = WIFI_NOT_INITIATED;
        }
        break;

    case UDP_FIRST_TX:
        // Allow some time before..
        if (cWiFiCheckTimer() >= 2) {
            WiFiConnectionState = UDP_VISIBLE;
        }
        break;

    case UDP_VISIBLE:
        // TX'ing beacon via UDP
        // We're tx'ing beacons, but waiting for connection
        // Are there any "dating" PC's?
        // Non-blocking test

        WiFiConnectionState = UDP_CONNECTED;

        break;

    case UDP_CONNECTED:
        // We have an active negotiation connection
        // Temp state between OK UDP negotiation
        // and the "real" TCP communication

        if (cWiFiInitTcpServer() == OK) {
            pr_dbg("\nTCP init OK @ UDP_CONNECTED\n");
            WiFiConnectionState = TCP_NOT_CONNECTED;
        }
        break;

    case TCP_NOT_CONNECTED:
        // Waiting for the PC to connect via TCP
        // Non-blocking test
        if (cWiFiWaitForTcpConnection() == OK) {
            pr_dbg("\nTCP_CONNECTED @ TCP_NOT_CONNECTED\n");
            TcpState = TCP_UP;
            WiFiConnectionState = TCP_CONNECTED;
            // We are connected so we can tell the world....
            // And we're ready to TX/RX :-)
            WiFiStatus = OK; // Not busy any longer
        }
        break;

    case TCP_CONNECTED:
        // We have a TCP connection established
        pr_dbg("\nTCP_CONNECTED @ TCP_CONNECTED.... And then.....\n");
        break;

    case CLOSED:
        // UDP/TCP closed
        break;
    }
}

/**
 * @brief Gets WiFi power status
 *
 * @return OK if WiFi is present and powered on, otherwise FAIL
 */
RESULT cWiFiGetOnStatus(void)
{
    RESULT Result = FAIL;

    if (wifi_technology && connman_technology_get_powered(wifi_technology)) {
        Result = OK;
    }

    return Result;
}

/**
 * @brief Turn WiFi on or off
 *
 * @param powered TRUE to turn on or FALSE to turn off
 * @return OK on success or FAIL if WiFi is not present or changing the power
 *         state failed
 */
static RESULT wifi_technology_set_powered(gboolean powered)
{
    RESULT Result = FAIL;

    if (wifi_technology) {
        GError *error = NULL;

        WiFiStatus = BUSY;

        if (connman_technology_call_set_property_sync(wifi_technology,
            "Powered", g_variant_new_variant(g_variant_new_boolean(powered)),
            NULL, &error))
        {
            Result = OK;
        } else {
            g_printerr("Failed to power %s wifi: %s\n", powered ? "on" : "off",
                       error->message);
            g_error_free(error);
        }

        WiFiStatus = OK;
    }

    return Result;
}

/**
 * @brief           Turn off WiFi
 *
 * @return          OK on success or FAIL if wifi is not present or wifi is
 *                  already turned on or turning on failed
 */
RESULT cWiFiTurnOn(void)
{
    pr_dbg("cWiFiTurnOn\n");

    return wifi_technology_set_powered(TRUE);
}

/**
 * @brief           Turn off WiFi
 *
 * @return          OK on success or FAIL if wifi is not present or wifi is
 *                  already turned off or turning off failed
 */
RESULT cWiFiTurnOff(void)
{
    pr_dbg("cWiFiTurnOff\n");

    return wifi_technology_set_powered(FALSE);
}

/**
 * @brief           Handle a change in the wifi technology power property
 */
static void on_wifi_powered_changed(void)
{
    gboolean powered = connman_technology_get_powered(wifi_technology);

    pr_dbg("on_wifi_powered_changed: %d\n", powered);

    if (powered) {
        // cWiFiGetLogicalName();

        WiFiConnectionState = WIFI_INIT;

        WiFiStatus = OK;
    } else {
        BeaconTx = NO_TX;
        cWiFiTcpClose();
        WiFiConnectionState = WIFI_NOT_INITIATED;
        InitState = NOT_INIT;
    }
}

/**
 * @brief           Handle property changes on ConnMan DBus objects.
 *
 * The different types of ConnMan object all use the same property API, so this
 * function can be used by all types of ConnMan objects.
 *
 * @param proxy     The DBus proxy object.
 * @param name      The property name
 * @param value     The property value
 */
static void on_connman_property_changed(GObject *proxy, const gchar *name,
                                        GVariant *value)
{
    GVariant *real_value, *entry, *properties;
    gchar *invalidated = NULL;

    // pr_dbg("PropertyChanged: %s - %s\n", name, g_variant_print(value, TRUE));

    // values are boxed
    real_value = g_variant_get_variant(value);
    g_dbus_proxy_set_cached_property(G_DBUS_PROXY(proxy), name, real_value);

    // trigger notify signal
    entry = g_variant_new_dict_entry(g_variant_new_string(name), value);
    properties = g_variant_new_array(NULL, &entry, 1);
    g_signal_emit_by_name(proxy, "g-properties-changed", properties, &invalidated);

    g_variant_unref(real_value);
}

/**
 * @brief       Get a DBus proxy object for a ConnMan technology.
 *
 * Creates the proxy object and connects signals to handle changes.
 *
 * @param path  The path of the DBus object.
 * @return      The proxy object.
 */
static ConnmanTechnology *cWiFiGetTechnologyProxy(const gchar *path)
{
    ConnmanTechnology *proxy;
    GError *error = NULL;

    proxy = connman_technology_proxy_new_for_bus_sync(G_BUS_TYPE_SYSTEM,
        G_DBUS_PROXY_FLAGS_NONE, "net.connman", path, NULL, &error);
    if (proxy) {
        GVariant *properties;

        // ConnMan does not support org.freedestop.DBus.Properties so we have
        // to take care of this ourselves

        g_signal_connect(proxy, "property-changed",
                         G_CALLBACK(on_connman_property_changed), NULL);
        // There is a race condition where the properties could change before
        // we connect the "property-changed" signal. So, we have to get the
        // properties again to be sure we have the correct values.
        if (connman_technology_call_get_properties_sync(proxy, &properties, NULL, &error)) {
            GVariantIter iter;
            GVariant *item;

            g_variant_iter_init(&iter, properties);
            while ((item = g_variant_iter_next_value(&iter))) {
                GVariant *key = g_variant_get_child_value(item, 0);
                GVariant *value = g_variant_get_child_value(item, 1);

                connman_technology_emit_property_changed(proxy,
                    g_variant_get_string(key, NULL), value);

                g_variant_unref(key);
                g_variant_unref(value);
                g_variant_unref(item);
            }
            g_variant_unref(properties);
        } else {
            g_printerr("Error getting properties for technology: %s\n",
                       error->message);
            g_error_free(error);
        }
    } else {
        g_printerr("Error creating connman technology proxy: %s\n", error->message);
        g_error_free(error);
    }

    return proxy;
}

/**
 * @brief       Handle the removal of the wifi technology.
 *
 * Frees the global instance.
 */
static void on_wifi_technology_removed(void)
{
    g_object_unref(wifi_technology);
    wifi_technology = NULL;
}

/**
 * @brief       Handle the removal of the ethernet technology.
 *
 * Frees the global instance.
 */
static void on_ethernet_technology_removed(void)
{
    g_object_unref(ethernet_technology);
    ethernet_technology = NULL;
}

/**
 * @brief               Handle the addition of a technology.
 *
 * @param object        The ConnMan manager object that received the signal.
 * @param path          The path that was added.
 * @param properties    The technology's properties (ignored)
 */
static void on_technology_added(ConnmanManager *object, gchar *path,
                                GVariant *properties)
{
    GVariant *type;
    const gchar *type_string;

    type = g_variant_lookup_value(properties, "Type", NULL);
    type_string = g_variant_get_string(type, NULL);
    pr_dbg("on_technology_added: %s\n", type_string);

    if (g_strcmp0(type_string, "wifi") == 0) {
        // WiFiStatus = OK;
        wifi_technology = cWiFiGetTechnologyProxy(path);
        g_signal_connect(wifi_technology, "notify::powered",
                         G_CALLBACK(on_wifi_powered_changed), NULL);
        g_object_notify(G_OBJECT(wifi_technology), "powered");
    } else if (g_strcmp0(type_string, "ethernet") == 0) {
        ethernet_technology = cWiFiGetTechnologyProxy(path);
    }
    // TODO: we also need to grab bluetooth to power it on/off.
    // Connman takes control of bluetooth power, so it can't be done
    // from bluez.

    g_variant_unref(type);
}

/**
 * @brief           Handle the removal of a technology.
 *
 * @param object    The ConnMan manager object that received the signal.
 * @param path      The path that was removed.
 */
static void on_technology_removed(ConnmanManager *object, gchar *path)
{
    pr_dbg("on_technology_removed: %s\n", path);

    if (wifi_technology && g_strcmp0(path,
        g_dbus_proxy_get_object_path(G_DBUS_PROXY(wifi_technology))) == 0)
    {
        on_wifi_technology_removed();
    }
    else if (ethernet_technology && g_strcmp0(path,
        g_dbus_proxy_get_object_path(G_DBUS_PROXY(ethernet_technology))) == 0)
    {
        on_ethernet_technology_removed();
    }
}

/**
 * @brief   Get a dbus proxy object for net.connman.Services
 *
 * @return  The proxy object
 */
static ConnmanService *cWiFiGetConnmanServiceProxy(const gchar *path)
{
    ConnmanService *proxy;
    GError *error = NULL;

    proxy = connman_service_proxy_new_for_bus_sync(G_BUS_TYPE_SYSTEM,
        G_DBUS_PROXY_FLAGS_NONE, "net.connman", path, NULL, &error);
    if (proxy) {
        GVariant *properties;

        // ConnMan does not support org.freedestop.DBus.Properties so we have
        // to take care of this ourselves

        g_signal_connect(proxy, "property-changed",
                         G_CALLBACK(on_connman_property_changed), NULL);
        // There is a race condition where the properties could change before
        // we connect the "property-changed" signal. So, we have to get the
        // properties again to be sure we have the correct values.
        if (connman_service_call_get_properties_sync(proxy, &properties, NULL, &error)) {
            GVariantIter iter;
            GVariant *item;

            g_variant_iter_init(&iter, properties);
            while ((item = g_variant_iter_next_value(&iter))) {
                GVariant *key = g_variant_get_child_value(item, 0);
                GVariant *value = g_variant_get_child_value(item, 1);

                connman_service_emit_property_changed(proxy,
                    g_variant_get_string(key, NULL), value);

                g_variant_unref(key);
                g_variant_unref(value);
                g_variant_unref(item);
            }
            g_variant_unref(properties);
        } else {
            g_printerr("Error getting properties for service: %s\n",
                       error->message);
            g_error_free(error);
        }
    } else {
        g_printerr("Error creating connman service proxy: %s\n", error->message);
        g_error_free(error);
    }

    return proxy;
}

static void on_service_state_changed(ConnmanService *proxy)
{
    const char *state = connman_service_get_state(proxy);

    pr_dbg("%s: state changed: %s\n", connman_service_get_name(proxy), state);
}

/**
 * @brief           Compares the object path of a proxy to an object path
 *
 * @param proxy     The proxy instance.
 * @param path      The object path to compare to.
 * @return          0 if path matches the proxy's path.
 */
static gint compare_proxy_path(GDBusProxy *proxy, const gchar *path)
{
    const gchar *proxy_path = g_dbus_proxy_get_object_path(proxy);

    return g_strcmp0(proxy_path, path);
}

/**
 * @brief           Handles service change events
 *
 * Updates the global service_list and sorts it to match the order received
 * from ConnMan.
 *
 * @param object    The ConnMan manager instance.
 * @param changed   A list of services that have changed.
 * @param removed   A list of object paths that have been removed.
 */
static void on_services_changed(ConnmanManager *object, GVariant *changed,
                                GStrv removed)
{
    GVariantIter iter;
    gchar *path;
    GVariant **properties;
    GList *new_list = NULL;

    pr_dbg("on_services_changed\n");

    // handle removed items first to make later search more efficient
    for (; *removed; removed++) {
        GList *match;

        path = *removed;
        match = g_list_find_custom(service_list, path,
                                   (GCompareFunc)compare_proxy_path);
        if (match) {
            service_list = g_list_remove_link(service_list, match);
            g_object_unref(G_OBJECT(match->data));
            g_list_free1(match);
            pr_dbg("removed: %s\n", path);
        } else {
            g_critical("Failed to remove %s", path);
        }
    }

    g_variant_iter_init(&iter, changed);
    while (g_variant_iter_loop(&iter, "(oa{sv})", &path, &properties)) {
        GList *match;

        match = g_list_find_custom(service_list, path,
                                   (GCompareFunc)compare_proxy_path);
        if (match) {
            service_list = g_list_remove_link(service_list, match);
            new_list = g_list_concat(match, new_list);
            pr_dbg("changed: %s\n", path);
        } else {
            ConnmanService *proxy = cWiFiGetConnmanServiceProxy(path);
            new_list = g_list_prepend(new_list, proxy);
            g_signal_connect(proxy, "notify::state",
                             G_CALLBACK(on_service_state_changed), NULL);
            on_service_state_changed(proxy);
            pr_dbg("added: %s\n", path);
        }
        // path and properties are freed by g_variant_iter_loop
    }

    if (new_list) {
        g_warn_if_fail(service_list == NULL);
        service_list = g_list_reverse(new_list);
    }

    if (service_list_state == DATA16_MAX) {
        service_list_state = 1;
    } else {
        service_list_state++;
    }
}

/**
 * @brief   Get a dbus proxy object for net.connman.Manager
 *
 * This also connects signals to the proxy for monitoring technologies and
 * services.
 *
 * @return  The proxy object
 */
static ConnmanManager *cWiFiGetConnmanManagerProxy(void)
{
    ConnmanManager *proxy;
    GError *error = NULL;

    // TODO: Watch dbus for connman
    // It would be better to watch the bus so that we can handle restart of
    // connman without having to restart lms2012.
    proxy = connman_manager_proxy_new_for_bus_sync(G_BUS_TYPE_SYSTEM,
        G_DBUS_PROXY_FLAGS_NONE, "net.connman", "/", NULL, &error);
    if (proxy) {
        GVariant *technologies;
        GVariant *services;

        // We are interested in monitoring technologies

        g_signal_connect(proxy, "technology-added",
                         G_CALLBACK(on_technology_added), NULL);
        g_signal_connect(proxy, "technology-removed",
                         G_CALLBACK(on_technology_removed), NULL);
        if (connman_manager_call_get_technologies_sync(proxy, &technologies, NULL, &error)) {
            GVariantIter iter;
            GVariant *item;

            g_variant_iter_init(&iter, technologies);
            while ((item = g_variant_iter_next_value(&iter))) {
                GVariant *path = g_variant_get_child_value(item, 0);
                GVariant *properties = g_variant_get_child_value(item, 1);

                connman_manager_emit_technology_added(proxy,
                    g_variant_get_string(path, NULL), properties);

                g_variant_unref(path);
                g_variant_unref(properties);
                g_variant_unref(item);
            }
            g_variant_unref(technologies);
        } else {
            g_printerr("Error getting technologies: %s\n", error->message);
            g_error_free(error);
        }

        // We also want to monitor services

        g_signal_connect(proxy, "services-changed",
                         G_CALLBACK(on_services_changed), NULL);
        if (connman_manager_call_get_services_sync(proxy, &services, NULL, &error)) {
            const gchar *removed = NULL;

            connman_manager_emit_services_changed(proxy, services, &removed);
            g_variant_unref(services);
        } else {
            g_printerr("Error getting technologies: %s\n", error->message);
            g_error_free(error);
        }
    } else {
        g_printerr("Error creating connman manager proxy: %s\n", error->message);
        g_error_free(error);
    }

    return proxy;
}

RESULT cWiFiExit(void)
{
    RESULT Result;

    // TODO: Do we want to always turn off WiFi on exit? This is what LEGO does.
    Result = OK; //cWiFiTurnOff();
    if (connman_manager) {
        if (wifi_technology) {
            on_wifi_technology_removed();
        }
        if (ethernet_technology) {
            on_ethernet_technology_removed();
        }
        g_list_free_full(service_list, (GDestroyNotify)g_object_unref);
        service_list = NULL;
        g_object_unref(connman_manager);
        connman_manager = NULL;
    }

    return Result;
}

RESULT cWiFiInit(void)
{
    RESULT Result = FAIL;

    pr_dbg("\ncWiFiInit START %d\n", WiFiStatus);

    BeaconTx = NO_TX;
    // We're sleeping until user select ON
    WiFiConnectionState = WIFI_NOT_INITIATED;
    InitState = NOT_INIT;
    TcpReadState = TCP_IDLE;
    cWiFiSetBtSerialNo();
    cWiFiSetBrickName();
    connman_manager = cWiFiGetConnmanManagerProxy();
    if (connman_manager) {
        Result = OK;
    }

    pr_dbg("\nWiFiStatus = %d\n", WiFiStatus);

    return Result;
}
