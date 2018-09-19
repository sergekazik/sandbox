#include "rmnp_api.h"
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <errno.h>
#include <pthread.h>

/*
 * TODO: replace with proper definitions
 */
#define BOT_NOTIFY_ERROR    printf
#define BOT_NOTIFY_DEBUG    printf
#define BOT_NOTIFY_WARNING  printf

/* --------------------------------------------------------
 * static definitions
 * --------------------------------------------------------*/
#define INVALID_SOCKET          (-1)
#define SOCKET_TIMEOUT          (-1)
#define RMNP_DEFAULT_ADDR       "127.0.0.1"
#define RMNP_DEFAULT_PORT       2407
#define RMNP_DEFAULT_TIMEOUT    5000        // ms
#define MAX_ATT_MTU_SIZE        512
#define NOTIFICATION_PLD_SIZE   (MAX_ATT_MTU_SIZE+4)


#define MessageType_SESSION     0   // Open/Close session
#define MessageType_POWER       1   // Power On/Off Bluetooth Adapter
#define MessageType_CONFIG      2   // Config Bluetooth Adapter and Advertisement elements
#define MessageType_ADV         3   // Start/Stop BLE Advertisement
#define MessageType_ADD_SVC     4   // Add GATT ServiceSingle Service UUID
#define MessageType_ADD_ATTR    5   // Add GATT AttributeCharacteristic or Descriptor properties
#define MessageType_UPDATE_ATTR 6   // Update GATT AttributeUpdate previously added attribute
#define NotifyType_CONNECT      7   // Notifies when R-BLE_Server was connected or disconnected by remote Bluetooth GATT Client
#define NotifyType_DATA_READ    8   // Notifies when Attribute value was read by remote Bluetooth GATT Client
#define NotifyType_DATA_WRITE   9   // Notifies when Attribute value was updated (rewritten) by remote Bluetooth GATT Client

#define GOTOERR(_err) ret = _err; goto error;

static struct Rmnp_Internal
{
    char ip_addr[16];       // server IP
    int port_server;        // server port to send to
    int port_listen;        // own port to listen on = port_server + 1
    int sock_main;          // main communication socket with server: request-response
    int sock_ntf;           // listening socket for notifications
    int timeout_ms;         // server response timeout
    int session_id;         // client-server session id
    data_access_cb cb;      // pointer to client cb function
    pthread_t lth_id;       // listening thread id
    volatile bool lth_done; // listening thread signal to exit
} gsInternal;
#define Globals (&gsInternal)

struct Message_Header_t
{
    uint16_t type;          // Message type as defined by Table 1
    uint16_t error;         // Server response error code where zero (0) value means NO ERROR
    uint16_t session_id;    // Session ID assigned by R-BLE_Server when responding to Session Open request
    uint16_t size;          // Size of the message including Header and following message specific payload
};

struct Message_Update_t
{
    Message_Header_t hdr;
    uint16_t   size;        // Unsigned 16 bit integer Size of the attribute value to update
    uint8_t    attr_idx;    // Unsigned 8 bit integer Index of the attribute in sequence in order it was added with rmnp_add_attribute()
    uint8_t    value[MAX_ATT_MTU_SIZE];  // attribute new value
};

/* --------------------------------------------------------
 * Static functions
 * --------------------------------------------------------*/
///
/// \brief cleanup_all
///
static void cleanup_all()
{
    // first - stop listening pthread if running
    if (Globals->lth_id)
    {
        Globals->lth_done = true;
        pthread_join(Globals->lth_id, NULL);
    }

    strcpy(Globals->ip_addr, RMNP_DEFAULT_ADDR);
    Globals->port_server = RMNP_DEFAULT_PORT;
    Globals->port_listen = RMNP_DEFAULT_PORT+1;

    if (Globals->sock_main != INVALID_SOCKET)
    {
        close(Globals->sock_main);
        Globals->sock_main = INVALID_SOCKET;
    }
    if (Globals->sock_ntf != INVALID_SOCKET)
    {
        close(Globals->sock_ntf);
        Globals->sock_ntf = INVALID_SOCKET;
    }

    Globals->timeout_ms = RMNP_DEFAULT_TIMEOUT;
    Globals->session_id = 0;
    Globals->cb = NULL;
    Globals->lth_id = 0;
    Globals->lth_done = false;
}

///
/// \brief send_receive
/// \param msg
/// \return error code and msg->session_id in the given message
///
static Rmnp_Error_t send_receive(Message_Header_t *msg)
{
    if ((Globals->port_listen == INVALID_SOCKET) || (Globals->port_listen == INVALID_SOCKET))
    {
        return NOT_INITIALIZED;
    }

    struct sockaddr_in server_addr;
    socklen_t addr_len = sizeof(server_addr);
    uint16_t msg_type = msg->type;

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(Globals->port_server);
    server_addr.sin_addr.s_addr = inet_addr(Globals->ip_addr);

    int brw = sendto(Globals->sock_main, (const char *)msg, msg->size, MSG_CONFIRM, (const struct sockaddr *) &server_addr, addr_len);
    if (brw != msg->size)
    {
        return SOCKET_SEND_FAILED;
    }

    // listen response from server
    if (0 >= (brw = recvfrom(Globals->sock_main, (char *)msg, sizeof(Message_Header_t), MSG_WAITALL, (struct sockaddr *) &server_addr, &addr_len)))
    {
        return (brw == -1) ? SOCKET_RECV_TIMEOUT : SOCKET_RECV_FAILED;
    }
    else if (msg_type != msg->type)
    {
        return REQUEST_RESPONSE_MISMATCH;
    }
    return (Rmnp_Error_t) msg->error; // error code returned by Server as result of the requested operation
}

///
/// \brief start_stop_advertisement
/// \param onoff
/// \return
///
static Rmnp_Error_t start_stop_advertisement(int onoff)
{
    Rmnp_Error_t ret = NO_ERROR;
    // start/stop LE advertisement message to server
    struct Message_Adv_t
    {
        Message_Header_t hdr;
        uint8_t         on_off;            // This unsigned 8 bit value indicates request to OPEN a new session when set to “1” and CLOSE when set to “0”
    } adv = {{MessageType_POWER, NO_ERROR, (uint16_t) Globals->session_id, sizeof(Message_Adv_t)}, (uint8_t) onoff};

    // send ad msg
    if (NO_ERROR != (ret = send_receive((Message_Header_t*) &adv)))
    {
        BOT_NOTIFY_ERROR("adv req failed err = %d, %s (%d)", ret, strerror(errno), errno);
    }
    return ret;
}

///
/// \brief listening_thread_func
/// \param data
/// \return
///
static void* listening_thread_func(void *data __attribute__ ((unused)))
{
    struct sockaddr_in server_addr;
    socklen_t addr_len = sizeof(server_addr);
    struct Notification_t
    {
        Message_Header_t hdr;
        uint8_t          pld[NOTIFICATION_PLD_SIZE];
    } msg;

    for (int bread = recvfrom(Globals->sock_ntf, (char *)&msg, sizeof(msg), MSG_WAITALL, (struct sockaddr *) &server_addr, &addr_len); !Globals->lth_done;)
    {
        if (bread > 0)
        {
            if (Globals->cb) switch (msg.hdr.type)
            {
            case NotifyType_CONNECT:
                (*(Globals->cb))((Rmnp_Callback_Type_t) msg.pld[0], -1, NULL, 0);
                break;

            case NotifyType_DATA_READ:
                (*(Globals->cb))(Attr_data_read, msg.pld[0], NULL, 0);
                break;

            case NotifyType_DATA_WRITE:
                if (Message_Update_t *update = (Message_Update_t*) &msg)
                {
                    (*(Globals->cb))(Attr_data_write, update->attr_idx, update->value, update->size);
                }
                break;

            default:
                BOT_NOTIFY_WARNING("Unexpected notification of type %d, - ignored", msg.hdr.type);
            }
        }
        else if (SOCKET_TIMEOUT != bread)
        {
            BOT_NOTIFY_ERROR("%s recvfrom err = %d, %s (%d) - thread Abort.", __FUNCTION__, bread, strerror(errno), errno);
            break;
        }
    }

    BOT_NOTIFY_DEBUG("listening_thread done!");
    pthread_exit(NULL);
    return (void*) 0;
}

/* --------------------------------------------------------/
 * RMNP API implementation
 * --------------------------------------------------------*/

///
/// \brief rmnp_init
/// \param tm_ms
/// \param port
/// \param ip
/// \return
///
Rmnp_Error_t rmnp_init(int tm_ms, int port, const char *ip)
{
    Rmnp_Error_t ret = NO_ERROR;
    struct timeval tv;
    // open session message to server
    struct Message_Session_t
    {
        Message_Header_t hdr;
        uint8_t         on_off;            // This unsigned 8 bit value indicates request to OPEN a new session when set to “1” and CLOSE when set to “0”
        uint8_t         force_shutdown;    // When set to “1” requests to Power Off Bluetooth Adapter
        uint8_t         force_override;    // When set to “1” requests to close any opened session regardless of the provided session ID
    } session = {{MessageType_SESSION, NO_ERROR, 0, sizeof(Message_Session_t)},1,0,0};

    cleanup_all();

    if (port > 0)
    {
        Globals->port_server = port;
        Globals->port_listen = port+1;
    }
    if (tm_ms > 0)
    {
        Globals->timeout_ms = tm_ms;
    }
    if (ip != NULL)
    {
        strcpy(Globals->ip_addr, ip);
    }

    // create UDP sockets
    if ((Globals->sock_main = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
    {
        BOT_NOTIFY_ERROR("cannot create msg socket %s (%d)", strerror(errno), errno);
        GOTOERR(SOCKET_CREATE_FAILED);
    }

    if ((Globals->sock_ntf = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        BOT_NOTIFY_ERROR("cannot create notify socket %s (%d)", strerror(errno), errno);
        GOTOERR(SOCKET_CREATE_FAILED);
    }

    struct sockaddr_in myaddr;
    memset((char *)&myaddr, 0, sizeof(myaddr));
    myaddr.sin_family = AF_INET;
    myaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    myaddr.sin_port = htons(Globals->port_listen);

    if (bind(Globals->sock_ntf, (struct sockaddr *)&myaddr, sizeof(myaddr)) < 0)
    {
        BOT_NOTIFY_ERROR("bind failed %s (%d)", strerror(errno), errno);
        GOTOERR(SOCKET_BIND_FAILED);
    }

    tv.tv_sec = Globals->timeout_ms > 0 ? ((int)Globals->timeout_ms / 1000) : 0xFFFF;
    tv.tv_usec = Globals->timeout_ms > 0 ? (Globals->timeout_ms % 1000) * 1000 : 0;

    if (setsockopt(Globals->sock_ntf, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv)) < 0)
    {
        BOT_NOTIFY_ERROR("setsockopt(SO_RCVTIMEO) %d ms error %s (%d)", Globals->timeout_ms, strerror(errno), errno);
        GOTOERR(SOCKET_SET_TIMEOUT_FAILED);
    }

    // open session
    if (NO_ERROR == (ret = send_receive((Message_Header_t*) &session)))
    {
        Globals->session_id = session.hdr.session_id;
        // power on message to server
        struct Message_Power_t
        {
            Message_Header_t hdr;
            uint8_t         on_off;            // This unsigned 8 bit value indicates request to OPEN a new session when set to “1” and CLOSE when set to “0”
        } power = {{MessageType_POWER, NO_ERROR, (uint16_t) Globals->session_id, sizeof(Message_Power_t)},1};

        // power on
        if (NO_ERROR != (ret = send_receive((Message_Header_t*) &power)))
        {
            BOT_NOTIFY_ERROR("power on request failed ret = %d, %s (%d)", ret, strerror(errno), errno);
            GOTOERR(POWER_COMMAND_FAILED);
        }
    }
    else
    {
        BOT_NOTIFY_ERROR("open session failed ret = %d, %s (%d)", ret, strerror(errno), errno);
        GOTOERR(OPEN_SESSION_FAILED);
    }
    return ret;

error:
    cleanup_all();
    return ret;
}

///
/// \brief rmnp_shutdown
/// \return
///
Rmnp_Error_t rmnp_shutdown()
{
    Rmnp_Error_t ret = NO_ERROR;
    // close session message to server
    struct Message_Session_t
    {
        Message_Header_t hdr;
        uint8_t         on_off;            // This unsigned 8 bit value indicates request to OPEN a new session when set to “1” and CLOSE when set to “0”
        uint8_t         force_shutdown;    // When set to “1” requests to Power Off Bluetooth Adapter
        uint8_t         force_override;    // When set to “1” requests to close any opened session regardless of the provided session ID
    } session = {{MessageType_SESSION, NO_ERROR, (uint16_t) Globals->session_id, sizeof(Message_Session_t)},0,1,0};

    // close session with force_shutdown to power off
    if (NO_ERROR != (ret = send_receive((Message_Header_t*) &session)))
    {
        BOT_NOTIFY_ERROR("close session failed ret = %d, %s (%d)", ret, strerror(errno), errno);
    }
    cleanup_all();
    return ret;
}

///
/// \brief rmnp_config
/// \return
///
Rmnp_Error_t rmnp_config(uint32_t dev_class, char* name, char* mac)
{
    Rmnp_Error_t ret = NO_ERROR;
    // configuration message to server
    struct Message_Configure_t
    {
        Message_Header_t hdr;
        uint32_t    device_class;   // Unsigned 32 bit integer Bluetooth Core Specification v 5.0 section 3.2.4
        char        device_name[64];// 64 byte string User friendly Bluetooth device name as specified in section 3.2.2. of Bluetooth Core Specification v 5.0
        char        mac_address[18];// 18 byte string Bluetooth Core Specification v 5.0 section 2.14.2 in textual format “XX:XX:XX:XX:XX:XX”
    } config = {{MessageType_CONFIG, NO_ERROR, (uint16_t) Globals->session_id, sizeof(Message_Configure_t)},0,"",""};

    if (dev_class > 0)
    {
        config.device_class = dev_class;
    }
    if (name != NULL)
    {
        strncpy(config.device_name, name, sizeof(config.device_name));
    }
    if (mac != NULL)
    {
        strncpy(config.mac_address, mac, sizeof(config.mac_address));
    }

    // send configuration
    if (NO_ERROR != (ret = send_receive((Message_Header_t*) &config)))
    {
        BOT_NOTIFY_ERROR("config failed ret = %d, %s (%d)", ret, strerror(errno), errno);
    }
    return ret;
}

///
/// \brief rmnp_register_callback
/// \return NO_ERROR
/// can be used to register or unregister callback when called with NULL
///
Rmnp_Error_t rmnp_register_callback(data_access_cb cb)
{
    Globals->cb =cb;
    return NO_ERROR;
}

///
/// \brief rmnp_add_service
/// \return
///
Rmnp_Error_t rmnp_add_service(uint8_t *uuid, uint32_t number_of_attr)
{
    if ((uuid == NULL) || (number_of_attr == 0))
    {
        return INVALID_PARAMETER;
    }

    Rmnp_Error_t ret = NO_ERROR;
    // request to add service
    struct Message_Sevice_t
    {
        Message_Header_t hdr;
        uint8_t    uuid[16];    // 16 bytes array containing 128-bit UUID (Universally Unique Identifier) of the GATT Service to be added
        uint32_t   count;       // Unsigned 32 bit integer Total number of attribute per service
    } service = {{MessageType_ADD_SVC, NO_ERROR, (uint16_t) Globals->session_id, sizeof(Message_Sevice_t)}, "", number_of_attr};

    memcpy(service.uuid, uuid, sizeof(service.uuid));

    // send request to add service
    if (NO_ERROR != (ret = send_receive((Message_Header_t*) &service)))
    {
        BOT_NOTIFY_ERROR("add svc failed ret = %d, %s (%d)", ret, strerror(errno), errno);
    }
    return ret;
}

///
/// \brief rmnp_add_attribute
/// \param uuid
/// \param name
/// \param max_len
/// \param size
/// \param type
/// \param prop
/// \param value
/// \return
///
Rmnp_Error_t rmnp_add_attribute(uint8_t *uuid, char* name, int max_len, int size, uint8_t type, uint8_t prop, const void* value)
{
    if ((uuid == NULL) || (max_len <=0 ) || ((size > 0) && (value == NULL)) || (size > MAX_ATT_MTU_SIZE))
    {
        return INVALID_PARAMETER;
    }

    Rmnp_Error_t ret = NO_ERROR;
    // request to add attribute
    struct Message_Attribute_t
    {
        Message_Header_t hdr;
        uint8_t    uuid[16];    // 16 bytes array containing 128-bit UUID (Universally Unique Identifier) of the Attribute to be added
        uint8_t    name[16];    // 16 byte string User-friendly attribute name for debugging
        uint16_t   max_length;  // unsigned 16 bit integer Maximum allowed size of the attribute value
        uint16_t   size;        // unsigned 16 bit integer Current size of the attribute value
        uint8_t    type;        // unsigned 8 bit integer Type of Attribute specifying “0” for Characteristic or “1” for Descriptor
        uint8_t    properties;  // unsigned 8 bit integer Bit-mask defining READ, WRITE and NOTIFY
        uint8_t    value[MAX_ATT_MTU_SIZE];  // attribute value
    } attr = {
        {MessageType_ADD_ATTR, NO_ERROR, (uint16_t) Globals->session_id, (uint16_t) (sizeof(Message_Attribute_t)-(MAX_ATT_MTU_SIZE-size))},
        "", "", (uint16_t) max_len, (uint16_t) size, type, prop, ""
    };

    memcpy(attr.uuid, uuid, sizeof(attr.uuid));
    if (name != NULL)
    {
        memcpy(attr.name, name, sizeof(attr.name));
    }
    if (size > 0)
    {
        memcpy(attr.value, value, size);
    }

    // send request to add attribute
    if (NO_ERROR != (ret = send_receive((Message_Header_t*) &attr)))
    {
        BOT_NOTIFY_ERROR("add attr failed ret = %d, %s (%d)", ret, strerror(errno), errno);
    }
    return ret;
}

///
/// \brief rmnp_add_attribute
/// \param attr
/// \return
///
Rmnp_Error_t rmnp_add_attribute(Attr_Define_t &attr)
{
    return rmnp_add_attribute(attr.uuid, attr.name, attr.max_len, attr.size, attr.type, attr.prop, attr.value);
}

///
/// \brief rmnp_update_attribute
/// \return
///
Rmnp_Error_t rmnp_update_attribute(int attr_idx, int size, const void* value)
{
    if ((attr_idx < 0) || ((size > 0) && (value == NULL)) || (size > MAX_ATT_MTU_SIZE))
    {
        return INVALID_PARAMETER;
    }

    Rmnp_Error_t ret = NO_ERROR;

    // request to add attribute
    Message_Update_t update = {
        {MessageType_UPDATE_ATTR, NO_ERROR, (uint16_t) Globals->session_id, (uint16_t) (sizeof(Message_Update_t)-(MAX_ATT_MTU_SIZE-size))},
        (uint16_t) size, (uint8_t) attr_idx, ""
    };

    if (size > 0)
    {
        memcpy(update.value, value, size);
    }

    // send request to update attribute
    if (NO_ERROR != (ret = send_receive((Message_Header_t*) &update)))
    {
        BOT_NOTIFY_ERROR("update failed ret = %d, %s (%d)", ret, strerror(errno), errno);
    }
    return ret;
}

///
/// \brief rmnp_start_advertisement
/// \return
///
Rmnp_Error_t rmnp_start_advertisement()
{
    Rmnp_Error_t ret = start_stop_advertisement(1);
    if (NO_ERROR == ret)
    {   // if listening notification thread is not running yet - start it
        if (!Globals->lth_id)
        {   // check listening socket was created
            if (Globals->sock_ntf == INVALID_SOCKET)
            {
                ret = NOT_INITIALIZED;
            }
            else
            {   // start listening notification thread
                Globals->lth_done = false;
                if (NO_ERROR != pthread_create(&Globals->lth_id, 0, listening_thread_func, NULL))
                {
                    BOT_NOTIFY_ERROR("failed to start notification pthread ret = %d, %s (%d)", ret, strerror(errno), errno);
                    ret = PTHREAD_CREATE_FAILED;
                }
            }
        }
    }
    return ret;
}

///
/// \brief rmnp_stop_advertisement
/// \return
///
Rmnp_Error_t rmnp_stop_advertisement()
{
    return start_stop_advertisement(0);
}

