#include "icommon.h"
#include <time.h>

// static vars, status
static uint8_t gsSessionId = 0;

int handle_request_msg(Comm_Msg_t *msg)
{
    int ret = NO_ERROR;

    printf("got msg %d %s\n", msg->type, get_msg_name(msg));

    if (!msg)
    {
        return INVALID_PARAMETER;
    }

    switch (msg->type)
    {
    case MSG_SESSION:
    {
        Session_t *session = (Session_t *) &msg->data;
        if (session->on_off)
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
        }
        else
        {
            if ((gsSessionId == msg->session_id) || (session->force_override))
            {
                if (session->force_shutdown)
                {
                    Ble::GattSrv::getInstance()->Shutdown();
                }
                gsSessionId = 0;
            }
            else
            {
                ret = SERVER_BUSY;
            }
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
            {Ble::Config::LocalDeviceName,        {strlen(data->device_name)?1:0,   (char*) data->device_name, Ble::ConfigArgument::None}},
            {Ble::Config::MACAddress,             {strlen(data->mac_address)?1:0,   (char*) data->mac_address, Ble::ConfigArgument::None}},
            {Ble::Config::LocalClassOfDevice,     {data->device_class?1:0,          NULL,                      data->device_class}},
            {Ble::Config::EOL,                    {0,                               NULL,                      Ble::ConfigArgument::None}},
        };
        ret = Ble::GattSrv::getInstance()->Configure(config);
        break;
    }

    case MSG_ADVERTISEMENT:
    {
        Advertisement_t *data = (Advertisement_t*) &msg->data;
        if (data->on_off)
        {
            // start adv
        }
        else
        {
            // stop adv
        }
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



int main(int argc, char** argv )
{
    Comm_Msg_t msg;

    if (Ble::Error::NONE != parse_command_line(argc, argv))
    {
        die("invalid parameters");
    }

    if (Ble::Error::NONE != init_comm(SERVER))
    {
        die("failed to init communication");
    }

    while (1)
    {
        //Receive message
        if (Ble::Error::NONE != recv_comm(FROM_CLIENT, &msg))
        {
            die("recv failed");
        }

        // handle request, set response
        msg.error = (Error_Type_t) handle_request_msg(&msg);

        if (Ble::Error::NONE != send_comm(TO_CLIENT, &msg, sizeof(msg)))
        {
            die("send failed");
        }

        if ((msg.type == MSG_SESSION) && (!msg.data.session.on_off))
        {
            break;
        }
    }

    shut_comm(SERVER);
    return 0;
}

