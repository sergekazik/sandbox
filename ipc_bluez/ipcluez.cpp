#include "icommon.h"
#include <time.h>

// static vars, status
static uint8_t gsSessionId = 0;
static Ble::ServiceInfo_t gClientService = {0,{0},0, NULL};

void cleanup_attribute_value(Ble::AttributeInfo_t *attr)
{
    if (attr != NULL)
    {
        if (attr->Value && attr->AllocatedValue)
        {
            free(attr->Value);
        }
        attr->Value = NULL;
        attr->ValueLength = 0;
        attr->AllocatedValue = 0;
    }
}

void cleanup_client_service()
{
    Ble::GattSrv::getInstance()->CleanupServiceList();
    if (gClientService.AttributeList)
    {
        free(gClientService.AttributeList);
        memset(&gClientService, 0, sizeof(gClientService));
    }
}

int allocate_zero_attr_table(int alloc_size)
{
    int ret = Ble::Error::NONE;
    // allocate clean attribute table
    gClientService.AttributeList = (Ble::AttributeInfo_t*) malloc(alloc_size);
    if (!gClientService.AttributeList)
    {
        ret = Ble::Error::MEMORY_ALLOOCATION;
    }
    else
    {
        memset(gClientService.AttributeList, 0, alloc_size);
    }
    return ret;
}

int append_attribute(Comm_Msg_t *msg)
{
    if (!gClientService.AttributeList)
    {
        return Ble::Error::MEMORY_ALLOOCATION;
    }
    else for (int i = 0; i < (int) gClientService.NumberAttributes; i++)
    {
        if (0 == gClientService.AttributeList[i].MaximumValueLength)
        {
            Ble::AttributeInfo_t *attr = &gClientService.AttributeList[i];
            cleanup_attribute_value(attr);
            memcpy(attr, &msg->data.add_attribute.attr, sizeof(Ble::AttributeInfo_t));

            // handle payload
            if (msg->data.add_attribute.attr.ValueLength && (msg->data.add_attribute.attr.ValueLength == (msg->hdr.size - sizeof(Comm_Msg_t))))
            {
                attr->ValueLength = msg->data.add_attribute.attr.ValueLength;
                if (NULL == (attr->Value = (char*) malloc(attr->ValueLength)))
                {
                    attr->ValueLength = 0;
                    return Ble::Error::MEMORY_ALLOOCATION;
                }
                memcpy(attr->Value, (char*) msg + sizeof(Comm_Msg_t), attr->ValueLength);
                attr->AllocatedValue = 1;
            }
#ifdef DEBUG_ENABLED
            {
                char print_string_tmp[attr->ValueLength+1];
                memcpy(print_string_tmp, attr->Value, attr->ValueLength);
                print_string_tmp[attr->ValueLength] = '\0';
                DEBUG_PRINTF("\tadded attr %d [%s] val [%d] %s", i, attr->AttributeName, attr->ValueLength, print_string_tmp);
            }
#endif
            return Ble::Error::NONE;
        }
    }
    return Ble::Error::INVALID_PARAMETER;
}

int update_attribute(Update_Attribute_t *data)
{
    int ret = Ble::Error::NONE;
    Ble::AttributeInfo_t *attr = NULL;
    Ble::GattSrv* gatt = Ble::GattSrv::getInstance();

    if (!gClientService.AttributeList)
    {
        return Ble::Error::MEMORY_ALLOOCATION;
    }
    else if ((data->attr_idx > 0) && (data->attr_idx < (int) gClientService.NumberAttributes))
    {
        attr = &gClientService.AttributeList[data->attr_idx];
    }
    else
    {
        return Ble::Error::INVALID_PARAMETER;
    }
#ifdef DIRECT_UPDATE
    cleanup_attribute_value(attr);
    if (data->size > 0)
    {
        if (NULL == (attr->Value = (char*) malloc(attr->ValueLength)))
            return Ble::Error::MEMORY_ALLOOCATION;
        attr->ValueLength = data->size;
        attr->AllocatedValue = 1;
        memcpy(attr->Value, data->data, attr->ValueLength);
    }
#else
    ret = gatt->GATTUpdateCharacteristic(data->attr_idx, (const char*) data->data, data->size);
#endif

    if (ret == Ble::Error::NONE)
    {
#ifdef DEBUG_ENABLED
        char print_string_tmp[attr->ValueLength+1];
        memcpy(print_string_tmp, attr->Value, attr->ValueLength);
        print_string_tmp[attr->ValueLength] = '\0';
        DEBUG_PRINTF("\tupdated attr %d [%s] val [%d] %s", data->attr_idx, attr->AttributeName, attr->ValueLength, print_string_tmp);
#endif
        // Notify if connected
        if (attr->CharacteristicPropertiesMask & GATT_PROPERTY_NOTIFY)
        {
            ret = gatt->NotifyCharacteristic(data->attr_idx, (const char*) data->data, data->size);
            DEBUG_PRINTF("Notify characteristic err = %d", ret);
            ret = Ble::Error::NONE;
        }
    }
    return ret;
}

static void attr_access_callback(int aAttribueIdx, Ble::Property::Access aAccessType)
{
    Comm_Msg_t _msg, *msg = &_msg;
    msg->hdr.error = Ble::Error::NONE;
    msg->hdr.session_id = gsSessionId;
    msg->hdr.size = sizeof(Common_Header_t);

    switch (aAccessType)
    {
    case Ble::Property::Access::Connected:
    case Ble::Property::Access::Disconnected:
        msg->hdr.type = MSG_NOTIFY_CONNECT_STATUS;
        msg->hdr.size += sizeof(Notify_Connect_Status_t);
        msg->data.notify_connect.on_off = (aAccessType == Ble::Property::Access::Connected)?1:0;
        break;

    case Ble::Property::Access::Read:
        msg->hdr.type = MSG_NOTIFY_DATA_READ;
        msg->hdr.size += sizeof(Notify_Data_Read_t);
        msg->data.notify_data_read.attr_idx = aAttribueIdx;
        break;

    case Ble::Property::Access::Write:
        msg->hdr.type = MSG_NOTIFY_DATA_WRITE;
        msg->hdr.size += sizeof(Notify_Data_Write_t);
        msg->data.notify_data_write.attr_idx = aAttribueIdx;
        msg->data.notify_data_write.size = gClientService.AttributeList[aAttribueIdx].ValueLength;

        if (gClientService.AttributeList[aAttribueIdx].ValueLength > 1)
        {
            // re-alloc message to include Value
            int new_size = sizeof(Common_Header_t) + sizeof(Notify_Data_Write_t) + gClientService.AttributeList[aAttribueIdx].ValueLength -1;
            msg = (Comm_Msg_t*) malloc(new_size);
            if (msg)
            {
                *msg = _msg; // restore header and data values
                msg->hdr.size = new_size;
                memcpy(msg->data.notify_data_write.data, gClientService.AttributeList[aAttribueIdx].Value, gClientService.AttributeList[aAttribueIdx].ValueLength);
            }
            else
            {   // Notify Client about Error
                DEBUG_PRINTF("ERROR: Notify Client Ble::Property::Access::Write failed to allocate memory");
                msg = &_msg; // restore pointer
                msg->hdr.error = Ble::Error::MEMORY_ALLOOCATION;
            }
        }
        else
        {
            msg->data.notify_data_write.data[0] = (gClientService.AttributeList[aAttribueIdx].Value != NULL)?*gClientService.AttributeList[aAttribueIdx].Value:0;
        }
        break;

    default:
        DEBUG_PRINTF("Unexpected aAccessType = %d", aAccessType);
        msg->hdr.error = Ble::Error::INVALID_COMMAND;
        break;
    }

    // send to Client
    if (msg->hdr.error == Ble::Error::NONE)
    {
        int ret = send_comm(TO_CLIENT, msg, msg->hdr.size);
        DEBUG_PRINTF("Notify Client aAccessType = %d, err = %d", aAccessType, ret);
    }

    // if msg was allocated - release it
    if (msg != &_msg)
    {
        free(msg);
    }
}

int handle_request_msg(Comm_Msg_t *msg)
{
    int ret = Ble::Error::NONE;
    Ble::GattSrv* gatt = Ble::GattSrv::getInstance();

    DEBUG_PRINTF("got msg %d %s", msg->hdr.type, get_msg_name(msg));

    if (!msg)
    {
        return Ble::Error::INVALID_PARAMETER;
    }
    else if (gsSessionId && (msg->hdr.session_id != gsSessionId))
    {
        return Ble::Error::RESOURCE_UNAVAILABLE; // wrong session or client
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
                ret = Ble::Error::RESOURCE_UNAVAILABLE;
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
                    gatt->Shutdown();
                }
                gsSessionId = 0;
            }
            else
            {
                ret = Ble::Error::RESOURCE_UNAVAILABLE;
            }
        }
        break;
    }

    case MSG_POWER:
    {
        Power_t *data = (Power_t *) &msg->data;
        if (data->on_off)
        {
            ret = gatt->Initialize();
        }
        else
        {
            // unregister attribute access callback
            gatt->UnregisterCharacteristicAccessCallback(attr_access_callback);
            ret = gatt->Shutdown();
        }
        break;
    }

    case MSG_CONFIG:
    {
        Config_t *data = (Config_t *) &msg->data;

        Ble::Config::DeviceConfig_t config[] =
        { // config tag                             count                           params
            {Ble::Config::LocalDeviceName,        {strlen(data->device_name)?1:0,   (char*) data->device_name, Ble::ConfigArgument::None}},
            {Ble::Config::MACAddress,             {strlen(data->mac_address)?1:0,   (char*) data->mac_address, Ble::ConfigArgument::None}},
            {Ble::Config::LocalClassOfDevice,     {data->device_class?1:0,          NULL,                      data->device_class}},
            {Ble::Config::EOL,                    {0,                               NULL,                      Ble::ConfigArgument::None}},
        };

        ret = gatt->Configure(config);
        break;
    }

    case MSG_ADVERTISEMENT:
    {
        Advertisement_t *data = (Advertisement_t*) &msg->data;
        if (data->on_off)
        {
            ret = gatt->StartAdvertising(NULL);
        }
        else
        {
            ret = gatt->StopAdvertising(NULL);
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

            if (Ble::Error::NONE == (ret = allocate_zero_attr_table(alloc_size)))
            {
                // congig gatt server with a new service table
                Ble::Config::DeviceConfig_t config[] =
                { // config tag                             count                           params
                    {Ble::Config::ServiceTable,           {1,   (char*) &gClientService, Ble::ConfigArgument::None}},
                    {Ble::Config::EOL,                    {0,   NULL,                    Ble::ConfigArgument::None}},
                };
                ret = gatt->Configure(config);
            }

            // register attribute access callback
            if (ret == Ble::Error::NONE)
            {
                gatt->RegisterCharacteristicAccessCallback(attr_access_callback);
            }
        }
        else
        {
            ret = Ble::Error::INVALID_PARAMETER;
        }
        break;
    }

    case MSG_ADD_ATTRIBUTE:
    {
        ret = append_attribute(msg);
        break;
    }

    case MSG_UPDATE_ATTRIBUTE:
    {
        ret = update_attribute((Update_Attribute_t *) &msg->data);
        break;
    }

    default:
        ret = Ble::Error::INVALID_PARAMETER;
        break;
    }
    return ret;
}

int main(int argc, char** argv )
{
    int ret;
    char recv_buff[sizeof(Comm_Msg_t) + ATT_MTU_MAX];
    Comm_Msg_t *msg = (Comm_Msg_t *) recv_buff;

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
        if (Ble::Error::NONE != (ret = recv_comm(FROM_CLIENT, recv_buff, sizeof(recv_buff))))
        {
            die("recv failed", ret);
        }

        // handle request, set response
        msg->hdr.error = handle_request_msg(msg);

        if (Ble::Error::NONE != (ret = send_comm(TO_CLIENT, msg, sizeof(Common_Header_t))))
        {
            die("send failed", ret);
        }

        if ((msg->hdr.type == MSG_SESSION) && (0 == msg->data.session.on_off))
        {
            break;
        }
    }

    shut_comm(SERVER);
    return 0;
}

