/*
 *
 * Copyright 2018 Amazon.com, Inc. and its affiliates. All Rights Reserved.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; version 2
 * of the License.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/gpl-2.0.html>
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>

#include "gatt_api.h"

#define SERVER      true
#define CLIENT      false
#define TO_SERVER   false
#define TO_CLIENT   true
#define FROM_SERVER false
#define FROM_CLIENT true

#ifdef DEBUG_ENABLED
#define DEBUG_PRINTF(_args_) {printf _args_; printf("\n");}
#define DEBUG_RETURN(_val) printf("%s %d: ", __FUNCTION__, __LINE__);DEBUG_PRINTF((#_val)); return _val;
#else
#define DEBUG_RETURN(_val) return _val
#define DEBUG_PRINTF(_args_)
#define TRACE()
#endif

#ifdef TRACE_ALL
#define TRACE_DEBUG_PRINTF DEBUG_PRINTF
#define TRACE() {printf("%s %d\n", __FUNCTION__, __LINE__);fflush(stdout);}
#else
#define TRACE_DEBUG_PRINTF(_args_)
#define TRACE()
#endif

typedef enum msg_type
{
    // communication session with the server
    MSG_SESSION = 0,

    // controller / addapter
    MSG_POWER,
    MSG_CONFIG,

    // advertisement
    MSG_ADVERTISEMENT,

    // GATT attribute operations
    MSG_ADD_SERVICE,
    MSG_ADD_ATTRIBUTE,
    MSG_UPDATE_ATTRIBUTE,

    // Server notifications to Client
    MSG_NOTIFY_CONNECT_STATUS,
    MSG_NOTIFY_DATA_READ,
    MSG_NOTIFY_DATA_WRITE,

    MSG_COUNT_MAX,
    // don't add actual message or notification types below

    // client controlling commands - not used in comm - for sample/demo only
    CMD_WAIT_NOTIFICATIONS,
    CMD_SLEEP_SECONDS,
    CMD_PAUSE_GETCHAR,
    CMD_SERVER_EXIT
} Msg_Type_t;

typedef struct session
{
    uint8_t on_off;         // 1 - open session; 0 - close
    uint8_t force_shutdown; // 1 - force shutdown
    uint8_t force_override; // 1 - close any open session
} Session_t;

typedef struct _power
{
    uint8_t on_off; // 1 - on; 0 - off
} Power_t;

typedef struct _config
{
    uint32_t device_class;
    char device_name[DEV_NAME_LEN];
    char mac_address[DEV_MAC_ADDR_LEN];
} Config_t;

typedef struct _advertisement
{
    uint8_t on_off; // 1 - start; 0 - stop
} Advertisement_t;

typedef struct _add_service
{
    uint16_t uuid;
    uint8_t         count;
} Add_Service_t;

typedef struct _add_attribute
{
    uint16_t        uuid;
    int8_t          name[ATTR_NAME_LEN];
    uint16_t        max_length;
    uint16_t        size;
    uint8_t         type;
    uint8_t         properties;
    uint8_t         data[1];
} Add_Attribute_t;

typedef struct _update_attribute
{
    uint16_t size;
    uint8_t  attr_idx;
    uint8_t  data[1];
} Update_Attribute_t;

typedef struct _notify_connect_status
{
    uint8_t on_off; // 1 - connected; 0 - disconnected
} Notify_Connect_Status_t;

typedef struct _notify_data_read
{
    uint8_t attr_idx;
} Notify_Data_Read_t;

typedef Update_Attribute_t Notify_Data_Write_t;

typedef struct _common_header
{
    uint16_t type;      // message type
    uint16_t error;     // error (filled by server in response)
    uint16_t session_id;// session id (filled by server in session open response)
    uint16_t size;      // actual size of the message including following payload
} Common_Header_t;

typedef struct _comm_msg
{
    Common_Header_t             hdr;
    union {
        Session_t               session;
        Power_t                 power;
        Config_t                config;
        Advertisement_t         advertisement;
        Add_Service_t           add_service;
        Add_Attribute_t         add_attribute;
        Update_Attribute_t      update_attribute;
        Notify_Connect_Status_t notify_connect;
        Notify_Data_Read_t      notify_data_read;
        Notify_Data_Write_t     notify_data_write;
    } data;
} Comm_Msg_t;

// helper defitions
typedef struct _define_update
{
    uint16_t size;
    uint8_t  attr_idx;
    const char  *data;
} Define_Update_t;

typedef struct _define_attribute
{
    uint16_t        uuid;
    int8_t          name[ATTR_NAME_LEN];
    uint16_t        max_length;
    uint16_t        size;
    uint8_t         type;
    uint8_t         properties;
    const char     *data;
} Define_Attribute_t;

/**
 * @brief die
 * @param s - string prefix
 * @param err - error code to print out
 */
void die(const char *s, int err);

/**
 * @brief parse_command_line
 * @param argc
 * @param argv
 * @return int - error code
 */
int parse_command_line(int argc, char** argv);

/**
 * @brief get_msg_name
 * @param cm
 * @return const char *
 */
const char *get_msg_name(Comm_Msg_t *cm);

/**
 * @brief get_err_name
 * @param ret
 * @return const char*
 */
const char* get_err_name(int ret);

/**
 * @brief init_comm
 * @param bServer
 * @return int - error code
 */
int init_comm(bool bServer);

/**
 * @brief send_to_server
 * @param buffer
 * @param size
 * @return int - error code
 */
int send_to_server(Comm_Msg_t *buffer, int size);

/**
 * @brief recv_from_client
 * @param buffer
 * @param size
 * @return int - error code
 */
int recv_from_client(Comm_Msg_t *buffer, int size);

/**
 * @brief resp_to_client
 * @param msg
 * @param size
 * @return int - error code
 */
int resp_to_client(Comm_Msg_t *msg, int size);

/**
 * @brief resp_from_server
 * @param msg
 * @param size
 * @param timeout_ms
 * @return int - error code
 */
int resp_from_server(Comm_Msg_t *msg, int size, int timeout_ms);

/**
 * @brief notify_client
 * @param msg
 * @param size
 * @return int - error code
 */
int notify_client(Comm_Msg_t *msg, int size);

/**
 * @brief wait_notification
 * @param buffer
 * @param size
 * @param timeout_ms
 * @return int - error code
 */
int wait_notification(Comm_Msg_t *buffer, int size, int timeout_ms);

/**
 * @brief shut_comm
 * @return int - error code
 */
int shut_comm();

/**
 * @brief format_attr_add_msg
 * @param stash
 * @param attr_new
 * @return Comm_Msg_t * - pointer to formatted message; must be freed if != stash
 */
Comm_Msg_t *format_attr_add_msg(Comm_Msg_t *stash, Define_Attribute_t *attr_new);

/**
 * @brief format_attr_updated_msg
 * @param type
 * @param stash
 * @param attr_new
 * @return Comm_Msg_t * - pointer to formatted message; must be freed if != stash
 */
Comm_Msg_t *format_attr_updated_msg(Msg_Type_t type, Comm_Msg_t *stash, Define_Update_t *attr_new);

/**
 * @brief format_message_payload
 * @param session_id
 * @param type
 * @param msg
 * @param data
 * @return void* - pointer to new message; must be freed if != NULL; otherwise payload added to msg
 */
void* format_message_payload(uint16_t session_id, Msg_Type_t type, Comm_Msg_t &msg, void* data);

