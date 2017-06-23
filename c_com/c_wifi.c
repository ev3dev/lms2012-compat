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

#define C_WIFI_HIDDEN_NAME "[HIDDEN]"
#define C_WIFI_CONNMAN_AGENT_DBUS_PATH "/org/ev3dev/lms2012/connman/agent"

#define C_WIFI_SERVICE_SSID_QUARK c_wifi_service_ssid_quark()
#define C_WIFI_SERVICE_PASSPHRASE_QUARK c_wifi_service_passphrase_quark()
#define C_WIFI_SERVICE_CONNECT_RESULT_QUARK c_wifi_service_connect_result_quark()

typedef struct {
    ConnmanService *proxy;
    GSocket *broadcast;
    GSocketAddress *broadcast_address;
    guint broadcast_source_id;
    GSocketService *service;
    GSocketConnection *connection;
} ConnectionData;

// States the TCP connection can be in (READ)
typedef enum {
    TCP_IDLE                = 0x00,
    TCP_WAIT_ON_START       = 0x01,
    TCP_WAIT_ON_LENGTH      = 0x02,
    TCP_WAIT_ON_FIRST_CHUNK = 0x04,
    TCP_WAIT_ON_ONLY_CHUNK  = 0x08,
    TCP_WAIT_COLLECT_BYTES  = 0x10
} TCP_READ_STATE;

static char BtSerialNo[13];              // Storage for the BlueTooth Serial Number
static char BrickName[NAME_LENGTH + 1];  // BrickName for discovery and/or friendly info

static RESULT WiFiStatus = OK;

static UWORD TcpTotalLength = 0;
static UWORD TcpRestLen = 0;
static TCP_READ_STATE TcpReadState = TCP_IDLE;
static uint TcpReadBufPointer = 0;

static ConnectionData *connection_data = NULL;
static ConnmanManager *connman_manager = NULL;
static ConnmanTechnology *ethernet_technology = NULL;
static ConnmanTechnology *wifi_technology = NULL;
static GList *service_list = NULL;
static DATA16 service_list_state = 1;
static GList *removed_list = NULL;
static ConnmanAgent *connman_agent = NULL;

// ******************************************************************************

static G_DEFINE_QUARK(cWifiServiceSsidQuark, c_wifi_service_ssid)
static G_DEFINE_QUARK(cWifiServicePassphraseQuark, c_wifi_service_passphrase)
static G_DEFINE_QUARK(cWifiServiceConnectResultQuark, c_wifi_service_connect_result)

/**
 * @brief           Move a service up in the service list.
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

    service_list = g_list_remove(service_list, service);
    service_list = g_list_insert(service_list, service, Index - 1);
}

/**
 * @brief           Move a service down in the service list.
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

    if (!connman_service_call_move_after_sync(service,
        g_dbus_proxy_get_object_path(next), NULL, &error))
    {
        g_printerr("Failed to move down: %s\n", error->message);
        g_error_free(error);
    }

    service_list = g_list_remove(service_list, service);
    service_list = g_list_insert(service_list, service, Index + 1);
}

void cWiFiSetEncryptToWpa2(int Index)
{
    WiFiStatus = OK;
    // TODO: Connman does not allow us to select the security.
}

void cWiFiSetEncryptToNone(int Index)
{
    WiFiStatus = OK;
    // TODO: Connman does not allow us to select the security.
}

/**
 * @brief           Get the encryption type for the specified connection.
 *
 * @param Index     The index of the service int the service list.
 *
 * @return          The encryption type.
 */
ENCRYPT cWifiGetEncrypt(int Index)
{
    ConnmanService *service = g_list_nth_data(service_list, Index);

    if (service) {
        const gchar *const *security;
        security = connman_service_get_security (service);
        for (; *security; security++) {
            if (g_strcmp0(*security, "psk") == 0) {
                return ENCRYPT_WPA2;
            }
        }
    }

    return ENCRYPT_NONE;
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
    ConnmanService *service;
    RESULT Result = FAIL;

    service = g_list_nth_data(service_list, 0);
    if (service) {
        GVariant *enet;
        GVariant *address;

        enet = connman_service_get_ethernet(service);
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

    // Apparently the EV3 desktop software doesn't know (much) about null
    // terminators. If we don't zero the entire array, there will be junk
    // displayed after the name when editing the name in the Wireless Setup
    // dialog in the desktop software.
    memset(ApName, 0, Length);

    service = g_list_nth_data(service_list, Index);
    if (service) {
        const gchar *name;

        // hidden WiFi SSIDs will return NULL
        name = connman_service_get_name(service);
        if (name) {
            snprintf(ApName, Length, "%s", name);
        } else {
            snprintf(ApName, Length, "%s", C_WIFI_HIDDEN_NAME);
        }
        Result = OK;
    } else {
        strncpy(ApName, "None", Length);
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

static void cWifiConnectToApFinish(GObject *source_object,
                                   GAsyncResult *res,
                                   gpointer user_data)
{
    ConnmanService *proxy = CONNMAN_SERVICE(source_object);
    GError *error = NULL;

    if (connman_service_call_connect_finish(proxy, res, &error)) {
        g_object_set_qdata(source_object, C_WIFI_SERVICE_CONNECT_RESULT_QUARK,
                           GINT_TO_POINTER(START));
    } else {
        g_object_set_qdata(source_object, C_WIFI_SERVICE_CONNECT_RESULT_QUARK,
                           GINT_TO_POINTER(FAIL));
        g_printerr("Connect failed: %s\n", error->message);
        g_error_free(error);
    }
}

/**
 * @brief           Connect to the service at the specified index.
 *
 * @param Index     Index of the service in the service_list.
 *
 * @return          OK on success, BUSY if waiting for completion or FAIL on
 *                  error.
 */
RESULT cWiFiConnectToAp(int Index)
{
    ConnmanService *proxy;
    RESULT Result;

    pr_dbg("cWiFiConnectToAp(int Index = %d)\n", Index);

    proxy = g_list_nth_data(service_list, Index);
    g_return_val_if_fail(proxy != NULL, FAIL);

    Result = GPOINTER_TO_INT(g_object_get_qdata(G_OBJECT(proxy),
                             C_WIFI_SERVICE_CONNECT_RESULT_QUARK));
    if (Result == OK) {
        // if we are not already trying to connect, start a connection request
        connman_service_call_connect(proxy, NULL, cWifiConnectToApFinish, NULL);
        g_object_set_qdata(G_OBJECT(proxy), C_WIFI_SERVICE_CONNECT_RESULT_QUARK,
                           GINT_TO_POINTER(BUSY));
        WiFiStatus = BUSY;
        Result = BUSY;
    }
    else if (Result == START) {
        // the service was successfully connected
        g_object_set_qdata(G_OBJECT(proxy), C_WIFI_SERVICE_CONNECT_RESULT_QUARK,
                           GINT_TO_POINTER(OK));
        WiFiStatus = OK;
        Result = OK;
    }
    else if (Result == FAIL) {
        // connection failed - pass error to caller and reset connection state (quark)
        g_object_set_qdata(G_OBJECT(proxy), C_WIFI_SERVICE_CONNECT_RESULT_QUARK,
                           GINT_TO_POINTER(OK));
        WiFiStatus = FAIL;
    }
    // implicit else if (Result == BUSY) { // do nothing }

    return Result;
}

// Make the pre-shared key from
// Supplied SSID and PassPhrase
// And store it in ApTable[Index]
RESULT cWiFiMakePsk(char *ApSsid, char *PassPhrase, int Index)
{
    GObject *service;
    RESULT Result = FAIL;

    WiFiStatus = BUSY;

    service = g_list_nth_data(service_list, Index);
    if (service) {
        g_free(g_object_get_qdata(service, C_WIFI_SERVICE_SSID_QUARK));
        g_object_set_qdata(service, C_WIFI_SERVICE_SSID_QUARK, g_strdup(ApSsid));
        g_free(g_object_get_qdata(service, C_WIFI_SERVICE_PASSPHRASE_QUARK));
        g_object_set_qdata(service, C_WIFI_SERVICE_PASSPHRASE_QUARK, g_strdup(PassPhrase));
        Result = OK;
    }

    WiFiStatus = OK;

    return Result;
}

/**
 * @brief           Gets the index of a service for a given name
 *
 * @param Name      The name to search for.
 * @param Index     Pointer to store the index if found.
 * @return          OK if a match was found, otherwise FAIL
 */
RESULT cWiFiGetIndexFromName(const char *Name, UBYTE *Index)
{
    GList *item;
    RESULT Result = FAIL;

    // Publicly, hidden APs are listed as C_WIFI_HIDDEN_NAME, but in connman,
    // they are NULL.
    if (g_strcmp0(Name, C_WIFI_HIDDEN_NAME) == 0) {
        Name = NULL;
    }

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
 * This just triggers a scan in the background and returns immediately.
 *
 * @return          OK if scan was started, otherwise FAIL.
 */
RESULT cWiFiScanForAPs()
{
    RESULT Result = FAIL;

    pr_dbg("cWiFiScanForAPs\n");

    WiFiStatus = BUSY;

    // if there are any removals pending, we can remove them from service_list
    // safely now.
    while (removed_list) {
        ConnmanService *service = g_list_nth_data(removed_list, 0);

        service_list = g_list_remove(service_list, service);
        removed_list = g_list_remove(removed_list, service);
        g_object_unref(service);
    }

    if (wifi_technology && connman_technology_get_powered(wifi_technology)) {
        connman_technology_call_scan(wifi_technology, NULL, NULL, NULL);
        Result = OK;
    }

    WiFiStatus = OK;

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

/**
 * @brief               Read the serial number from file
 */
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

/**
 * @brief               Read the brick name from file
 */
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

/**
 * @brief               Broadcast advertisement that we are here.
 *
 * This sends a UDP broadcast message to let other programs know that this is
 * an "EV3" and we are listening for connections.
 *
 * @param user_data     A ConnectionData struct.
 * @return              G_SOURCE_CONTINUE if the broadcast was successful,
 *                      otherwise G_SOURCE_REMOVE.
 */
static gboolean broadcast_udp(gpointer user_data)
{
    ConnectionData *data = user_data;
    GError *error = NULL;
    gchar message[128];

    cWiFiSetBtSerialNo(); // Be sure to have updated data :-)
    cWiFiSetBrickName();  // -
    g_snprintf(message, 128,
               "Serial-Number: %s\r\nPort: %d\r\nName: %s\r\nProtocol: EV3\r\n",
               BtSerialNo, TCP_PORT, BrickName);

    if (g_socket_send_to(data->broadcast, G_SOCKET_ADDRESS(data->broadcast_address),
                         message, strlen(message), NULL, &error) < 0)
    {
        g_printerr("Failed to send UDP broadcast: %s\n", error->message);
        g_error_free(error);

        return G_SOURCE_REMOVE;
    }

    return G_SOURCE_CONTINUE;
}

/**
 * @brief               Start broadcasting the UDP beacon.
 *
 * @param data          The connection data.
 */
static void cWiFiStartBroadcast(ConnectionData *data)
{
    if (data->broadcast_source_id == 0) {
        data->broadcast_source_id = g_timeout_add_seconds(BEACON_TIME,
                                                          broadcast_udp, data);
    }
}

/**
 * @brief               Stop broadcasting the UDP beacon.
 *
 * @param data          The connection data.
 */
static void cWiFiStopBroadcast(ConnectionData *data)
{
    if (data->broadcast_source_id != 0) {
        g_source_remove(data->broadcast_source_id);
        data->broadcast_source_id = 0;
    }
}

/**
 * @brief               Write data to a TCP socket.
 *
 * This is a non-blocking operation, so bytes may not be written if the socket
 * is busy. Also returns 0 if there is no active connection.
 *
 * @param Buffer        The data to write.
 * @param Length        The number of bytes to write.
 * @return              The number of bytes actually written.
 */
UWORD cWiFiWriteTcp(UBYTE* Buffer, UWORD Length)
{
    gssize DataWritten = 0;                 // Nothing written (BUSY)

    if (Length > 0) {
#if 0 // this makes lots of noise
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

        if (connection_data && connection_data->connection) {
            GSocket *socket = g_socket_connection_get_socket(connection_data->connection);
            GError *error = NULL;

            DataWritten = g_socket_send(socket, (gchar *)Buffer, Length, NULL, &error);
            if (DataWritten == -1) {
                if (!g_error_matches(error, G_IO_ERROR, G_IO_ERROR_WOULD_BLOCK)) {
                    g_printerr("Failed to write data: %s\n", error->message);
                }
                g_error_free(error);
                DataWritten = 0;
            }
        }
    }

    return DataWritten;
}

/**
 * @brief           Close the active TCP connection
 *
 * @param data      The connection data.
 * @return          OK if closing succeeded, otherwise FAIL.
 */
static RESULT cWiFiTcpClose(ConnectionData *data)
{
    GIOStream *connection = G_IO_STREAM(data->connection);
    GError *error = NULL;
    RESULT Result = FAIL;

    data->connection = NULL;
    if (g_io_stream_close(connection, NULL, &error)) {
        Result = OK;
    } else {
        g_printerr("Failed to close connection: %s\n", error->message);
        g_error_free(error);
    }
    g_object_unref(connection);

    return Result;
}

/**
 * @brief           Reset the TCP connection state.
 *
 * @param data      The connection data.
 * @return          OK on success, otherwise FAIL.
 */
static RESULT cWiFiResetTcp(void)
{
    RESULT Result;

    pr_dbg("\nRESET - client disconnected!\n");

    TcpReadState = TCP_IDLE;
    Result = cWiFiTcpClose(connection_data);
    cWiFiStartBroadcast(connection_data);

    return Result;
}

/**
 * @brief           Reads data from the active TCP connection.
 *
 * This is a non-blocking operation, so it will return 0 if there is no data
 * to read. It also returns 0 if there is not an active connection.
 *
 * @param Buffer    A pre-allocated buffer to store the read data.
 * @param Length    The length of the buffer (number of bytes to read).
 * @return          The number of bytes actually read.
 */
UWORD cWiFiReadTcp(UBYTE* Buffer, UWORD Length)
{
    gssize DataRead = 0;

    if (connection_data && connection_data->connection) {
        GSocket *socket = g_socket_connection_get_socket(connection_data->connection);
        GError *error = NULL;
        gsize read_length;

        // setup for read

        switch (TcpReadState) {
        case TCP_IDLE:
            // Do Nothing
            return 0;
        case TCP_WAIT_ON_START:
            TcpReadBufPointer = 0;
            read_length = 100; // Fixed TEXT
            break;
        case TCP_WAIT_ON_LENGTH:
            // We can should read the length of the message
            // The packets can be split from the client
            // I.e. Length bytes (2) can be send as a subset
            // the Sequence can also arrive as a single pair of bytes
            // and the finally the payload will be received

            // Begin on new buffer :-)
            TcpReadBufPointer = 0;
            read_length = 2;
            break;
        case TCP_WAIT_ON_ONLY_CHUNK:
            read_length = TcpRestLen;
            break;
        case TCP_WAIT_ON_FIRST_CHUNK:
            read_length = Length - 2;
            break;
        case TCP_WAIT_COLLECT_BYTES:
            TcpReadBufPointer = 0;
            if (TcpRestLen < Length) {
                read_length = TcpRestLen;
            } else {
                read_length = Length;
            }
            break;
        default:
            // Should never go here...
            TcpReadState = TCP_IDLE;
            return 0;
        }

        // do the actual read

        DataRead = g_socket_receive(socket, (gchar *)Buffer + TcpReadBufPointer,
                                    read_length, NULL, &error);
        if (DataRead == -1) {
            if (!g_error_matches(error, G_IO_ERROR, G_IO_ERROR_WOULD_BLOCK)) {
                g_printerr("Failed to read data: %s\n", error->message);
            }
            g_error_free(error);
            DataRead = 0;
        } else {
            // handle the read data

            switch (TcpReadState) {
            case TCP_IDLE:
                break;
            case TCP_WAIT_ON_START:
                pr_dbg("TCP_WAIT_ON_START:\n");
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
                pr_dbg("TCP_WAIT_ON_LENGTH:\n");
                if (DataRead == 0) {
                    // We've a disconnect
                    cWiFiResetTcp();
                    break;
                }

                TcpRestLen = (UWORD)(Buffer[0] + Buffer[1] * 256);
                TcpTotalLength = (UWORD)(TcpRestLen + 2);
                if (TcpTotalLength > Length) {
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
#if 0 // this makes lots of noise
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
            }
        }
    }

    return DataRead;
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
        WiFiStatus = OK;
    } else {
        // cWiFiTcpClose();
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
            gchar *name;
            GVariant *value;

            g_variant_iter_init(&iter, properties);
            while (g_variant_iter_loop(&iter, "{sv}", &name, &value)) {
                connman_technology_emit_property_changed(proxy, name,
                                                g_variant_new_variant(value));
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
            gchar *name;
            GVariant *value;

            g_variant_iter_init(&iter, properties);
            while (g_variant_iter_loop(&iter, "{sv}", &name, &value)) {
                connman_service_emit_property_changed(proxy, name,
                                                      g_variant_new_variant(value));
            }
            g_variant_unref(properties);
        } else {
            g_printerr("Error getting properties for service: %s\n",
                       error->message);
            g_error_free(error);
        }

        // this is used to keep track of state when connecting
        g_object_set_qdata(G_OBJECT(proxy), C_WIFI_SERVICE_CONNECT_RESULT_QUARK,
                           GINT_TO_POINTER(OK));
    } else {
        g_printerr("Error creating connman service proxy: %s\n", error->message);
        g_error_free(error);
    }

    return proxy;
}

/**
 * brief                Handle incoming TCP connection.
 *
 * @param service       The service object.
 * @param connection    The connection object.
 * @param source_object User-defined object or NULL
 * @param user_data     User-defined data.
 * @return              TRUE to stop other handlers, otherwise FALSE.
 */
static gboolean on_tcp_incoming(GSocketService *service,
                                GSocketConnection *connection,
                                GObject *source_object,
                                gpointer user_data)
{
    ConnectionData *data = user_data;

    pr_dbg("on_tcp_incoming\n");

    if (data->connection) {
        // we already have a connection, so ignore additional connections
        return FALSE;
    }

    cWiFiStopBroadcast(data);
    data->connection = g_object_ref(connection);
    g_socket_set_blocking(g_socket_connection_get_socket(connection), FALSE);
    TcpReadState = TCP_WAIT_ON_START;

    return TRUE;
}

/**
 * @brief               Start the connection service.
 *
 * This starts broadcasting UDP messages and listening for TCP connections.
 * Communications are restricted to the proxy's subnet.
 *
 * @param proxy         The service to use for the connections.
 */
static void cWifiStartConnection(ConnmanService *proxy)
{
    ConnectionData *data;
    GVariant *ipv4, *value;
    GInetAddress *address;
    GSocketAddress *socket_address;
    GError *error = NULL;
    guint32 bytes, mask;

    pr_dbg("cWifiStartConnection\n");

    data = g_malloc0(sizeof(ConnectionData));
    data->proxy = proxy;

    // init the UDP broadcast socket

    data->broadcast = g_socket_new(G_SOCKET_FAMILY_IPV4, G_SOCKET_TYPE_DATAGRAM,
                                   G_SOCKET_PROTOCOL_UDP, &error);
    if (!data->broadcast) {
        g_printerr("Failed to create UDP socket:%s\n", error->message);
        g_error_free(error);
        goto err1;
    }

    // compute the UDP broadcast address

    ipv4 = connman_service_get_ipv4(proxy);

    value = g_variant_lookup_value(ipv4, "Address", NULL);
    address = g_inet_address_new_from_string(g_variant_get_string(value, NULL));
    g_variant_unref(value);
    memcpy(&bytes, g_inet_address_to_bytes(address), sizeof(bytes));
    socket_address = g_inet_socket_address_new(address, TCP_PORT);
    g_object_unref(address);

    value = g_variant_lookup_value(ipv4, "Netmask", NULL);
    address = g_inet_address_new_from_string(g_variant_get_string(value, NULL));
    g_variant_unref(value);
    memcpy(&mask, g_inet_address_to_bytes(address), sizeof(mask));
    g_object_unref(address);

    // init the UDP broadcast address

    bytes |= ~mask;
    address = g_inet_address_new_from_bytes((guint8 *)&bytes, G_SOCKET_FAMILY_IPV4);
    g_socket_set_broadcast(data->broadcast, TRUE);
    data->broadcast_address = g_inet_socket_address_new(address, BROADCAST_PORT);
    g_object_unref(address);

    // init the TCP service

    data->service = g_socket_service_new();
    if (!g_socket_listener_add_address(G_SOCKET_LISTENER(data->service),
                                       socket_address, G_SOCKET_TYPE_STREAM,
                                       G_SOCKET_PROTOCOL_TCP, NULL,
                                       NULL, &error))
    {
        g_object_unref(socket_address);
        g_printerr("Failed to create TCP listener:%s\n", error->message);
        g_error_free(error);
        goto err2;
    }
    g_object_unref(socket_address);
    g_signal_connect(data->service, "incoming", G_CALLBACK(on_tcp_incoming), data);

    // start the service and the broadcast

    connection_data = data;
    g_socket_service_start(data->service);
    cWiFiStartBroadcast(data);

    return;

err2:
    g_object_unref(data->service);
    g_object_unref(data->broadcast);
err1:
    g_free(data);
}

/**
 * @brief               Stop the active connection.
 *
 * @param proxy         The service that owns the connection (currently ignored)
 */
static void cWiFiStopConnection(ConnmanService *proxy)
{
    ConnectionData *data = connection_data;

    pr_dbg("cWifiStartConnection\n");

    connection_data = NULL;

    cWiFiStopBroadcast(data);
    g_socket_service_stop(data->service);
    if (data->connection) {
        cWiFiTcpClose(data);
    }
    g_socket_listener_close(G_SOCKET_LISTENER(data->service));
    g_object_unref(data->service);
    g_object_unref(data->broadcast_address);
    g_object_unref(data->broadcast);
    g_free(data);
}

/**
 * @brief               Handle changes in service state
 *
 * If a service is connected and there is not already an active TCP connection,
 * this starts a new connection. If the service belonging to the active TCP
 * connection is disconnected, the TCP connection is stopped.
 *
 * @param proxy         The service that changed state
 */
static void on_service_state_changed(ConnmanService *proxy)
{
    const char *state = connman_service_get_state(proxy);

    pr_dbg("on_service_state_changed: %s: %s\n", connman_service_get_name(proxy),
           state);

    // These two states mean that we have a valid network connection
    if (g_strcmp0(state, "ready") == 0 || g_strcmp0(state, "online") == 0) {
        // It is possible for connman to have multiple active connections.
        // Only the first connection to become ready/online gets used for
        // communications.
        if (!connection_data) {
            cWifiStartConnection(proxy);
            return;
        }
    } else if (connection_data && connection_data->proxy == proxy) {
        // If this service was disconnected and it was being used for
        // communication, then we need to stop it.
        cWiFiStopConnection(proxy);
    }
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
 * Updates the global service_list. Once an item is in service_list, we ignore
 * the order supplied from connman. This is necessary because lms expects the
 * order to not change since it uses "index" to access services.
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

    pr_dbg("on_services_changed\n");

    g_variant_iter_init(&iter, changed);
    while (g_variant_iter_loop(&iter, "(oa{sv})", &path, &properties)) {
        GList *match;

        match = g_list_find_custom(service_list, path,
                                   (GCompareFunc)compare_proxy_path);
        if (!match) {
            // only add the service if it is not already in the list
            ConnmanService *proxy = cWiFiGetConnmanServiceProxy(path);
            service_list = g_list_append(service_list, proxy);
            g_signal_connect(proxy, "notify::state",
                             G_CALLBACK(on_service_state_changed), NULL);
            on_service_state_changed(proxy);
            pr_dbg("added: %s\n", path);
        }
        // path and properties are freed by g_variant_iter_loop
    }

    if (service_list_state == DATA16_MAX) {
        service_list_state = 1;
    } else {
        service_list_state++;
    }

    for (; *removed; removed++) {
        GList *match;

        path = *removed;
        match = g_list_find_custom(service_list, path,
                                   (GCompareFunc)compare_proxy_path);
        if (match) {
            // removed services are saved since we can't change the index of
            // items in service_list here. They will be removed when we call
            // cWiFiScanForAPs
            removed_list = g_list_prepend(removed_list, match->data);
            pr_dbg("removed: %s\n", path);
        } else {
            g_critical("Failed to remove %s", path);
        }
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

static gboolean on_connman_agent_report_error(ConnmanAgent *object,
                                              GDBusMethodInvocation *invocation,
                                              const gchar *arg_service,
                                              const gchar *arg_error)
{
    g_printerr("ConnMan agent error on %s: %s\n", arg_service, arg_error);

    connman_agent_complete_report_error(object, invocation);

    return TRUE;
}

static gboolean on_connman_agent_request_input(ConnmanAgent *object,
                                               GDBusMethodInvocation *invocation,
                                               const gchar *arg_service,
                                               GVariant *arg_fields)
{
    GList *match;
    GObject *service;
    GVariantDict args, result;

    match = g_list_find_custom(service_list, arg_service,
                               (GCompareFunc)compare_proxy_path);
    if (!match) {
        g_dbus_method_invocation_return_dbus_error(invocation,
            "net.connman.Agent.Error.Canceled",
            "Could not find matching service.");

        return TRUE;
    }

    service = match->data;
    g_variant_dict_init(&args, arg_fields);
    g_variant_dict_init(&result, NULL);
    if (g_variant_dict_contains(&args, "Passphrase")) {
        GVariantDict passphrase_dict;

        g_variant_dict_init(&passphrase_dict,
                            g_variant_dict_lookup_value(&args, "Passphrase", NULL));

        g_variant_dict_insert(&result, "Passphrase", "s",
            g_object_get_qdata(service, C_WIFI_SERVICE_PASSPHRASE_QUARK));

        connman_agent_complete_request_input(object, invocation,
                                             g_variant_dict_end(&result));
    }
    else {
        g_dbus_method_invocation_return_dbus_error(invocation,
            "net.connman.Agent.Error.Canceled",
            "Unexpected agent request.");
    }

    return TRUE;
}

static gboolean on_connman_agent_cancel(ConnmanAgent *object,
                                        GDBusMethodInvocation *invocation)
{
    g_printerr("ConnMan agent canceled\n");

    connman_agent_complete_cancel(object, invocation);

    return TRUE;
}

static void cWiFiRegisterAgent(void)
{
    GDBusConnection *connection;
    GError *error = NULL;

    connection = g_bus_get_sync(G_BUS_TYPE_SYSTEM, NULL, &error);
    if (!connection) {
        g_printerr("Failed to get system bus: %s\n", error->message);
        g_clear_error(&error);
    }

    connman_agent = connman_agent_skeleton_new();
    g_signal_connect(connman_agent,
                     "handle-report-error",
                     G_CALLBACK(on_connman_agent_report_error),
                     NULL);
    g_signal_connect(connman_agent,
                     "handle-request-input",
                     G_CALLBACK(on_connman_agent_request_input),
                     NULL);
    g_signal_connect(connman_agent,
                     "handle-cancel",
                     G_CALLBACK(on_connman_agent_cancel),
                     NULL);
    if (!g_dbus_interface_skeleton_export(G_DBUS_INTERFACE_SKELETON(connman_agent),
                                          connection,
                                          C_WIFI_CONNMAN_AGENT_DBUS_PATH,
                                          &error))
    {
        g_printerr("Failed export connman agent: %s\n", error->message);
        g_clear_error(&error);
    }

    if (!connman_manager_call_register_agent_sync(connman_manager,
                                                  C_WIFI_CONNMAN_AGENT_DBUS_PATH,
                                                  NULL,
                                                  &error))
    {
        g_printerr("Failed register connman agent: %s\n", error->message);
        g_clear_error(&error);
    }

    g_object_unref(connection);
}

RESULT cWiFiExit(void)
{
    GError *error = NULL;
    RESULT Result;

    // TODO: Do we want to always turn off WiFi on exit? This is what LEGO does.
    Result = OK; //cWiFiTurnOff();

    if (connman_agent) {
        if (!connman_manager_call_unregister_agent_sync(connman_manager,
                                                        C_WIFI_CONNMAN_AGENT_DBUS_PATH,
                                                        NULL,
                                                        &error))
        {
            g_printerr("Failed to unregister connman agent: %s\n", error->message);
            g_clear_error(&error);
        }
        g_dbus_interface_skeleton_unexport(G_DBUS_INTERFACE_SKELETON(connman_agent));
        g_object_unref(connman_agent);
    }

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

    // We're sleeping until user select ON
    TcpReadState = TCP_IDLE;
    cWiFiSetBtSerialNo();
    cWiFiSetBrickName();
    connman_manager = cWiFiGetConnmanManagerProxy();
    if (connman_manager) {
        cWiFiRegisterAgent();
        Result = OK;
    }

    pr_dbg("\nWiFiStatus = %d\n", WiFiStatus);

    return Result;
}
