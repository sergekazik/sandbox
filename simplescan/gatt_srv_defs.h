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
#define RING_BLE_GATT_SERVER_TEST_DEFINE(__arg1, __arg2) static int __arg2(ParameterList_t *aParams) { return gGattSrvInst ? ((GattSrv*)gGattSrvInst)->__arg2(aParams) : BleApi::NOT_IMPLEMENTED_ERROR; }
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
#define RING_BLE_PAIRING_SERVICES_DEFINE(__arg0, __arg1, __arg2, __arg3, __arg4, __arg5)    \
                static CharacteristicInfo_t Srv0Attr_##__arg1 = { __arg2, GATM_SECURITY_PROPERTIES_NO_SECURITY, __arg5,  FALSE, __arg3, 0, (Byte_t *) NULL };
#else // DEVELOPMENT_TEST_NO_SECURITY = 0
#define RING_BLE_PAIRING_SERVICES_DEFINE(__arg0, __arg1, __arg2, __arg3, __arg4, __arg5)    \
                static CharacteristicInfo_t Srv0Attr_##__arg1 = { __arg2, __arg4, __arg5,  FALSE, __arg3, 0, (Byte_t *) NULL };
#endif // DEVELOPMENT_TEST_NO_SECURITY = 0
#endif

#ifdef RING_PAIRING_TABLE_ATTR_ENUM
#undef RING_PAIRING_TABLE_ATTR_ENUM
#define RING_BLE_GATT_SERVER_TEST_DEFINE(__arg1, __arg2)
#define RING_BLE_PAIRING_SERVICES_DEFINE(__arg0, __arg1, __arg2, __arg3, __arg4, __arg5)   e##__arg1,
#endif

#ifdef RING_PAIRING_TABLE_SERVICE_DEFINE_TABLE
#undef RING_PAIRING_TABLE_SERVICE_DEFINE_TABLE
#define RING_BLE_GATT_SERVER_TEST_DEFINE(__arg1, __arg2)
#define RING_BLE_PAIRING_SERVICES_DEFINE(__arg0, __arg1, __arg2, __arg3, __arg4, __arg5) { atCharacteristic, __arg0, (void *) &Srv0Attr_##__arg1, #__arg1 },
#endif

#ifndef RING_BLE_PAIRING_SERVICES_DEFINE
#define RING_BLE_PAIRING_SERVICES_DEFINE(__arg0, __arg1, __arg2, __arg3, __arg4, __arg5)
#endif

#ifndef MAKEUUID16
#define MAKEUUID16(_q, _w, _e, _r, _t, _y, _u, _i, _o, _p, _a, _s, _d, _f, _g, _h) \
        {0x##_h, 0x##_g, 0x##_f, 0x##_d, 0x##_s, 0x##_a, 0x##_p, 0x##_o, 0x##_i, 0x##_u, 0x##_y, 0x##_t, 0x##_r, 0x##_e, 0x##_w, 0x##_q}
#endif

RING_BLE_PAIRING_SERVICES_DEFINE( 1, PEER_PUBLIC_KEY_WRITE  ,GATM_CHARACTERISTIC_PROPERTIES_WRITE, 256, GATM_SECURITY_PROPERTIES_NO_SECURITY,   MAKEUUID16(97,60,D0,77,A2,34,46,86,9E,21,D0,87,33,3C,2D,15))
RING_BLE_PAIRING_SERVICES_DEFINE( 3, PUBLIC_PAYLOAD_READ    ,(GATM_CHARACTERISTIC_PROPERTIES_READ | GATM_CHARACTERISTIC_PROPERTIES_WRITE),  256, GATM_SECURITY_PROPERTIES_NO_SECURITY,   MAKEUUID16(97,60,D0,77,A2,34,46,86,9E,20,D0,87,33,3C,2D,13))
RING_BLE_PAIRING_SERVICES_DEFINE( 5, WIFI_LIST_READ         ,GATM_CHARACTERISTIC_PROPERTIES_READ,  64, GATM_SECURITY_PROPERTIES_ENCRYPTED,     MAKEUUID16(97,60,D0,77,A2,34,46,86,9E,20,D0,87,33,3C,2C,08))
RING_BLE_PAIRING_SERVICES_DEFINE( 7, WIFI_SSID_WRITE        ,GATM_CHARACTERISTIC_PROPERTIES_WRITE, 64, GATM_SECURITY_PROPERTIES_ENCRYPTED,     MAKEUUID16(97,60,D0,77,A2,34,46,86,9E,21,D0,87,33,3C,2C,04))
RING_BLE_PAIRING_SERVICES_DEFINE( 9, WIFI_PASS_WRITE        ,GATM_CHARACTERISTIC_PROPERTIES_WRITE, 64, GATM_SECURITY_PROPERTIES_ENCRYPTED,     MAKEUUID16(97,60,D0,77,A2,34,46,86,9E,21,D0,87,33,3C,2C,05))
RING_BLE_PAIRING_SERVICES_DEFINE(11, START_WIFI_PAIRING     ,GATM_CHARACTERISTIC_PROPERTIES_WRITE, 32, GATM_SECURITY_PROPERTIES_NO_SECURITY,   MAKEUUID16(97,60,D0,77,A2,34,46,86,9E,21,D0,87,33,3C,2C,09))
RING_BLE_PAIRING_SERVICES_DEFINE(13, START_ETH_PAIRING      ,GATM_CHARACTERISTIC_PROPERTIES_WRITE, 32, GATM_SECURITY_PROPERTIES_NO_SECURITY,   MAKEUUID16(97,60,D0,77,A2,34,46,86,9E,21,D0,87,33,3C,2E,01))
RING_BLE_PAIRING_SERVICES_DEFINE(15, SERIAL_READ            ,GATM_CHARACTERISTIC_PROPERTIES_READ,  64, GATM_SECURITY_PROPERTIES_ENCRYPTED,     MAKEUUID16(97,60,D0,77,A2,34,46,86,9E,20,D0,87,33,3C,2C,14))
RING_BLE_PAIRING_SERVICES_DEFINE(17, STATE_READ             ,(GATM_CHARACTERISTIC_PROPERTIES_NOTIFY | GATM_CHARACTERISTIC_PROPERTIES_READ),  16, GATM_SECURITY_PROPERTIES_ENCRYPTED,     MAKEUUID16(97,60,D0,77,A2,34,46,86,9E,20,D0,87,33,3C,2C,10))
RING_BLE_PAIRING_SERVICES_DEFINE(19, IFCONFIG_READ          ,GATM_CHARACTERISTIC_PROPERTIES_READ,  64, GATM_SECURITY_PROPERTIES_ENCRYPTED,     MAKEUUID16(97,60,D0,77,A2,34,46,86,9E,20,D0,87,33,3C,2C,11))
RING_BLE_PAIRING_SERVICES_DEFINE(21, REGCODE_READ           ,GATM_CHARACTERISTIC_PROPERTIES_READ,  64, GATM_SECURITY_PROPERTIES_ENCRYPTED,     MAKEUUID16(97,60,D0,77,A2,34,46,86,9E,20,D0,87,33,3C,2C,06))
RING_BLE_PAIRING_SERVICES_DEFINE(23, CONNECTIVITY_READ      ,GATM_CHARACTERISTIC_PROPERTIES_READ,  64, GATM_SECURITY_PROPERTIES_ENCRYPTED,     MAKEUUID16(97,60,D0,77,A2,34,46,86,9E,20,D0,87,33,3C,2C,12))
RING_BLE_PAIRING_SERVICES_DEFINE(25, LAT_WRITE              ,GATM_CHARACTERISTIC_PROPERTIES_WRITE, 32, GATM_SECURITY_PROPERTIES_ENCRYPTED,     MAKEUUID16(97,60,D0,77,A2,34,46,86,9E,20,D0,87,33,3C,2C,15))
RING_BLE_PAIRING_SERVICES_DEFINE(27, LNG_WRITE              ,GATM_CHARACTERISTIC_PROPERTIES_WRITE, 32, GATM_SECURITY_PROPERTIES_ENCRYPTED,     MAKEUUID16(97,60,D0,77,A2,34,46,86,9E,20,D0,87,33,3C,2C,16))
RING_BLE_PAIRING_SERVICES_DEFINE(29, SAVE_LOC_INFO          ,GATM_CHARACTERISTIC_PROPERTIES_WRITE, 32, GATM_SECURITY_PROPERTIES_ENCRYPTED,     MAKEUUID16(97,60,D0,77,A2,34,46,86,9E,20,D0,87,33,3C,2C,18))
RING_BLE_PAIRING_SERVICES_DEFINE(31, TZ_WRITE               ,GATM_CHARACTERISTIC_PROPERTIES_WRITE, 64, GATM_SECURITY_PROPERTIES_ENCRYPTED,     MAKEUUID16(97,60,D0,77,A2,34,46,86,9E,20,D0,87,33,3C,2C,17))
RING_BLE_PAIRING_SERVICES_DEFINE(33, WIFI_SSID_READ         ,GATM_CHARACTERISTIC_PROPERTIES_READ,  64, GATM_SECURITY_PROPERTIES_ENCRYPTED,     MAKEUUID16(97,60,D0,77,A2,34,46,86,9E,20,D0,87,33,3C,2C,04))
RING_BLE_PAIRING_SERVICES_DEFINE(35, ZIPCODE_WRITE          ,GATM_CHARACTERISTIC_PROPERTIES_WRITE, 32, GATM_SECURITY_PROPERTIES_ENCRYPTED,     MAKEUUID16(97,60,D0,77,A2,34,46,86,9E,21,D0,87,33,3C,2C,07))
RING_BLE_PAIRING_SERVICES_DEFINE(37, SECRET_CODE_WRITE      ,GATM_CHARACTERISTIC_PROPERTIES_WRITE, 64, GATM_SECURITY_PROPERTIES_ENCRYPTED,     MAKEUUID16(97,60,D0,77,A2,34,46,86,9E,21,D0,87,33,3C,2D,16))
RING_BLE_PAIRING_SERVICES_DEFINE(39, MAC_ADDRESS_READ       ,GATM_CHARACTERISTIC_PROPERTIES_READ,  64, GATM_SECURITY_PROPERTIES_ENCRYPTED,     MAKEUUID16(97,60,D0,77,A2,34,46,86,9E,21,D0,87,33,3C,2D,17))
RING_BLE_PAIRING_SERVICES_DEFINE(41, SETUP_ID_WRITE         ,GATM_CHARACTERISTIC_PROPERTIES_WRITE, 64, GATM_SECURITY_PROPERTIES_ENCRYPTED,     MAKEUUID16(97,60,D0,77,A2,34,46,86,9E,21,D0,87,33,3C,2D,18))


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
