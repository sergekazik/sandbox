#ifdef RING_BLE_GATT_SERVER_TEST_DEFINE
#undef RING_BLE_GATT_SERVER_TEST_DEFINE
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
