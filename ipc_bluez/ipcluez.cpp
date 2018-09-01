#include "icommon.h"
#include <time.h>

// static vars, status
static uint8_t gsSessionId = 0;
static Ble::ServiceInfo_t gClientService = {0,{0},0, NULL};

void cleanup_client_service()
{
    Ble::GattSrv::getInstance()->CleanupServiceList();
    if (gClientService.AttributeList)
    {
        free(gClientService.AttributeList);
        memset(&gClientService, 0, sizeof(gClientService));
    }
}

int handle_request_msg(Comm_Msg_t *msg)
{
    int ret = NO_ERROR;

    printf("got msg %d %s\n", msg->hdr.type, get_msg_name(msg));

    if (!msg)
    {
        return INVALID_PARAMETER;
    }
    else if (gsSessionId && (msg->hdr.session_id != gsSessionId))
    {
        return SERVER_BUSY; // wrong session or client
    }

    switch (msg->hdr.type)
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
                msg->hdr.session_id = gsSessionId = (time(NULL)%0x7F)+1;
            }
        }
        else
        {
            if ((gsSessionId == msg->hdr.session_id) || (session->force_override))
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
        { // config tag                             count                           params
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
            ret = Ble::GattSrv::getInstance()->StartAdvertising(NULL);
        }
        else
        {
            ret = Ble::GattSrv::getInstance()->StopAdvertising(NULL);
        }
        break;
    }

    case MSG_ADD_SERVICE:
    {
        Add_Service_t *data = (Add_Service_t *) &msg->data;
        if (data->desc.NumberAttributes > 0)
        {
            int alloc_size = data->desc.NumberAttributes * sizeof(Ble::AttributeInfo_t);

            // only single service is supported at this time
            cleanup_client_service();

            // copy service description from msg.data
            gClientService = data->desc;

            // allocate clean attribute table
            gClientService.AttributeList = (Ble::AttributeInfo_t*) malloc(alloc_size);
            if (!gClientService.AttributeList)
            {
                ret = MEMORY_ERROR;
                break;
            }
            memset(gClientService.AttributeList, 0, alloc_size);

            // congig gatt server with a new service table
            Ble::DeviceConfig_t config[] =
            { // config tag                             count                           params
                {Ble::Config::ServiceTable,           {1,   (char*) &gClientService, Ble::ConfigArgument::None}},
                {Ble::Config::EOL,                    {0,   NULL,                    Ble::ConfigArgument::None}},
            };
            ret = Ble::GattSrv::getInstance()->Configure(config);
        }
        else
        {
            ret = INVALID_PARAMETER;
        }
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
    int ret;

    if (Ble::Error::NONE != (ret = parse_command_line(argc, argv)))
    {
        die("invalid parameters", ret);
    }

    if (Ble::Error::NONE != (ret = init_comm(SERVER)))
    {
        die("failed to init communication", ret);
    }

    while (1)
    {
        //Receive message
        if (Ble::Error::NONE != (ret = recv_comm(FROM_CLIENT, &msg)))
        {
            die("recv failed", ret);
        }

        // handle request, set response
        msg.hdr.error = (Error_Type_t) handle_request_msg(&msg);

        if (Ble::Error::NONE != (ret = send_comm(TO_CLIENT, &msg, sizeof(Common_Header_t))))
        {
            die("send failed", ret);
        }

        if ((msg.hdr.type == MSG_SESSION) && (!msg.data.session.on_off))
        {
            break;
        }
    }

    shut_comm(SERVER);
    return 0;
}

