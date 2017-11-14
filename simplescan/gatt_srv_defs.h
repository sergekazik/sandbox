#ifndef __GATT_SRV_DEFS_H__
#define __GATT_SRV_DEFS_H__

enum CharacteristicProperty_t
{
    PROPERTY_R__     = GATM_CHARACTERISTIC_PROPERTIES_READ,
    PROPERTY__W_     = GATM_CHARACTERISTIC_PROPERTIES_WRITE,
    PROPERTY_RW_     = GATM_CHARACTERISTIC_PROPERTIES_READ | GATM_CHARACTERISTIC_PROPERTIES_WRITE,
    PROPERTY___N     = GATM_CHARACTERISTIC_PROPERTIES_NOTIFY,
    PROPERTY_R_N     = GATM_CHARACTERISTIC_PROPERTIES_READ | GATM_CHARACTERISTIC_PROPERTIES_NOTIFY,
    PROPERTY__WN     = GATM_CHARACTERISTIC_PROPERTIES_WRITE | GATM_CHARACTERISTIC_PROPERTIES_NOTIFY,
    PROPERTY_RWN     = GATM_CHARACTERISTIC_PROPERTIES_READ | GATM_CHARACTERISTIC_PROPERTIES_WRITE | GATM_CHARACTERISTIC_PROPERTIES_NOTIFY,
};

#define MAKEUUID16(_q, _w, _e, _r, _t, _y, _u, _i, _o, _p, _a, _s, _d, _f, _g, _h) \
        {0x##_h, 0x##_g, 0x##_f, 0x##_d, 0x##_s, 0x##_a, 0x##_p, 0x##_o, 0x##_i, 0x##_u, 0x##_y, 0x##_t, 0x##_r, 0x##_e, 0x##_w, 0x##_q}
#define MAKERINGUUID(_attr_idx) MAKEUUID16(00,00,FA,CE,00,00,10,00,80,00,00,80,5F,9B,35,_attr_idx)
#define MAKEPAYLOAD(_pld) (unsigned int) strlen(_pld), (Byte_t *) _pld

#define BLE_PACKET_LEN  18
#endif // __GATT_SRV_DEFS_H__

#ifdef RING_BLE_GATT_SERVER_TEST_DEFINE
#undef RING_BLE_GATT_SERVER_TEST_DEFINE
#endif

#ifdef RING_BLE_PAIRING_SERVICES_DEFINE
#undef RING_BLE_PAIRING_SERVICES_DEFINE
#endif

#ifdef RING_BLE_DEF_ADD_COMMAND
#undef RING_BLE_DEF_ADD_COMMAND
#define RING_BLE_GATT_SERVER_TEST_DEFINE(__arg1, __arg2)  AddCommand(UpperCaseUpdate(idx), __arg2);
#endif

#ifdef RING_BLE_DEF_CPP_WRAPPER
#undef RING_BLE_DEF_CPP_WRAPPER
#define RING_BLE_GATT_SERVER_TEST_DEFINE(__arg1, __arg2) static int __arg2(ParameterList_t *aParams) { return gBleApi ? ((GattSrv*)gBleApi)->__arg2(aParams) : Ble::Error::NOT_IMPLEMENTED; }
#endif

#ifdef RING_BLE_DEF_CMD
#undef RING_BLE_DEF_CMD
#define RING_BLE_GATT_SERVER_TEST_DEFINE(__arg1, __arg2) __arg1,
#endif

#ifdef RING_BLE_PRINT_HELP
#undef RING_BLE_PRINT_HELP
#define RING_BLE_GATT_SERVER_TEST_DEFINE(__arg1, __arg2) printf("*                  %-2d) %-43s*\r\n", idx++, __arg1);
#endif

#ifdef RING_PAIRING_TABLE_SERVICE_DECL
#undef RING_PAIRING_TABLE_SERVICE_DECL
#define RING_BLE_GATT_SERVER_TEST_DEFINE(__arg1, __arg2)

#if DEVELOPMENT_TEST_NO_SECURITY
#define RING_BLE_PAIRING_SERVICES_DEFINE(__arg0, __arg1, __arg2, __arg3, __arg4, __arg5, __argpld)    \
                static CharacteristicInfo_t Srv0Attr_##__arg1 = { __arg2, GATM_SECURITY_PROPERTIES_NO_SECURITY, __arg5,  FALSE, __arg3*BLE_PACKET_LEN, __argpld };
#else // DEVELOPMENT_TEST_NO_SECURITY = 0
#define RING_BLE_PAIRING_SERVICES_DEFINE(__arg0, __arg1, __arg2, __arg3, __arg4, __arg5, __argpld)    \
                static CharacteristicInfo_t Srv0Attr_##__arg1 = { __arg2, __arg4, __arg5,  FALSE, __arg3*BLE_PACKET_LEN,  __argpld };
#endif // DEVELOPMENT_TEST_NO_SECURITY = 0
#endif

#ifdef RING_PAIRING_TABLE_ATTR_ENUM
#undef RING_PAIRING_TABLE_ATTR_ENUM
#define RING_BLE_GATT_SERVER_TEST_DEFINE(__arg1, __arg2)
#define RING_BLE_PAIRING_SERVICES_DEFINE(__arg0, __arg1, __arg2, __arg3, __arg4, __arg5, __argpld)   e##__arg1,
#endif

#ifdef RING_PAIRING_TABLE_SERVICE_DEFINE_TABLE
#undef RING_PAIRING_TABLE_SERVICE_DEFINE_TABLE
#define RING_BLE_GATT_SERVER_TEST_DEFINE(__arg1, __arg2)
#define RING_BLE_PAIRING_SERVICES_DEFINE(__arg0, __arg1, __arg2, __arg3, __arg4, __arg5, __argpld) { atCharacteristic, __arg0, (void *) &Srv0Attr_##__arg1, #__arg1 },
#endif

#ifndef RING_BLE_PAIRING_SERVICES_DEFINE
#define RING_BLE_PAIRING_SERVICES_DEFINE(__arg0, __arg1, __arg2, __arg3, __arg4, __arg5, __argpld)
#endif

RING_BLE_PAIRING_SERVICES_DEFINE( 1, SET_PUBLIC_KEY    , PROPERTY__W_, 28, GATM_SECURITY_PROPERTIES_NO_SECURITY, MAKERINGUUID(01), MAKEPAYLOAD(""))
RING_BLE_PAIRING_SERVICES_DEFINE( 3, GET_PUBLIC_PAYLOAD, PROPERTY_RW_, 28, GATM_SECURITY_PROPERTIES_NO_SECURITY, MAKERINGUUID(02), MAKEPAYLOAD(""))
RING_BLE_PAIRING_SERVICES_DEFINE( 5, GET_NETWORKS      , PROPERTY_R__, 28, GATM_SECURITY_PROPERTIES_ENCRYPTED,   MAKERINGUUID(03), MAKEPAYLOAD(""))
RING_BLE_PAIRING_SERVICES_DEFINE( 7, SET_NETWORK       , PROPERTY__W_,  8, GATM_SECURITY_PROPERTIES_ENCRYPTED,   MAKERINGUUID(04), MAKEPAYLOAD(""))
RING_BLE_PAIRING_SERVICES_DEFINE( 9, GET_PAIRING_STATE , PROPERTY_R_N,  1, GATM_SECURITY_PROPERTIES_ENCRYPTED,   MAKERINGUUID(07), MAKEPAYLOAD("IDLE"))
RING_BLE_PAIRING_SERVICES_DEFINE(11, SET_LANGUAGE      , PROPERTY__W_,  1, GATM_SECURITY_PROPERTIES_ENCRYPTED,   MAKERINGUUID(08), MAKEPAYLOAD(""))
RING_BLE_PAIRING_SERVICES_DEFINE(13, SET_ZIPCODE       , PROPERTY__W_,  1, GATM_SECURITY_PROPERTIES_ENCRYPTED,   MAKERINGUUID(09), MAKEPAYLOAD(""))
RING_BLE_PAIRING_SERVICES_DEFINE(15, GET_WIFI_STATUS   , PROPERTY_R__,  1, GATM_SECURITY_PROPERTIES_ENCRYPTED,   MAKERINGUUID(0A), MAKEPAYLOAD("DISCONNECTED"))
RING_BLE_PAIRING_SERVICES_DEFINE(17, GET_SSID_WIFI     , PROPERTY_R__,  2, GATM_SECURITY_PROPERTIES_ENCRYPTED,   MAKERINGUUID(0B), MAKEPAYLOAD("Doorbells"))
RING_BLE_PAIRING_SERVICES_DEFINE(19, GET_SERIAL_NUMBER , PROPERTY_R__,  2, GATM_SECURITY_PROPERTIES_ENCRYPTED,   MAKERINGUUID(0C), MAKEPAYLOAD(""))
RING_BLE_PAIRING_SERVICES_DEFINE(21, GET_MAC_ADDRESS   , PROPERTY_R__,  1, GATM_SECURITY_PROPERTIES_ENCRYPTED,   MAKERINGUUID(0D), MAKEPAYLOAD("38:D2:69:F2:7E:01"))


RING_BLE_GATT_SERVER_TEST_DEFINE("Initialize"                              ,Initialize)
RING_BLE_GATT_SERVER_TEST_DEFINE("Shutdown"                                ,Shutdown)
RING_BLE_GATT_SERVER_TEST_DEFINE("QueryDebugZoneMask"                      ,QueryLocalRemoteDebugZoneMask)
RING_BLE_GATT_SERVER_TEST_DEFINE("SetDebugZoneMask"                        ,SetLocalRemoteDebugZoneMask)
RING_BLE_GATT_SERVER_TEST_DEFINE("SetDebugZoneMaskPID"                     ,SetDebugZoneMaskPID)
RING_BLE_GATT_SERVER_TEST_DEFINE("ShutdownService"                         ,ShutdownService)
RING_BLE_GATT_SERVER_TEST_DEFINE("RegisterEventCallback"                   ,RegisterEventCallback)
RING_BLE_GATT_SERVER_TEST_DEFINE("UnRegisterEventCallback"                 ,UnRegisterEventCallback)
RING_BLE_GATT_SERVER_TEST_DEFINE("QueryDevicePower"                        ,QueryDevicePower)
RING_BLE_GATT_SERVER_TEST_DEFINE("SetDevicePower"                          ,SetDevicePower)
RING_BLE_GATT_SERVER_TEST_DEFINE("QueryLocalDeviceProperties"              ,QueryLocalDeviceProperties)
RING_BLE_GATT_SERVER_TEST_DEFINE("SetLocalDeviceName"                      ,SetLocalDeviceName)
RING_BLE_GATT_SERVER_TEST_DEFINE("SetLocalClassOfDevice"                   ,SetLocalClassOfDevice)
RING_BLE_GATT_SERVER_TEST_DEFINE("SetDiscoverable"                         ,SetDiscoverable)
RING_BLE_GATT_SERVER_TEST_DEFINE("SetConnectable"                          ,SetConnectable)
RING_BLE_GATT_SERVER_TEST_DEFINE("SetPairable"                             ,SetPairable)
RING_BLE_GATT_SERVER_TEST_DEFINE("StartDeviceDiscovery"                    ,StartDeviceDiscovery)
RING_BLE_GATT_SERVER_TEST_DEFINE("StopDeviceDiscovery"                     ,StopDeviceDiscovery)
RING_BLE_GATT_SERVER_TEST_DEFINE("QueryRemoteDeviceList"                   ,QueryRemoteDeviceList)
RING_BLE_GATT_SERVER_TEST_DEFINE("QueryRemoteDeviceProperties"             ,QueryRemoteDeviceProperties)
RING_BLE_GATT_SERVER_TEST_DEFINE("AddRemoteDevice"                         ,AddRemoteDevice)
RING_BLE_GATT_SERVER_TEST_DEFINE("DeleteRemoteDevice"                      ,DeleteRemoteDevice)
RING_BLE_GATT_SERVER_TEST_DEFINE("UpdateRemoteDeviceAppData"               ,UpdateRemoteDeviceApplicationData)
RING_BLE_GATT_SERVER_TEST_DEFINE("DeleteRemoteDevices"                     ,DeleteRemoteDevices)
RING_BLE_GATT_SERVER_TEST_DEFINE("PairWithRemoteDevice"                    ,PairWithRemoteDevice)
RING_BLE_GATT_SERVER_TEST_DEFINE("CancelPairWithRemoteDevice"              ,CancelPairWithRemoteDevice)
RING_BLE_GATT_SERVER_TEST_DEFINE("UnPairRemoteDevice"                      ,UnPairRemoteDevice)
RING_BLE_GATT_SERVER_TEST_DEFINE("QueryRemoteDeviceServices"               ,QueryRemoteDeviceServices)
RING_BLE_GATT_SERVER_TEST_DEFINE("QueryRemoteDeviceServiceSupported"       ,QueryRemoteDeviceServiceSupported)
RING_BLE_GATT_SERVER_TEST_DEFINE("QueryRemoteDevicesForService"            ,QueryRemoteDevicesForService)
RING_BLE_GATT_SERVER_TEST_DEFINE("QueryRemoteDeviceServiceClasses"         ,QueryRemoteDeviceServiceClasses)
RING_BLE_GATT_SERVER_TEST_DEFINE("AuthenticateRemoteDevice"                ,AuthenticateRemoteDevice)
RING_BLE_GATT_SERVER_TEST_DEFINE("EncryptRemoteDevice"                     ,EncryptRemoteDevice)
RING_BLE_GATT_SERVER_TEST_DEFINE("ConnectWithRemoteDevice"                 ,ConnectWithRemoteDevice)
RING_BLE_GATT_SERVER_TEST_DEFINE("DisconnectRemoteDevice"                  ,DisconnectRemoteDevice)
RING_BLE_GATT_SERVER_TEST_DEFINE("SetRemoteDeviceLinkActive"               ,SetRemoteDeviceLinkActive)
RING_BLE_GATT_SERVER_TEST_DEFINE("CreateSDPRecord"                         ,CreateSDPRecord)
RING_BLE_GATT_SERVER_TEST_DEFINE("DeleteSDPRecord"                         ,DeleteSDPRecord)
RING_BLE_GATT_SERVER_TEST_DEFINE("AddSDPAttribute"                         ,AddSDPAttribute)
RING_BLE_GATT_SERVER_TEST_DEFINE("DeleteSDPAttribute"                      ,DeleteSDPAttribute)
RING_BLE_GATT_SERVER_TEST_DEFINE("EnableBluetoothDebug"                    ,EnableBluetoothDebug)
RING_BLE_GATT_SERVER_TEST_DEFINE("RegisterAuthentication"                  ,RegisterAuthentication)
RING_BLE_GATT_SERVER_TEST_DEFINE("UnRegisterAuthentication"                ,UnRegisterAuthentication)
RING_BLE_GATT_SERVER_TEST_DEFINE("PINCodeResponse"                         ,PINCodeResponse)
RING_BLE_GATT_SERVER_TEST_DEFINE("PassKeyResponse"                         ,PassKeyResponse)
RING_BLE_GATT_SERVER_TEST_DEFINE("UserConfirmationResponse"                ,UserConfirmationResponse)
RING_BLE_GATT_SERVER_TEST_DEFINE("ChangeSimplePairingParameters"           ,ChangeSimplePairingParameters)
RING_BLE_GATT_SERVER_TEST_DEFINE("RegisterGATTCallback"                    ,RegisterGATMEventCallback)
RING_BLE_GATT_SERVER_TEST_DEFINE("UnRegisterGATTCallback"                  ,UnRegisterGATMEventCallback)
RING_BLE_GATT_SERVER_TEST_DEFINE("QueryGATTConnections"                    ,GATTQueryConnectedDevices)
RING_BLE_GATT_SERVER_TEST_DEFINE("SetLocalDeviceAppearance"                ,SetLocalDeviceAppearance)
RING_BLE_GATT_SERVER_TEST_DEFINE("StartAdvertising"                        ,StartAdvertising)
RING_BLE_GATT_SERVER_TEST_DEFINE("StopAdvertising"                         ,StopAdvertising)
RING_BLE_GATT_SERVER_TEST_DEFINE("RegisterService"                         ,GATTRegisterService)
RING_BLE_GATT_SERVER_TEST_DEFINE("UnRegisterService"                       ,GATTUnRegisterService)
RING_BLE_GATT_SERVER_TEST_DEFINE("IndicateCharacteristic"                  ,GATTIndicateCharacteristic)
RING_BLE_GATT_SERVER_TEST_DEFINE("NotifyCharacteristic"                    ,GATTNotifyCharacteristic)
RING_BLE_GATT_SERVER_TEST_DEFINE("ListCharacteristics"                     ,ListCharacteristics)
RING_BLE_GATT_SERVER_TEST_DEFINE("ListDescriptors"                         ,ListDescriptors)
RING_BLE_GATT_SERVER_TEST_DEFINE("QueryPublishedServices"                  ,GATTQueryPublishedServices)
RING_BLE_GATT_SERVER_TEST_DEFINE("SetAdvertisingInterval"                  ,SetAdvertisingInterval)
RING_BLE_GATT_SERVER_TEST_DEFINE("SetAndUpdateConnectionAndScanBLEParameters",SetAndUpdateConnectionAndScanBLEParameters)
RING_BLE_GATT_SERVER_TEST_DEFINE("SetAuthenticatedPayloadTimeout"          ,SetAuthenticatedPayloadTimeout)
RING_BLE_GATT_SERVER_TEST_DEFINE("QueryAuthenticatedPayloadTimeout"        ,QueryAuthenticatedPayloadTimeout)
RING_BLE_GATT_SERVER_TEST_DEFINE("EnableSCOnly"                            ,EnableSCOnly)
RING_BLE_GATT_SERVER_TEST_DEFINE("RegenerateP256LocalKeys"                 ,RegenerateP256LocalKeys)
RING_BLE_GATT_SERVER_TEST_DEFINE("OOBGenerateParameters"                   ,OOBGenerateParameters)
RING_BLE_GATT_SERVER_TEST_DEFINE("ChangeLEPairingParameters"               ,ChangeLEPairingParameters)
