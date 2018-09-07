#include "icommon.h"
#include <pthread.h>
#include <errno.h>

// static vars, status
static uint8_t giSessionId = 0;

// sample UUID definitions and macros
#define MAKE_UUID_128(_h, _g, _f, _d, _s, _a, _p, _o, _i, _u, _y, _t, _r, _e, _w, _q) \
    {0x##_h, 0x##_g, 0x##_f, 0x##_d, 0x##_s, 0x##_a, 0x##_p, 0x##_o, 0x##_i, 0x##_u, 0x##_y, 0x##_t, 0x##_r, 0x##_e, 0x##_w, 0x##_q}

#define MAKE_UUID(_attr_idx) MAKE_UUID_128(97,60,AB,BA,A2,34,46,86,9E,20,D0,87,33,3C,2C,_attr_idx)
#define CCCDESC_UUID()       MAKE_UUID_128(00,00,29,02,00,00,10,00,80,00,00,80,5F,9B,34,FB)
#define MAKE_PAYLOAD(_pld)  (unsigned int) strlen(_pld), (char *) _pld

// sample Attribute Table definition
// defines 2 characteristics and 1 descriptor
static Ble::AttributeInfo_t attr_table[] =
{
    {Ble::atCharacteristic, 1, "SAMPLE_ATTR-1", Ble::Property::RW_, GATT_SECURITY_NONE, MAKE_UUID(01), 0, 160, MAKE_PAYLOAD("value1")},
    {Ble::atCharacteristic, 3, "SAMPLE_ATTR-2", Ble::Property::RWN, GATT_SECURITY_NONE, MAKE_UUID(02), 0, 160, MAKE_PAYLOAD("value2")},
    {Ble::atDescriptor,     5, "SAMPLE_DESC-1", Ble::Property::RW_, GATT_SECURITY_NONE, CCCDESC_UUID(),0, 16,  2, (char*) "\x01\x00"}
};

// sample to demo "update value" operation
static Define_Update_t attr_upd = {
    .size = strlen("updated_val"),
    .attr_idx = 1,
    .data = (uint8_t*) "updated_val"
};

// sample service definitions
#define SERVICE_UUID   MAKE_UUID_128(97,60,AB,BA,A2,34,46,86,9E,00,FC,BB,EE,33,73,F7)
static Ble::ServiceInfo_t service = {0, SERVICE_UUID, sizeof(attr_table)/sizeof(Ble::AttributeInfo_t), attr_table};

// sample of communication sequence
typedef struct {
    Msg_Type_t type;
    void* data;
} SampleStruct_t;

SampleStruct_t msg_list[] = {
    { MSG_SESSION,          (void*) Ble::ConfigArgument::Enable},
    { MSG_POWER,            (void*) Ble::ConfigArgument::PowerOn},
    { MSG_CONFIG,           /* config*/ NULL        },
    { MSG_ADD_SERVICE,      (void*) &service        },
    { MSG_ADD_ATTRIBUTE,    (void*) &attr_table[0]  },
    { MSG_ADD_ATTRIBUTE,    (void*) &attr_table[1]  },
    { MSG_ADD_ATTRIBUTE,    (void*) &attr_table[2]  },
    { MSG_UPDATE_ATTRIBUTE, (void*) &attr_upd       },
    { MSG_ADVERTISEMENT,    (void*) Ble::ConfigArgument::Start},
    { CMD_WAIT_NOTIFICATIONS,    /* listen */ NULL  },
    { MSG_ADVERTISEMENT,    (void*) Ble::ConfigArgument::Stop},
    { MSG_POWER,            (void*) Ble::ConfigArgument::PowerOff},
    { MSG_SESSION,          (void*) Ble::ConfigArgument::Disable},
};

///
/// \brief sample format_message_payload before sending to Server
/// \param type
/// \param msg
/// \param data
/// \return
///
void* format_message_payload(Msg_Type_t type, Comm_Msg_t &msg, void* data)
{
    msg.hdr.size = sizeof(Common_Header_t);
    msg.hdr.error = Ble::Error::NONE;
    msg.hdr.session_id = giSessionId;
    msg.hdr.type = type;

    switch (type)
    {
    case MSG_SESSION:
        msg.hdr.size += sizeof(Session_t);
        msg.data.session.on_off = (uint8_t)(unsigned long)data;
        break;

    case MSG_POWER:
        msg.hdr.size += sizeof(Power_t);
        msg.data.power.on_off = (uint8_t)(unsigned long)data;
        break;

    case MSG_CONFIG:
        msg.hdr.size += sizeof(Config_t);
        msg.data.config.device_class = 0x000430;
        strcpy(msg.data.config.device_name, "IpClint-24");
        strcpy(msg.data.config.mac_address, "AA:AA:BB:BB:CC:CC");
        break;

    case MSG_ADVERTISEMENT:
        msg.hdr.size += sizeof(Advertisement_t);
        msg.data.advertisement.on_off = (uint8_t)(unsigned long)data;
        break;

    case MSG_ADD_SERVICE:
        msg.hdr.size += sizeof(Add_Service_t);
        msg.data.add_service.desc = *((Ble::ServiceInfo_t*)data);
        break;

    case MSG_ADD_ATTRIBUTE:
        if (data != NULL)
        {   // for MSG_ADD_ATTRIBUTE case data contains attribute description as Add_Attribute_t
            msg.hdr.size += sizeof(Add_Attribute_t);
            msg.data.add_attribute = *((Add_Attribute_t*)data);

            // if contains payload - re-allocate message to include payload = Value
            if (msg.data.add_attribute.attr.ValueLength > 0)
            {
                msg.hdr.size = sizeof(Comm_Msg_t) + msg.data.add_attribute.attr.ValueLength;
                uint8_t *msg_new = (uint8_t *) malloc(msg.hdr.size);
                if (msg_new)
                {
                    memcpy(msg_new, &msg, sizeof(Comm_Msg_t));
                    memcpy(msg_new + sizeof(Comm_Msg_t), msg.data.add_attribute.attr.Value, msg.data.add_attribute.attr.ValueLength);
                    return msg_new;
                }
            }
        }
        break;

    case MSG_UPDATE_ATTRIBUTE:
        if (data != NULL)
        {   // for MSG_UPDATE_ATTRIBUTE case data contains Define_Update_t; the updated Value of the attribute may be NULL
            msg.hdr.size += sizeof(Update_Attribute_t);
            Define_Update_t* def = (Define_Update_t*)data;

            msg.data.update_attribute.attr_idx = def->attr_idx;
            msg.data.update_attribute.size = def->size;

            // if updated Value is not NULL - re-allocate message to include payload = Value
            if (msg.data.update_attribute.size > 0)
            {
                int new_size = msg.hdr.size + msg.data.update_attribute.size - 1;
                uint8_t *msg_new = (uint8_t *) malloc(new_size);
                if (msg_new)
                {
                    memcpy(msg_new, &msg, msg.hdr.size);
                    memcpy(((Comm_Msg_t*)msg_new)->data.update_attribute.data, def->data, msg.data.update_attribute.size);
                    msg.hdr.size = new_size;
                    return msg_new;
                }
            }
        }
        break;

    case MSG_NOTIFY_CONNECT_STATUS:
    case MSG_NOTIFY_DATA_READ:
    case MSG_NOTIFY_DATA_WRITE:
    default:
        printf("WARNING! Wrong handler - Notification and commands are not processed here\n");
        break;
    }
    return NULL;
}

///
/// \brief sample handle_response_message or notification from Server
/// \param msg
/// \return
///
int handle_response_message(Comm_Msg_t &msg)
{
    int ret = Ble::Error::NONE;

    if (msg.hdr.error != Ble::Error::NONE)
    {
        printf("ERROR in response: %d\n", msg.hdr.error);
        ret = msg.hdr.error;
    }
    else switch (msg.hdr.type)
    {
    // server responses
    case MSG_SESSION:
        giSessionId = msg.hdr.session_id;
        printf("giSessionId = %d %s\n", giSessionId, msg.data.session.on_off?"opened":"closed");
        break;

    case MSG_POWER:
    case MSG_CONFIG:
    case MSG_ADVERTISEMENT:
    case MSG_ADD_SERVICE:
    case MSG_ADD_ATTRIBUTE:
    case MSG_UPDATE_ATTRIBUTE:
        break;

    // server notifications
    case MSG_NOTIFY_CONNECT_STATUS:
        if (msg.data.notify_connect.on_off)
        {
            printf("MSG_NOTIFY_CONNECT_STATUS // connect\n");
        }
        else
        {
            printf("MSG_NOTIFY_CONNECT_STATUS // disconnect\n");
        }
        break;

    case MSG_NOTIFY_DATA_READ:
        printf("MSG_NOTIFY_DATA_READ // attr->idx = %d\n", msg.data.notify_data_read.attr_idx);
        break;

    case MSG_NOTIFY_DATA_WRITE:
        printf("MSG_NOTIFY_DATA_WRITE // attr->idx = %d val = %s\n", msg.data.notify_data_write.attr_idx, msg.data.notify_data_write.data);
        break;
    }
    return ret;
}

static volatile char server_notify_listener_done = 0;
static void* server_notify_listener(void *data __attribute__ ((unused)))
{
    while (server_notify_listener_done == 0)
    {
        Comm_Msg_t msg;
        int ret = recv_comm(FROM_SERVER, (char*) &msg, sizeof(msg), 333);

        if (ret == Ble::Error::TIMEOUT)
        {
            if (server_notify_listener_done)
                break;
            else
                continue;
        }
        else if (ret != Ble::Error::NONE)
        {
            printf("failed recv notification from server in server_notify_listener\nerr = %d %s, stop listening.\n", ret, get_err_name(ret));
            break;
        }
        printf ("got notify: msg type %d, %s error =  %d\n", msg.hdr.type, get_msg_name(&msg), msg.hdr.error);
        handle_response_message(msg);
    }

// done:
    printf("server_notify_listener exited\n");
    fflush(stdout);
    pthread_exit(NULL);
    return (void*) 0;
}

///
/// \brief preprocess_client_commands
/// \param cmd
/// \return
///
int preprocess_client_commands(SampleStruct_t *cmd)
{
    switch (cmd->type)
    {
    case CMD_PAUSE_GETCHAR:
        printf("\nCMD_PAUSE_GETCHAR\\>");
        getchar();
        break;

    case CMD_SLEEP_SECONDS:
        printf("\nCMD_SLEEP_SECONDS %d sec\n", (int)(unsigned long)cmd->data);
        sleep((int)(unsigned long)cmd->data);
        break;

    case CMD_WAIT_NOTIFICATIONS:
    {
        pthread_t thread_id;
        server_notify_listener_done = 0;

        if (Ble::Error::NONE != pthread_create(&thread_id, NULL, server_notify_listener, NULL))
        {
            printf("failed to create pthread %s (%d)\n", strerror(errno), errno);
            return Ble::Error::PTHREAD_ERROR;
        }

        printf("\nCMD_WAIT_NOTIFICATIONS listening from server...\npress ENTER to stop listening and continue\\>\n\n");
        getchar();

        // set done and wait for exit
        server_notify_listener_done = 1;
        pthread_join(thread_id, NULL);
        break;
    }
    default:
        return Ble::Error::IGNORED;
    }
    return Ble::Error::NONE;
}

///
/// \brief sample client main
/// \param argc
/// \param argv
/// \return
///
int main(int argc, char** argv )
{
    Comm_Msg_t msg;
    int ret;

    if (Ble::Error::NONE != (ret = parse_command_line(argc, argv)))
    {
        die("invalid parameters", ret);
    }

    if (Ble::Error::NONE != (ret = init_comm(CLIENT)))
    {
        die("failed to init communication", ret);
    }

    for (uint32_t i = 0; i < sizeof(msg_list)/sizeof(SampleStruct_t); i++)
    {
        if (Ble::Error::IGNORED == preprocess_client_commands(&msg_list[i]))
        {
            Comm_Msg_t *msg_to_send = (Comm_Msg_t *) format_message_payload(msg_list[i].type, msg, msg_list[i].data);

            ret = send_comm(TO_SERVER, ((NULL != msg_to_send) ? msg_to_send : &msg), msg.hdr.size);

            // if allocated by format_message_payload - release it
            if (msg_to_send)
            {
                free(msg_to_send);
                msg_to_send = NULL;
            }

            if (Ble::Error::NONE != ret)
            {
                shut_comm(CLIENT);
                die("msg send to server failed", ret);
            }
            else
            {
                printf ("Message Sent to server, type %d %s, size = %d\n", msg.hdr.type, get_msg_name(&msg), msg.hdr.size);
                if (Ble::Error::NONE != (ret = recv_comm(FROM_SERVER, (char*) &msg, sizeof(msg), 5000)))
                {
                    shut_comm(CLIENT);
                    die("failed recv from server", ret);
                }
                printf ("got rsp: msg type %d, %s error =  %d\n", msg.hdr.type, get_msg_name(&msg), msg.hdr.error);
                handle_response_message(msg);
            }
        }
    }

    shut_comm(CLIENT);
    return 0;
}
