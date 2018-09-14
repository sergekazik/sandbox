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

// sample Adapter configuration
static Config_t config_data = {0x000430, "IpClint-24", "AA:AA:BB:BB:CC:CC"};

// sample Attribute Table definition
// defines 2 characteristics and 1 descriptor
static Define_Attribute_t attr_table[] =
{   // uuid         name            max size    type                properties          value
    {MAKE_UUID(01), "SAMPLE_ATTR-1", 64, 6, GATT_TYPE_CHARACTERISTIC, Ble::Property::RW_, "value1"},
    {MAKE_UUID(02), "SAMPLE_ATTR-2", 64, 6, GATT_TYPE_CHARACTERISTIC, Ble::Property::RWN, "value2"},
    {CCCDESC_UUID(),"SAMPLE_DESC-1", 32, 2, GATT_TYPE_DESCRIPTOR,     Ble::Property::RW_, "\x01\x00"}
};

// sample to demonstrate "update value" operation
static Define_Update_t attr_upd = {strlen("updated_val"), 1, (uint8_t*) "updated_val"};

// sample service definitions
#define SERVICE_UUID   MAKE_UUID_128(97,60,AB,BA,A2,34,46,86,9E,00,FC,BB,EE,33,73,F7)
static Add_Service_t service = {SERVICE_UUID, 3};

// sample of communication sequence
typedef struct {
    Msg_Type_t type;
    void* data;
} SampleStruct_t;

SampleStruct_t msg_list[] = {
    { MSG_SESSION,          (void*) Ble::ConfigArgument::Enable},
    { MSG_POWER,            (void*) Ble::ConfigArgument::PowerOn},
    { MSG_CONFIG,           (void*) &config_data    },
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
/// \brief handle_response_message or notification from Server
/// \param msg
/// \return
///
int handle_response_message(Comm_Msg_t &msg)
{
    int ret = Ble::Error::NONE;

    if (msg.hdr.error != Ble::Error::NONE)
    {
        DEBUG_PRINTF(("ERROR in response: %d\n", msg.hdr.error));
        ret = msg.hdr.error;
    }
    else switch (msg.hdr.type)
    {
    // server responses
    case MSG_SESSION:
        giSessionId = msg.hdr.session_id;
        DEBUG_PRINTF(("giSessionId = %d %s\n", giSessionId, msg.data.session.on_off?"opened":"closed"));
        break;

    // server notifications
    case MSG_NOTIFY_CONNECT_STATUS:
        if (msg.data.notify_connect.on_off)
        {
            DEBUG_PRINTF(("MSG_NOTIFY_CONNECT_STATUS // connect\n"));
        }
        else
        {
            DEBUG_PRINTF(("MSG_NOTIFY_CONNECT_STATUS // disconnect\n"));
        }
        break;

    case MSG_NOTIFY_DATA_READ:
        DEBUG_PRINTF(("MSG_NOTIFY_DATA_READ // attr->idx = %d\n", msg.data.notify_data_read.attr_idx));
        break;

    case MSG_NOTIFY_DATA_WRITE:
        DEBUG_PRINTF(("MSG_NOTIFY_DATA_WRITE // attr->idx = %d val = %s\n", msg.data.notify_data_write.attr_idx, msg.data.notify_data_write.data));
        break;

    case MSG_POWER:
    case MSG_CONFIG:
    case MSG_ADVERTISEMENT:
    case MSG_ADD_SERVICE:
    case MSG_ADD_ATTRIBUTE:
    case MSG_UPDATE_ATTRIBUTE:
    default:
        DEBUG_PRINTF(("unexpected response to msg type %d [%s]\n", msg.hdr.type, get_msg_name(&msg)));
        ret = Ble::Error::INVALID_PARAMETER;
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
            DEBUG_PRINTF(("failed recv notification from server in server_notify_listener\nerr = %d %s, stop listening.\n", ret, get_err_name(ret)));
            break;
        }
        printf ("got notify: msg type %d, %s error =  %d\n", msg.hdr.type, get_msg_name(&msg), msg.hdr.error);
        handle_response_message(msg);
    }

// done:
    DEBUG_PRINTF(("server_notify_listener exited\n"));
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
        DEBUG_PRINTF(("\nCMD_PAUSE_GETCHAR\\>"));
        getchar();
        break;

    case CMD_SLEEP_SECONDS:
        DEBUG_PRINTF(("\nCMD_SLEEP_SECONDS %d sec\n", (int)(unsigned long)cmd->data));
        sleep((int)(unsigned long)cmd->data);
        break;

    case CMD_WAIT_NOTIFICATIONS:
    {
        pthread_t thread_id;
        server_notify_listener_done = 0;

        if (Ble::Error::NONE != pthread_create(&thread_id, NULL, server_notify_listener, NULL))
        {
            DEBUG_PRINTF(("failed to create pthread %s (%d)\n", strerror(errno), errno));
            return Ble::Error::PTHREAD_ERROR;
        }

        DEBUG_PRINTF(("\nCMD_WAIT_NOTIFICATIONS listening from server...\npress ENTER to stop listening and continue\\>\n\n"));
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
            Comm_Msg_t *msg_to_send = (Comm_Msg_t *) format_message_payload(giSessionId, msg_list[i].type, msg, msg_list[i].data);

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
