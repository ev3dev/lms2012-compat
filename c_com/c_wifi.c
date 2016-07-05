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

#include "c_wifi.h"


// Volatile data
// Used for search AP's (all active - known/new)
#define MAX_AP_ENTRIES 16
static aps ApTable[MAX_AP_ENTRIES];
static int ApTableSize = 0;

// Persistent data
// Used for storing OLD and KNOWN AP's
#define MAX_AP_STORAGE_ENTRIES 16
static aps ApStoreTable[MAX_AP_STORAGE_ENTRIES];
static int ApStoreTableSize; 

static aps TempApStorage;     // As the name says - used when moving around in the list(s)

static char BtSerialNo[13];   // Storage for the BlueTooth Serial Number

static char BrickName[NAME_LENGTH + 1];  // BrickName for discovery and/or friendly info

static char MyIp4Address[16]; // Our / "The local IP" address

static char MyBroadCastAdr[16]; // Broadcast address (the MASK included)

static char MyHwMacAddress[18]; // HardWare MAC address

static char LogicalIfName[6]; //Only wlan0->wlan9 + /0x0

/*// Known friends - i.e. friendly pc's etc. STORED as a PASS-NAME
#define MAX_ACCESS_NAME_LENGTH 20
#define MAX_ACCESS_NAME_COUNT 20
static char AccessNameTable[MAX_ACCESS_NAME_COUNT][MAX_ACCESS_NAME_LENGTH];
*/

#define WPA_SUPPLICANT_GLOBAL_SOCKET    "/var/run/wpa_supplicant-global"
#define WPA_SUPPLICANT_CONTROL_SOCKET   "/var/run/wpa_supplicant"

// The HARDWARE_SEARCH_STRING is hardcoded for now - future releases will/should have
// the HARDWARE_SEARCH_STRINGS (S!) loaded from a file which has an ASCII definition
// of available H/W and a description of the drivers to load. I.e. a simple and download-
// able way of integrating new and more drivers. NO commandline stuff.

#define HARDWARE_SEARCH_STRING_01         "ID 0846:9030 NetGear, Inc."
#define WIFI_VENDOR_ID_01    "0846"
#define WIFI_DONGLE_ID_01    "9030"

#define HARDWARE_SEARCH_STRING_02         "ID 7392:7811 Edimax Technology Co., Ltd"
#define WIFI_VENDOR_ID_02    "7392"
#define WIFI_DONGLE_ID_02    "7811"


// The following could be set to variable/editable strings via methodes, but for now....

//static char* ControlPath = "/var/run/wpa_supplicant/";
//static char* CtrlConfigPathName = "/etc/wpa_supplicant.conf";

#define WPA_APPLIKATION_PATH            ""  // Not defined yet

static struct wpa_ctrl *ctrl_conn;      // Control "handle" for the wpa_control interface

struct timeval TimerStartVal, TimerCurrentVal;
struct timeval DongleCheckStartVal, DongleCheckCurrentVal;

unsigned int TimeOut = 0;

char Buffer[1024];
char *BufPtr = Buffer;
int BufLen = sizeof(Buffer);

KNOWN_HW KnownWiFiDongle = HW_NOT_KNOWN;

UBYTE TcpReadBuffer[1024];
uint TcpReadBufPointer = 0;

WIFI_STATE WiFiConnectionState = WIFI_NOT_INITIATED;
WIFI_INIT_STATE InitState = NOT_INIT;
// UDP
int UdpSocketDescriptor = 0;
char Server[32];  // PlaceHolder for the UDP listen Server Name/IP
int UdpRxCount, UdpTxCount;
struct sockaddr_in ServerAddr;
socklen_t ServerAddrLen = sizeof(ServerAddr);
fd_set UdpReadFlags, UdpWriteFlags;
struct timeval UdpWait;
int BroadCast = 1;
BEACON_MODE BeaconTx = NO_TX;
TCP_STATES TcpState = TCP_DOWN;
int WiFiStatus = OK;
RESULT WiFiOnStatus = FAIL;
int TcpConnectionSocket = 0;                /*  connection socket         */
struct sockaddr_in servaddr, client_addr;
int TCPListenServer = 0;
UWORD TcpTotalLength = 0;
UWORD TcpRestLen = 0;
UBYTE TcpReadState = TCP_IDLE;

// ******************************************************************************

// Start the Timer
void cWiFiStartTimer(void)
{
    gettimeofday(&TimerStartVal, NULL);
}

RESULT cWiFiKillUdHcPc(void)
{
    RESULT Ret = FAIL;
    char Cmd[48];

    // Only one instance at a time :-)

#ifdef DEBUG
    printf("We'll try to kill udhcpc\n");
#endif
    strcpy(Cmd, "killall udhcpc &> /dev/null");

#ifdef DEBUG
    printf("\nCmd = %s\n", Cmd);
#endif

    if (system(Cmd) == 0) {
#ifdef DEBUG
        printf("\nThe kill Cmd returns OK!!\n");
#endif
        Ret = OK;
    }

    return Ret;
}

void cWiFiStartDongleCheckTimer(void)
{
    // Get starttime
    gettimeofday(&DongleCheckStartVal, NULL);
}

int cWiFiTimeFromLastDongleCheck(void)
{
    // Get elapsed time from last dongle check
    gettimeofday(&DongleCheckCurrentVal, NULL);

    return (int)(DongleCheckCurrentVal.tv_sec - DongleCheckStartVal.tv_sec);
}

// Get Elapsed time in seconds
int cWiFiCheckTimer(void)
{
    // Get actual time and calculate elapsed time
    gettimeofday(&TimerCurrentVal, NULL);

    return (int)(TimerCurrentVal.tv_sec - TimerStartVal.tv_sec);
}

// Used primarily for debug (development :-))
void wpa_control_message_callback(char *message, size_t length)
{
#ifdef DEBUG
    printf("%s\n", message);
#endif
}

int do_wpa_command(struct wpa_ctrl *control, char *command)
{
    char command_return[10];
    // Leave space for a terminating ASCIIZ
    size_t len_command_return = sizeof(command_return) - 1;
    int ret_value;

    memset(command_return, 0x00, len_command_return); // Reset to be sure!!

    // add more "nanny"ware look into good ref.... 8-)
    if (control != NULL) {
        ret_value = wpa_ctrl_request(control, command, strlen(command),
                                     command_return, &len_command_return,
                                     wpa_control_message_callback);

        command_return[len_command_return] = '\0';
    } else {
        ret_value = -1;
    }

    return ret_value;
}

// Opens - i.e. startup the WpaSupplicant in the background
int cWiFiStartWpaSupplicant(char *ConfigPath, char *IFName)
{
    // Fire up the WPA Supplicant - returns 0 (zero) if OK
    char StartCmd[128];

    // Build the command string for startting the Control
    strcpy(StartCmd, "./wpa_supplicant -Dwext -i");
    strcat(StartCmd, IFName);
    strcat(StartCmd, " -c");
    strcat(StartCmd, ConfigPath);
    strcat(StartCmd, " -B");
    strcat(StartCmd, "&> /dev/null");

#ifdef DEBUG
    printf("\nStart WPA_Supplicant: %s\n", StartCmd);
#endif

    return system(StartCmd);
}

struct wpa_ctrl *cWiFiOpenCtrlInterFace(char *CtrlPath, char *InterFace)
{
    char OpenCtrlCmd[64];

    strcpy(OpenCtrlCmd, CtrlPath);
    strcat(OpenCtrlCmd, InterFace);

    return wpa_ctrl_open(OpenCtrlCmd);
}

// Read already know "stuff" from the persistent storage at startup
int cWiFiPopulateKnownApList(void)
{
    int RetVal = 0;
    char FileName[128];
    FILE *PersistentFile = NULL;
    int ArrayIterator = 0;
    aps OneApRecord;

    strcpy(FileName, WIFI_PERSISTENT_PATH);
    strcat(FileName, "/");
    strcat(FileName, WIFI_PERSISTENT_FILENAME);

    ApStoreTableSize = 0; // ZERO size - NO known - if none in file
    PersistentFile = fopen(FileName, "rb");

#ifdef DEBUG
    printf("\nKnownlist:\n");
#endif

    if (PersistentFile) {
        while (fread(&OneApRecord, sizeof OneApRecord, 1, PersistentFile) == 1) {
            memcpy(&(ApStoreTable[ArrayIterator++]), &OneApRecord, sizeof(aps));
#ifdef DEBUG
            printf("Populating ApStoreTable[x] == KnownList\n\r");
            printf("\nApStoreTable[%d].mac_address = %s\n", (ArrayIterator - 1),
                   ApStoreTable[ArrayIterator - 1].mac_address);
            printf("\nApStoreTable[%d].security = %s\n", (ArrayIterator - 1),
                   ApStoreTable[ArrayIterator - 1].security);
            printf("\nApStoreTable[%d].friendly_name = %s\n", (ArrayIterator - 1),
                   ApStoreTable[ArrayIterator - 1].friendly_name);
            printf("\nApStoreTable[%d].pre_shared_key = %s\n", (ArrayIterator - 1),
                   ApStoreTable[ArrayIterator - 1].pre_shared_key);
            printf("\nApStoreTable[%d].ap_flags = %d\n", (ArrayIterator - 1),
                   ApStoreTable[ArrayIterator - 1].ap_flags);
#endif
        }
        fclose(PersistentFile);
        ApStoreTableSize = ArrayIterator; // Store the KNOWN table size

        //******************************* TEST STUFF ****************************
        //
        // Used for a stable "environment" - i.e. the AP's are stable and always there :-)
        //
//#define TEST
#undef TEST
#ifdef TEST
        printf("\nSimulated KnownList:\n");

        ApStoreTableSize = 5;

        strcpy(ApStoreTable[0].mac_address, "00:24:01:43:69:6e");
        strcpy(ApStoreTable[0].security, "[WPA2-PSK-CCMP][WPS][ESS]");
        strcpy(ApStoreTable[0].friendly_name, "FFF");
        strcpy(ApStoreTable[0].pre_shared_key,
               "dbc9c6cf6d4d8e9405842d2e4ab726f33f10de62c30254ff19512af0f");
        ApStoreTable[0].ap_flags = 0x0C;

        strcpy(ApStoreTable[1].mac_address, "00:24:01:43:69:01");
        strcpy(ApStoreTable[1].security, "[WPA2]");
        strcpy(ApStoreTable[1].friendly_name, "OOO");
        strcpy(ApStoreTable[1].pre_shared_key, "");
        ApStoreTable[1].ap_flags = 0x0C;

        strcpy(ApStoreTable[2].mac_address, "00:24:01:43:69:02");
        strcpy(ApStoreTable[2].security, "[WPA2-PSK-CCMP]");
        strcpy(ApStoreTable[2].friendly_name, "AAA");
        strcpy(ApStoreTable[2].pre_shared_key, "");
        ApStoreTable[2].ap_flags = 0x0C;

        strcpy(ApStoreTable[3].mac_address, "00:24:01:43:69:03");
        strcpy(ApStoreTable[3].security, "[WPA2-PSK-CCMP][ESS]");
        strcpy(ApStoreTable[3].friendly_name, "JJJ");
        strcpy(ApStoreTable[3].pre_shared_key, "");
        ApStoreTable[3].ap_flags = 0x0C;

        strcpy(ApStoreTable[4].mac_address, "00:24:01:43:69:04");
        strcpy(ApStoreTable[4].security, "[WPA2-PSK][ESS]");
        strcpy(ApStoreTable[4].friendly_name, "KKK");
        strcpy(ApStoreTable[4].pre_shared_key, "");
        ApStoreTable[4].ap_flags = 0x0C;

        /*strcpy(ApStoreTable[5].mac_address, "00:24:01:43:69:05");
        strcpy(ApStoreTable[5].security, "");
        strcpy(ApStoreTable[5].friendly_name, "Hugo05");
        strcpy(ApStoreTable[5].pre_shared_key, "");
        ApStoreTable[5].ap_flags = 0x08;*/
#endif
    //*************************** End of TEST STUFF ****************************

    } else {
        RetVal = -1;
    }

    return RetVal;
}

// At exit of main application
// Store known "stuff" in the (new) correct order
int cWiFiStoreKnownApList(void)
{
    int RetVal = 0;
    char FileName[128];
    FILE *PersistentFile = NULL;
    int ArrayIterator = 0;
    aps OneApRecord;

    strcpy(FileName, WIFI_PERSISTENT_PATH);
    strcat(FileName, "/");
    strcat(FileName, WIFI_PERSISTENT_FILENAME);

#ifdef DEBUG
    printf("AP_FLAG_ADJUST_FOR_STORAGE = %X\n\r", AP_FLAG_ADJUST_FOR_STORAGE);
    printf("Persistent FileName = %s ApStoreTableSize = %d\n\r", FileName,
           ApStoreTableSize);
#endif

    PersistentFile = fopen(FileName, "wb");
    if (PersistentFile) {
        for(ArrayIterator = 0; ArrayIterator < ApTableSize; ArrayIterator++) {

            // if (((ApStoreTable[ArrayIterator].ap_flags) & KNOWN) == KNOWN) NOT USED only HARD known ones
            if (((ApTable[ArrayIterator].ap_flags) & KNOWN) == KNOWN) {
                //memcpy(&OneApRecord, &(ApStoreTable[ArrayIterator]), sizeof(aps)); NOT USED only HARD known ones
                memcpy(&OneApRecord, &(ApTable[ArrayIterator]), sizeof(aps));

                OneApRecord.ap_flags &= AP_FLAG_ADJUST_FOR_STORAGE;

#ifdef  DEBUG
                printf("cWiFiStoreKnownApList ApStoreTableSize = %d, ArrayIterator = %d\n\r",
                       ApStoreTableSize, ArrayIterator);
                printf("OneApRecord.ap_flags before: %X\n\r", OneApRecord.ap_flags);
#endif

                fwrite(&OneApRecord, sizeof OneApRecord, 1, PersistentFile);
            }

#ifdef DEBUG
            printf("OneApRecord.ap_flags after, just after save: %X\n\r",
                   OneApRecord.ap_flags);
            printf("Writing KnownList I.e. ApStoreTable[x] to Persistent file\n\r");
            printf("\nApStoreTable[%d].mac_address = %s\n", (ArrayIterator),
                   ApStoreTable[ArrayIterator].mac_address);
            printf("\nApStoreTable[%d].security = %s\n", (ArrayIterator),
                   ApStoreTable[ArrayIterator].security);
            printf("\nApStoreTable[%d].friendly_name = %s\n", (ArrayIterator),
                   ApStoreTable[ArrayIterator].friendly_name);
            printf("\nApStoreTable[%d].pre_shared_key = %s\n", (ArrayIterator),
                   ApStoreTable[ArrayIterator].pre_shared_key);
            printf("\nApStoreTable[%d].ap_flags = %d\n", (ArrayIterator),
                   ApStoreTable[ArrayIterator].ap_flags);
#endif
        }
        //RetVal = ApStoreTableSize;NOT USED only HARD known ones
        RetVal = ApTableSize;
        fclose(PersistentFile);
    } else {
#ifdef DEBUG
        printf("Handle to PersistentFile == NULL :-(\n\r");
#endif

        RetVal = -1;
    }

    return RetVal;
}

// Restore a preserved record to ACTUAL
void cWiFiRestoreActualApRecord(int Destination)
{
    memcpy(&(ApTable[Destination].mac_address), &TempApStorage, sizeof(aps));
}

// Preserve an ACTUAL record
void cWiFiPreserveActualApRecord(int Index)
{
    memcpy(&TempApStorage, &(ApTable[Index].mac_address), sizeof(aps));
}

void cWiFiMoveUpInStoreList(int Index)
{
    // Make entry "n" become "n-1" i.e. index--
    if (Index > 0) {
        memcpy(&(ApStoreTable[Index - 1]), &(ApStoreTable[Index]), sizeof(aps));
    }
}

void cWiFiMoveDownInStoreList(int Index)
{
    // Make entry "n" become "n+1" i.e. index++
    if (Index < (MAX_AP_STORAGE_ENTRIES - 2)) {
        memcpy(&(ApStoreTable[Index + 1]), &(ApStoreTable[Index]), sizeof(aps));
    }
}

void cWiFiMoveUpInActualList(int Index)
{
    // Make "n" become "n-1" - lower index, but higher order (newer/active etc.)
    if (Index > 0) {
        memcpy(&(ApTable[Index - 1]), &(ApTable[Index]), sizeof(aps));
    }
}

void cWiFiMoveDownInActualList(int Index)
{
    // Make "n" become "n+1" - lower priority (older/inactive etc.)
    if (Index < (MAX_AP_ENTRIES - 2)) {
        memcpy(&(ApTable[Index + 1]), &(ApTable[Index]), sizeof(aps));
    }
}

// Direct UI function
void cWiFiMoveUpInList(int Index)
{
    // Make "n" become "n-1" and "n-1" moved down to "n"

     if (Index > 0) {
        // We can move up
        cWiFiPreserveActualApRecord(Index - 1);
        // n to n-1
        memcpy(&(ApTable[Index - 1]), &(ApTable[Index]), sizeof(aps));
        cWiFiRestoreActualApRecord(Index);
    }
}

RESULT cWiFiRemoveNetwork(void)
{
    RESULT Ret = FAIL;
    char CmdReturn[10];
#ifdef DEBUG
    int RetVal;
#endif
    // We leave space for a terminating /0x00
    size_t LenCmdReturn = sizeof(CmdReturn) - 1;

#ifdef DEBUG
    printf("Remove Network called\n\r");
#endif

    memset(CmdReturn, 0x00, LenCmdReturn); // Reset to be sure!!

    if (ctrl_conn)
    {
#ifdef DEBUG
        RetVal = 
#endif
        wpa_ctrl_request(ctrl_conn, "REMOVE_NETWORK all",
                         strlen("REMOVE_NETWORK all"),
                         CmdReturn, &LenCmdReturn, NULL);
        sleep(3); // Force some cycles
        CmdReturn[LenCmdReturn] = '\0';

#ifdef DEBUG
        printf("Remove returns %s - RetVal = %d\n\r", CmdReturn, RetVal);
#endif

        if (strstr(CmdReturn, "OK")) {
#ifdef DEBUG
            printf("Remove Network returned OK\n\r");
#endif
            Ret = OK;
        } else {
#ifdef DEBUG
            printf("Remove Network was NOT OK :-(\n\r");
#endif
        }
    }

    return Ret;
}

RESULT cWiFiTcpClose(void)
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
#ifdef DEBUG
            printf("In the do_while\n");
#endif
            res = read(TcpConnectionSocket, buffer, 100);
            if (res < 0 ) {
                break;
            }
        } while (res != 0);

#ifdef DEBUG
        printf("\nError calling Tcp shutdown()\n");
#endif
    }

    TcpConnectionSocket = 0;
    WiFiStatus = OK;  // We are at a known state :-)
    WiFiConnectionState = UDP_VISIBLE;

    return Result;
}

void cWiFiUdpClientClose(void)
{
    WiFiStatus = FAIL;                          // Async announcement of FAIL
    WiFiConnectionState = WIFI_INITIATED;
    BeaconTx = NO_TX;                           // Disable Beacon
    if (close(UdpSocketDescriptor) == 0) {
        WiFiStatus = OK;                          // Socket kill
    }
}

// Direct UI function
void cWiFiMoveDownInList(int Index)
{
    // Make "n" become "n+1" and "n+1" moved up to "n"
    if (Index < (MAX_AP_ENTRIES - 2)) {
        cWiFiPreserveActualApRecord(Index + 1);
        memcpy(&(ApTable[Index + 1]), &(ApTable[Index]), sizeof(aps));
        cWiFiRestoreActualApRecord(Index);
    }
}

// Direct UI function
// Delete "n" - making "n+1, n+2" => "n, n+1"
void cWiFiDeleteInList(int Index)
{
    int i;

    WiFiStatus = FAIL; // Until other....

    // DO NOT outside or ACTIVE!!
    if ((Index < ApTableSize) && ((ApTable[0].ap_flags & CONNECTED) != CONNECTED)) {
        for (i = Index + 1; i < (ApTableSize + 1); i++) {
            memcpy(&(ApTable[i - 1]), &(ApTable[i]), sizeof(aps));
        }
        ApTableSize--;
        WiFiStatus = OK; // Somecleaning done :-)
    }
}

// Delete "n" - making "n+1, n+2" => "n, n+1"
void cWiFiDeleteInStoreList(int Index)
{
    int i;

    if (Index < ApStoreTableSize) {
      for (i = Index; i < (ApStoreTableSize - 1); i++) {
          memcpy(&(ApStoreTable[i]), &(ApStoreTable[i + 1]), sizeof(aps));
      }
      ApStoreTableSize--;
    }
}

// Lower priority
void cWifiMoveAllActualDown(int SourcePointer)
{
    int i;

    i = SourcePointer - 1;
    do {
        cWiFiMoveDownInActualList(i--);
    } while(i >= 0);
}

// Lower priority
void cWiFiMoveAllStoreDown(int SourcePointer)
{
    int i;

    i = SourcePointer - 1;
    do {
        cWiFiMoveDownInStoreList(i--);
    } while(i >= 0);
}

// Move from STORED to ACTUAL
void cWiFiCopyStoreToActual(int StoreIndex, int ActualIndex)
{
    memcpy(&(ApTable[StoreIndex]), &(ApStoreTable[ActualIndex]), sizeof(aps));
}

// Move from ACTUAL to STORED
void cWiFiCopyActualToStore(int ActualIndex, int StoreIndex)
{
    memcpy(&(ApStoreTable[StoreIndex]), &(ApTable[ActualIndex]), sizeof(aps));
}

// Check for entry in STORED list - if present -> DELETE it
RESULT cWiFiCheckAndDelete(int Index)
{
    int FoundIndex;
    RESULT Found = FAIL;
    int i;

    for (i = 0; i < ApStoreTableSize; i++) {
        if (strcmp(ApTable[Index].mac_address, ApStoreTable[i].mac_address) == 0) {
            FoundIndex = i;
            Found = OK;
        }
    }
    if (Found == OK) {
        cWiFiDeleteInStoreList(FoundIndex);
        // If found and deleted
        ApStoreTableSize--;
    }

    return Found; // true if FOUND and DELETED
}

// When a connection is made to a new AP
void cWiFiAddToKnownApList(int Index)
{
    // Place in Persistent data as TOP item - ALWAYS
    int SourcePointer;

    // First adjust for storage - is the table already full??
    if (ApStoreTableSize < (MAX_AP_STORAGE_ENTRIES - 1)) {
        SourcePointer = ApStoreTableSize;
    } else {
        SourcePointer = ApStoreTableSize-1;
    }

    // Move all entries one down
    cWiFiMoveAllStoreDown(SourcePointer);
    // Copy to TOP
    cWiFiCopyActualToStore(Index, 0);

    ApStoreTableSize++;
    if (ApStoreTableSize > (MAX_AP_STORAGE_ENTRIES - 1)) {
        ApStoreTableSize = (MAX_AP_STORAGE_ENTRIES - 1);
    }
}

// Preserve a STORED record
void cWiFiPreserveStorageApRecord(int Index)
{
    memcpy(&TempApStorage, &(ApStoreTable[Index].mac_address), sizeof(aps));
}

// Restore a preserved record to STORED
void cWiFiRestoreStorageApRecord(int Destination)
{
    memcpy(&(ApStoreTable[Destination].mac_address), &TempApStorage, sizeof(aps));
}

// Merge newly scanned and visible AP's with the KNOWN LIST
// Prioritized - LAST used first, second used next (every-
// thing visible and known) also prioritized first.
// 1) KNOWN and VISIBLE first
// 2) VISIBLE
// 3) KNOWN but NOT visible
RESULT cWiFiMergeActualAndKnownTable(void)
{
    int KnownListIterator;
    int VisibleListIterator;
    RESULT Found = FAIL;
    int KnownCounter = 0; // Points into the KNOWN part of the scanned
    int i;

    for (KnownListIterator = 0; KnownListIterator < ApStoreTableSize; KnownListIterator++) {
        VisibleListIterator = 0;
        Found = FAIL;

        do {
            if (strcmp(ApTable[VisibleListIterator].mac_address,
                       ApStoreTable[KnownListIterator].mac_address) == 0)
            {   // Known and visible
                // Move this Known to ApTable[KnownListIterator]

                // First Save entry @ ApTable[VisibleListIterator]

                // Then move all between KnownCounter (topmost) and VisibleListIterator (lowest) 1 position down

                for(i = VisibleListIterator; i > KnownCounter; i--) {
                    cWiFiMoveDownInActualList(i - 1);
                }

                cWiFiCopyStoreToActual(KnownListIterator, KnownCounter);  // Move from STORED to ACTUAL

                ApTable[KnownCounter].ap_flags |=  (KNOWN + VISIBLE);     // Set flags

                Found = OK;
                KnownCounter++; // Used for populate rest of list with KNOWN if any
            }

            if (Found == FAIL) {
                VisibleListIterator++;
                if (VisibleListIterator >= MAX_AP_ENTRIES) {
                    Found = OK; // Even if NOT found
                    // Add to KNOWN but NOT visible in bottom of list
                    cWiFiCopyStoreToActual(ApTableSize, KnownListIterator);

                    ApTable[ApTableSize].ap_flags |=  KNOWN;
                    ApTable[ApTableSize].ap_flags &= ~(CONNECTED);
                    ApTable[ApTableSize].ap_flags &= ~(VISIBLE);
                    ApTableSize++;
                }
            }
        }
        while(Found == FAIL);
    }

    return OK;
}

RESULT cWiFiTerminate(void)
{
    RESULT Ret = FAIL;
    char CmdReturn[10];
#ifdef DEBUG
    int RetVal;
#endif
    // We leave space for a terminating /0x00
    size_t LenCmdReturn = sizeof(CmdReturn) - 1;

    memset(CmdReturn, 0x00, LenCmdReturn); // Reset to be sure!!

    if (ctrl_conn) {
#ifdef DEBUG
        RetVal =
#endif
        wpa_ctrl_request(ctrl_conn, "TERMINATE", strlen("TERMINATE"), CmdReturn,
                         &LenCmdReturn, NULL);
        CmdReturn[LenCmdReturn] = '\0';

#ifdef DEBUG
        printf("WiFi terminate : RetVal %d , CmdReturn %s\n\r", RetVal, CmdReturn);
#endif

        if (strstr(CmdReturn, "OK") != NULL) {
            Ret = OK;
        }
    }

    return Ret;
}

RESULT cWiFiAddNetwork(void)
{
    RESULT Ret = FAIL;
    char aCmdReturn[128];
#ifdef DEBUG
    int RetVal;
#endif
    size_t LenaCmdReturn = sizeof(aCmdReturn) - 1; // We leave space for a terminating /0x00

    memset(aCmdReturn, 0x00, LenaCmdReturn); // Reset to be sure!!

#ifdef DEBUG
    printf("Beginning of AddNetwork\n\r");
#endif

    if (ctrl_conn) {
#ifdef DEBUG
        printf("Internal in AddNetwork - before the real call\n\r");
#endif

#ifdef DEBUG
        RetVal =
#endif
        wpa_ctrl_request(ctrl_conn, "ADD_NETWORK", strlen("ADD_NETWORK"),
                         aCmdReturn, &LenaCmdReturn, NULL);
        sleep(3); // Force some cycles
        aCmdReturn[LenaCmdReturn] = '\0';

#ifdef DEBUG
        printf("Add returns %s - RetVal = %d\n\r", aCmdReturn, RetVal);
#endif

        if ((aCmdReturn[0] == '0') && (aCmdReturn[1] == 10)) { // LF
#ifdef DEBUG
            printf("0 (zero) in return at AddNetwork\n\r");
#endif

            Ret = OK;
        } else {
#ifdef DEBUG
            printf("AddNetwork NOT OK 0 = %d, 1 = %d !!\n", aCmdReturn[0], aCmdReturn[1]);
#endif
        }
    }

    return Ret;
}

// The WPA_Supplicant should also be happy, when APs are invisible
RESULT cWiFiSetScanSsidToOne(void)
{
    RESULT Ret = FAIL;
    char CmdReturn[10];
    char Cmd[128];
    // We leave space for a terminating /0x00
    size_t LenCmdReturn = sizeof(CmdReturn) - 1;

    WiFiStatus = BUSY;
    memset(CmdReturn, 0x00, LenCmdReturn); // Reset to be sure!!

#ifdef DEBUG
    printf("\n\rEntry of SET_ScanSsidToOne to ONE\n\r");
#endif

    if (ctrl_conn) {
        strcpy(Cmd, "SET_NETWORK 0 scan_ssid 1");

        wpa_ctrl_request(ctrl_conn, Cmd, strlen(Cmd), CmdReturn, &LenCmdReturn, NULL);

        CmdReturn[LenCmdReturn] = '\0';

#ifdef DEBUG
        printf("ap_scan: CMD= %s, Return= %s", Cmd, CmdReturn);
#endif

        if (strstr(CmdReturn, "OK") != NULL) {
            Ret = OK;
            WiFiStatus = OK;
        } else {
            WiFiStatus = FAIL;
        }
    }

    return Ret;
}

RESULT cWiFiSetSsid(char *Ssid)
{
    RESULT Ret = FAIL;
    char CmdReturn[10];
    char Cmd[128];
    // We leave space for a terminating /0x00
    size_t LenCmdReturn = sizeof(CmdReturn) - 1;

    WiFiStatus = BUSY;
    memset(CmdReturn, 0x00, LenCmdReturn); // Reset to be sure!!

#ifdef DEBUG
    printf("\n\rEntry of SET_SSID in c_wifi\n\r");
#endif

    if (ctrl_conn) {
        strcpy(Cmd, "SET_NETWORK 0 ssid \"");
        strcat(Cmd, Ssid);
        strcat(Cmd, "\"");

        wpa_ctrl_request(ctrl_conn, Cmd, strlen(Cmd), CmdReturn, &LenCmdReturn,
                         NULL);

        CmdReturn[LenCmdReturn] = '\0';

#ifdef DEBUG
        printf("SetSSID: CMD= %s, Return= %s", Cmd, CmdReturn);
#endif

        if (strstr(CmdReturn, "OK") != NULL) {
            Ret = OK;
            WiFiStatus = OK;
        } else {
            WiFiStatus = FAIL;
        }
    }

    return Ret;
}

RESULT cWiFiSetKeyManagToWpa2(void)
{
    RESULT Ret = FAIL;
    char CmdReturn[10];
    // We leave space for a terminating /0x00
    size_t LenCmdReturn = sizeof(CmdReturn) - 1;

    memset(CmdReturn, 0x00, LenCmdReturn); // Reset to be sure!!

    if (ctrl_conn) {
        wpa_ctrl_request(ctrl_conn, "SET_NETWORK 0 key_mgmt WPA-PSK",
                         strlen("SET_NETWORK 0 key_mgmt WPA-PSK"), CmdReturn,
                         &LenCmdReturn, NULL);

        CmdReturn[LenCmdReturn] = '\0';
        if (strstr(CmdReturn, "OK") != NULL) {
            Ret = OK;
        }
    }

    return Ret;
}

RESULT cWiFiSetKeyManagToNone(void)
{
    RESULT Ret = FAIL;
    char CmdReturn[10];
    // We leave space for a terminating /0x00
    size_t LenCmdReturn = sizeof(CmdReturn) - 1;

    memset(CmdReturn, 0x00, LenCmdReturn); // Reset to be sure!!

    if (ctrl_conn) {
        wpa_ctrl_request(ctrl_conn, "SET_NETWORK 0 key_mgmt NONE",
                         strlen("SET_NETWORK 0 key_mgmt NONE"), CmdReturn,
                         &LenCmdReturn, NULL);

        CmdReturn[LenCmdReturn] = '\0';
        if (strstr(CmdReturn, "OK") != NULL) {
            Ret = OK;
        }
    }

    return Ret;
}

RESULT cWiFiSetPsk(char *Psk)
{
    RESULT Ret = FAIL;
    char CmdReturn[10];
    char Cmd[128];
    // We leave space for a terminating /0x00
    size_t LenCmdReturn = sizeof(CmdReturn) - 1;

    memset(CmdReturn, 0x00, LenCmdReturn); // Reset to be sure!!

    if (ctrl_conn) {
        strcpy(Cmd, "SET_NETWORK 0 psk ");
        strcat(Cmd, Psk);

        wpa_ctrl_request(ctrl_conn, Cmd, strlen(Cmd), CmdReturn, &LenCmdReturn,
                         NULL);

        CmdReturn[LenCmdReturn] = '\0';
        if (strstr(CmdReturn, "OK") != NULL) {
            Ret = OK;
      }
    }

    return Ret;
}

RESULT cWiFiGetApMacAddr(char* MacAddr, int Index)
{
  // Get the MAC address of the Indexed AP
  RESULT Result = FAIL;
  if(!(Index > (ApTableSize - 1)))
  {
    strcpy(MacAddr, ApTable[Index].mac_address);
    Result = OK;
  }
  return Result;
}

void cWiFiSetEncryptToWpa2(int Index)
{
  WiFiStatus = BUSY;  // NOT async, but if...

  ApTable[Index].ap_flags |= WPA2;

  #ifdef DEBUG
    printf("\nApTable[Index = %d].security before = %s\n", Index, ApTable[Index].security);
  #endif

  strcpy(ApTable[Index].security, "WPA2");

  #ifdef DEBUG
    printf("\nApTable[Index = %d].security after = %s\n", Index, ApTable[Index].security);
  #endif

    WiFiStatus = OK; // Clean properety settings
}

void cWiFiSetEncryptToNone(int Index)
{
  WiFiStatus = BUSY;  // NOT async, but if...
  ApTable[Index].ap_flags &= ~WPA2;
  strcpy(ApTable[Index].security, "NONE");
  WiFiStatus = OK;    // Clean properety settings
}

void cWiFiSetKnown(int Index)
{
  ApTable[Index].ap_flags |= KNOWN;
}

void cWiFiDeleteAsKnown(int LocalIndex)
{
  // Leave entry only remove the KNOWN flag
  WiFiStatus = OK;
  ApTable[LocalIndex].ap_flags &= ~KNOWN;
}

RESULT cWiFiAddHidden(char *HiddenApName, char *Security, char *PassWord)
{
  RESULT Result = FAIL;
  int Index = ApTableSize;

      if(ApTableSize < MAX_AP_ENTRIES)
      {
        Index = ApTableSize++;
        if(ApTableSize >= MAX_AP_ENTRIES) // Can only be <=, but....
          ApTableSize = MAX_AP_ENTRIES;
      }
      else
      {
        ApTableSize = MAX_AP_ENTRIES;     // To be sure..
        Index = MAX_AP_ENTRIES - 1;
      }

      ApTable[Index].ap_flags = 0;        // Clear flags - valid values set below
      strcpy(ApTable[Index].friendly_name, HiddenApName);

      if(strstr(Security, "WPA2") != NULL)
      {
        ApTable[Index].ap_flags |= WPA2;
        strcpy(ApTable[Index].security, "WPA2");  // Only exact
      }

      ApTable[Index].ap_flags |= KNOWN;           // It's hidden so.... we must know it
                                                  // else we would not be here ;-)

      if(cWiFiMakePsk(HiddenApName, PassWord, Index) == OK)
      {
        Result = OK;
      }

  return Result;
}

RESULT cWiFiGetHiddenMacAddr(char* MacAddr, int Index)
{
  // Get the MAC address of the Indexed "HIDDEN" AP
  RESULT Result = FAIL;
  char IwConfigCmd[32];
  FILE *FilePointer;

  if(!(Index > (ApTableSize - 1)))
  {
    // First get the "HIDDEN" MAC addr.
    strcpy(IwConfigCmd, "iwconfig");
    strcat(IwConfigCmd, " ");
    strcat(IwConfigCmd, LogicalIfName);
    FilePointer   = popen(IwConfigCmd, "r");
    if (FilePointer)
    {
      char *OneLine = NULL;
      char *TempPointer;
      size_t NumberOfBytes;

      while((getline(&OneLine, &NumberOfBytes, FilePointer) > 0) && OneLine)
      {
        if((OneLine = strstr(OneLine, "Access Point:")) > 0)
        {
          OneLine += 14;
          TempPointer = OneLine + 17;
          *TempPointer = '\0';
          strcpy(MacAddr, OneLine);
          Result = OK;
          break;
        }
      }
    }
    pclose(FilePointer);
    // Store it and return it
    strcpy(ApTable[Index].mac_address, MacAddr);
    Result = OK;
  }
  return Result;
}

RESULT  cWiFiGetIpAddr(char* IpAddress)
{
  RESULT Result = FAIL;

  if(strstr(MyIp4Address, "???") == NULL)
  {
    strcpy(IpAddress, MyIp4Address);
    Result = OK;
  }
  return Result;
}

RESULT  cWiFiGetMyMacAddr(char* MacAddress)
{
  RESULT Result = FAIL;

  if(strstr(MyHwMacAddress, "??") == NULL)
  {
    strcpy(MacAddress, MyHwMacAddress);
    Result = OK;
  }
  return Result;
}

RESULT cWiFiFindMacAddr(void)
{
  // Get the MAC address of the HardWare Interface using the Logical "InterFace"
  // e.g. wlan0 and store it in the fixed location
  // MyHwMacAddress

  RESULT Result = FAIL;
  char IfConfigCmd[32];
  strcpy(IfConfigCmd, "ifconfig");
  strcat(IfConfigCmd, " ");
  strcat(IfConfigCmd, LogicalIfName);
  FILE *FilePointer = popen(IfConfigCmd, "r");

  const char *Source;
  char *Destination;


  if (FilePointer)
  {
    char *OneLine = NULL;
    char *TempPointer;
    size_t NumberOfBytes;

    while((getline(&OneLine, &NumberOfBytes, FilePointer) > 0) && OneLine)
    {
      if((OneLine = strstr(OneLine, "HWaddr")) > 0)
      {
        OneLine+=7;
        TempPointer = OneLine + 17;
        *TempPointer = '\0';

        Source = OneLine;
        Destination = OneLine;

        do   // new
        {
          while(*Source == ':')
            ++Source;
        }
        while((*Destination++ = *Source++) != '\0');
        strcpy(MyHwMacAddress, OneLine);
        Result = OK;

        #ifdef DEBUG
          printf("cWiFiFindMacAddr -> MAC adresse = %s\n\r", MyHwMacAddress);
        #endif

        break;
      }
    }
  }
  pclose(FilePointer);
  return Result;
}

RESULT cWiFiDeepDongleSearch(void)
{
  // Is a KNOWN Dongle attached? We uses the
  // "KNOWN Manufacture ID and Device ID" to
  // detect the HardWare Dongle. No Drivers has to
  // be loaded before using :-)

  RESULT Result = OK;
  /*RESULT Result = FAIL;

  FILE *FilePointer = NULL;
  char FindDongle[10];
  char *OneLine = NULL;
  size_t NumberOfBytes;

  strcpy(FindDongle, "lsusb");

  FilePointer = popen(FindDongle, "r");

  if (FilePointer)
  {
    while((getline(&OneLine, &NumberOfBytes, FilePointer) > 0) && OneLine)
    {
      if((OneLine = strstr(OneLine, HARDWARE_SEARCH_STRING)) > 0)
      {
        Result = OK;
        break;
      }
    }
    pclose(FilePointer);
  }*/
  return Result;
}

RESULT cWiFiKnownDongleAttached(void)
{
  RESULT Result = OK;
  /*FILE *pIdVendor = NULL;
  FILE *pIdProduct = NULL;
  char VendorBuffer[64];
  char ProductBuffer[64];
  //char *VendorId;
  //char *ProductId;
  RESULT Result = FAIL;

  pIdVendor = fopen("/sys/bus/usb/devices/1-1/idVendor", "r");

  pIdProduct = fopen("/sys/bus/usb/devices/1-1/idProduct", "r");

  if((pIdVendor != NULL) && (pIdProduct != NULL))
  {
    if(fgets(VendorBuffer, sizeof (VendorBuffer), pIdVendor) != NULL)
    {
      if(fgets(ProductBuffer, sizeof (ProductBuffer), pIdProduct) != NULL)
      {
        if((strstr(ProductBuffer, WIFI_DONGLE_ID) > 0) && (strstr(VendorBuffer, WIFI_VENDOR_ID) > 0))
          Result = OK;
      }
    }
  }

  if(pIdVendor != NULL)
    fclose(pIdVendor);

  if(pIdProduct != NULL)
    fclose(pIdProduct);

  if(Result != OK) // Not found - Do we have a hub?
  {
    Result = cWiFiDeepDongleSearch();
  }*/
  return Result;
}

RESULT cWiFiFindIpAddr(void)
{
  // Get the IP addresses of the Interface named in the var "InterFaceName"
  // e.g. wlan0, wlan2.. and store it in the fixed locations
  // MyIp4Address, MyBroadCastAdr

  RESULT Result = FAIL;
  char IfConfigCmd[32];
  strcpy(IfConfigCmd, "ifconfig");
  strcat(IfConfigCmd, " ");
  strcat(IfConfigCmd, LogicalIfName);

  #ifdef DEBUG
    printf("\nIfConfigCmd = %s\n", IfConfigCmd);
  #endif

  FILE *FilePointer = popen(IfConfigCmd, "r");

  if (FilePointer)
  {
    char *OneLine = NULL;
    char *TempPointer;
    size_t NumberOfBytes;
    uint TempIp0;
    uint TempIp1;
    uint TempIp2;
    uint TempIp3;

    while((getline(&OneLine, &NumberOfBytes, FilePointer) > 0) && OneLine)
    {
      if((OneLine = strstr(OneLine, "inet addr:")) > 0)
      {
        OneLine+=10;
        if ((TempPointer = strchr(OneLine, ' ')) > 0)
        {
          *TempPointer = '\0';
          strcpy(MyIp4Address, OneLine);

          OneLine = TempPointer + 1;
          if((OneLine = strstr(OneLine, "Bcast:")) > 0)
          {
            OneLine+=6;
            if ((TempPointer = strchr(OneLine, ' ')) > 0)
            {
              *TempPointer = '\0';
              strcpy(MyBroadCastAdr, OneLine);
              if(strstr(MyBroadCastAdr, "0.0.0.0"))
              {
                sscanf(MyIp4Address, "%d.%d.%d.%d", &TempIp3, &TempIp2, &TempIp1, &TempIp0);
                 sprintf(MyBroadCastAdr, "%d.%d.%d.%d", TempIp3, TempIp2, TempIp1, 255);
              }

              Result = OK;
              break;
            }
          }
        }
      }
    }
    pclose(FilePointer);
  }
  return Result;
}

RESULT cWiFiRequestIpAdr(char *Interface)
{
  RESULT Ret = FAIL;
  char Cmd[48];

  #ifdef DEBUG
    printf("\nIP address = %s\n", MyIp4Address);
  #endif

  // strcpy(Cmd, "busybox udhcpc -t 5 -q -i");
  // strcpy(Cmd, "busybox udhcpc -t5 -A2 -n -i");
  // added quit on lease
  strcpy(Cmd, "udhcpc -t5 -A2 -nq -i");
  strcat(Cmd, Interface);
  strcat(Cmd, " &> /dev/null");

  #ifdef DEBUG
    printf("\nCmd = %s\n", Cmd);
  #endif

  if(system(Cmd) == 0)
  {
    #ifdef DEBUG
      printf("\nsystem(Cmd) == 0\n");
    #endif

    cWiFiFindIpAddr(); // Get and Store IP address

    #ifdef DEBUG
      printf("\nAfter cWiFiFindIpAddr()\n");
    #endif

    Ret = OK;
  }
  return Ret;
}

RESULT cWiFiSetPairWiseCcmp(void)
{
  RESULT Ret = FAIL;
  char CmdReturn[10];
  size_t LenCmdReturn = sizeof(CmdReturn) - 1; // We leave space for a terminating /0x00

  memset(CmdReturn, 0x00, LenCmdReturn); // Reset to be sure!!

  if(ctrl_conn != NULL)
  {
    wpa_ctrl_request(ctrl_conn, "SET_NETWORK 0 pairwise CCMP TKIP",
                     strlen("SET_NETWORK 0 pairwise CCMP TKIP"), CmdReturn,
                     &LenCmdReturn, NULL);

    CmdReturn[LenCmdReturn] = '\0';
    if(strstr(CmdReturn, "OK") != NULL)
    {
      Ret = OK;
    }
  }
  return Ret;
}

RESULT cWiFiSetGroupCcmp(void)
{
  RESULT Ret = FAIL;
  char CmdReturn[10];
  size_t LenCmdReturn = sizeof(CmdReturn) - 1; // We leave space for a terminating /0x00

  memset(CmdReturn, 0x00, LenCmdReturn); // Reset to be sure!!

  if(ctrl_conn != NULL)
  {
    wpa_ctrl_request(ctrl_conn, "SET_NETWORK 0 group CCMP TKIP",
                     strlen("SET_NETWORK 0 group CCMP TKIP"), CmdReturn,
                     &LenCmdReturn, NULL);
    CmdReturn[LenCmdReturn] = '\0';
    if(strstr(CmdReturn, "OK") != NULL)
    {
      Ret = OK;
    }
  }
  return Ret;
}



RESULT cWiFiSetProtoRsn(void)
{
  RESULT Ret = FAIL;
  char CmdReturn[10];
  size_t LenCmdReturn = sizeof(CmdReturn) - 1; // We leave space for a terminating /0x00

  memset(CmdReturn, 0x00, LenCmdReturn); // Reset to be sure!!

  if(ctrl_conn != NULL)
  {
    wpa_ctrl_request(ctrl_conn, "SET_NETWORK 0 proto RSN", strlen("SET_NETWORK 0 proto RSN"),
                     CmdReturn, &LenCmdReturn, NULL);

    CmdReturn[LenCmdReturn] = '\0';
    if(strstr(CmdReturn, "OK") != NULL)
    {
      Ret = OK;
    }
  }
  return Ret;
}

RESULT cWiFiSetEnableNetwork(void)
{
  RESULT Ret = FAIL;
  char CmdReturn[10];
  size_t LenCmdReturn = sizeof(CmdReturn) - 1; // We leave space for a terminating /0x00

  memset(CmdReturn, 0x00, LenCmdReturn); // Reset to be sure!!

  if(ctrl_conn != NULL)
  {
    wpa_ctrl_request(ctrl_conn, "ENABLE_NETWORK 0", strlen("ENABLE_NETWORK 0"),
                     CmdReturn, &LenCmdReturn, NULL);

    CmdReturn[LenCmdReturn] = '\0';
    if(strstr(CmdReturn, "OK") != NULL)
    {
      Ret = OK;
    }
  }
  return Ret;
}

RESULT cWiFiWpaStatus(void)
{
  RESULT Ret = FAIL;
  char CmdReturn[256];
  size_t LenCmdReturn = sizeof(CmdReturn) - 1; // We leave space for a terminating /0x00

  memset(CmdReturn, 0x00, LenCmdReturn); // Reset to be sure!!

  do
  {
    // TODO Some escape
    if(ctrl_conn != NULL)
    {
      wpa_ctrl_request(ctrl_conn, "STATUS", strlen("STATUS"), CmdReturn,
                       &LenCmdReturn, NULL);

      CmdReturn[LenCmdReturn] = '\0';

      #ifdef DEBUG
        printf("cWiFiWpaStatus = %s\n\r", CmdReturn);
      #endif
    }
  }
  while(strstr(CmdReturn, "bssid") == NULL);

  Ret = OK;
  return Ret;
}

RESULT cWiFiDisconnect(void)
{
  RESULT Ret = FAIL;
  char CmdReturn[10];
  size_t LenCmdReturn = sizeof(CmdReturn) - 1; // We leave space for a terminating /0x00

  memset(CmdReturn, 0x00, LenCmdReturn); // Reset to be sure!!

  if(ctrl_conn != NULL)
  {
    wpa_ctrl_request(ctrl_conn, "DISCONNECT", strlen("DISCONNECT"), CmdReturn,
                     &LenCmdReturn, NULL);

    sleep(2);
    CmdReturn[LenCmdReturn] = '\0';

    if(strstr(CmdReturn, "OK") != NULL)
    {
      #ifdef DEBUG
        printf("Disconnect OK\n\r");
      #endif

      Ret = OK;
    }
  }
  return Ret;
}


RESULT cWiFiReconnect(void)
{
  RESULT Ret = FAIL;
  char CmdReturn[10];
  size_t LenCmdReturn = sizeof(CmdReturn) - 1;  // We leave space for a terminating /0x00

  memset(CmdReturn, 0x00, LenCmdReturn);        // Reset to be sure!!

  #ifdef DEBUG
    printf("Start of Reconnect...\n\r");
  #endif

  if(ctrl_conn != NULL)
  {
    wpa_ctrl_request(ctrl_conn, "RECONNECT", strlen("RECONNECT"), CmdReturn,
                     &LenCmdReturn, NULL);
    CmdReturn[LenCmdReturn] = '\0';

    if(strstr(CmdReturn, "OK") != NULL)
    {
      #ifdef DEBUG
        printf("Reconnect OK\n\r");
      #endif

      Ret = OK;
    }
  }
  return Ret;
}

RESULT cWiFiGetName(char *ApName, int Index, char Length) // Get the FriendlyName owned by ApTable[Index]
{
  RESULT Result = FAIL;
  if(strlen(ApTable[Index].friendly_name) >= 1)
  {
    strncpy(ApName, ApTable[Index].friendly_name, (int)(Length - 1));

    #ifdef DEBUG
      printf("\ncWiFiGetName called with Index = %d => %s\n", Index, ApTable[Index].friendly_name);
    #endif

    Result = OK;
  }
  return Result;
}

RESULT cWiFiSetName(char *ApName, int Index)  // Set the FriendlyName @ ApTable[Index]
{
  RESULT Result = OK;

  #ifdef DEBUG
    printf("\nApTable[Index = %d].friendly_name before = %s\n", Index, ApTable[Index].friendly_name);
    printf("\nSetName name = %s, Index = %d\n", ApName, Index);
    if(WiFiStatus == BUSY) printf("\nWiFiStatus == BUSY\n");
  #endif

    strcpy(ApTable[Index].friendly_name, ApName);
    // If we changes name etc. We must have some thoughts
    // about an OK state of the WiFi or we want it to be
    WiFiStatus = OK;

  #ifdef DEBUG
    printf("\nApTable[Index = %d].friendly_name after = %s\n", Index, ApTable[Index].friendly_name);
  #endif

  return Result;
}


unsigned char cWiFiGetFlags(int Index)  // Get Flags owned by ApTable[Index]
{
    return ApTable[Index].ap_flags;
}

void cWiFiClearAll()
{
  int j;

  if(ApTableSize > 1)
  {
    for(j = 0; j < ApTableSize; j++)
      ApTable[j].ap_flags &= (UBYTE)(~CONNECTED);
  }
}

void cWiFiTcpShutDown(int iSocket)
{
  UBYTE buffer[128];
  int res;

  shutdown(iSocket, 2);

  do
  {
    res = read(iSocket, buffer, 100);
    if (res < 0) break;
  }
  while(res != 0);

  close(iSocket);

}

void cWiFiCleanUpOldStuff(void)
{
  cWiFiTcpClose();

  cWiFiUdpClientClose();

  //Disconnect
  cWiFiDisconnect();
  //Kill udhcpc
  cWiFiKillUdHcPc();

  //Remove current network
  cWiFiRemoveNetwork();
  sleep(3); // Give some time for cleaning before polluting again ;-)
}

RESULT cWiFiMakeConnectionToAp(int Index)
{
  RESULT Result = FAIL;
  RESULT LocalResult = FAIL;
  int Tries;

    #ifdef DEBUG
    	printf("cWiFiMakeConnectionToAp() with Index %d, WiFiStatus = %d\n\r", Index, WiFiStatus);
    #endif

    Tries = 0;
    do
    {
      if(cWiFiAddNetwork() == OK)
      {
        #ifdef DEBUG
          printf("Try no: %d\n\r", Tries);
        #endif

        LocalResult = OK;
        break;
      }
      else
      {
        #ifdef DEBUG
          printf("AddNetwork returns garbage\n\r");
        #endif

        sleep(3);               // Force some cycles

        cWiFiRemoveNetwork();
      }
      Tries++;
    }
    while((Tries < 3) && (LocalResult == FAIL));

    if(LocalResult == FAIL)
    {
      #ifdef DEBUG
    	  printf("\nERROR_001\n");
      #endif

      return Result;
    }

    // Set the SSID - already known
    if(cWiFiSetSsid(ApTable[Index].friendly_name) != OK)
    {
      #ifdef DEBUG
    	  printf("\nERROR_002\n");
      #endif

      return Result;
    }

    if(cWiFiSetScanSsidToOne() != OK)
    {
      #ifdef DEBUG
        printf("\nERROR_cWiFiSetScanSsidToOne()\n");
      #endif

      return Result;
    }

    // WPA2 capability?
    if((ApTable[Index].ap_flags & WPA2) == WPA2)
    {
      #ifdef DEBUG
    	  printf("\nWPA2 flag set\n");
      #endif

    	// Set KEY MANAGEMENT
    	if(cWiFiSetKeyManagToWpa2() != OK)
    	{
        #ifdef DEBUG
    	    printf("\nERROR_003\n");
        #endif

    	  return Result;
    	}

    	// Set PSK (pre-shared key)
    	if(strlen(ApTable[Index].pre_shared_key) < 1)
    	{
    	  #ifdef DEBUG
          printf("\nERROR_004 pre_shared_key = %s, Index = %d, friendly_name = %s\n", ApTable[Index].pre_shared_key, Index, ApTable[Index].friendly_name);
        #endif

          return Result;
    	}
    	else
    	{
    	  if(cWiFiSetPsk(ApTable[Index].pre_shared_key) != OK)
    	  {
          #ifdef DEBUG
            printf("\nERROR_005 pre_shared_key = %s, Index = %d, friendly_name = %s\n", ApTable[Index].pre_shared_key, Index, ApTable[Index].friendly_name);
          #endif

          return Result;
    	  }
    	}

    	// Set PAIR WISE CIPHERS
    	if(cWiFiSetPairWiseCcmp() != OK)
    	{
        #ifdef DEBUG
          printf("\nERROR_006\n");
        #endif

        return Result;
    	}

    	// Set GROUP CIPHERS
    	if(cWiFiSetGroupCcmp() != OK)
    	{
        #ifdef DEBUG
          printf("\nERROR_007\n");
        #endif

        return Result;
    	}

    	// SET PROTOCOL
    	if(cWiFiSetProtoRsn() != OK)
    	{
        #ifdef DEBUG
          printf("\nERROR_008\n");
        #endif

        return Result;
    	}
    }
    else
    {
      // Without WPA2!!
      // Set KEY MANAGEMENT to NONE
      if(cWiFiSetKeyManagToNone() != OK)
      {
        #ifdef DEBUG
          printf("\nERROR_009\n");
        #endif

        return Result;
      }

    }

    // ENABLE NETWORK
    if(cWiFiSetEnableNetwork() != OK)
    {
      #ifdef DEBUG
        printf("\nERROR_010\n");
      #endif

      return Result;
    }

    sleep(1);                     // Force some system cycles

    // Reconnect
    cWiFiReconnect();

    // We need to get an (new) IP-address - network could have changed as well

    strcpy(MyIp4Address, "???");  // Just to be sure - it's the only stuff

    cWiFiKillUdHcPc();            // No leftover

    sleep(1);                     // Force some system cycles

    cWiFiRequestIpAdr(LogicalIfName);

    #ifdef DEBUG
      printf("Here is the (new) IP address: %s\n\r", MyIp4Address);
    #endif

    if(strstr(MyIp4Address, "???") == NULL)
    {
      #ifdef DEBUG
      	printf("MyIp4Address is OK - and NOT set to ??? => %s", MyIp4Address);
    	#endif

			Result = OK; // Assigned an IP
    }

    #ifdef DEBUG
      printf("\nDid we reach the end with an OK? cWiFiMakeConnectionToAp(Index)\n");
      if(Result == OK)
        printf("\nResult == OK\n");
      else
        printf("\nResult == NOT!!!! OK\n");
    #endif

    return Result;  // Only OK if an valid association & an IP address has been leased
                    // Association could give OK even when not fully associated, but a valid
                    // IP address shows (some) valid connection :-)
}

void cWiFiClearConnectFlags(void)
{
  int j;

    if(ApTableSize > 1)
    {
      for(j = 0; j < ApTableSize; j++)
        ApTable[j].ap_flags &= (UBYTE)(~CONNECTED);
    }
}

RESULT cWiFiConnectToAp(int Index)
{
  RESULT Result = FAIL;

  // Make ApTable[Index] active - "if we're lucky" ;-)

  WiFiStatus = BUSY;

  #ifdef DEBUG
    printf("\ncWiFiConnectToAp(int Index = %d)\n", Index);
  #endif

  // Kill any connection info
  cWiFiClearConnectFlags();

  if(cWiFiMakeConnectionToAp(Index) == OK)
  {
    Result = OK;

    #ifdef DEBUG
      printf("\ncWiFiMakeConnectionToAp(Index = %d) == OK)\n", Index);
    #endif

    // Move the active to TOP
    // Save selected for TOP placing
    cWiFiPreserveActualApRecord(Index);

    // Move all others down one step until index
    cWifiMoveAllActualDown(Index);

    // Restore and Save @ TOP
    cWiFiRestoreActualApRecord(0);

    // Delete if already in KNOWN list
    cWiFiCheckAndDelete(0);

    // Mark the ACTUAL as KNOWN (i.e. set flag)
    ApTable[0].ap_flags |= KNOWN;

    // Mark the ACTUAL as CONNECTED (i.e. set flag)
    ApTable[0].ap_flags |= CONNECTED;

    // This VERY Ap should now be stored as MOST WANTED @ next WiFi session
    cWiFiAddToKnownApList(0);

    WiFiConnectionState = UDP_NOT_INITIATED;
    WiFiStatus = OK;
  }
  else
  {
    WiFiConnectionState = READY_FOR_AP_SEARCH;
    WiFiStatus = FAIL;

    #ifdef DEBUG
      printf("\nConnect FAILed.. READY_FOR_AP_SEARCH again\n");
    #endif
  }

  return Result;
}

RESULT cWiFiMakePsk(char *ApSsid, char *PassPhrase, int Index)  // Make the pre-shared key from
{                                                               // Supplied SSID and PassPhrase
  RESULT Ret = OK;                                              // And store it in ApTable[Index]
  char WpaPassPhraseCmd[64];

  WiFiStatus = BUSY;  // NOT async, but if...

  #ifdef DEBUG
    printf("\ncWiFiMakePsk\n");
    printf("\n\rSSID = %s, PassPhrase = %s, Index = %d\n", ApSsid, PassPhrase, Index);
  #endif

  // Construct CommandLine
  strcpy(WpaPassPhraseCmd, WPA_APPLIKATION_PATH);
  strcat(WpaPassPhraseCmd, "./wpa_passphrase \"");
  strcat(WpaPassPhraseCmd, ApSsid);
  strcat(WpaPassPhraseCmd, "\" ");
  strcat(WpaPassPhraseCmd, PassPhrase);

  #ifdef DEBUG
    printf("WpaPassPhraseCmd = ->%s<-\n", WpaPassPhraseCmd);
  #endif

  FILE *FilePointer = popen(WpaPassPhraseCmd, "r");
  int i;

  if (FilePointer)
  {
    char *PskLine = NULL;
    size_t NumberOfBytes;

    for (i = 0; i < 4; i++)
    {
      if(!((getline(&PskLine, &NumberOfBytes, FilePointer) > 0) && PskLine))
      {
        //Ret = FAIL;
        //WiFiStatus = FAIL;
      }
      else
      {
        Ret = OK;
        //WiFiStatus = OK;
      }
    }

    if(Ret == OK)
    {
      #ifdef DEBUG
        printf("\nPre_Shared_Key Made = %s, %d\n", PskLine, NumberOfBytes);
        printf("\nApTable[Index = %d].pre_shared_key = %s before\n", Index, ApTable[Index].pre_shared_key);
      #endif

      strcpy(ApTable[Index].pre_shared_key, PskLine + 5);
      ApTable[Index].pre_shared_key[strlen(ApTable[Index].pre_shared_key)- 1] = '\x0'; // remove linefeed

      #ifdef DEBUG
        printf("\nApTable[Index = %d].pre_shared_key = %s after store etc.\n", Index, ApTable[Index].pre_shared_key);
        printf("\nApTable[Index = %d].ap_flags = %d after store etc.\n", Index, ApTable[Index].ap_flags);
      #endif
      WiFiStatus = OK;
    }
    pclose(FilePointer);
  }
  return Ret;
}

RESULT cWiFiGetIndexFromName(char *Name, UBYTE *Index)
{
  int i = 0;
  int ListEnd = 0;
  RESULT Result = FAIL;

  ListEnd = cWiFiGetApListSize() + 1;

  #ifdef DEBUG
    if(WiFiStatus == BUSY)
      printf("\nWiFiStatus == BUSY\n");
    else
      printf("\nWiFiStatus == NOT!!!! BUSY\n");

      printf("\nName2Index = %s\n", Name);
  #endif

  while(i < ListEnd)
  {
    #ifdef DEBUG
      printf("\nApTable[%d].friendly_name = %s\n", i, ApTable[i].friendly_name);
    #endif

    if(strcmp(ApTable[i].friendly_name, Name) == 0) // Found
    {
      *Index = (UBYTE)i;                            // Index return
      Result = OK;                                  // Found OK

      #ifdef DEBUG
        printf("\nOK Index = %d\n", i);
      #endif

      break;
    }
    else
      i++;
  }

  #ifdef DEBUG
    printf("\nName2Index Index = %d\n", i);
  #endif

  return Result;    // OK if FOUND else FAIL
}

int cWiFiIsItAFriend(char *mac_address) // Is the actual AP known?
{
  int j;
  int RetVal = UNKNOWN;

  for (j = 0; j < ApStoreTableSize; j++)
  {
    if (strcmp(mac_address,ApStoreTable[j].mac_address) == 0)
    {
      RetVal = KNOWN;
      break;
    }
  }
  return RetVal;
}

void cWiFiSortBySignalLevel()
{
  int HighestPower = -120;
  int SrcIndex = 0;
  int i;
  int StartIndex = 0;
  aps TempAp;

  while(StartIndex < ApTableSize)
  {
    for(i = StartIndex; i < ApTableSize; i++)
    {
      if(atoi(ApTable[i].signal_level) > HighestPower)
      {
        SrcIndex = i;
        HighestPower = atoi(ApTable[i].signal_level);
      }
    }

    // exchange highest found with were are we
    memcpy(&TempAp, &(ApTable[StartIndex].mac_address), sizeof(aps));
    memcpy(&(ApTable[StartIndex].mac_address), &(ApTable[SrcIndex].mac_address), sizeof(aps));
    memcpy(&(ApTable[SrcIndex].mac_address), &TempAp, sizeof(aps));

    StartIndex++;
    HighestPower = -120;  // reset
  }
}

RESULT cWiFiStoreActualApList()           // Store the latest SCAN result(s)
{                                         // If SSID not broadcasted, the FreindlyName is set to "???"
                                          // WPA status flag and Known flag also set if...
  int Status = -1;                        // Not responding
  char* StrElement;
  int EndOfTable = 0;
  int TableEntryPointer = 0;
  RESULT Result = FAIL;

  char SubElement[129]; // Safety for networks with a lot of security strings
  char ScanResponse[2048];
  size_t LenScanResponse = sizeof(ScanResponse) - 1;  // Space for a trailing /0x10

  WiFiStatus = BUSY;  // Should already be BUSY

  if (ctrl_conn != NULL)
  {
    Status = wpa_ctrl_request(  ctrl_conn,
                                  "SCAN_RESULTS",
                                  strlen("SCAN_RESULTS"),
                                  ScanResponse,
                                  &LenScanResponse,
                                  NULL);

    #ifdef DEBUG
      printf("\nScanresult:\n %s\n", ScanResponse);
    #endif

    if((LenScanResponse > 32) && (Status == 0))   // At least some of the header on return
    {                                             // and OK Status return
      ScanResponse[LenScanResponse] = '\x0a';     // Last line END

      StrElement = strtok(ScanResponse, "\x0a"); // Dummy read to remove HEADER

      do
      {
        StrElement = strtok (NULL, "\x09");
        if((StrElement != NULL) && (StrElement[2] == ':'))
        {
          strcpy(ApTable[TableEntryPointer].mac_address, StrElement);

          StrElement = strtok (NULL, "\x09");
          strcpy(ApTable[TableEntryPointer].frequency, StrElement);

          StrElement = strtok (NULL, "\x09");
          strcpy(ApTable[TableEntryPointer].signal_level, StrElement);

          // Read Security and SSID if it is shown
          StrElement = strtok (NULL, "\x0a");

          if(StrElement[strlen(StrElement) - 1] == '\x09')
          {
            // NO SSID SHOWN
            StrElement[strlen(StrElement) - 1] = '\x00';
            strcpy(ApTable[TableEntryPointer].security, StrElement);
            strcpy(ApTable[TableEntryPointer].friendly_name, "??????");
          }
          else
          {
            // We have to separate SECURITY AND SSID
            sscanf( StrElement, "%s", ApTable[TableEntryPointer].security);

            // Get rest, which contains the SSID with spaces include - if any :-)
            strncpy(SubElement, StrElement + strlen(ApTable[TableEntryPointer].security) + 1, (strlen(StrElement) - strlen(ApTable[TableEntryPointer].security)));

            SubElement[strlen(StrElement) + 1] = 0x00; // END

            strcpy(ApTable[TableEntryPointer].friendly_name, SubElement);
          }

          // Reset AP_FLAGS
          ApTable[TableEntryPointer].ap_flags = 0x00;

          // Add Visible to Flags (Would not be here, if NOT) but set for the UI to use
          ApTable[TableEntryPointer].ap_flags |= VISIBLE;

          // Check for security
          if(strstr(ApTable[TableEntryPointer].security, "WPA2") != NULL)
          {
            ApTable[TableEntryPointer].ap_flags |= WPA2;
          }

          // Is it a known "fellow"?
          if(cWiFiIsItAFriend(&(ApTable[TableEntryPointer].mac_address[0]))== KNOWN)
          {
            ApTable[TableEntryPointer].ap_flags |= KNOWN;
          }

          TableEntryPointer++;
          if(TableEntryPointer > MAX_AP_ENTRIES)
          {
            EndOfTable = 1;
            TableEntryPointer = MAX_AP_ENTRIES;
          }
        }
        else EndOfTable = 1;
      }
      while(EndOfTable < 1);

      ApTableSize = TableEntryPointer;

      if(TableEntryPointer > 0)
      {
        cWiFiSortBySignalLevel(); // If many equal named APs the very strongest should be at top
      }

      if(TableEntryPointer > 0)
      {
        // Merge ACTUAL- and KNOWN LIST into a prioritized list
        cWiFiMergeActualAndKnownTable();
        cWiFiClearAll();   // Reset previous CONNECT-flag
      }
    }
  }

  // ******************************** TEST STUFF *******************************************************
  //
  // Used as a stable and known "environment" - i.e. the AP's are stable and always there :-)
  //
        //#define TEST
		    #undef TEST
        #ifdef TEST
          printf("\nSimulated Scanned List:\n");

          ApTableSize = 6;

         /* strcpy(ApTable[0].mac_address, "00:24:01:43:69:10");
          strcpy(ApTable[0].security, "[WPA2-PSK-CCMP][WPS][ESS]");
          strcpy(ApTable[0].friendly_name, "New10");
          strcpy(ApTable[0].pre_shared_key, "dbc9c6cf6d4d8e9405842d2e4ab726f33f10de62c30254ff19512af0f");
          ApTable[0].ap_flags = 0x05;

          strcpy(ApTable[1].mac_address, "00:24:01:43:69:11");
          strcpy(ApTable[1].security, "[WPA2]");
          strcpy(ApTable[1].friendly_name, "New11");
          strcpy(ApTable[1].pre_shared_key, "");
          ApTable[1].ap_flags = 0x05;

          strcpy(ApTable[2].mac_address, "00:24:01:43:69:12");
          strcpy(ApTable[2].security, "[WPA2-PSK-CCMP]");
          strcpy(ApTable[2].friendly_name, "New12");
          strcpy(ApTable[2].pre_shared_key, "");
          ApTable[2].ap_flags = 0x05;

          strcpy(ApTable[3].mac_address, "00:24:01:43:69:13");
          strcpy(ApTable[3].security, "[WPA2-PSK-CCMP][ESS]");
          strcpy(ApTable[3].friendly_name, "New13");
          strcpy(ApTable[3].pre_shared_key, "");
          ApTable[3].ap_flags = 0x05;*/

          strcpy(ApTable[0].mac_address, "00:24:01:43:69:14");
          strcpy(ApTable[0].security, "[WPA2-PSK][ESS]");
          strcpy(ApTable[0].friendly_name, "XXX");
          strcpy(ApTable[0].pre_shared_key, "");
          ApTable[0].ap_flags = 0x05;

          strcpy(ApTable[1].mac_address, "00:24:01:43:69:02");
          strcpy(ApTable[1].security, "[WPA2-PSK-CCMP]");
          strcpy(ApTable[1].friendly_name, "AAA");
          strcpy(ApTable[1].pre_shared_key, "");
          ApTable[1].ap_flags = 0x0C;

          strcpy(ApTable[2].mac_address, "00:24:01:43:69:6e");
          strcpy(ApTable[2].security, "[WPA2-PSK-CCMP][WPS][ESS]");
          strcpy(ApTable[2].friendly_name, "FFF");
          strcpy(ApTable[2].pre_shared_key, "dbc9c6cf6d4d8e9405842d2e4ab726f33f10de62c30254ff19512af0f");
          ApStoreTable[2].ap_flags = 0x0C;

          strcpy(ApTable[3].mac_address, "00:24:01:43:69:01");
          strcpy(ApTable[3].security, "[WPA2]");
          strcpy(ApTable[3].friendly_name, "OOO");
          strcpy(ApTable[3].pre_shared_key, "");
          ApTable[3].ap_flags = 0x0C;

          strcpy(ApTable[4].mac_address, "00:24:01:43:69:98");
          strcpy(ApTable[4].security, "");
          strcpy(ApTable[4].friendly_name, "HHH");
          strcpy(ApTable[4].pre_shared_key, "");
          ApTable[4].ap_flags = 0x01;

          strcpy(ApTable[5].mac_address, "00:24:01:43:69:99");
          strcpy(ApTable[5].security, "");
          strcpy(ApTable[5].friendly_name, "WWW");
          strcpy(ApTable[5].pre_shared_key, "");
          ApTable[5].ap_flags = 0x01;

          printf("\rApTable before MERGE:\n");
          int k;
          for(k = 0; k < ApTableSize; k++)
          {
            printf("\rApTable[%d].mac_address = %s\n", k, ApTable[k].mac_address);
            printf("\rApTable[%d].security = %s\n", k, ApTable[k].security);
            printf("\rApTable[%d].friendly_name = %s\n", k, ApTable[k].friendly_name);
            printf("\rApTable[%d].pre_shared_key = %s\n", k, ApTable[k].pre_shared_key);
            printf("\rApTable[%d].ap_flags = %d\n", k, ApTable[k].ap_flags);
          }

        cWiFiMergeActualAndKnownTable();

          printf("\nApTable after MERGE:\n");
          for(k = 0; k < ApTableSize; k++)
          {
            printf("\rApTable[%d].mac_address = %s\n", k, ApTable[k].mac_address);
            printf("\rApTable[%d].security = %s\n", k, ApTable[k].security);
            printf("\rApTable[%d].friendly_name = %s\n", k, ApTable[k].friendly_name);
            printf("\rApTable[%d].pre_shared_key = %s\n", k, ApTable[k].pre_shared_key);
            printf("\rApTable[%d].ap_flags = %d\n", k, ApTable[k].ap_flags);
          }
          #endif

//######################################### END OF TEST STUFF ##################################################

    Result = OK;
    WiFiStatus = OK;
    WiFiConnectionState = AP_LIST_UPDATED;

    #ifdef DEBUG
      printf("\nStored list OK - STATUS = OK\n");
    #endif

    return Result;
}

RESULT cWiFiScanForAPs()
{
  RESULT Result = FAIL;                               // HARD ERROR
  char ScanResponse[10];
  size_t LenScanResponse = sizeof(ScanResponse) - 1;  // Space for a trailing /0

  WiFiConnectionState = SEARCH_APS;

  WiFiStatus = FAIL;      // If an OK search is started, we get BUSY otherwise we always get
                          // a SOFT ERROR (FAIL)
  BeaconTx = NO_TX;
  cWiFiStartTimer();      // Start the Timer

  cWiFiClearAll();        // Reset previous CONNECT-flag
  cWiFiCleanUpOldStuff(); // Kill OLD portion in the list etc.

  if(cWiFiGetOnStatus() == OK)
  {
    if (ctrl_conn != NULL)
    {
      wpa_ctrl_request(ctrl_conn, "SCAN", strlen("SCAN"), ScanResponse,
                       &LenScanResponse, NULL);

      ScanResponse[LenScanResponse] = '\0';

      if(strstr(ScanResponse, "OK") != NULL)
      {
        WiFiConnectionState = SEARCH_PENDING;

        #ifdef DEBUG
          printf("\nSCAN list OK - STATUS = BUSY\n");
        #endif
          WiFiStatus = BUSY;
          Result = OK;
      }
      else
      {
        WiFiConnectionState = WIFI_INITIATED;  // OK init, but NO SEARCH
        // The FAIL return is already set above!

        #ifdef DEBUG
          printf("\nSCAN list FAILED - STATUS = FAIL\n");
        #endif
      }
    }
  } // End if(cWiFiGetOnStatus == OK)

  return Result;
}

void cWiFiIncApListSize(void)
{
  if(ApTableSize < MAX_AP_ENTRIES)
  {
    ApTableSize++;
  }
}

int cWiFiGetApListSize(void)
{
  return ApTableSize;
}

int cWiFiWpaPing(void)    // "PING" the WpaControl InterFace
{
  int RetVal = -1;        // Not responding
  int Status = 0;
  char PingResponse[32];
  size_t Len_PingResponse = sizeof(PingResponse) - 1; // Space for a trailing /0

  if (ctrl_conn != NULL)
  {
    Status = wpa_ctrl_request(  ctrl_conn,
                                  "PING",
                                  strlen("PING"),
                                  PingResponse,
                                  &Len_PingResponse,
                                  NULL);

    PingResponse[Len_PingResponse] = '\0';
    if(strcmp(PingResponse, "PONG") == 0)
      RetVal = 0;
    else
      RetVal = Status;
  }
  return RetVal;
}

RESULT cWiFiGetLogicalName(void)  // Get the Logical Name of the Interface
{                                 // E.g. wlan2

  RESULT  Result = FAIL;
  char IwConfigCmd[32];
  strcpy(IwConfigCmd, "./iwconfig");
  FILE *FilePointer = popen(IwConfigCmd, "r");

  if (FilePointer)
  {
    char *OneLine = NULL;
    size_t NumberOfBytes;
    while((getline(&OneLine, &NumberOfBytes, FilePointer) > 0) && OneLine)
    {
      //if(strstr(OneLine, "IEEE 802"))
      //{
          OneLine[5] = 0x00;
          strcpy(LogicalIfName, "wlan0"); // Fixed for now....
          Result = OK;
      //    break;
      //}
    }
    pclose(FilePointer);
  }
  return Result;
}

RESULT cWiFiGetStatus(void)
{
  #ifdef DEBUG
    printf("WiFiStatus => GetResult = %d\n\r", WiFiStatus);
  #endif
  return WiFiStatus;
}

RESULT  cWiFiTxingBeacons(void) // Are we active in tx beacons??
{
  RESULT  Result = FAIL;
  if(BeaconTx == TX_BEACON)
    Result = OK;
  return Result;
}

RESULT cWiFiTcpConnected(void)  // TCP connection established?
{                               // Can we communicate...
  RESULT  Result = FAIL;
  if(TcpState == TCP_UP)
    Result = OK;
  return Result;
}

RESULT  cWiFiInitTcpServer()
{
  RESULT Result = OK;
  int Temp, SetOn;

  WiFiStatus = FAIL;
  /*  Create a listening socket IPv4, TCP and only single protocol */

  #ifdef DEBUG
    printf("Start of cWiFiInitTcpServer()...TCPListenServer = %d \n\r", TCPListenServer);
  #endif

  if ( TCPListenServer == 0)   // close(TCPListenServer);
  {
    #ifdef DEBUG
      printf("TCPListenServer == 0 in cWiFiInitTcpServer()...\n\r");
    #endif

	  if ( (TCPListenServer  = socket(AF_INET, SOCK_STREAM, 0)) < 0 )
	  {
    	#ifdef DEBUG
		    printf("\nError creating listening socket in cWiFiInitTcpServer()...\n");
    	#endif

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
    	#ifdef DEBUG
		    printf("\nError calling bind()\n");
		    printf("errno = %d\n\r", errno);
    	#endif

		return Result;  // Bail out with a SOFT error in WiFiStatus
	  }
  } // End of the "DO A SINGLE TIME"

	  if ( listen(TCPListenServer, 1) < 0 )
	  {
    	#ifdef DEBUG
		    printf("\nError calling listen()\n");
    	#endif

		return Result;  // Bail out with a SOFT error in WiFiStatus
	  }

	  // Else show debug text below.....
  	  #ifdef DEBUG
	  	  printf("\nWAITING for a CLIENT.......\n");
  	  #endif

  WiFiStatus = OK;
  return OK;  // Create socket, bind and listen succeeded
              // Or reuse
}

RESULT cWiFiWaitForTcpConnection(void)
{
  uint size = sizeof(servaddr);
  RESULT Result = BUSY;

  WiFiStatus = OK;  // We're waiting in a "positive" way ;-)

  usleep(1000);	    // Just to be sure some cycles ... :-)

  if((TcpConnectionSocket = accept(TCPListenServer, (struct sockaddr *)&servaddr, &size) ) < 0)
  {
      #ifdef DEBUG
        printf("\nError calling accept() - returns: %d\r", TcpConnectionSocket);
      #endif
  }
  else
  {
    Result = OK;
    WiFiStatus = OK;
    TcpReadState = TCP_WAIT_ON_START;

    #ifdef DEBUG
    //printf("\nConnected.... :-) %s : %d\n", inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));
      printf("\nConnected.... :-) %s : %d\n", inet_ntoa(servaddr.sin_addr), ntohs(servaddr.sin_port));
    #endif
  }

  #ifdef DEBUG
    printf("\nNow we're waiting for input TCP ...\n");
  #endif

  return Result;
}

UWORD cWiFiWriteTcp(UBYTE* Buffer, UWORD Length)
{
  uint DataWritten = 0;                 // Nothing written (BUSY)
  struct timeval WriteTimeVal;          // Always called from COM, so FAIL == also 0
  fd_set  WriteFdSet;

  if(Length > 0)
  {
    #ifdef DEBUG
      printf("\ncWiFiWriteTcp Length: %d\n", Length);
      // Code below used for "hunting" packets of correct length
      // but with length bytes set to "0000" and payload all zeroed

      int i;
      if((Buffer[0] == 0) && (Buffer[1] == 0))
      {
    	  printf("\ncERROR in first 2 entries - WiFiWriteTcp Length: %d\n", Length);
    	  for (i = 0; i < Length; i++)
    	  {
    		  printf("\nFAIL!!! Buffer[%d] = 0x%x\n", i, Buffer[i]);
    	  }
      }
    #endif

    if(WiFiConnectionState == TCP_CONNECTED)
    {
      WriteTimeVal.tv_sec = 0;  // NON blocking test use
      WriteTimeVal.tv_usec = 0; // I.e. NO timeout
      FD_ZERO(&WriteFdSet);
      FD_SET(TcpConnectionSocket, &WriteFdSet);

      select(TcpConnectionSocket + 1, NULL, &WriteFdSet, NULL, &WriteTimeVal);
      if (FD_ISSET(TcpConnectionSocket, &WriteFdSet))
      {
        // We can Write
        DataWritten = write(TcpConnectionSocket, Buffer, Length);

        #ifdef DEBUG
          if (DataWritten != Length)
            printf("\nDataWritten = %d, Length = %d\n", DataWritten, Length);
        #endif

        // DataWritten = Data sent, zero = socket busy or -1 = FAIL
      }
    }
  }
  return DataWritten;
}

RESULT cWiFiResetTcp(void)
{
  RESULT Result = FAIL;

  #ifdef DEBUG
    printf("\nRESET - client disconnected!\n");
  #endif

  Result = cWiFiTcpClose();
  return Result;
}

UWORD cWiFiReadTcp(UBYTE* Buffer, UWORD Length)
{
   int DataRead = 0; // Nothing read also sent if NOT initiated
                      // COM always polls!!
   struct timeval ReadTimeVal;
   fd_set  ReadFdSet;

   if(WiFiConnectionState == TCP_CONNECTED)
   {
     ReadTimeVal.tv_sec = 0;  // NON blocking test use
     ReadTimeVal.tv_usec = 0; // I.e. NO timeout
     FD_ZERO(&ReadFdSet);
     FD_SET(TcpConnectionSocket, &ReadFdSet);

     select(TcpConnectionSocket + 1, &ReadFdSet, NULL, NULL, &ReadTimeVal);
     if (FD_ISSET(TcpConnectionSocket, &ReadFdSet))
     {
       #ifdef DEBUG
         printf("\nTcpReadState = %d\n", TcpReadState);
       #endif

       switch(TcpReadState)
       {
         case TCP_IDLE:                 // Do Nothing
                                        break;

         case TCP_WAIT_ON_START:
                                        #ifdef DEBUG
                                         printf("TCP_WAIT_ON_START:\n");
                                        #endif

                                        DataRead = read(TcpConnectionSocket, Buffer, 100); // Fixed TEXT

                                        #ifdef DEBUG
                                          printf("\nDataRead = %d, Buffer = \n", DataRead);
                                          int ii;
                                          if(DataRead > 0)
                                          {
                                        	  for (ii = 0; ii < DataRead; ii++)
                                        	  {
                                        		  printf("0x%x, ", Buffer[ii]);
                                        	  }
                                          }
                                          else
                                            printf("DataRead shows FAIL: %d", DataRead);
                                            printf("\n\r");
                                        #endif

                                        if(DataRead == 0)   // We've a disconnect
                                        {
                                          cWiFiResetTcp();
                                          break;
                                        }

                                        if(strstr((char*)Buffer, "ET /target?sn=") > 0)
                                        {
                                          #ifdef DEBUG
                                            printf("\nTCP_WAIT_ON_START and  ET /target?sn= found :-) DataRead = %d, Length = %d, Buffer = %s\n", DataRead, Length, Buffer);
                                          #endif

                                          // A match found => UNLOCK
                                          // Say OK back
                                          cWiFiWriteTcp((UBYTE*)"Accept:EV340\r\n\r\n", 16);
                                          TcpReadState = TCP_WAIT_ON_LENGTH;
                                        }

                                        DataRead = 0; // No COM-module activity yet
                                        break;

         case TCP_WAIT_ON_LENGTH:       // We can should read the length of the message
        	 	 	 	 	 	 	 	                // The packets can be split from the client
        	 	 	 	 	 	 	 	                // I.e. Length bytes (2) can be send as a subset
        	 	 	 	 	 	 	 	                // the Sequence can also arrive as a single pair of bytes
        	 	 	 	 	 	 	 	                // and the finally the payload will be received

        	 	 	 	 	 	 	 	                TcpReadBufPointer = 0;	// Begin on new buffer :-)

                                        #ifdef DEBUG
                                          printf("TCP_WAIT_ON_LENGTH:\n");
                                        #endif

                                        DataRead = read(TcpConnectionSocket, Buffer, 2);
                                        if(DataRead == 0)   // We've a disconnect
                                        {
                                          cWiFiResetTcp();
                                          break;
                                        }

                                        TcpRestLen = (UWORD)(Buffer[0] + Buffer[1] * 256);
                                        TcpTotalLength = (UWORD)(TcpRestLen + 2);
                                        if(TcpTotalLength > Length)
                                          TcpReadState = TCP_WAIT_ON_FIRST_CHUNK;
                                        else
                                          TcpReadState = TCP_WAIT_ON_ONLY_CHUNK;

                                        TcpReadBufPointer += DataRead;	// Position in ReadBuffer adjust
                                        DataRead = 0;                   // Signal NO data yet

                                        #ifdef DEBUG
                                          printf("\n\r*************** NEW TX *************\n");
                                          printf("TCP_WAIT_ON_LENGTH TcpRestLen = %d, Length = %d\n", TcpRestLen, Length);
                                        #endif

                                        break;

         case TCP_WAIT_ON_ONLY_CHUNK:
                                        #ifdef DEBUG
                                          printf("TCP_WAIT_ON_ONLY_CHUNK: BufferStart = %d\n", TcpReadBufPointer);
                                        #endif

                                        DataRead = read(TcpConnectionSocket, &(Buffer[TcpReadBufPointer]), TcpRestLen);

                                        #ifdef DEBUG
                                          printf("DataRead = %d\n",DataRead);
                                          printf("BufferPointer = %p\n", &(Buffer[TcpReadBufPointer]));
                                        #endif

                                        if(DataRead == 0)   // We've a disconnect
                                        {
                                          cWiFiResetTcp();
                                          break;
                                        }

                                        TcpReadBufPointer += DataRead;

                                        if(TcpRestLen == DataRead)
                                        {
                                        	DataRead = TcpTotalLength;			// Total count read
                                        	TcpReadState = TCP_WAIT_ON_LENGTH;
                                        }
                                        else
                                        {
                                        	TcpRestLen -= DataRead;	// Still some bytes in this only chunk
                                        	DataRead = 0;			// No COMM job yet
                                        }

                                        #ifdef DEBUG
                                        	int i;

                                        	for(i = 0; i < TcpTotalLength; i++)
                                        	{
                                        		printf("ReadBuffer[%d] = 0x%x\n", i, Buffer[i]);
                                        	}
										                    #endif

                                        #ifdef DEBUG
                                          printf("TcpRestLen = %d, DataRead incl. 2 = %d, Length = %d\n", TcpRestLen, DataRead, Length);
                                        #endif

                                        break;

         case TCP_WAIT_ON_FIRST_CHUNK:
                                        #ifdef DEBUG
                                          printf("TCP_WAIT_ON_FIRST_CHUNK:\n");
                                        #endif

                                        DataRead = read(TcpConnectionSocket, &(Buffer[TcpReadBufPointer]), (Length - 2));
                                        if(DataRead == 0)   // We've a disconnect
                                        {
                                          cWiFiResetTcp();
                                          break;
                                        }

                                        #ifdef DEBUG
                                          printf("DataRead = %d\n", DataRead);
                                        #endif

                                        TcpRestLen -= DataRead;
                                        TcpReadState = TCP_WAIT_COLLECT_BYTES;
                                        DataRead += 2;

                                        #ifdef DEBUG
                                          printf("\nTCP_WAIT_ON_FIRST_CHUNK TcpRestLen = %d, DataRead incl. 2 = %d, Length = %d\n", TcpRestLen, DataRead, Length);
                                        #endif

                                        break;


         case TCP_WAIT_COLLECT_BYTES:
                                        #ifdef DEBUG
                                          printf("TCP_WAIT_COLLECT_BYTES:\n");
                                        #endif

                                        TcpReadBufPointer = 0;
                                        if(TcpRestLen < Length)
                                          DataRead = read(TcpConnectionSocket, &(Buffer[TcpReadBufPointer]), TcpRestLen);
                                        else
                                          DataRead = read(TcpConnectionSocket, &(Buffer[TcpReadBufPointer]), Length);

                                        #ifdef DEBUG
                                          printf("DataRead = %d\n", DataRead);
                                        #endif

                                        if(DataRead == 0)   // We've a disconnect
                                        {
                                          cWiFiResetTcp();
                                          break;
                                        }

                                        TcpRestLen -= DataRead;
                                        if(TcpRestLen == 0)
                                        {
                                          TcpReadState = TCP_WAIT_ON_LENGTH;
                                        }

                                        #ifdef DEBUG
                                          printf("\nTCP_WAIT_COLLECT_BYTES TcpRestLen = %d, DataRead incl. 2 = %d, Length = %d\n", TcpRestLen, DataRead, Length);
                                        #endif

                                        break;

         default:                        // Should never go here....
                                         TcpReadState = TCP_IDLE;
                                         break;
       }
     }
   }

   return DataRead;
}

RESULT cWiFiBeaconAnswer(void)          // Wait for a "dating" PC
{
  // This wait is performed as a NON-BLOCKING wait
  RESULT Result = FAIL;
  struct timeval  TimeOutVal;
  fd_set  ReadFdSet;
  TimeOutVal.tv_sec = 0;
  TimeOutVal.tv_usec = 0; // No Timeout val only use for NON-blocking test

  ServerAddr.sin_addr.s_addr = htonl(INADDR_ANY);

  FD_ZERO(&ReadFdSet);
  FD_SET(UdpSocketDescriptor, &ReadFdSet);

  select(UdpSocketDescriptor + 1, &ReadFdSet, NULL, NULL, &TimeOutVal);

  if (FD_ISSET(UdpSocketDescriptor, &ReadFdSet))
  {
    // Beacon Answer is ready - we hope
    UdpRxCount = recvfrom(UdpSocketDescriptor, BufPtr, BufLen, 0, (struct sockaddr *)&ServerAddr, &ServerAddrLen);
    if(UdpRxCount < 0)
    {
      #ifdef DEBUG
        printf("\nUDP Client - recvfrom() ERROR UdpRxCount < 0!!!!\n");
      #endif
    }
    else
    {

      #ifdef DEBUG
        printf("\nUDP client received the following: \"%s\" message\\n", BufPtr);
        printf(" \nfrom port %d, address %s\n", ntohs(ServerAddr.sin_port), inet_ntoa(ServerAddr.sin_addr));
      #endif

      Result = OK;
    }
  }
  return Result;
}

RESULT cWiFiTransmitBeacon(void)

{
  RESULT Result = FAIL;

  ServerAddr.sin_port = htons(BROADCAST_PORT);
  ServerAddr.sin_addr.s_addr = inet_addr(MyBroadCastAdr);

  #ifdef DEBUG
    printf("\nUDP BROADCAST to port %d, address %s\n", ntohs(ServerAddr.sin_port), inet_ntoa(ServerAddr.sin_addr));
  #endif

  cWiFiSetBtSerialNo(); // Be sure to have updated data :-)
  cWiFiSetBrickName();  // -
  sprintf(Buffer,"Serial-Number: %s\nPort: %d\nName: %s\nProtocol: EV3\n", BtSerialNo, TCP_PORT, BrickName);

  UdpTxCount =  sendto(UdpSocketDescriptor, Buffer, strlen(Buffer), 0, (struct sockaddr *)&ServerAddr, sizeof(ServerAddr));
  if(UdpTxCount < 0)
  {
	  #ifdef DEBUG
      printf("\nUDP SendTo ERROR : %d\n\r", UdpTxCount);
    #endif

    cWiFiUdpClientClose();  // Kill the auto-beacon stuff
  }
  else
  {
	  #ifdef DEBUG
      printf("\nUDP Client - SendTo() is OK! UdpTxCount = %d\n", UdpTxCount);
      printf("\nWaiting on a reply from UDP server...zzzzz zzzz - Send UNICAST only to me :-)\n");
    #endif

    Result = OK;
  }
  return Result;
}

RESULT cWiFiInitUdpConnection(void)
{
  RESULT Result = FAIL;
  int Temp;

  WiFiConnectionState = INIT_UDP_CONNECTION;

  /* Get a socket descriptor for UDP client (Beacon) */
  if((UdpSocketDescriptor = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
  {
    #ifdef DEBUG
      printf("\nUDP Client - socket() error\n");
    #endif
  }
  else
  {
    #ifdef DEBUG
      printf("\nUDP Client - socket() is OK!\n");
      printf("\nBroadCast Adr from Ifconfig = %s\n", MyBroadCastAdr);
    #endif
    ServerAddr.sin_family = AF_INET;
    ServerAddr.sin_port = htons(BROADCAST_PORT);

    if((ServerAddr.sin_addr.s_addr = inet_addr(MyBroadCastAdr)) == (unsigned long)INADDR_NONE)
    {
      #ifdef DEBUG
        printf("\nHOST addr == INADDR_NONE :-( \n");
      #endif
    }
    else
    {
      if(setsockopt(UdpSocketDescriptor, SOL_SOCKET, SO_BROADCAST, &BroadCast, sizeof(BroadCast)) < 0)
      {
        #ifdef DEBUG
          printf("\nCould not setsockopt SO_BROADCAST\n");
        #endif
      }
      else
      {
        Temp = fcntl(UdpSocketDescriptor, F_GETFL, 0);
        fcntl(UdpSocketDescriptor, F_SETFL, Temp | O_NONBLOCK); // Make the socket NON_BLOCKING

        #ifdef DEBUG
          printf("\nSet SOCKET NON-BLOCKING :-)...\n");
        #endif

        Result = OK;
      }
    }

    #ifdef DEBUG
      printf("\nINIT UDP ServerAddr.sin_port = %d, ServerAddr.sin_addr.s_addr = %s\n", ntohs(ServerAddr.sin_port), inet_ntoa(ServerAddr.sin_addr));
    #endif

  }
  return Result;
}

void cWiFiSetBtSerialNo(void)
{
  FILE *File;

    // Get the file-based BT SerialNo
    File  =  fopen("./settings/BTser","r");
    if(File != NULL) // if something to read
    {
      fgets(BtSerialNo, BLUETOOTH_SER_LENGTH, File);
      fclose(File);
    }
}

void cWiFiSetBrickName(void)
{
  FILE *File;

  // Get the file-based BrickName
  File  =  fopen("./settings/BrickName","r");
  if(File != NULL) // if something to read
  {
    fgets(BrickName, BRICK_HOSTNAME_LENGTH, File);
    fclose(File);
  }
}

int cWiFiCheckForLoadedModule(char *NameToCheck)
{
  FILE *pProcModules;
  char ReadBuffer[64];
  const char WhiteSpaces[] = " ,-;:."; // "- " should be the ones, but...
  char *DriverName;
  int RetResult = FALSE;

  pProcModules = fopen("/proc/modules", "r");

  while (fgets(ReadBuffer, sizeof (ReadBuffer), pProcModules) != NULL)
  {
    DriverName = strtok(ReadBuffer, WhiteSpaces);
    if(strcmp(DriverName, NameToCheck) == 0)
    {
      #ifdef DEBUG
        printf("NameToCheck found: %s\n", NameToCheck);
      #endif
      RetResult = TRUE;
      break;
    }
    else
    {
      #ifdef DEBUG
        printf("NameToCheck NOT found: %s\n", NameToCheck);
      #endif
    }
  }
  fclose(pProcModules);
  return RetResult;
}

void cWiFiLoadAthHwModules(void)
{
}

void cWiFiUnLoadAthHwModules(void)
{
}

void cWiFiControl(void)
{
    struct stat st;
    char Command[128];

    // Do we have to TX the beacons?
    if (BeaconTx == TX_BEACON) {
        // If is it time for it?
        if (cWiFiCheckTimer() >= BEACON_TIME) {
            // Did we manage to TX one?
            if (cWiFiTransmitBeacon() != OK) {
                // Error handling - TODO: Should be user friendly
            } else {
#ifdef DEBUG
                printf("\nOK beacon TX\n");
#endif
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
#ifdef DEBUG
            printf("\nWIFI_INIT, NOT_INIT foer FindMacAddr... %d\n", WiFiStatus);
#endif
            cWiFiFindMacAddr(); // Get the H/W MAC-address
#ifdef DEBUG
            printf("\nWIFI_INIT, NOT_INIT efter FindMacAddr... %d\n", WiFiStatus);
#endif
            cWiFiStartTimer();
#ifdef DEBUG
            printf("\nWIFI_INIT, NOT_INIT foer system... %d\n", WiFiStatus);
#endif
            cWiFiStartWpaSupplicant("/etc/wpa_supplicant.conf", LogicalIfName);
            //system("./wpa_supplicant -Dwext -iwlan<X> -c/etc/wpa_supplicant.conf -B");
            InitState = LOAD_SUPPLICANT;
            break;

        case LOAD_SUPPLICANT:
            TimeOut = cWiFiCheckTimer();
            if (TimeOut < WIFI_INIT_TIMEOUT) {
                strcpy(Command, "/var/run/wpa_supplicant/");
                strcat(Command, LogicalIfName);
                if (stat(Command, &st) == 0) {
#ifdef DEBUG
                    printf("\nWIFI_INIT, LOAD_SUPPLICANT => STAT OK %d\n", WiFiStatus);
#endif
                    // Ensure (help) Interface to become ready
                    strcpy(Command, "ifconfig ");
                    strcat(Command, LogicalIfName);
                    strcat(Command, " down > /dev/null");

                    system(Command);

                    strcpy(Command, "ifconfig ");
                    strcat(Command, LogicalIfName);
                    strcat(Command, " up > /dev/null");

                    system(Command);

                    InitState = WAIT_ON_INTERFACE;
                }
                //else keep waiting
            } else {
#ifdef DEBUG
                printf("\nWIFI_INIT, LOAD_SUPPLICANT => Timed out\n");
#endif
                WiFiStatus = FAIL;
                // We're sleeping until user select ON
                WiFiConnectionState = WIFI_NOT_INITIATED;
                InitState = NOT_INIT;
            }
            break;

        case WAIT_ON_INTERFACE:
            // Wait for the Control stuff to be ready

            // Get "handle" to Control Interface
            strcpy(Command, "/var/run/wpa_supplicant/");
            strcat(Command, LogicalIfName);

            if ((ctrl_conn = wpa_ctrl_open(Command)) > 0) {
                if (cWiFiWpaPing() == 0) {
#ifdef DEBUG
                    printf("\nWIFI_INIT, WAIT_ON_INTERFACE => Ping OK %d\n", WiFiStatus);
#endif
                    cWiFiPopulateKnownApList();
                    WiFiStatus = OK;
                    WiFiConnectionState = WIFI_INITIATED;
                    InitState = DONE;
                } else {
#ifdef DEBUG
                    printf("\nWIFI_INIT, WAIT_ON_INTERFACE => PING U/S\n");
#endif
                    WiFiStatus = FAIL;
                    cWiFiExit();
                }
            }
            break;
        case DONE:
            break;
        }
        break;

    case WIFI_INITIATED:
        // Temporary state - WiFi lower Stuff turned ON
        WiFiOnStatus = OK;
        cWiFiStartDongleCheckTimer();
#ifdef DEBUG
        printf("\nWIFI_INITIATED %d\n", WiFiStatus);
#endif
        WiFiConnectionState = READY_FOR_AP_SEARCH;
#ifdef DEBUG
        printf("\nREADY for search -> %d\n", WiFiStatus);
#endif
        break;

    case READY_FOR_AP_SEARCH:
        // We can select SEARCH i.e. Press Connections on the U.I.
        // We have the H/W stack up and running
        break;

    case SEARCH_APS:
        // Polite wait
#ifdef DEBUG
        printf("\nSEARCH_APS:\n");
        printf("\ncIndex = %d => %s\n", 0, ApTable[0].friendly_name);
        printf("\ncIndex = %d => %s\n", 1, ApTable[1].friendly_name);
        printf("\ncIndex = %d => %s\n", 2, ApTable[2].friendly_name);
#endif
        break;

    case SEARCH_PENDING:
        // Wait some time for things to show up...
#ifdef DEBUG
        printf("cWiFiCheckTimer() = %d\r", cWiFiCheckTimer());
#endif
        // Give some time for the stuff to show up
        // Get Elapsed time in seconds
        if (20 <= cWiFiCheckTimer()) {
            // Getting the list and update the visible list
            cWiFiStoreActualApList();
        }
#ifdef DEBUG
        printf("\nSEARCH_PENDING:\n");
        printf("\ncIndex = %d => %s\n", 0, ApTable[0].friendly_name);
        printf("\ncIndex = %d => %s\n", 1, ApTable[1].friendly_name);
        printf("\ncIndex = %d => %s\n", 2, ApTable[2].friendly_name);
#endif
        break;

    case AP_LIST_UPDATED:
        // Relaxed state until connection wanted
#ifdef DEBUG
        printf("\nAP_LIST_UPDATED ApTableSize = %d:\n", ApTableSize);
        printf("\ncIndex = %d => %s\n", 0, ApTable[0].friendly_name);
        printf("\ncIndex = %d => %s\n", 1, ApTable[1].friendly_name);
        printf("\ncIndex = %d => %s\n", 2, ApTable[2].friendly_name);
#endif
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
#ifdef DEBUG
        printf("\nHer er UDP_NOT_INITIATED\n");
#endif
        WiFiConnectionState = INIT_UDP_CONNECTION;
        break;

    case INIT_UDP_CONNECTION:
        WiFiStatus = BUSY;                    // We're still waiting
        memset(Buffer, 0x00, sizeof(Buffer)); // Reset TX buffer
#ifdef DEBUG
        printf("\nLige foer cWiFiInitUdpConnection()\n");
#endif

        if (cWiFiInitUdpConnection() == OK) {
#ifdef DEBUG
            printf("\nUDP connection READY @ INIT_UDP_CONNECTION\n");
#endif
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
#ifdef DEBUG
            printf("\nUDP connection FAILed @ INIT_UDP_CONNECTION\n");
#endif
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
#ifdef DEBUG
            printf("\nTCP init OK @ UDP_CONNECTED\n");
#endif
            WiFiConnectionState = TCP_NOT_CONNECTED;
        }
        break;

    case TCP_NOT_CONNECTED:
        // Waiting for the PC to connect via TCP
        // Non-blocking test
        if (cWiFiWaitForTcpConnection() == OK) {
#ifdef DEBUG
            printf("\nTCP_CONNECTED @ TCP_NOT_CONNECTED\n");
#endif
            TcpState = TCP_UP;
            WiFiConnectionState = TCP_CONNECTED;
            // We are connected so we can tell the world....
            // And we're ready to TX/RX :-)
            WiFiStatus = OK; // Not busy any longer
        }
        break;

    case TCP_CONNECTED:
        // We have a TCP connection established
#ifdef DEBUG
        printf("\nTCP_CONNECTED @ TCP_CONNECTED.... And then.....\n");
#endif
        break;

    case CLOSED:
        // UDP/TCP closed
        break;
    }
    // Íf some logic fired up
    if (WiFiOnStatus == OK) {
        // Check for the "volatile" USB dongle connection
        // Don't misuse the CPU cycles
        if (cWiFiTimeFromLastDongleCheck() >= TIME_FOR_WIFI_DONGLE_CHECK) {
#ifdef DEBUG
            printf("Check for Dongle....");
#endif
            if (cWiFiKnownDongleAttached() != OK) {
#ifdef DEBUG
                printf("FAIL!!\n");
#endif
                cWiFiTurnOff();  // No H/W
            } else {
                cWiFiStartDongleCheckTimer();
#ifdef DEBUG
                printf("OK!!\n");
#endif
            }
        }
    }
}


RESULT cWiFiGetOnStatus(void)
{
    return WiFiOnStatus;
}

RESULT cWiFiTurnOn(void)
{
    RESULT  Result = FAIL;
    char Command[64];

    // We will use some (a lot of) time before we're ready
    WiFiStatus = BUSY;

    // Only try to initiated un- stuff ;-)
    if (WiFiConnectionState == WIFI_NOT_INITIATED) {
        cWiFiInit();
        cWiFiLoadAthHwModules();  // Load the foundation for the rest
        cWiFiGetLogicalName();

#ifdef DEBUG
        printf("\ncWiFiTurnOn and LOGIC NAME FOUND: %s\n", LogicalIfName);
#endif

        // Remove the Control Interface - if exist...
        strcpy(Command, "rm /var/run/wpa_supplicant/");
        strcat(Command, LogicalIfName);

        strcat(Command, " &> /dev/null");
        system(Command);

        Result  =  OK;
        WiFiConnectionState = WIFI_INIT;
        WiFiOnStatus = OK;
    } else {
        // Just debugging stuff
#ifdef DEBUG
        printf("\ncWiFiTurnOn refused ALREADY ON or should be....\n");
#endif
    }

    WiFiStatus = OK;

    return Result;
}

RESULT cWiFiTurnOff(void)
{
    RESULT  Result = FAIL;
    int PC = 0;

#ifdef DEBUG
    printf("cWiFiTurnOff called...\n");
#endif

    WiFiOnStatus = FAIL;

    if (ctrl_conn) {
        cWiFiKillUdHcPc();
        BeaconTx = NO_TX;

        // Store persistent data - i.e. all the old and new connections and -data

        PC = cWiFiStoreKnownApList();
        if (PC >= 0) {
            Result = OK;
        }

#ifdef DEBUG
        printf("cWiFiStoreKnownApList() returned %d\n", PC);
#endif

        cWiFiTcpClose();
        cWiFiUdpClientClose();
        cWiFiTerminate();

#ifdef DEBUG
        if (WiFiStatus == OK) {
            printf("At TurnOff cWiFiTerminate()-> WiFiStatus == OK\n");
        } else {
            printf("At TurnOff cWiFiTerminate()-> WiFiStatus == NOT ok\n");
        }
#endif

        WiFiConnectionState = WIFI_NOT_INITIATED;
        InitState = NOT_INIT;
        wpa_ctrl_close(ctrl_conn);
        ctrl_conn = NULL;
        // Unload the foundation for the rest
        cWiFiUnLoadAthHwModules();
        KnownWiFiDongle = HW_NOT_KNOWN;
    }

#ifdef DEBUG
    if (WiFiStatus == OK) {
        printf("At TurnOff -> WiFiStatus == OK\n");
    } else {
        printf("At TurnOff -> WiFiStatus == NOT ok\n");
    }
#endif
    // NO FAIL BREAKing
    WiFiStatus = OK;
    Result  =  OK;
    // We should hopefully be disconnected as well
    // I.e. Turned OFF!
    ApTable[0].ap_flags &= ~(CONNECTED);

    return Result;
}

RESULT cWiFiExit(void)
{
    RESULT Result;

    Result = cWiFiTurnOff();

    return Result;
}

RESULT cWiFiInit(void)
{
    RESULT Result = FAIL;

#ifdef DEBUG
    printf("\ncWiFiInit START %d\n", WiFiStatus);
#endif

    WiFiOnStatus = FAIL;
    BeaconTx = NO_TX;
    // We're sleeping until user select ON
    WiFiConnectionState = WIFI_NOT_INITIATED;
    InitState = NOT_INIT;
    TcpReadState = TCP_IDLE;
    // Set IP address to UNKNOWN
    strcpy(MyIp4Address, "???");
    // Set MAC address of Interface to UNKNOWN (NOT read yet)
    strcpy(MyHwMacAddress, "??:??:??:??:??:??");
    cWiFiSetBtSerialNo();
    cWiFiSetBrickName();
    WiFiStatus = OK;
    Result = OK;

#ifdef DEBUG
    printf("\nWiFiStatus = %d\n", WiFiStatus);
#endif

    return Result;
}
