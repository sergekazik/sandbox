#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include "icommon.h"
#include "gatt_api.h"

static uint8_t gsSessionId = 0;

#ifdef DEBUG_ENABLED
static const char *debug_msg[] =
{
    "MSG_OPEN_SESSION",
    "MSG_CLOSE_SESSION",
    "MSG_POWER_ON",
    "MSG_CONFIG",
    "MSG_POWER_OFF",
    "MSG_START_ADVERTISEMENT",
    "MSG_STOP_ADVERTISEMENT",
    "MSG_ADD_SERVICE",
    "MSG_ADD_ATTRIBUTE",
    "MSG_UPDATE_ATTRIBUTE",
    "MSG_NOTIFY_STATUS_CHANGE",
    "MSG_NOTIFY_DATA_READ",
    "MSG_NOTIFY_DATA_WRITE",
};
#endif

void print_out_msg(Comm_Msg_t *cm)
{
#ifdef DEBUG_ENABLED
    printf("ipcluez got: %d, %s\n", cm->type, debug_msg[cm->type]);
#endif
}

void die(const char *s)
{
    perror(s);
    exit(1);
}

int handle_request_msg(Comm_Msg_t *msg)
{
    int ret = NO_ERROR;

    print_out_msg(msg);

    if (!msg)
    {
        return INVALID_PARAMETER;
    }

    switch (msg->type)
    {
    case MSG_OPEN_SESSION:
    {
        // Open_Session_t *data = (Open_Session_t *) &msg->data;
        if (gsSessionId > 0)
        {
            ret = SERVER_BUSY;
        }
        else
        {
            msg->session_id = gsSessionId = (time(NULL)%0x7F)+1;
        }
        break;
    }

    case MSG_CLOSE_SESSION:
    {
        Close_Session_t *data = (Close_Session_t *) &msg->data;
        if ((gsSessionId == msg->session_id) || (data->force_override))
        {
            if (data->force_shutdown)
            {
                Ble::GattSrv::getInstance()->Shutdown();
            }
            gsSessionId = 0;
        }
        else
        {
            ret = SERVER_BUSY;
        }
        break;
    }

    case MSG_POWER:
    {
        Power_t *data = (Power_t *) &msg->data;
        if (data->on_off)
        {
            ret = Ble::GattSrv::getInstance()->Initialize();
        }
        else
        {
            ret = Ble::GattSrv::getInstance()->Shutdown();
        }
        break;
    }

    case MSG_CONFIG:
    {
        Config_t *data = (Config_t *) &msg->data;
        Ble::DeviceConfig_t config[] =
        { // config tag                         count   params
            {Ble::Config::LocalDeviceName,        {strlen(data->device_name)?1:0,   {{(char*) data->device_name, Ble::ConfigArgument::None}}}},
            {Ble::Config::MACAddress,             {strlen(data->mac_address)?1:0,   {{(char*) data->mac_address, Ble::ConfigArgument::None}}}},
            {Ble::Config::LocalClassOfDevice,     {data->device_class?1:0,          {{NULL, data->device_class}}}},
            {Ble::Config::EOL,                    {0,   {{NULL, Ble::ConfigArgument::None}}}},
        };
        ret = Ble::GattSrv::getInstance()->Configure(config);
        break;
    }

    case MSG_START_ADVERTISEMENT:
    {
        Start_Advertisement_t *data = (Start_Advertisement_t*) &msg->data;
        break;
    }

    case MSG_STOP_ADVERTISEMENT:
    {
        Stop_Advertisement_t *data = (Stop_Advertisement_t *) &msg->data;
        break;
    }

    case MSG_ADD_SERVICE:
    {
        Add_Service_t *data = (Add_Service_t *) &msg->data;
        break;
    }

    case MSG_ADD_ATTRIBUTE:
    {
        Add_Attribute_t *data = (Add_Attribute_t *) &msg->data;
        break;
    }

    case MSG_UPDATE_ATTRIBUTE:
    {
        Update_Attribute_t *data = (Update_Attribute_t *) &msg->data;
        break;
    }
    default:
        ret = INVALID_PARAMETER;
        break;
    }
    return ret;
}

int main()
{
    int msqid, rsp_msg_id;
    key_t key;
    Comm_Msgbuf_t rcvbuffer;
    int msgflg = IPC_CREAT | 0666;

    key = SHARED_KEY;
    if ((msqid = msgget(key, msgflg )) < 0)
    {
      die("msgget()");
    }

    key = NOTIFY_KEY;
    if ((rsp_msg_id = msgget(key, msgflg )) < 0)
    {
      die("msgget()");
    }

    while (1)
    {
         //Receive an answer of message type 1.
        if (msgrcv(msqid, &rcvbuffer, sizeof(Comm_Msgbuf_t), 1, 0) < 0)
        {
          die("msgrcv");
        }

        // handle request provide response
        rcvbuffer.msg.error = (Error_Type_t) handle_request_msg(&rcvbuffer.msg);
        int buflen = sizeof(Comm_Msg_t) + 1;

        if (msgsnd(rsp_msg_id, &rcvbuffer, buflen, IPC_NOWAIT) < 0)
        {
            die("msgrcv");
        }

        if (rcvbuffer.msg.type == MSG_CLOSE_SESSION)
        {
            break;
        }
    }
    return 0;
}

