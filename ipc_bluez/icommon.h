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
#define DEBUG_PRINTF(...) {printf(__VA_ARGS__); printf("\n");}
#define TRACE() {printf("%s %d\n", __FUNCTION__, __LINE__);fflush(stdout);}
#else
#define DEBUG_PRINTF
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

    // client controlling commands - not used in communicatoin for sample/demo only
    CMD_WAIT_NOTIFICATIONS,
    CMD_SLEEP_SECONDS,
    CMD_PAUSE_GETCHAR
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
    Ble::ServiceInfo_t desc;
} Add_Service_t;

typedef struct _add_attribute
{
    Ble::AttributeInfo_t attr;
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
    uint8_t  *data;
} Define_Update_t;

///
/// \brief die
/// \param s
///
void die(const char *s, int err);

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
/// \brief format_attr_add_msg
/// \param stash
/// \param attr_new
/// \return
///
Comm_Msg_t *format_attr_add_msg(Comm_Msg_t *stash, Add_Attribute_t *attr_new);

///
/// \brief format_attr_updated_msg
/// \param stash
/// \param attr_idx
/// \param attr_new
/// \return pointer to stash or pointer of the newly allocated Comm_Msg_t message
///
Comm_Msg_t *format_attr_updated_msg(Msg_Type_t type, Comm_Msg_t *stash, Define_Update_t *attr_new);

/// debug only
const char *get_msg_name(Comm_Msg_t *cm);
const char* get_err_name(int ret);

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
/// \param buffer
/// \param size
/// \return
///
int recv_comm(bool bServer, char *buffer, int size, int timeout_ms = 0);

///
/// \brief shut_comm
/// \param bServer
/// \return
///
int shut_comm(bool bServer);

