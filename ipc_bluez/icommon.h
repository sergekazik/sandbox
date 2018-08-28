 #include <stdint.h>

#define SHARED_KEY 0x52696e67
#define MAXSIZE     128

typedef enum msg_type
{
    // establish / end communication session with the server
    MSG_OPEN_SESSION = 0,
    MSG_CLOSE_SESSION,

    // controller / addapter
    MSG_POWER_ON,
    MSG_CONFIG,
    MSG_POWER_OFF,

    // advertisement
    MSG_START_ADVERTISEMENT,
    MSG_STOP_ADVERTISEMENT,

    // GATT attribute operations
    MSG_ADD_SERVICE,
    MSG_ADD_ATTRIBUTE,
    MSG_UPDATE_ATTRIBUTE,

    // Notifications request
    MSG_REGISTER_ON_CONNECT,
    MSG_REGISTER_ON_DISCONNECT,
    MSG_REGISTER_ON_DATA_READ,
    MSG_REGISTER_ON_DATA_WRITE,

    // Server notifications to Client
    MSG_NOTIFY_ON_CONNECT,
    MSG_NOTIFY_ON_DISCONNECT,
    MSG_NOTIFY_ON_DATA_READ,
    MSG_NOTIFY_ON_DATA_WRITE,

} Msg_Type_t;

typedef enum error_type
{
    NO_ERROR = 0,
    INVALID_PARAMETER,
    UNAVAILABLE,
    GENERAL_ERROR
} Error_Type_t;

typedef struct Open_session
{

} Open_Session_t;

typedef struct Close_session
{

} Close_Session_t;

typedef struct Power_on
{

} Power_On_t;

typedef struct Config
{

} Config_t;

typedef struct Power_off
{

} Power_Off_t;

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

} Add_Attribute_t;

typedef struct Update_attribute
{

} Update_Attribute_t;

typedef struct Register_on_connect
{

} Register_On_Connect_t;

typedef struct Register_on_disconnect
{

} Register_On_Disconnect_t;

typedef struct Register_on_data_read
{

} Register_On_Data_Read_t;

typedef struct Register_on_data_write
{

} Register_On_Data_Write_t;

typedef struct Notify_on_connect
{

} Notify_On_Connect_t;

typedef struct Notify_on_disconnect
{

} Notify_On_Disconnect_t;

typedef struct Notify_on_data_read
{

} Notify_On_Data_Read_t;

typedef struct Notify_on_data_write
{

} Notify_On_Data_Write_t;

typedef struct comm_msg
{
    Msg_Type_t      type;
    Error_Type_t    error;
    uint8_t         session_id;
    union {
        Open_Session_t           open_session;
        Close_Session_t          close_session;
        Power_On_t               power_on;
        Config_t                 config;
        Power_Off_t              power_off;
        Start_Advertisement_t    start_advertisement;
        Stop_Advertisement_t     stop_advertisement;
        Add_Service_t            add_service;
        Add_Attribute_t          add_attribute;
        Update_Attribute_t       update_attribute;
        Register_On_Connect_t    register_connect;
        Register_On_Disconnect_t register_disconnect;
        Register_On_Data_Read_t  register_data_read;
        Register_On_Data_Write_t register_data_write;
        Notify_On_Connect_t      notify_connect;
        Notify_On_Disconnect_t   notify_disconnect;
        Notify_On_Data_Read_t    notify_data_read;
        Notify_On_Data_Write_t   notify_data_write;
    } data;

} Comm_Msg_t;

typedef struct comm_msgbuf
{
    __syscall_slong_t mtype;	/* type of received/sent message */
    Comm_Msg_t msg;
} Comm_Msgbuf_t;

