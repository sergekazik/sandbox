/*---------------------------------------------------------------------
 *               ------------------      -----------------            *
 *               | Ring App Setup |      |   test_ble    |            *
 *               ------------------      -----------------            *
 *                          |                |                        *
 *                          |   --------------------                  *
 *                          |   |gatt_src_test.cpp |                  *
 *                          |   --------------------                  *
 *                          |          |                              *
 *                  ---------------------------------                 *
 *                  |    RingBleApi.cpp abstract    |                 *
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
#include "Bot_Notifier.h"
#include "RingBleApi.hh"

using namespace Ring;
using namespace Ring::Ble;

BleApi::BleApi() :
    mInitialized(false)
    ,mServiceCount(0)
    ,mServiceTable(NULL)
    ,mDeviceClass("0x000430")
    ,mDeviceName("RingSetup-BT")
    ,mOnCharCb(NULL)
{
}

/********************************************************************************
 * Common API for WILINK and BCM43
 * resolved in the WILINK18/RingGattApi.cpp and
 *                 BCM43/RingGattApi.cpp
 *******************************************************************************/
int BleApi::Configure(DeviceConfig_t* aConfig)
{
    int ret_val = Error::UNDEFINED;

    while (aConfig != NULL && aConfig->tag != Ble::Config::EOL)
    {
        switch (aConfig->tag)
        {
        case Ble::Config::ServiceTable:
            if ((aConfig->params.NumberofParameters > 0) && (aConfig->params.Params[0].strParam != NULL))
            {
                mServiceCount = aConfig->params.Params[0].intParam;
                mServiceTable = (ServiceInfo_t *) aConfig->params.Params[0].strParam;
                BOT_NOTIFY_DEBUG("Service Table configure succeeded, count=%d", mServiceCount);
                ret_val = Error::NONE;
            }
            else
            {
                /* One or more of the necessary parameters is/are invalid.     */
                BOT_NOTIFY_ERROR("Usage: Ble::Config::ServiceTable: ServiceInfo_t *ptr, int numOfSvc");
                ret_val = Error::INVALID_PARAMETERS;
            }
            break;

        case Ble::Config::LocalDeviceName:
            ret_val = SetLocalDeviceName(&aConfig->params);
            break;

        case Ble::Config::LocalClassOfDevice:
            ret_val = SetLocalClassOfDevice(&aConfig->params);
            break;

        case Ble::Config::Discoverable:
            ret_val = SetDiscoverable(&aConfig->params);
            break;

        case Ble::Config::Connectable:
            ret_val = SetConnectable(&aConfig->params);
            break;

        case Ble::Config::Pairable:
            ret_val = SetPairable(&aConfig->params);
            break;

        case Ble::Config::RemoteDeviceLinkActive:
            ret_val = SetRemoteDeviceLinkActive(&aConfig->params);
            break;

        case Ble::Config::LocalDeviceAppearance:
            ret_val = SetLocalDeviceAppearance(&aConfig->params);
            break;

        case Ble::Config::AdvertisingInterval:
            ret_val = SetAdvertisingInterval(&aConfig->params);
            break;

        case Ble::Config::AndUpdateConnectionAndScanBLEParameters:
            ret_val = SetAndUpdateConnectionAndScanBLEParameters(&aConfig->params);
            break;

        case Ble::Config::AuthenticatedPayloadTimeout:
            ret_val = SetAuthenticatedPayloadTimeout(&aConfig->params);
            break;

        case Ble::Config::RegisterGATTCallback:
            ret_val = RegisterGATMEventCallback(&aConfig->params);
            break;

        case Ble::Config::RegisterService:
            ret_val = GATTRegisterService(&aConfig->params);
            break;

        case Ble::Config::RegisterAuthentication:
            ret_val = RegisterAuthentication(&aConfig->params);
            break;

        case Ble::Config::SetSimplePairing:
            ret_val = ChangeSimplePairingParameters(&aConfig->params);
            break;

        case Ble::Config::EnableBluetoothDebug:
            ret_val = EnableBluetoothDebug(&aConfig->params);
            break;

        default:
            BOT_NOTIFY_ERROR("Device Config: unknown tag = %d", aConfig->tag);
            ret_val = Error::INVALID_PARAMETERS;
            break;
        }
        if (ret_val != Error::NONE)
            break;
        aConfig++;
    }
    return ret_val;
}

int BleApi::RegisterCharacteristicAccessCallback(onCharacteristicAccessCallback aCb)
{
    if (!aCb)
        return Error::INVALID_PARAMETERS;
    if (!mInitialized)
        return Error::NOT_INITIALIZED;
    if (mOnCharCb)
        return Error::INVALID_STATE;

    mOnCharCb = aCb;
    return Error::NONE;
}

int BleApi::UnregisterCharacteristicAccessCallback(onCharacteristicAccessCallback aCb)
{
    if (!aCb)
        return Error::INVALID_PARAMETERS;
    if (!mInitialized)
        return Error::NOT_INITIALIZED;
    if (!mOnCharCb || mOnCharCb != aCb)
        return Error::INVALID_STATE;

    mOnCharCb = NULL;
    return Error::NONE;
}

