/*---------------------------------------------------------------------
 *               ------------------      -----------------            *
 *               | Ring App Setup |      |   test_ble    |            *
 *               ------------------      -----------------            *
 *                          |                |                        *
 *                          |   --------------------                  *
 *                          |   |gatt_src_test.cpp |                  *
 *                          |   --------------------                  *
 *                          |     |          |                        *
 *              ----------------------       |                        *
 *              | RingBlePairing.cpp |       |                        *
 *              ----------------------       |                        *
 *                          |                |                        *
 *                  ---------------------------------                 *
 *                  |    RinmBleApi.cpp abstract    |                 *
 *                  ---------------------------------                 *
 *                  |  RingGattSrv  |  RingGattSrv  |                 *
 *                  |   WILINK18    |    BCM43      |                 *
 *                  --------------------------------                  *
 *                        |                  |                        *
 *       ------------------------       ---------------               *
 *       |      TIBT lib        |       |   BlueZ     |               *
 *       ------------------------       ---------------               *
 *                  |                        |                        *
 *       ------------------------       ----------------              *
 *       | TI WiLink18xx BlueTP |       | libbluetooth |              *
 *       ------------------------       ----------------              *
 *--------------------------------------------------------------------*/
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <signal.h>
#include <unistd.h>

#include "Bot_Notifier.h"
#include "RingBleApi.hh"
#include "RingGattApi.hh"
#include "RingBlePairing.hh"

#include "gatt_svc_defs.h"

using namespace Ring;
using namespace Ring::Ble;

/* -------------------------------------------------------------*
 * section of auto-declaration
 * the define file may be included multiple times to generate
 * different stuctures correlated by order of definition
 * -------------------------------------------------------------*/
#define RING_PAIRING_TABLE_SERVICE_DECL
#include "gatt_svc_defs.h"

#define RING_PAIRING_TABLE_SERVICE_DEFINE_TABLE
static AttributeInfo_t SrvTable0[] = {
    #include "gatt_svc_defs.h"
};

#define RING_PAIRING_TABLE_ATTR_ENUM
enum GattAttributeIndexByName {
    #include "gatt_svc_defs.h"
    RING_PAIRING_TABLE_ATTR_MAX
};
// compiler time check for characteristics max
static_assert(RING_PAIRING_TABLE_ATTR_MAX < RING_CHARACTERISTICS_MAX, "RING_CHARACTERISTICS_MAX < RING_PAIRING_TABLE_ATTR_MAX");

#define RING_PAIRING_SERVICE_INFO_DEFINE
#include "gatt_svc_defs.h"

/* -------------------------------------------------------------*
 * static callback function declaration
 * -------------------------------------------------------------*/
static void OnAttributeAccessCallback(int aServiceIdx, int aAttributeIdx, Ble::Property::Access aAccessType);
std::function<int(int, void*, int)> ringDataCb = NULL;

/* -------------------------------------------------------------*
 * BlePairing class implementation
 * -------------------------------------------------------------*/
BlePairing* BlePairing::instance = NULL;
char BlePairing::mMacAddress[DEV_MAC_ADDR_LEN] = "XX:XX:XX:XX:XX:XX";

#define DECRYPTED_MSG_BUFFER_SIZE           2000
Crypto::Server *BlePairing::mCrypto = NULL;
static const uint8_t sign_priv[] = {
    0x2b,0x3a,0x3d,0x35,0x75,0xe5,0x94,0xec,0x77,0xf5,0xeb,0x96,0xf3,0xb9,0xda,0xc6,
    0x8a,0x00,0x21,0xdd,0x5a,0x9c,0x15,0xf0,0x0e,0xa7,0x46,0xc0,0xf8,0x21,0x22,0x38,
    0x9f,0x9c,0x2c,0x9a,0xc1,0xbd,0x07,0x7d,0xd9,0x2f,0xeb,0xa3,0x89,0x34,0x5e,0x0a,
    0x11,0xa3,0x85,0x72,0x84,0x66,0x7a,0xc4,0x85,0x56,0xf9,0x2d,0x36,0x0f,0xdf,0x97,
};

#define ATTRIBUTE_OFFSET(_idx) sServiceTable[RING_PAIRING_SVC_IDX].AttributeList[_idx].AttributeOffset

// singleton instance
BlePairing* BlePairing::getInstance() {
    if (instance == NULL)
        instance = new BlePairing();
    return instance;
}

// private constructor
BlePairing::BlePairing() :
    mPairingServiceIndex(0),
    mAdvertisingRequested(false)
{
    if (NULL == (mBleApi = GattSrv::getInstance()))
    {
        BOT_NOTIFY_INFO("BlePairing failed to obtain BleApi instance");
    }

    // set other config parameters
    mLocalClassOfDevice = 0x000430;
    mAdvertisingTimeout_sec = 600;
    mAdvIntervalMin_ms = 2000;
    mAdvIntervalMax_ms = 3000;

    // set definitions from gatt_svc_defs.h
    mServiceTable = sServiceTable;
    mServiceCount = RING_GATT_SERVICES_COUNT;

    // initialize with default string, ringnm will set proper device name later
    strncpy(mRingDeviceName, "RingSetup-BT", DEV_NAME_LEN);
}

///
/// \brief BlePairing::Initialize
/// \return Ble::Error
///
int BlePairing::Initialize(char *aDeviceName, uint8_t * mac)
{
    int ret_val = Error::UNDEFINED;

    // BLE Device Configuration
    if (aDeviceName != NULL)
    {
        // override device name
        strncpy(mRingDeviceName, aDeviceName, DEV_NAME_LEN);
    }

    if (mac != NULL)
    {
        AttributeInfo_t *attribute_list = mServiceTable[RING_PAIRING_SVC_IDX].AttributeList;
        sprintf(mMacAddress, "%02X:%02X:%02X:%02X:%02X:%02X",mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
        ((CharacteristicInfo_t*) (attribute_list[GET_MAC_ADDRESS].Attribute))->Value = (Byte_t*) mMacAddress;
        ((CharacteristicInfo_t*) (attribute_list[GET_MAC_ADDRESS].Attribute))->ValueLength = strlen(mMacAddress);
    }

    DeviceConfig_t config[] =
    { // config tag                         count   params
        {Ble::Config::ServiceTable,           {1,   {{(char*) mServiceTable, mServiceCount}}}},
        {Ble::Config::LocalDeviceName,        {1,   {{(char*) mRingDeviceName, Ble::ConfigArgument::None}}}},
        {Ble::Config::LocalClassOfDevice,     {1,   {{NULL, mLocalClassOfDevice}}}},
        {Ble::Config::Discoverable,           {1,   {{NULL, Ble::ConfigArgument::Enable}}}},
        {Ble::Config::Connectable,            {1,   {{NULL, Ble::ConfigArgument::Enable}}}},
        {Ble::Config::Pairable,               {1,   {{NULL, Ble::ConfigArgument::Enable}}}},
        {Ble::Config::LocalDeviceAppearance,  {1,   {{NULL, BleApi::BLE_APPEARANCE_GENERIC_COMPUTER}}}},
        {Ble::Config::AdvertisingInterval,    {2,   {{NULL, mAdvIntervalMin_ms}, {NULL, mAdvIntervalMax_ms}}}},
        {Ble::Config::RegisterGATTCallback,   {0,   {{NULL, Ble::ConfigArgument::None}}}},
        {Ble::Config::RegisterService,        {1,   {{NULL, mPairingServiceIndex}}}},
        {Ble::Config::RegisterAuthentication, {0,   {{NULL, Ble::ConfigArgument::None}}}},
        {Ble::Config::SetSimplePairing,       {2,   {{NULL, Ble::ConfigArgument::None}, {NULL, Ble::ConfigArgument::None}}}},
        {Ble::Config::EnableBluetoothDebug,   {2,   {{NULL, Ble::ConfigArgument::Enable}, {NULL, Ble::ConfigArgument::Terminal}}}},
        {Ble::Config::EOL,                    {0,   {{NULL, Ble::ConfigArgument::None}}}},
    };

    // try one more time
    if (mBleApi == NULL)
    {
        mBleApi = GattSrv::getInstance();
    }

    // if still no luck - return error
    if (mBleApi == NULL)
    {
        BOT_NOTIFY_ERROR("BlePairing failed to obtain BleApi instance");
        ret_val = Error::FAILED_INITIALIZE;
    }
    else
    {
        if (Ble::Error::NONE != (ret_val = mBleApi->Initialize()))
        {
            BOT_NOTIFY_ERROR("mBleApi->Initialize() failed.");
        }
        else if (Ble::Error::NONE != (ret_val = mBleApi->SetDevicePower(Ble::ConfigArgument::PowerOn)))
        {
            BOT_NOTIFY_ERROR("mBleApi->SetDevicePower(ON) failed.");
        }
        else if (Ble::Error::NONE != (ret_val = mBleApi->Configure(config)))
        {
            BOT_NOTIFY_ERROR("mBleApi->Configure failed, ret = %d, Abort.", ret_val);
        }
        else if (Ble::Error::NONE != (ret_val = mBleApi->RegisterCharacteristicAccessCallback(OnAttributeAccessCallback)))
        {
            BOT_NOTIFY_ERROR("mBleApi->RegisterCharacteristicAccessCallback failed, ret = %d, Abort.", ret_val);
        }
    }

    return ret_val;
}

///
/// \brief BlePairing::StartAdvertising
/// \return  Ble::Error
///
int BlePairing::StartAdvertising(int aTimeout)
{
    int ret_val = Error::UNDEFINED;

    // mAdvertisingRequested is set to true here because it is not indicator
    // of advertisement is in progress or not, but the fact the user wanted to start it
    mAdvertisingRequested = true;

    if (mBleApi == NULL)
    {
        BOT_NOTIFY_WARNING("BlePairing failed to obtain BleApi instance");
        ret_val = Error::NOT_INITIALIZED;
    }
    else
    {
        if (aTimeout > 0)
        {
            // override default mAdvertisingTimeout_sec
            mAdvertisingTimeout_sec = aTimeout;
        }

        // start advertising
        unsigned int adv_flags = Advertising::Discoverable  | Advertising::Connectable      |
                                 Advertising::AdvertiseName | Advertising::AdvertiseTxPower | Advertising::AdvertiseAppearance;
        ParameterList_t params = {2, {{NULL, adv_flags}, {NULL, mAdvertisingTimeout_sec}}};

        if (Ble::Error::NONE != (ret_val = mBleApi->StartAdvertising(&params)))
        {
            BOT_NOTIFY_ERROR("mBleApi->StartAdvertising failed, ret = %d, Abort.", ret_val);
        }
    }
    return ret_val;
}

///
/// \brief BlePairing::StopAdvertising
/// \return
///
int BlePairing::StopAdvertising()
{
    int ret_val = Error::UNDEFINED;

    // mAdvertisingRequested is set to false here because it is not indicator
    // of advertisement is in progress or not, but the fact the user wanted to stop it
    mAdvertisingRequested = false;

    if (mBleApi == NULL)
    {
        BOT_NOTIFY_WARNING("BlePairing failed to obtain BleApi instance");
        ret_val = Error::NOT_INITIALIZED;
    }
    else
    {
        ParameterList_t params = {1, {{0, 0}}};
        if (Ble::Error::NONE != (ret_val = mBleApi->StopAdvertising(&params)))
        {
            // ret_val can be != Error::NONE also if adv was not started or already expired
            BOT_NOTIFY_WARNING("mBleApi->StopAdvertising ret = %d", ret_val);
        }
    }
    return ret_val;
}

///
/// \brief BlePairing::Shutdown
/// \return
///
int BlePairing::Shutdown()
{
    int ret_val = Error::UNDEFINED;

    if (mBleApi == NULL)
    {
        BOT_NOTIFY_WARNING("BlePairing failed to obtain BleApi instance");
        ret_val = Error::NOT_INITIALIZED;
    }
    else
    {   // deactivate GATT Server
        if (mAdvertisingRequested)
            this->StopAdvertising();

        ParameterList_t null_params= {0,   {{NULL, Ble::ConfigArgument::None}}};
        ParameterList_t svc_params = {1,   {{NULL, mPairingServiceIndex}}};

        if (Ble::Error::NONE != (ret_val = mBleApi->GATTUnRegisterService(&svc_params)))
        {
            BOT_NOTIFY_ERROR("mBleApi->GATTUnRegisterService failed.");
        }
        if (Ble::Error::NONE != (ret_val = mBleApi->UnRegisterGATMEventCallback(&null_params)))
        {
            BOT_NOTIFY_ERROR("mBleApi->UnRegisterGATMEventCallback failed.");
        }
        if (Ble::Error::NONE != (ret_val = mBleApi->UnRegisterAuthentication(&null_params)))
        {
            BOT_NOTIFY_ERROR("mBleApi->UnRegisterAuthentication failed.");
        }
        if (Ble::Error::NONE != (ret_val = mBleApi->UnregisterCharacteristicAccessCallback(OnAttributeAccessCallback)))
        {
            BOT_NOTIFY_ERROR("mBleApi->UnRegisterCharacteristicAccessCallback failed.");
        }
        // TODO: do we want to power off the BLE device completely and shutdown?
        if (Ble::Error::NONE != (ret_val = mBleApi->SetDevicePower(Ble::ConfigArgument::PowerOff)))
        {
            BOT_NOTIFY_ERROR("mBleApi->SetDevicePower(OFF) failed.");
        }
        else if (Ble::Error::NONE != (ret_val = mBleApi->Shutdown()))
        {
            BOT_NOTIFY_ERROR("mBleApi->Shutdown failed.");
        }
    }
    return ret_val;
}

///
/// \brief BlePairing::Status
/// \return
///
int BlePairing::PrintStatus()
{
    int ret_val = Error::UNDEFINED;

    if (mBleApi == NULL)
    {
        BOT_NOTIFY_WARNING("BlePairing failed to obtain BleApi instance");
        ret_val = Error::NOT_INITIALIZED;
    }
    else if (Ble::Error::NONE != (ret_val = mBleApi->QueryLocalDeviceProperties(NULL)))
    {
        BOT_NOTIFY_ERROR("mBleApi->QueryLocalDeviceProperties failed.");
    }
    return ret_val;
}

///
/// \brief OnAttributeAccessCallback
/// \param aServiceIdx
/// \param aAttributeIdx
/// \param aAccessType
///
static void OnAttributeAccessCallback(int aServiceIdx, int aAttributeIdx, Ble::Property::Access aAccessType)
{
    static const AttributeInfo_t *attribute_list = sServiceTable[RING_PAIRING_SVC_IDX].AttributeList;

    if (aAccessType == Ble::Property::Disconnected)
    {
        // special case to re-enable advertisement if it was susspended by connection, but
        // user didn't cancel the request to start it
        BlePairing *pairing_inst = BlePairing::getInstance();

        if (pairing_inst)
        {
            BOT_NOTIFY_DEBUG("Ble::Property::Disconnected, ads = %d, %s", pairing_inst->isAdvertisingRequested()?1:0, pairing_inst->isAdvertisingRequested()?"restarting":"");
            if (pairing_inst->isAdvertisingRequested())
            {
                pairing_inst->StartAdvertising();
            }
        }
        return;
    }
    else if (aAccessType == Ble::Property::Connected)
    {
        BOT_NOTIFY_DEBUG("Ble::Property::Connected...");
        return;
    }

    BOT_NOTIFY_INFO("OnAttributeAccessCallback Ble::Property::%s for %s %s\n",
           aAccessType == Ble::Property::Read ? "Read": aAccessType == Ble::Property::Write ? "Write":"Confirmed",
           sServiceTable[aServiceIdx].ServiceName, sServiceTable[aServiceIdx].AttributeList[aAttributeIdx].AttributeName);

    BleApi* bleApi = GattSrv::getInstance();
    if (bleApi == NULL)
    {
        BOT_NOTIFY_ERROR("OnAttributeAccessCallback failed to obtain BleApi instance");
        return;
    }

    Ble::GATT_UUID_t uuid;
    uuid.UUID_Type     = Ble::guUUID_128;
    uuid.UUID.UUID_128 = ((CharacteristicInfo_t*) (attribute_list[aAttributeIdx].Attribute))->CharacteristicUUID;
    bleApi->DisplayGATTUUID(&uuid, "Characteristic: ", 0);

    switch (aAccessType)
    {
        case Ble::Property::Read:
            bleApi->DisplayAttributeValue(aServiceIdx, aAttributeIdx, "Read");

            // ringnm callback
            if (ringDataCb)
            {
                ringDataCb(aAttributeIdx, NULL, 0);
            }
            break;

        case Ble::Property::Write:
            bleApi->DisplayAttributeValue(aServiceIdx, aAttributeIdx, "Write");
            {
                char decrypted[DECRYPTED_MSG_BUFFER_SIZE];
                int decrypted_len = sizeof(decrypted);

                void *data = (void *) ((CharacteristicInfo_t*) (attribute_list[aAttributeIdx].Attribute))->Value;
                int len = ((CharacteristicInfo_t*) (attribute_list[aAttributeIdx].Attribute))->ValueLength;

                // processed internally by Pairing
                if (aAttributeIdx == SET_PUBLIC_KEY)
                {
                    BOT_NOTIFY_DEBUG("got %d bytes of SET_PUBLIC_KEY, preparing PUBLIC_PAYLOAD", len);

                    if (BlePairing::mCrypto)
                    {
                        delete BlePairing::mCrypto;
                        BlePairing::mCrypto = NULL;
                    }

                    if (NULL != (BlePairing::mCrypto = new Ring::Ble::Crypto::Server((char*) data, len, sign_priv)))
                    {
                        char server_public[0xff];
                        int server_public_lenght = sizeof(server_public);
                        BlePairing::mCrypto->GetPublicPayload(server_public, server_public_lenght);
                        BOT_NOTIFY_DEBUG("Crypto->GetPublicPayload ret %d bytes of PUBLIC_PAYLOAD\n", server_public_lenght);

                        BlePairing::getInstance()->updateAttribute(GET_PUBLIC_PAYLOAD, server_public, server_public_lenght);
                    }
                    else
                        BOT_NOTIFY_ERROR("failed to create Crypto server!\n");
                }
                else if (BlePairing::mCrypto)
                {
                    // decode all other written values
                    if (Crypto::Error::NO_ERROR == BlePairing::mCrypto->Decrypt((char*) data, len, decrypted, decrypted_len))
                    {
                        data = (void*) decrypted;
                        len = decrypted_len;
                    }
                    else
                        BOT_NOTIFY_ERROR("failed to decrypt payload value!\n");
                }
                else
                    BOT_NOTIFY_ERROR("BlePairing::mCrypto not initialized - failed to decrypt payload value!\n");

                // ringnm callback
                if (ringDataCb)
                {
                    ringDataCb(aAttributeIdx, data, len);
                }
            }
            break;

        default:
            bleApi->DisplayAttributeValue(aServiceIdx, aAttributeIdx, "not handled aAccessType");
            break;
    }
}

int BlePairing::registerRingDataCallback(std::function<int(int, void*, int)> callback)
{
    BOT_NOTIFY_INFO("registering ring data callback");
    ringDataCb = callback;

    return Error::NONE;
}

int BlePairing::updateAttribute(int attr_idx, const char * str_data, int len)
{
    BleApi* bleApi = GattSrv::getInstance();
    if (bleApi == NULL)
    {
        BOT_NOTIFY_ERROR("updateAttribute failed to obtain BleApi instance");
        return Error::FAILED_INITIALIZE;
    }

    unsigned int service_id = mServiceTable[RING_PAIRING_SVC_IDX].ServiceID;

    // only when str_data is valid
    if (str_data)
    {
        bleApi->GATTUpdateCharacteristic(service_id, ATTRIBUTE_OFFSET(attr_idx), (Byte_t*) str_data, (len > 0) ? len : strlen(str_data));
    }

    switch (attr_idx)
    {
        case GET_NET_INFO:
            bleApi->NotifyCharacteristic(RING_PAIRING_SVC_IDX, GET_PAIRING_STATE, "NETWORK_INFO_UPDATED");
            break;

        case GET_AP_LIST:
            bleApi->NotifyCharacteristic(RING_PAIRING_SVC_IDX, GET_PAIRING_STATE, "AP_LIST_UPDATED");
            break;

        case SET_PROVISION:
            bleApi->NotifyCharacteristic(RING_PAIRING_SVC_IDX, GET_PAIRING_STATE, "PROVISIONED");
            break;

        case GET_WIFI_STATUS:
            bleApi->NotifyCharacteristic(RING_PAIRING_SVC_IDX, GET_PAIRING_STATE, "WIFI_STATUS_UPDATED");
            break;

        case SET_PUBLIC_KEY:
            bleApi->NotifyCharacteristic(RING_PAIRING_SVC_IDX, GET_PAIRING_STATE, "PAYLOAD_READY");
            break;

        default:
            break;
    }

    return Error::NONE;
}

int BlePairing::updateServiceTable(int attr_idx, const char *str_data, int len)
{
    int ret_val = Error::UNDEFINED;
    if (mServiceTable == NULL)
        ret_val = Error::NOT_INITIALIZED;
    else if (attr_idx >= (int) mServiceTable[RING_PAIRING_SVC_IDX].NumberAttributes)
        ret_val = Error::INVALID_PARAMETERS;
    else {
        CharacteristicInfo_t *attr = (CharacteristicInfo_t*) mServiceTable[RING_PAIRING_SVC_IDX].AttributeList[attr_idx].Attribute;
        if (len > (int) attr->MaximumValueLength)
            ret_val = Error::INVALID_PARAMETERS;
        else {
            if (attr->Value && ((int) attr->ValueLength == len) && str_data && !memcmp(attr->Value, str_data, len))
            {
                // the same - skip
                ret_val = Error::NONE;
            }
            else {
                if (attr->Value && attr->AllocatedValue) {
                    free(attr->Value);
                    attr->AllocatedValue = 0;
                }
                attr->Value = NULL;
                attr->ValueLength = 0;

                if (str_data && len) {
                    attr->Value = (unsigned char*) malloc(len);
                    if (attr->Value) {
                        attr->AllocatedValue = 1;
                        memcpy(attr->Value, str_data, attr->ValueLength = len);
                    }
                }
                ret_val = Error::NONE;
            }
        }
    }
    return ret_val;
}
