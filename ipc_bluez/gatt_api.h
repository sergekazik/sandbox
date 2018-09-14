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

namespace Config {
typedef struct
{
    int             count;
    char           *strParam;
    unsigned int    intParam;
} Parameter_t;

enum Tag {
    EOL                  = 0, // End of list
    ServiceTable,
    LocalDeviceName,
    LocalClassOfDevice,
    MACAddress
};
typedef struct
{
    Config::Tag         tag;
    Config::Parameter_t params;
} DeviceConfig_t;
}

#define GATT_SECURITY_NONE      0x00000000
#define GATT_PROPERTY_READ      0x00000002
#define GATT_PROPERTY_WRITE     0x00000008
#define GATT_PROPERTY_NOTIFY    0x00000010

typedef enum
{
    atCharacteristic = 0,
    atDescriptor     = 1
} AttributeType_t;

typedef struct attributeinfo
{
    uint8_t    AttributeType;
    uint8_t    AttributeOffset;
    char       AttributeName[ATTR_NAME_LEN];
    uint8_t    PropertiesMask;
    uint8_t    SecurityMask;
    UUID_128_t AttributeUUID;
    uint8_t    AllocatedValue;
    uint16_t   MaxValueLength;
    uint16_t   ValueLength;
    char      *Value;
} AttributeInfo_t;

typedef struct serviceinfo
{
    UUID_128_t       ServiceUUID;
    unsigned int     NumberAttributes;
    AttributeInfo_t *AttributeList;
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
        R__     = GATT_PROPERTY_READ,
        _W_     = GATT_PROPERTY_WRITE,
        RW_     = GATT_PROPERTY_READ | GATT_PROPERTY_WRITE,
        __N     = GATT_PROPERTY_NOTIFY,
        R_N     = GATT_PROPERTY_READ | GATT_PROPERTY_NOTIFY,
        _WN     = GATT_PROPERTY_WRITE | GATT_PROPERTY_NOTIFY,
        RWN     = GATT_PROPERTY_READ | GATT_PROPERTY_WRITE | GATT_PROPERTY_NOTIFY,
    };
}

namespace Advertising { enum Flags {
    // Flags is a bitmask in the following table
    LocalPublicAddress  = 0x00000001, // [Local Random Address] - if not set
    Discoverable        = 0x00000002, // [Non Discoverable] - if not set
    Connectable         = 0x00000004, // [Non Connectable] - if not set
    AdvertiseName       = 0x00000010, // [Advertise Name off] - if not set
    AdvertiseTxPower    = 0x00000020, // [Advertise Tx Power off] - if not set
    AdvertiseAppearance = 0x00000040, // [Advertise Appearance off] - if not set
    PeerPublicAddress   = 0x00000100, // [Peer Random Address] - if not set
    DirectConnectable   = 0x00000200, // [Undirect Connectable] // When Connectable bit (0x0004) is set: - if not set
    LowDutyCycle        = 0x00000400, // [High Duty Cycle] // When Direct Connectable bit (0x0200) is set: - if not set
};}

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

    // https://www.bluetooth.com/specifications/gatt/viewer?attributeXmlFile=org.bluetooth.characteristic.gap.appearance.xml
    enum Appearance {
        BLE_APPEARANCE_UNKNOWN                  = 0,
        BLE_APPEARANCE_GENERIC_PHONE            = 64,
        BLE_APPEARANCE_GENERIC_COMPUTER         = 128,
        BLE_APPEARANCE_GENERIC_WATCH            = 192,
        BLE_APPEARANCE_GENERIC_SENSOR           = 1344,
        BLE_APPEARANCE_MOTION_SENSOR            = 1345,
    };

    typedef void (*onCharacteristicAccessCallback) (int aAttribueIdx, Ble::Property::Access aAccessType);

private:
    static GattSrv* instance;
    GattSrv();
    char mDeviceName[DEV_NAME_LEN];            // device name
    char mMacAddress[DEV_MAC_ADDR_LEN];
    onCharacteristicAccessCallback  mOnCharCb; // callback to client function on characteristic change - Note: single user only

public:
    int Initialize();
    int Configure(Ble::Config::DeviceConfig_t* aConfig);

    int SetDevicePower(Ble::ConfigArgument::Arg aOnOff);

    int Shutdown();

    int SetLocalDeviceName(Ble::Config::Parameter_t *aParams);
    int SetLocalClassOfDevice(Ble::Config::Parameter_t *aParams);

    int RegisterCharacteristicAccessCallback(onCharacteristicAccessCallback aCb);
    int UnregisterCharacteristicAccessCallback(onCharacteristicAccessCallback aCb);

    int QueryLocalDeviceProperties(Ble::Config::Parameter_t *aParams);
    int SetLocalDeviceAppearance(Ble::Config::Parameter_t *aParams);

    // connection and security
    int SetRemoteDeviceLinkActive(Ble::Config::Parameter_t *aParams);
    int RegisterAuthentication(Ble::Config::Parameter_t *aParams);
    int ChangeSimplepairingParameters(Ble::Config::Parameter_t *aParams);

    // GATT
    int RegisterGATMEventCallback(Ble::Config::Parameter_t *aParams);
    int GATTRegisterService(Ble::Config::Parameter_t *aParams);
    int GATTUpdateCharacteristic(int attr_idx, const char *str_data, int len);
    int NotifyCharacteristic(int aAttributeIdx, const char* aPayload, int len=0);

    // Advertising
    int SetAdvertisingInterval(Ble::Config::Parameter_t *aParams);
    int StartAdvertising(Ble::Config::Parameter_t *aParams);
    int StopAdvertising(Ble::Config::Parameter_t *aParams);

    int SetAuthenticatedPayloadTimeout(Ble::Config::Parameter_t *aParams);
    int SetAndUpdateConnectionAndScanBLEParameters(Ble::Config::Parameter_t *aParams);
    void CleanupServiceList(void);

    // debug
    int EnableBluetoothDebug(Ble::Config::Parameter_t *aParams);

    // helper functions
    int GetAttributeIdxByOffset(unsigned int AttributeOffset);
    int ProcessRegisteredCallback(Ble::Property::Access aEventType, int aAttrIdx);

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
