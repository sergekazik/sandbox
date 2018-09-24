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

#ifndef _SERVER_GATT_API_H_
#define _SERVER_GATT_API_H_

#include <stdio.h>
#include <stdlib.h>

#include <bluetooth/bluetooth.h>
#include <bluetooth/hci.h>
#include <bluetooth/hci_lib.h>

#define GATT_CHARACTERISTICS_MAX            32
#define BLE_READ_PACKET_MAX                 22      /* varies 18-22 on multiple read/write */
#define ATT_MTU_MAX                         512     /* maximum allowed value size of 512 bytes.*/
#define GATT_SVC_NUM_OF_HANDLERS_MAX        48
#define DEV_CLASS_LEN                       16      /* device class bitmask as a string */
#define DEV_NAME_LEN                        64      /* text string = device name */
#define DEV_MAC_ADDR_LEN                    18      /* text string = device mac address */
#define ATTR_NAME_LEN                       16

struct server_ref
{
    struct bt_att *att;
    struct gatt_db *db;
    struct bt_gatt_server *gatt;
};

namespace Ble {

typedef uint8_t UUID_128_t[16];

#define GATT_PROPERTY_READ          0x02
#define GATT_PROPERTY_WRITE         0x08
#define GATT_PROPERTY_NOTIFY        0x10
#define GATT_TYPE_CHARACTERISTIC    0x00
#define GATT_TYPE_DESCRIPTOR        0x01

typedef struct attributeinfo
{
#ifdef DEBUG_ENABLED
    char       attr_name[ATTR_NAME_LEN];
#endif
    uint8_t    attr_type;
    uint8_t    attr_offset;
    uint8_t    properties;
    UUID_128_t attr_uuid;
    uint8_t    dynamic_alloc;
    uint16_t   max_val_size;
    uint16_t   val_size;
    char      *value;
} AttributeInfo_t;

typedef struct serviceinfo
{
    UUID_128_t       svc_uuid;
    unsigned int     attr_num;
    AttributeInfo_t *attr_table;
} ServiceInfo_t;

namespace Error { enum Error {
    NONE                =  0, // NO ERROR, SUCCESS
    OPERATION_FAILED    = -1, // no command was specified to the parser.
    INVALID_COMMAND     = -2, // the Command does not exist for processing.
    IGNORED             = -3, // the Command was not expected and ignored by handler
    FUNCTION            = -4, // an error occurred in execution of the Command Function.
    TIMEOUT             = -5, // operation ended by timeout
    INVALID_PARAMETER   = -6, // an error occurred due to the fact that one or more of the required parameters were invalid.
    NOT_INITIALIZED     = -7, // an error occurred due to the fact that the GattSrv has not been initialized.
    UNDEFINED           = -8, // Not initialized value; not all paths of the function modify return value
    NOT_IMPLEMENTED     = -9, // Not yet implemented or not supported for this target
    NOT_FOUND           = -10,// Search not found
    INVALID_STATE       = -11,// Already set, single use error or not connected
    NOT_REGISTERED      = -12,// Callback is not registered
    FAILED_INITIALIZE   = -13,//
    PTHREAD_ERROR       = -14,// create or cancel error
    RESOURCE_UNAVAILABLE= -15,// server busy, adapter unavailable, etc.
    CB_REGISTER_FAILED  = -16,// registering callback failed
    REGISTER_SVC_ERROR  = -17,// registering gatt service failed
    MEMORY_ALLOOCATION  = -18,// memory allocation error
    // last
    ERROR_COUNT_MAX     = -19 // end of messages, don't add below
};}

namespace ConfigArgument { enum Arg {
    None    = 0,
    Enable  = 1,
    Disable = 0,
    PowerOn = 1,
    PowerOff = 0,
    Start = 1,
    Stop = 0
};}

namespace Property
{
    enum Access {
        Connected,
        Read,
        Write,
        Disconnected,
    };
    enum Permission
    {
        READ    = GATT_PROPERTY_READ,
        WRITE   = GATT_PROPERTY_WRITE,
        RW      = GATT_PROPERTY_READ | GATT_PROPERTY_WRITE,
        NOTIFY  = GATT_PROPERTY_NOTIFY,
        RWN     = GATT_PROPERTY_READ | GATT_PROPERTY_WRITE | GATT_PROPERTY_NOTIFY,
    };
}

typedef struct _GattServerInfo {
    int fd;
    int hci_socket;
    pthread_t hci_thread_id;
    server_ref *sref;
    uint16_t client_svc_handle;
    uint16_t client_attr_handle[GATT_CHARACTERISTICS_MAX];
} GattServerInfo_t;

class GattSrv
{
public:
    static GattSrv* getInstance();
    static GattServerInfo_t mServer;
    bool            mInitialized;              // initialization state
    ServiceInfo_t  *mServiceTable;             // pointer to populated service tbl

    typedef void (*onCharacteristicAccessCallback) (int aAttribueIdx, Ble::Property::Access aAccessType);

private:
    static GattSrv* instance;
    GattSrv();
    char mDeviceName[DEV_NAME_LEN];            // device name
    char mMacAddress[DEV_MAC_ADDR_LEN];
    onCharacteristicAccessCallback  mOnCharCb; // callback to client function on characteristic change - Note: single user only

public:
    int Initialize();
    int Shutdown();

    int SetServiceTable(ServiceInfo_t *aService);
    int SetMACAddress(char *aAddress);
    int SetLocalDeviceName(char *aName);
    int SetLocalClassOfDevice(int aClass);

    int RegisterCharacteristicAccessCallback(onCharacteristicAccessCallback aCb);
    int UnregisterCharacteristicAccessCallback(onCharacteristicAccessCallback aCb);

    int UpdateCharacteristic(int attr_idx, const char *str_data, int len);
    int NotifyCharacteristic(int aAttributeIdx, const char* aPayload, int len=0);

    // Advertising
    int StartAdvertising();
    int StopAdvertising();

    // helpers
    int GetAttributeIdxByOffset(unsigned int attr_offset);
    int ProcessRegisteredCallback(Ble::Property::Access aEventType, int aAttrIdx);
    void CleanupServiceList();

private:
    struct hci_dev_info di;
    int OpenSocket(struct hci_dev_info &di);
    void PrintDeviceHeader(struct hci_dev_info *di);
    void WriteBdaddr(int dd, char *opt);

public: // HCI methods
    void HCIreset(int ctl, int hdev);
    void HCIscan(int ctl, int hdev, char *opt);
    void HCIssp_mode(int hdev, char *opt);
    void HCIup(int ctl, int hdev);
    void HCIdown(int ctl, int hdev);
    void HCIle_adv(int hdev, char *opt);
    void HCIno_le_adv(int hdev);
    void HCIname(int hdev, char *opt);
    void HCIclass(int hdev, char *opt);
};

} /* namespace Ble */


#endif // _SERVER_GATT_API_H_
