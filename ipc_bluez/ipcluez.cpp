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
#include <time.h>
#include <assert.h>

// static vars, status
static uint8_t gsSessionId = 0;
static Ble::ServiceInfo_t gClientService = {{0},0, NULL};

void dump_uuid(Ble::UUID_128_t &uuid __attribute__((unused)))
{
    DEBUG_PRINTF(("UUID %02x%02x%02x%02x-%02x%02x-%02x%02x-%02x%02x-%02x%02x%02x%02x%02x%02x\n",
                 uuid[0], uuid[1], uuid[2], uuid[3], uuid[4], uuid[5], uuid[6], uuid[7],
                 uuid[8], uuid[9], uuid[10], uuid[11], uuid[12], uuid[13], uuid[14], uuid[15]));
}

///
/// \brief cleanup_attribute_value
/// \param attr
///
void cleanup_attribute_value(Ble::AttributeInfo_t *attr)
{
    if (attr != NULL)
    {
        if (attr->value && attr->dynamic_alloc)
        {
            free(attr->value);
        }
        attr->value = NULL;
        attr->val_size = 0;
        attr->dynamic_alloc = 0;
    }
}

///
/// \brief cleanup_client_service
///
void cleanup_client_service()
{
    Ble::GattSrv::getInstance()->CleanupServiceList();
    if (gClientService.attr_table)
    {
        free(gClientService.attr_table);
        memset(&gClientService, 0, sizeof(gClientService));
    }
}

///
/// \brief allocate_zero_attr_table
/// \param alloc_size
/// \return
///
int allocate_zero_attr_table(int alloc_size)
{
    int ret = Ble::Error::NONE;
    // allocate clean attribute table
    gClientService.attr_table = (Ble::AttributeInfo_t*) malloc(alloc_size);
    if (!gClientService.attr_table)
    {
        ret = Ble::Error::MEMORY_ALLOOCATION;
    }
    else
    {
        memset(gClientService.attr_table, 0, alloc_size);
    }
    return ret;
}

///
/// \brief append_attribute
/// \param msg
/// \return
///
int append_attribute(Comm_Msg_t *msg)
{
    if (!gClientService.attr_table)
    {
        return Ble::Error::MEMORY_ALLOOCATION;
    }
    else for (int i = 0; i < (int) gClientService.attr_num; i++)
    {
        if (0 == gClientService.attr_table[i].max_val_size)
        {
            Ble::AttributeInfo_t *attr = &gClientService.attr_table[i];
            cleanup_attribute_value(attr);

#ifdef DEBUG_ENABLED
            memcpy(attr->attr_name, msg->data.add_attribute.name, ATTR_NAME_LEN);
#endif
            memcpy(attr->attr_uuid, msg->data.add_attribute.uuid, sizeof(Ble::UUID_128_t));
            attr->max_val_size = msg->data.add_attribute.max_length;
            attr->val_size = msg->data.add_attribute.size;
            attr->attr_type = msg->data.add_attribute.type;
            attr->properties = msg->data.add_attribute.properties;
            attr->attr_offset = (i)?(gClientService.attr_table[i-1].attr_offset+(gClientService.attr_table[i-1].attr_type==GATT_TYPE_CHARACTERISTIC?2:1)):1;
            dump_uuid(attr->attr_uuid);

            // handle payload
            if (msg->data.add_attribute.size)
            {
                attr->val_size = msg->data.add_attribute.size;
                if (NULL == (attr->value = (char*) malloc(attr->val_size)))
                {
                    attr->val_size = 0;
                    return Ble::Error::MEMORY_ALLOOCATION;
                }

                // Note: msg->data.add_attribute.attr.value does not have valid pointer to payload
                memcpy(attr->value, msg->data.add_attribute.data, attr->val_size);
                attr->dynamic_alloc = 1;
            }
#ifdef DEBUG_ENABLED
            {
                char print_string_tmp[attr->val_size+1];
                memcpy(print_string_tmp, attr->value, attr->val_size);
                print_string_tmp[attr->val_size] = '\0';
                DEBUG_PRINTF(("\tadded attr %d [%s] val [%d] %s", i, attr->attr_name, attr->val_size, print_string_tmp));
            }
#endif
            return Ble::Error::NONE;
        }
    }
    return Ble::Error::INVALID_PARAMETER;
}

///
/// \brief update_attribute
/// \param data
/// \return
///
int update_attribute(Update_Attribute_t *data)
{
    int ret = Ble::Error::NONE;
    Ble::AttributeInfo_t *attr = NULL;
    Ble::GattSrv* gatt = Ble::GattSrv::getInstance();

    if (!gClientService.attr_table)
    {
        return Ble::Error::MEMORY_ALLOOCATION;
    }
    else if ((data->attr_idx > 0) && (data->attr_idx < (int) gClientService.attr_num))
    {
        attr = &gClientService.attr_table[data->attr_idx];
    }
    else
    {
        return Ble::Error::INVALID_PARAMETER;
    }

    if (Ble::Error::NONE == (ret = gatt->UpdateCharacteristic(data->attr_idx, (const char*) data->data, data->size)))
    {
#ifdef DEBUG_ENABLED
        char print_string_tmp[attr->val_size+1];
        memcpy(print_string_tmp, attr->value, attr->val_size);
        print_string_tmp[attr->val_size] = '\0';
        DEBUG_PRINTF(("\tupdated attr %d [%s] val [%d] %s", data->attr_idx, attr->attr_name, attr->val_size, print_string_tmp));
#endif
        // Notify if connected
        if (attr->properties & GATT_PROPERTY_NOTIFY)
        {
            ret = gatt->NotifyCharacteristic(data->attr_idx, (const char*) data->data, data->size);
            DEBUG_PRINTF(("Notify characteristic err = %d %s", ret, get_err_name(ret)));
            ret = Ble::Error::NONE;
        }
    }
    return ret;
}

///
/// \brief attr_access_callback
/// \param aAttribueIdx
/// \param aAccessType
///
static void attr_access_callback(int aAttribueIdx, Ble::Property::Access aAccessType)
{
    Comm_Msg_t stash, *msg = &stash;
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
        {
            Define_Update_t update = {
                (uint16_t) gClientService.attr_table[aAttribueIdx].val_size,
                (uint8_t) aAttribueIdx,
                (uint8_t*)gClientService.attr_table[aAttribueIdx].value
            };
            msg = format_attr_updated_msg(MSG_NOTIFY_DATA_WRITE, msg, &update);
        }
        break;

    default:
        DEBUG_PRINTF(("Unexpected aAccessType = %d", aAccessType));
        msg->hdr.error = Ble::Error::INVALID_COMMAND;
        break;
    }

    // send to Client
    if (msg->hdr.error == Ble::Error::NONE)
    {
        int ret = notify_client(msg, msg->hdr.size);
        DEBUG_PRINTF(("Notify Client aAccessType = %d, type = %d %s, err = %d %s", aAccessType, msg->hdr.type, get_msg_name(msg), ret, get_err_name(ret)));
    }

    // if new msg was allocated with malloc - release it
    if (msg != &stash)
    {
        assert(msg);
        free(msg);
    }
}

///
/// \brief handle_request_msg
/// \param msg
/// \return
///
int handle_request_msg(Comm_Msg_t *msg)
{
    int ret = Ble::Error::NONE;
    Ble::GattSrv* gatt = Ble::GattSrv::getInstance();

    DEBUG_PRINTF(("got msg %d %s", msg->hdr.type, get_msg_name(msg)));

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
        if (Ble::Error::NONE != (ret = gatt->SetLocalDeviceName((char*) ((Config_t *)&msg->data)->device_name)))
        {
            DEBUG_PRINTF(("ERROR %d from gatt->SetLocalDeviceName((char*) ((Config_t *)&msg->data)->device_name)))\n", ret));
        }
        else if (Ble::Error::NONE != (ret = gatt->SetMACAddress((char*) ((Config_t *)&msg->data)->mac_address)))
        {
            DEBUG_PRINTF(("ERROR %d from gatt->SetMACAddress((char*) ((Config_t *)&msg->data)->mac_address)))\n", ret));
        }
        else if (Ble::Error::NONE != (ret = gatt->SetLocalClassOfDevice(((Config_t *)&msg->data)->device_class)))
        {
            DEBUG_PRINTF(("ERROR %d from gatt->SetLocalClassOfDevice(((Config_t *)&msg->data)->device_class)))\n", ret));
        }
        break;
    }

    case MSG_ADVERTISEMENT:
    {
        Advertisement_t *data = (Advertisement_t*) &msg->data;
        if (data->on_off)
        {
            ret = gatt->StartAdvertising();
        }
        else
        {
            ret = gatt->StopAdvertising();
        }
        break;
    }

    case MSG_ADD_SERVICE:
    {
        Add_Service_t *data = (Add_Service_t *) &msg->data;
        if (data->count > 0)
        {
            int alloc_size = data->count * sizeof(Ble::AttributeInfo_t);

            // only single service is supported at this time
            cleanup_client_service();

            // set service parameters
            gClientService.attr_num = data->count;
            memcpy(gClientService.svc_uuid, data->uuid, sizeof(Ble::UUID_128_t));
            dump_uuid(gClientService.svc_uuid);

            if (Ble::Error::NONE == (ret = allocate_zero_attr_table(alloc_size)))
            {
                if (Ble::Error::NONE == (ret = gatt->SetServiceTable(&gClientService)))
                {
                    ret = gatt->RegisterCharacteristicAccessCallback(attr_access_callback);
                }
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


///
/// \brief main
/// \param argc
/// \param argv
/// \return
///
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
        if (Ble::Error::NONE != (ret = recv_from_client(msg, sizeof(recv_buff))))
        {
            die("recv failed", ret);
        }

        // handle request, set response
        msg->hdr.error = handle_request_msg(msg);

        if (Ble::Error::NONE != (ret = resp_to_client(msg, sizeof(Common_Header_t))))
        {
            die("send failed", ret);
        }

        if ((msg->hdr.type == MSG_SESSION) && (0 == msg->data.session.on_off))
        {
            break;
        }
    }

    shut_comm();
    return 0;
}

