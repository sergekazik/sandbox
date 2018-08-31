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
    MSG_NOTIFY_STATUS_CHANGE,
    MSG_NOTIFY_DATA_READ,
    MSG_NOTIFY_DATA_WRITE,

} Msg_Type_t;

typedef enum error_type
{
    NO_ERROR = 0,
    INVALID_PARAMETER,
    SERVER_BUSY,
    GENERAL_ERROR
} Error_Type_t;

typedef struct session
{
    uint8_t on_off;         // 1 - open session; 0 - close
    uint8_t force_shutdown; // 1 - force shutdown
    uint8_t force_override; // 1 - close any open session
} Session_t;

typedef struct Power
{
    uint8_t on_off; // 1 - on; 0 - off
} Power_t;

typedef struct Config
{
    uint32_t device_class;
    char device_name[DEV_NAME_LEN];
    char mac_address[DEV_MAC_ADDR_LEN];
} Config_t;

typedef struct advertisement
{
    uint8_t on_off; // 1 - start; 0 - stop
} Advertisement_t;

typedef struct Add_service
{
    uint8_t  svc_idx;
    uint16_t size;
    uint8_t  data[1];
} Add_Service_t;

typedef struct Add_attribute
{
    uint8_t  attr_idx;
    uint16_t size;
    uint8_t  data[1];
} Add_Attribute_t;

typedef struct Update_attribute
{
    uint8_t  attr_idx;
    uint16_t size;
    uint8_t  data[1];
} Update_Attribute_t;

typedef struct Notify_connect_status
{
    uint8_t on_off; // 1 - connected; 0 - disconnected
} Notify_Connect_Status_t;

typedef struct Notify_data_read
{
    uint8_t attr_idx;
} Notify_Data_Read_t;

typedef struct Notify_data_write
{
    uint8_t  attr_idx;
    uint16_t size;
    uint8_t  data[1];
} Notify_Data_Write_t;

typedef struct comm_msg
{
    Msg_Type_t      type;
    Error_Type_t    error;
    uint8_t         session_id;
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

///
/// \brief die
/// \param s
///
void die(const char *s);

///
/// \brief parse_command_line
/// \param argc
/// \param argv
/// \param gbIpc
/// \param guiKey
/// \param giPort
/// \return
///
int parse_command_line(int argc, char** argv);

///
/// \brief get_msg_name
/// \param cm
///
const char *get_msg_name(Comm_Msg_t *cm);

///
/// \brief init_comm
/// \param bServer
/// \return
///
int init_comm(bool bServer);

///
/// \brief send_comm
/// \param bServer
/// \param msg
/// \param size
/// \return
///
int send_comm(bool bServer, Comm_Msg_t *msg, int size);

///
/// \brief recv_comm
/// \param bServer
/// \param msg
/// \return
///
int recv_comm(bool bServer, Comm_Msg_t *msg);

///
/// \brief shut_comm
/// \param bServer
/// \return
///
int shut_comm(bool bServer);

