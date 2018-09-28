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

// sample Adapter configuration
static Config_t config_data = {0x000430, "IpClint-24", "AA:AA:BB:BB:CC:CC"};

// sample Attribute Table definition
// defines 2 characteristics and 1 descriptor
static Define_Attribute_t attr_table[] =
{   // uuid         name            max size    type                properties          value
    {{0x97,0x60,0xAB,0xBA,0xA2,0x34,0x46,0x86,0x9E,0x20,0xD0,0x87,0x33,0x3C,0x2C,0x01}, "SAMPLE_ATTR-1", 64, 6, GATT_TYPE_CHARACTERISTIC, Ble::Property::RW, "value1"},
    {{0x97,0x60,0xAB,0xBA,0xA2,0x34,0x46,0x86,0x9E,0x20,0xD0,0x87,0x33,0x3C,0x2C,0x02}, "SAMPLE_ATTR-2", 64, 6, GATT_TYPE_CHARACTERISTIC, Ble::Property::RWN, "value2"},
    {{0x00,0x00,0x29,0x02,0x00,0x00,0x10,0x00,0x80,0x00,0x00,0x80,0x5F,0x9B,0x34,0xFB}, "SAMPLE_DESC-1", 32, 2, GATT_TYPE_DESCRIPTOR,     Ble::Property::RW, "\x01\x00"}
};

// sample to demonstrate "update value" operation
static Define_Update_t attr_upd = {strlen("updated_val"), 1, (uint8_t*) "updated_val"};

// sample service definitions
static Add_Service_t service = {{0x97,0x60,0xAB,0xBA,0xA2,0x34,0x46,0x86,0x9E,0x00,0xFC,0xBB,0xEE,0x33,0x73,0xF7}, 3};

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
    // for debugging - not used in real communication
    { CMD_SERVER_EXIT,      (void*) NULL},

};

///
/// \brief handle_response_message or notification from Server
/// \param msg
/// \param bNotify
/// \return
///
static int handle_response_message(Comm_Msg_t &msg, bool bNotify = false)
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
        if (bNotify)
        {
            DEBUG_PRINTF(("unexpected notify msg type %d [%s]\n", msg.hdr.type, get_msg_name(&msg)));
            ret = Ble::Error::INVALID_PARAMETER;
        }
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
        int ret = wait_notification(&msg, sizeof(msg), 333 /*timeout ms*/);

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
        handle_response_message(msg, true);
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

            ret = send_to_server(((NULL != msg_to_send) ? msg_to_send : &msg), msg.hdr.size);

            // if allocated by format_message_payload - release it
            if (msg_to_send)
            {
                free(msg_to_send);
                msg_to_send = NULL;
            }

            if (Ble::Error::NONE != ret)
            {
                shut_comm();
                die("msg send to server failed", ret);
            }
            else if (msg.hdr.type == CMD_SERVER_EXIT)
            {
                printf ("---- end of test ----\n");
                break;
            }
            else
            {
                printf ("Message Sent to server, type %d %s, size = %d\n", msg.hdr.type, get_msg_name(&msg), msg.hdr.size);
                if (Ble::Error::NONE != (ret = resp_from_server(&msg, sizeof(msg), 5000 /*timeout ms*/)))
                {
                    shut_comm();
                    die("failed recv from server", ret);
                }
                printf ("got rsp: msg type %d, %s error =  %d\n", msg.hdr.type, get_msg_name(&msg), msg.hdr.error);
                handle_response_message(msg);
            }
        }
    }
    shut_comm();
    return 0;
}

