#include <stdint.h>
#include "gatt_api.h"

#define SHARED_KEY 0x52696e67
#define NOTIFY_KEY 0x676e6952

typedef enum msg_type
{
    // communication session with the server
    MSG_OPEN_SESSION = 0,
    MSG_CLOSE_SESSION,

    // controller / addapter
    MSG_POWER,
    MSG_CONFIG,

    // advertisement
    MSG_START_ADVERTISEMENT,
    MSG_STOP_ADVERTISEMENT,

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

typedef struct Open_session
{

} Open_Session_t;

typedef struct Close_session
{
    uint8_t force_shutdown; // 1 - force shutdown
    uint8_t force_override; // 1 - close any open session
} Close_Session_t;

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

typedef struct Start_advertisement
{

} Start_Advertisement_t;

typedef struct Stop_advertisement
{

} Stop_Advertisement_t;

typedef struct Add_service
{

} Add_Service_t;

typedef struct Add_attribute
{
    uint16_t size;
    uint8_t  data[1];
} Add_Attribute_t;

typedef struct Update_attribute
{
    uint8_t  attr_idx;
    uint16_t size;
    uint8_t  data[1];
} Update_Attribute_t;

typedef struct Notify_status_change
{
    uint8_t on_off; // 1 - on; 0 - off

} Notify_Status_Change_t;

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
        Open_Session_t          open_session;
        Close_Session_t         close_session;
        Power_t                 power;
        Config_t                config;
        Start_Advertisement_t   start_advertisement;
        Stop_Advertisement_t    stop_advertisement;
        Add_Service_t           add_service;
        Add_Attribute_t         add_attribute;
        Update_Attribute_t      update_attribute;
        Notify_Status_Change_t  notify_connect;
        Notify_Data_Read_t      notify_data_read;
        Notify_Data_Write_t     notify_data_write;
    } data;

} Comm_Msg_t;

typedef struct comm_msgbuf
{
    __syscall_slong_t mtype;	/* type of received/sent message */
    Comm_Msg_t msg;
} Comm_Msgbuf_t;

