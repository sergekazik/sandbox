static int HelpParam(ParameterList_t *TempParam __attribute__ ((unused)))
{
    /* Note the order they are listed here *MUST* match the order in     */
    /* which then are added to the Command Table.                        */
    printf("******************************************************************\r\n");
    printf("* 1) Initialize [0/1 - Register for Events].\r\n");
    printf("* 2) Shutdown\r\n");
    printf("* 3) QueryDebugZoneMask [0/1 - Local/Service] [Page Number - optional, default 0].\r\n");
    printf("* 4) SetDebugZoneMask [0/1 - Local/Service] [Debug Zone Mask].\r\n");
    printf("* 5) SetDebugZoneMaskPID [Process ID] [Debug Zone Mask].\r\n");
    printf("* 6) ShutdownService\r\n");
    printf("* 7) RegisterEventCallback, \r\n");
    printf("* 8) UnRegisterEventCallback, \r\n");
    printf("* 9) QueryDevicePower \r\n");
    printf("* 10)SetDevicePower [0/1 - Power Off/Power On].\r\n");
    printf("* 11)QueryLocalDeviceProperties \r\n");
    printf("* 12)SetLocalDeviceName \r\n");
    printf("* 13)SetLocalClassOfDevice [Class of Device].\r\n");
    printf("* 14)SetDiscoverable [Enable/Disable] [Timeout (Enable only)].\r\n");
    printf("* 15)SetConnectable [Enable/Disable] [Timeout (Enable only)].\r\n");
    printf("* 16)SetPairable [Enable/Disable] [Timeout (Enable only)].\r\n");
    printf("* 17)StartDeviceDiscovery [Type (1 = LE, 0 = BR/EDR)] [Duration].\r\n");
    printf("* 18)StopDeviceDiscovery [Type (1 = LE, 0 = BR/EDR)].\r\n");
    printf("* 19)QueryRemoteDeviceList [Number of Devices] [Filter (Optional)] [COD Filter (Optional)].\r\n");
    printf("* 20)QueryRemoteDeviceProperties [BD_ADDR] [Type (1 = LE, 0 = BR/EDR)] [Force Update].\r\n");
    printf("* 21)AddRemoteDevice [BD_ADDR] [[COD (Optional)] [Friendly Name (Optional)] [Application Info (Optional)]].\r\n");
    printf("* 22)DeleteRemoteDevice [BD_ADDR].\r\n");
    printf("* 23)UpdateRemoteDeviceAppData [BD_ADDR] [Data Valid] [Friendly Name] [Application Info].\r\n");
    printf("* 24)DeleteRemoteDevices [Device Delete Filter].\r\n");
    printf("* 25)PairWithRemoteDevice [BD_ADDR] [Type (1 = LE, 0 = BR/EDR)] [Pair Flags (optional)].\r\n");
    printf("* 26)CancelPairWithRemoteDevice [BD_ADDR].\r\n");
    printf("* 27)UnPairRemoteDevice [BD_ADDR] [Type (1 = LE, 0 = BR/EDR)] .\r\n");
    printf("* 28)QueryRemoteDeviceServices [BD_ADDR] [Type (1 = LE, 0 = BR/EDR)] [Force Update] [Bytes to Query (specified if Force is 0)].\r\n");
    printf("* 29)QueryRemoteDeviceServiceSupported [BD_ADDR] [Service UUID].\r\n");
    printf("* 30)QueryRemoteDevicesForService [Service UUID] [Number of Devices].\r\n");
    printf("* 31)QueryRemoteDeviceServiceClasses [BD_ADDR] [Number of Service Classes].\r\n");
    printf("* 32)AuthenticateRemoteDevice [BD_ADDR] [Type (LE = 1, BR/EDR = 0)].\r\n");
    printf("* 33)EncryptRemoteDevice [BD_ADDR] [Type (LE = 1, BR/EDR = 0)].\r\n");
    printf("* 34)ConnectWithRemoteDevice [BD_ADDR] [Connect LE (1 = LE, 0 = BR/EDR)] [ConnectFlags (Optional)].\r\n");
    printf("* 35)DisconnectRemoteDevice [BD_ADDR] [LE Device (1= LE, 0 = BR/EDR)] [Force Flag (Optional)].\r\n");
    printf("* 36)SetRemoteDeviceLinkActive [BD_ADDR].\r\n");
    printf("* 37)CreateSDPRecord\r\n");
    printf("* 38)DeleteSDPRecord [Service Record Handle].\r\n");
    printf("* 39)AddSDPAttribute [Service Record Handle] [Attribute ID] [Attribute Value (optional)].\r\n");
    printf("* 40)DeleteSDPAttribute [Service Record Handle] [Attribute ID].\r\n");
    printf("* 41)EnableBluetoothDebug [Enable (0/1)] [Type (1 - ASCII File, 2 - Terminal, 3 - FTS File)] [Debug Flags] [Debug Parameter String (no spaces)].\r\n");
    printf("* 42)RegisterAuthentication \r\n");
    printf("* 43)UnRegisterAuthentication \r\n");
    printf("* 44)PINCodeResponse [PIN Code].\r\n");
    printf("* 45)PassKeyResponse [Numeric Passkey (0 - 999999)].\r\n");
    printf("* 46)UserConfirmationResponse [Confirmation (0 = No, 1 = Yes)].\r\n");
    printf("* 47)ChangeSimplePairingParameters [I/O Capability (0 = Display Only, 1 = Display Yes/No, 2 = Keyboard Only, 3 = No Input/Output)] [MITM Requirement (0 = No, 1 = Yes)].\r\n");
    printf("* 48)RegisterGATTCallback \r\n");
    printf("* 49)UnRegisterGATTCallback \r\n");
    printf("* 50)QueryGATTConnections \r\n");
    printf("* 51)SetLocalDeviceAppearance \r\n");
    printf("* 52)StartAdvertising [Flags] [Duration] [BD ADDR].\r\n");
    printf("* 53)StopAdvertising [Flags].\r\n");
    printf("* 54)RegisterService [Service Index].\r\n");
    printf("* 55)UnRegisterService [Service Index].\r\n");
    printf("* 56)IndicateCharacteristic [Service Index ] [Attribute Offset] [BD_ADDR].\r\n");
    printf("* 57)NotifyCharacteristic [Service Index ] [Attribute Offset] [BD_ADDR].\r\n");
    printf("* 58)ListCharacteristics\r\n");
    printf("* 59)ListDescriptors\r\n");
    printf("* 60)QueryPublishedServices \r\n");
    printf("* 61)SetAdvertisingInterval [Advertising Interval Min] [Advertising Interval Max] (Range: 20..10240 in ms).\r\n");
    printf("* 62)SetAndUpdateConnectionAndScanBLEParameters\r\n");
    printf("* 63)SetAuthenticatedPayloadTimeout [BD_ADDR] [Authenticated Payload Timout (In ms)].\r\n");
    printf("* 64)QueryAuthenticatedPayloadTimeout [BD_ADDR].\r\n");
    printf("* 65)EnableSCOnly [mode (0 = mSC Only mode is off, 1 = mSC Only mode is on].\r\n");
    printf("* 66)RegenerateP256LocalKeys\r\n");
    printf("* 67)OOBGenerateParameters\r\n");
    printf("* 68)ChangeLEPairingParameters:\r\n");
    printf("* HH, MM, Help, Quit. \r\n");
    printf("*****************************************************************\r\n");

    return 0;
}

#ifdef USE_ADDITIONAL_TEST_DATA
/*********************************************************************
 * Services Declaration.
 *********************************************************************/

static CharacteristicInfo_t Srv1Attr1=
{
    /* Characteristic Properties.                                        */
    GATM_CHARACTERISTIC_PROPERTIES_READ | GATM_CHARACTERISTIC_PROPERTIES_WRITE | GATM_CHARACTERISTIC_PROPERTIES_INDICATE,

    /* Characteristic Security Properties.                               */
    GATM_SECURITY_PROPERTIES_AUTHENTICATED_ENCRYPTION_WRITE,

    /* Characteristic UUID.                                              */
    { 0x6A, 0xFE, 0x4D, 0x41, 0x6C, 0x52, 0x10, 0x14, 0x8B, 0x7A, 0x95, 0x02, 0x01, 0xB8, 0x65, 0xBC  },

    /* Value is allocated dynamically.                                   */
    FALSE,

    /* Maximum value length.                                             */
    117,

    /* Current value length.                                             */
    117,

    /* Current value.                                                    */
    (Byte_t *)"VUHhfCQDLvB~!%^&*()-_+={}[]|:;.<>?/#uoPXonUEjisFQjbWGlXmAoQsSSqJwexZsugfFtamHcTun~!%^&*()-_+={}[]|:;.<>?/#AfjLKCeuMia"
};

static CharacteristicInfo_t Srv1Attr2=
{
    /* Characteristic Properties.                                        */
    GATM_CHARACTERISTIC_PROPERTIES_READ | GATM_CHARACTERISTIC_PROPERTIES_WRITE | GATM_CHARACTERISTIC_PROPERTIES_NOTIFY | GATM_CHARACTERISTIC_PROPERTIES_AUTHENTICATED_SIGNED_WRITES,

    /* Characteristic Security Properties.                               */
    GATM_SECURITY_PROPERTIES_UNAUTHENTICATED_ENCRYPTION_READ | GATM_SECURITY_PROPERTIES_UNAUTHENTICATED_ENCRYPTION_WRITE | GATM_SECURITY_PROPERTIES_AUTHENTICATED_SIGNED_WRITES,

    /* Characteristic UUID.                                              */
    { 0x6A, 0xFE, 0x51, 0x96, 0x6C, 0x52, 0x10, 0x14, 0x8B, 0x7A, 0x95, 0x02, 0x01, 0xB8, 0x65, 0xBC  },

    /* Value is allocated dynamically.                                   */
    FALSE,

    /* Maximum value length.                                             */
    45,

    /* Current value length.                                             */
    0,

    /* Current value.                                                    */
    (Byte_t *)NULL
};

static CharacteristicInfo_t Srv1Attr3=
{
    /* Characteristic Properties.                                        */
    GATM_CHARACTERISTIC_PROPERTIES_WRITE,

    /* Characteristic Security Properties.                               */
    GATM_SECURITY_PROPERTIES_NO_SECURITY,

    /* Characteristic UUID.                                              */
    { 0x6A, 0xFE, 0x52, 0x8B, 0x6C, 0x52, 0x10, 0x14, 0x8B, 0x7A, 0x95, 0x02, 0x01, 0xB8, 0x65, 0xBC  },

    /* Value is allocated dynamically.                                   */
    FALSE,

    /* Maximum value length.                                             */
    50,

    /* Current value length.                                             */
    0,

    /* Current value.                                                    */
    (Byte_t *)NULL
};


/* The following array is the array containing attributes that are   */
/* registered by this service.                                       */
static AttributeInfo_t SrvTable1[] =
{
    { atCharacteristic, 1, (void *)&Srv1Attr1 },
    { atCharacteristic, 3, (void *)&Srv1Attr2 },
    { atCharacteristic, 5, (void *)&Srv1Attr3 }
};

/*********************************************************************/
/* End of Service Declaration.                                       */
/*********************************************************************/

/*********************************************************************/
/* Service Declaration.                                              */
/*********************************************************************/

static CharacteristicInfo_t Srv2Attr2=
{
    /* Characteristic Properties.                                        */
    GATM_CHARACTERISTIC_PROPERTIES_READ | GATM_CHARACTERISTIC_PROPERTIES_WRITE_WO_RESP | GATM_CHARACTERISTIC_PROPERTIES_WRITE,

    /* Characteristic Security Properties.                               */
    GATM_SECURITY_PROPERTIES_AUTHENTICATED_ENCRYPTION_READ | GATM_SECURITY_PROPERTIES_UNAUTHENTICATED_ENCRYPTION_WRITE,

    /* Characteristic UUID.                                              */
    { 0x6A, 0xFE, 0x55, 0xA3, 0x6C, 0x52, 0x10, 0x14, 0x8B, 0x7A, 0x95, 0x02, 0x01, 0xB8, 0x65, 0xBC  },

    /* Value is allocated dynamically.                                   */
    FALSE,

    /* Maximum value length.                                             */
    464,

    /* Current value length.                                             */
    0,

    /* Current value.                                                    */
    (Byte_t *)NULL
};

static CharacteristicInfo_t Srv2Attr3=
{
    /* Characteristic Properties.                                        */
    GATM_CHARACTERISTIC_PROPERTIES_READ | GATM_CHARACTERISTIC_PROPERTIES_WRITE_WO_RESP,

    /* Characteristic Security Properties.                               */
    GATM_SECURITY_PROPERTIES_NO_SECURITY,

    /* Characteristic UUID.                                              */
    { 0x6A, 0xFE, 0x56, 0x89, 0x6C, 0x52, 0x10, 0x14, 0x8B, 0x7A, 0x95, 0x02, 0x01, 0xB8, 0x65, 0xBC  },

    /* Value is allocated dynamically.                                   */
    FALSE,

    /* Maximum value length.                                             */
    36,

    /* Current value length.                                             */
    36,

    /* Current value.                                                    */
    (Byte_t *)"rlgzzRztTCqnCBBrsm~!%^&*()-_+={}[]|"
};

static CharacteristicInfo_t Srv2Attr4=
{
    /* Characteristic Properties.                                        */
    GATM_CHARACTERISTIC_PROPERTIES_READ | GATM_CHARACTERISTIC_PROPERTIES_WRITE,

    /* Characteristic Security Properties.                               */
    GATM_SECURITY_PROPERTIES_UNAUTHENTICATED_ENCRYPTION_READ | GATM_SECURITY_PROPERTIES_UNAUTHENTICATED_ENCRYPTION_WRITE,

    /* Characteristic UUID.                                              */
    { 0x6A, 0xFE, 0x5C, 0x40, 0x6C, 0x52, 0x10, 0x14, 0x8B, 0x7A, 0x95, 0x02, 0x01, 0xB8, 0x65, 0xBC  },

    /* Value is allocated dynamically.                                   */
    FALSE,

    /* Maximum value length.                                             */
    62,

    /* Current value length.                                             */
    0,

    /* Current value.                                                    */
    (Byte_t *)NULL
};

static DescriptorInfo_t Srv2Attr5=
{
    /* Descriptor Properties.                                            */
    GATM_DESCRIPTOR_PROPERTIES_READ,

    /* Descriptor Security Properties.                                   */
    GATM_SECURITY_PROPERTIES_UNAUTHENTICATED_ENCRYPTION_READ,

    /* Descriptor UUID.                                                  */
    { 0x6A, 0xFE, 0x5D, 0x2A, 0x6C, 0x52, 0x10, 0x14, 0x8B, 0x7A, 0x95, 0x02, 0x01, 0xB8, 0x65, 0xBC  },

    /* Value is allocated dynamically.                                   */
    FALSE,

    /* Maximum value length.                                             */
    264,

    /* Current value length.                                             */
    10,

    /* Current value.                                                    */
    (Byte_t *)"HelloWorld"
};

static CharacteristicInfo_t Srv2Attr6=
{
    /* Characteristic Properties.                                        */
    GATM_CHARACTERISTIC_PROPERTIES_READ | GATM_CHARACTERISTIC_PROPERTIES_WRITE,

    /* Characteristic Security Properties.                               */
    GATM_SECURITY_PROPERTIES_UNAUTHENTICATED_ENCRYPTION_READ | GATM_SECURITY_PROPERTIES_UNAUTHENTICATED_ENCRYPTION_WRITE,

    /* Characteristic UUID.                                              */
    { 0x6A, 0xFE, 0x5E, 0x32, 0x6C, 0x52, 0x10, 0x14, 0x8B, 0x7A, 0x95, 0x02, 0x01, 0xB8, 0x65, 0xBC  },

    /* Value is allocated dynamically.                                   */
    FALSE,

    /* Maximum value length.                                             */
    321,

    /* Current value length.                                             */
    321,

    /* Current value.                                                    */
    (Byte_t *)"qioFsYUBoESuvABqUCDyOXWmevWjxJsaYYMvpnoY~!%^&*()-_+={}[]|:;.<>?/#ShTtUGfemhchQnTAuZFTUwBSSSNhjDcBlASzZaMDsqWkCVAWCYIiqvIWhUcWUHwPIqKTLiYzQFNugrkvZWwqjRexoJeAiwEZrqjxHIukLiAWhUUoV~!%^&*()-_+={}[]|:;.<>?/#tgvnqYVCfywRZgBTARae~!%^&*()-_+={}[]|:;.<>?/#vmBGQSf~!%^&*()-_+={}[]|:;.<>?/#iJMviBnTpRvLjOTVwKUoAHbOdMTWRlvNXSrRNOyhAaFXRXBGxzOyMigBvOTHCNsuSgxscaBucAhmtuRocF~!%^&*()-_+={}[]|:;.<>?/#AAJbaYbMvoMzYspPdXMjPBGxv~!%^&*()-_+={}[]|:;.<>?/#fTTpc~!%^&*()-_+={}[]|:;.<>?/#mHfzsCpjnhdubiwGAhidAV"
};

static CharacteristicInfo_t Srv2Attr7=
{
    /* Characteristic Properties.                                        */
    GATM_CHARACTERISTIC_PROPERTIES_READ | GATM_CHARACTERISTIC_PROPERTIES_WRITE_WO_RESP,

    /* Characteristic Security Properties.                               */
    GATM_SECURITY_PROPERTIES_UNAUTHENTICATED_ENCRYPTION_READ | GATM_SECURITY_PROPERTIES_AUTHENTICATED_ENCRYPTION_WRITE,

    /* Characteristic UUID.                                              */
    { 0x6A, 0xFE, 0x63, 0x01, 0x6C, 0x52, 0x10, 0x14, 0x8B, 0x7A, 0x95, 0x02, 0x01, 0xB8, 0x65, 0xBC  },

    /* Value is allocated dynamically.                                   */
    FALSE,

    /* Maximum value length.                                             */
    186,

    /* Current value length.                                             */
    186,

    /* Current value.                                                    */
    (Byte_t *)"kGmFzGcmIWrFLGfeYBwzWRwXCKHlXYdnVPugxwJ~!%^&*()-_+={}[]|:;.<>?/#ZaimuLdaHNDALPEzzCtEaHSZyKHfuSAIa~!%^&*()-_+={}[]|:;.<>?/#nKxCfeXahNzeegKGZBErbDHiYciGOqy~!%^&*()-_+={}[]|:;.<>?/#~!%^&*()-_+={}[]|:;.<>?/#KomahJaknrBClz~!%^&*()-_+={}[]|:;.<>?/#cKQSngsQCDjPABccaYNstELvfkWzxMbIrZtzcxvoTxtSAlDfoysREDZLzW~!%^&*()-_+={}[]|:;.<>?/#CUtI~!%^&*()-_+={}[]|:;.<>?/#"
};

static CharacteristicInfo_t Srv2Attr8=
{
    /* Characteristic Properties.                                        */
    GATM_CHARACTERISTIC_PROPERTIES_WRITE,

    /* Characteristic Security Properties.                               */
    GATM_SECURITY_PROPERTIES_UNAUTHENTICATED_ENCRYPTION_WRITE,

    /* Characteristic UUID.                                              */
    { 0x6A, 0xFE, 0x66, 0xCA, 0x6C, 0x52, 0x10, 0x14, 0x8B, 0x7A, 0x95, 0x02, 0x01, 0xB8, 0x65, 0xBC  },

    /* Value is allocated dynamically.                                   */
    FALSE,

    /* Maximum value length.                                             */
    120,

    /* Current value length.                                             */
    0,

    /* Current value.                                                    */
    (Byte_t *)NULL
};

static DescriptorInfo_t Srv2Attr9=
{
    /* Descriptor Properties.                                            */
    GATM_DESCRIPTOR_PROPERTIES_READ,

    /* Descriptor Security Properties.                                   */
    GATM_SECURITY_PROPERTIES_NO_SECURITY,

    /* Descriptor UUID.                                                  */
    { 0x6A, 0xFE, 0x67, 0x9A, 0x6C, 0x52, 0x10, 0x14, 0x8B, 0x7A, 0x95, 0x02, 0x01, 0xB8, 0x65, 0xBC  },

    /* Value is allocated dynamically.                                   */
    FALSE,

    /* Maximum value length.                                             */
    172,

    /* Current value length.                                             */
    10,

    /* Current value.                                                    */
    (Byte_t *)"HelloWorld"
};


/* The following array is the array containing attributes that are   */
/* registered by this service.                                       */
static AttributeInfo_t SrvTable2[] =
{
    { atInclude, 1, NULL },
    { atCharacteristic, 2, (void *)&Srv2Attr2 },
    { atCharacteristic, 4, (void *)&Srv2Attr3 },
    { atCharacteristic, 6, (void *)&Srv2Attr4 },
    { atDescriptor, 8, (void *)&Srv2Attr5 },
    { atCharacteristic, 9, (void *)&Srv2Attr6 },
    { atCharacteristic, 11, (void *)&Srv2Attr7 },
    { atCharacteristic, 13, (void *)&Srv2Attr8 },
    { atDescriptor, 15, (void *)&Srv2Attr9 }
};

/*********************************************************************/
/* End of Service Declaration.                                       */
/*********************************************************************/

/*********************************************************************/
/* Service Declaration.                                              */
/*********************************************************************/

static CharacteristicInfo_t Srv3Attr2=
{
    /* Characteristic Properties.                                        */
    GATM_CHARACTERISTIC_PROPERTIES_READ,

    /* Characteristic Security Properties.                               */
    GATM_SECURITY_PROPERTIES_NO_SECURITY,

    /* Characteristic UUID.                                              */
    { 0x6A, 0xFE, 0x69, 0x37, 0x6C, 0x52, 0x10, 0x14, 0x8B, 0x7A, 0x95, 0x02, 0x01, 0xB8, 0x65, 0xBC  },

    /* Value is allocated dynamically.                                   */
    FALSE,

    /* Maximum value length.                                             */
    229,

    /* Current value length.                                             */
    10,

    /* Current value.                                                    */
    (Byte_t *)"HelloWorld"
};

static CharacteristicInfo_t Srv3Attr3=
{
    /* Characteristic Properties.                                        */
    GATM_CHARACTERISTIC_PROPERTIES_WRITE,

    /* Characteristic Security Properties.                               */
    GATM_SECURITY_PROPERTIES_NO_SECURITY,

    /* Characteristic UUID.                                              */
    { 0x6A, 0xFE, 0x69, 0xF2, 0x6C, 0x52, 0x10, 0x14, 0x8B, 0x7A, 0x95, 0x02, 0x01, 0xB8, 0x65, 0xBC  },

    /* Value is allocated dynamically.                                   */
    FALSE,

    /* Maximum value length.                                             */
    165,

    /* Current value length.                                             */
    0,

    /* Current value.                                                    */
    (Byte_t *)NULL
};

static CharacteristicInfo_t Srv3Attr4=
{
    /* Characteristic Properties.                                        */
    GATM_CHARACTERISTIC_PROPERTIES_WRITE,

    /* Characteristic Security Properties.                               */
    GATM_SECURITY_PROPERTIES_NO_SECURITY,

    /* Characteristic UUID.                                              */
    { 0x6A, 0xFE, 0x6A, 0xA7, 0x6C, 0x52, 0x10, 0x14, 0x8B, 0x7A, 0x95, 0x02, 0x01, 0xB8, 0x65, 0xBC  },

    /* Value is allocated dynamically.                                   */
    FALSE,

    /* Maximum value length.                                             */
    495,

    /* Current value length.                                             */
    0,

    /* Current value.                                                    */
    (Byte_t *)NULL
};

static DescriptorInfo_t Srv3Attr5=
{
    /* Descriptor Properties.                                            */
    GATM_DESCRIPTOR_PROPERTIES_READ | GATM_DESCRIPTOR_PROPERTIES_WRITE,

    /* Descriptor Security Properties.                                   */
    GATM_SECURITY_PROPERTIES_AUTHENTICATED_ENCRYPTION_READ,

    /* Descriptor UUID.                                                  */
    { 0x6A, 0xFE, 0x6B, 0x65, 0x6C, 0x52, 0x10, 0x14, 0x8B, 0x7A, 0x95, 0x02, 0x01, 0xB8, 0x65, 0xBC  },

    /* Value is allocated dynamically.                                   */
    FALSE,

    /* Maximum value length.                                             */
    243,

    /* Current value length.                                             */
    227,

    /* Current value.                                                    */
    (Byte_t *)"SDZSWqRxoK~!%^&*()-_+={}[]|:;.<>?/#BDsKpSyoekWZShYdZmPJEnZPJGCnqAzYGnSewzqxNKLQwHgZVVOevQkuBoZXLeIksvfaRwEjjEBcmVvjdTmvuEoTnDMSfSNaAKlkGLzkyOTzmsW~!%^&*()-_+={}[]|:;.<>?/#czlMJCADkIIDWJAqBnkcUowtmEa~!%^&*()-_+={}[]|:;.<>?/#xkhbaBCPvFBVodzsKqaoqCMDOXjLINajTIJUHoUWJAlAWKDeYsBfqBv~!%^&*()-_+={}[]|:;.<>?/#xyMxzvKpDSB~!%^&*()-_+={}[]|:;.<>?/#jnGiLaKg"
};


/* The following array is the array containing attributes that are   */
/* registered by this service.                                       */
static AttributeInfo_t SrvTable3[] =
{
    { atInclude, 1, NULL },
    { atCharacteristic, 2, (void *)&Srv3Attr2 },
    { atCharacteristic, 4, (void *)&Srv3Attr3 },
    { atCharacteristic, 6, (void *)&Srv3Attr4 },
    { atDescriptor, 8, (void *)&Srv3Attr5 }
};

/*********************************************************************/
/* End of Service Declaration.                                       */
/*********************************************************************/

/*********************************************************************/
/* Service Declaration.                                              */
/*********************************************************************/

static CharacteristicInfo_t Srv4Attr2=
{
    /* Characteristic Properties.                                        */
    GATM_CHARACTERISTIC_PROPERTIES_WRITE,

    /* Characteristic Security Properties.                               */
    GATM_SECURITY_PROPERTIES_NO_SECURITY,

    /* Characteristic UUID.                                              */
    { 0x6A, 0xFE, 0x6F, 0x22, 0x6C, 0x52, 0x10, 0x14, 0x8B, 0x7A, 0x95, 0x02, 0x01, 0xB8, 0x65, 0xBC  },

    /* Value is allocated dynamically.                                   */
    FALSE,

    /* Maximum value length.                                             */
    307,

    /* Current value length.                                             */
    0,

    /* Current value.                                                    */
    (Byte_t *)NULL
};

static CharacteristicInfo_t Srv4Attr3=
{
    /* Characteristic Properties.                                        */
    GATM_CHARACTERISTIC_PROPERTIES_WRITE_WO_RESP | GATM_CHARACTERISTIC_PROPERTIES_WRITE,

    /* Characteristic Security Properties.                               */
    GATM_SECURITY_PROPERTIES_AUTHENTICATED_ENCRYPTION_WRITE,

    /* Characteristic UUID.                                              */
    { 0x6A, 0xFE, 0x6F, 0xFE, 0x6C, 0x52, 0x10, 0x14, 0x8B, 0x7A, 0x95, 0x02, 0x01, 0xB8, 0x65, 0xBC  },

    /* Value is allocated dynamically.                                   */
    FALSE,

    /* Maximum value length.                                             */
    426,

    /* Current value length.                                             */
    0,

    /* Current value.                                                    */
    (Byte_t *)NULL
};

static DescriptorInfo_t Srv4Attr4=
{
    /* Descriptor Properties.                                            */
    GATM_DESCRIPTOR_PROPERTIES_READ | GATM_DESCRIPTOR_PROPERTIES_WRITE,

    /* Descriptor Security Properties.                                   */
    GATM_SECURITY_PROPERTIES_AUTHENTICATED_ENCRYPTION_READ | GATM_SECURITY_PROPERTIES_UNAUTHENTICATED_ENCRYPTION_WRITE,

    /* Descriptor UUID.                                                  */
    { 0x6A, 0xFE, 0x70, 0xE7, 0x6C, 0x52, 0x10, 0x14, 0x8B, 0x7A, 0x95, 0x02, 0x01, 0xB8, 0x65, 0xBC  },

    /* Value is allocated dynamically.                                   */
    FALSE,

    /* Maximum value length.                                             */
    490,

    /* Current value length.                                             */
    469,

    /* Current value.                                                    */
    (Byte_t *)"~!%^&*()-_+={}[]|:;.<>?/#urpPyrtiaBXDTzKNyWgozZuCTpXYlkOCVWCbUbZIUrqjUWkNflnkUthgfAJtXYOGsINpAoVBOQwHGcjSEviprvXzjzljZtALcMGPbenLdbHpBObmL~!%^&*()-_+={}[]|:;.<>?/#eigmIow~!%^&*()-_+={}[]|:;.<>?/#UGTUGlhZREyIGDZCqoqmFxYkmrXgKCWBeRypnRDUjJfwmRAJRIWLvzIPUnLBYOFllTUvgQaGwXBQLIwuF~!%^&*()-_+={}[]|:;.<>?/#KGqLVpsnKcXPdOKHDlvLXZukFDtHnnfGiESZCqYEtwGVOiabkWTCHxVgzzUMrAPThqbxnjewzCBOYYHOMRQKhgVrCCGrPfkYMLuJxQfQhBKzFwgByxcaoPnrCVskADR~!%^&*()-_+={}[]|:;.<>?/#vSZOsRyMHCN~!%^&*()-_+={}[]|:;.<>?/#AwbVVsUIGWVDxTwBjjqaOWzfXAEdHaOLnBVtUunpEDSszUNWnOlWQrZzaNbDdwx~!%^&*()-_+={}[]|:;.<>?/#hXxn~!%^&*()-_+={}[]|:;.<>?/#yAWs~!%^&*()-_+={}[]|:;.<>?/#dWmDMwXVkDPDkigimWc~!%^&*()-_+={}[]|:;.<>?/#WzsBmVhLGaRgFxAJrupOWlkrpNFWPO"
};

static CharacteristicInfo_t Srv4Attr5=
{
    /* Characteristic Properties.                                        */
    GATM_CHARACTERISTIC_PROPERTIES_WRITE,

    /* Characteristic Security Properties.                               */
    GATM_SECURITY_PROPERTIES_AUTHENTICATED_ENCRYPTION_WRITE,

    /* Characteristic UUID.                                              */
    { 0x6A, 0xFE, 0x77, 0x51, 0x6C, 0x52, 0x10, 0x14, 0x8B, 0x7A, 0x95, 0x02, 0x01, 0xB8, 0x65, 0xBC  },

    /* Value is allocated dynamically.                                   */
    FALSE,

    /* Maximum value length.                                             */
    487,

    /* Current value length.                                             */
    0,

    /* Current value.                                                    */
    (Byte_t *)NULL
};

static CharacteristicInfo_t Srv4Attr6=
{
    /* Characteristic Properties.                                        */
    GATM_CHARACTERISTIC_PROPERTIES_WRITE,

    /* Characteristic Security Properties.                               */
    GATM_SECURITY_PROPERTIES_AUTHENTICATED_ENCRYPTION_WRITE,

    /* Characteristic UUID.                                              */
    { 0x6A, 0xFE, 0x78, 0x46, 0x6C, 0x52, 0x10, 0x14, 0x8B, 0x7A, 0x95, 0x02, 0x01, 0xB8, 0x65, 0xBC  },

    /* Value is allocated dynamically.                                   */
    FALSE,

    /* Maximum value length.                                             */
    267,

    /* Current value length.                                             */
    0,

    /* Current value.                                                    */
    (Byte_t *)NULL
};

static DescriptorInfo_t Srv4Attr7=
{
    /* Descriptor Properties.                                            */
    GATM_DESCRIPTOR_PROPERTIES_WRITE,

    /* Descriptor Security Properties.                                   */
    GATM_SECURITY_PROPERTIES_NO_SECURITY,

    /* Descriptor UUID.                                                  */
    { 0x6A, 0xFE, 0x78, 0xFF, 0x6C, 0x52, 0x10, 0x14, 0x8B, 0x7A, 0x95, 0x02, 0x01, 0xB8, 0x65, 0xBC  },

    /* Value is allocated dynamically.                                   */
    FALSE,

    /* Maximum value length.                                             */
    189,

    /* Current value length.                                             */
    0,

    /* Current value.                                                    */
    (Byte_t *)NULL
};


/* The following array is the array containing attributes that are   */
/* registered by this service.                                       */
static AttributeInfo_t SrvTable4[] =
{
    { atInclude, 1, NULL },
    { atCharacteristic, 2, (void *)&Srv4Attr2 },
    { atCharacteristic, 4, (void *)&Srv4Attr3 },
    { atDescriptor, 6, (void *)&Srv4Attr4 },
    { atCharacteristic, 7, (void *)&Srv4Attr5 },
    { atCharacteristic, 9, (void *)&Srv4Attr6 },
    { atDescriptor, 11, (void *)&Srv4Attr7 }
};

/*********************************************************************/
/* End of Service Declaration.                                       */
/*********************************************************************/

/*********************************************************************/
/* Service Declaration.                                              */
/*********************************************************************/

static CharacteristicInfo_t Srv5Attr1=
{
    /* Characteristic Properties.                                        */
    GATM_CHARACTERISTIC_PROPERTIES_READ | GATM_CHARACTERISTIC_PROPERTIES_WRITE_WO_RESP | GATM_CHARACTERISTIC_PROPERTIES_WRITE,

    /* Characteristic Security Properties.                               */
    GATM_SECURITY_PROPERTIES_AUTHENTICATED_ENCRYPTION_READ | GATM_SECURITY_PROPERTIES_AUTHENTICATED_ENCRYPTION_WRITE,

    /* Characteristic UUID.                                              */
    { 0x6A, 0xFE, 0x7A, 0xCE, 0x6C, 0x52, 0x10, 0x14, 0x8B, 0x7A, 0x95, 0x02, 0x01, 0xB8, 0x65, 0xBC  },

    /* Value is allocated dynamically.                                   */
    FALSE,

    /* Maximum value length.                                             */
    326,

    /* Current value length.                                             */
    0,

    /* Current value.                                                    */
    (Byte_t *)NULL
};

static CharacteristicInfo_t Srv5Attr2=
{
    /* Characteristic Properties.                                        */
    GATM_CHARACTERISTIC_PROPERTIES_READ | GATM_CHARACTERISTIC_PROPERTIES_WRITE,

    /* Characteristic Security Properties.                               */
    GATM_SECURITY_PROPERTIES_AUTHENTICATED_ENCRYPTION_READ,

    /* Characteristic UUID.                                              */
    { 0x6A, 0xFE, 0x7B, 0xD8, 0x6C, 0x52, 0x10, 0x14, 0x8B, 0x7A, 0x95, 0x02, 0x01, 0xB8, 0x65, 0xBC  },

    /* Value is allocated dynamically.                                   */
    FALSE,

    /* Maximum value length.                                             */
    233,

    /* Current value length.                                             */
    233,

    /* Current value.                                                    */
    (Byte_t *)"VRWGsXStNGTcZJTqQkQxnTXZBdGjtjBKaAWWGvMFGxDleYXtztTYBvyQquoo~!%^&*()-_+={}[]|:;.<>?/#XuKgBLoyu~!%^&*()-_+={}[]|:;.<>?/#LuaLebVuyVIYaAgAEpEBJnkOoaRPz~!%^&*()-_+={}[]|:;.<>?/#jGwjjXaZrXuSkgJdHzDelfBzFRntFNZQNrxbgcU~!%^&*()-_+={}[]|:;.<>?/#PhdYpBIyxqaibS~!%^&*()-_+={}[]|:;.<>?/#xJtAOtvIUZwCToViALctNDDguIKjWgQMPJPaMSHbGwtCDrJOvnylIZMPWrXvIRibFImIHiEaqwroS"
};

static CharacteristicInfo_t Srv5Attr3=
{
    /* Characteristic Properties.                                        */
    GATM_CHARACTERISTIC_PROPERTIES_READ | GATM_CHARACTERISTIC_PROPERTIES_WRITE,

    /* Characteristic Security Properties.                               */
    GATM_SECURITY_PROPERTIES_UNAUTHENTICATED_ENCRYPTION_WRITE,

    /* Characteristic UUID.                                              */
    { 0x6A, 0xFE, 0x7F, 0xD9, 0x6C, 0x52, 0x10, 0x14, 0x8B, 0x7A, 0x95, 0x02, 0x01, 0xB8, 0x65, 0xBC  },

    /* Value is allocated dynamically.                                   */
    FALSE,

    /* Maximum value length.                                             */
    220,

    /* Current value length.                                             */
    220,

    /* Current value.                                                    */
    (Byte_t *)"abERkFtyw~!%^&*()-_+={}[]|:;.<>?/#PFyhGjfDuAdCqIgBllooqMAxQcvoVIOzwPI~!%^&*()-_+={}[]|:;.<>?/#KDfuLyHqdPlbonxkrtMcsHyrDKuIFHNFctzEsbdDlYyhpTs~!%^&*()-_+={}[]|:;.<>?/#uhHpehiGaLXhvqzGDcZjGqkYinUWovoDGXPHUoxErJTaLoIkESbgRMeHJAmEfwHsCzxF~!%^&*()-_+={}[]|:;.<>?/#fwecsjvwosyRUJxjZVoauSlBRglfi~!%^&*()-_+={}[]|:;.<>?/#MBXtEdkCzlrLhyYKkMcaeLnNaqO"
};

static CharacteristicInfo_t Srv5Attr4=
{
    /* Characteristic Properties.                                        */
    GATM_CHARACTERISTIC_PROPERTIES_WRITE_WO_RESP | GATM_CHARACTERISTIC_PROPERTIES_WRITE,

    /* Characteristic Security Properties.                               */
    GATM_SECURITY_PROPERTIES_AUTHENTICATED_ENCRYPTION_WRITE,

    /* Characteristic UUID.                                              */
    { 0x6A, 0xFE, 0x83, 0x6B, 0x6C, 0x52, 0x10, 0x14, 0x8B, 0x7A, 0x95, 0x02, 0x01, 0xB8, 0x65, 0xBC  },

    /* Value is allocated dynamically.                                   */
    FALSE,

    /* Maximum value length.                                             */
    296,

    /* Current value length.                                             */
    0,

    /* Current value.                                                    */
    (Byte_t *)NULL
};

static CharacteristicInfo_t Srv5Attr5=
{
    /* Characteristic Properties.                                        */
    GATM_CHARACTERISTIC_PROPERTIES_READ | GATM_CHARACTERISTIC_PROPERTIES_WRITE_WO_RESP,

    /* Characteristic Security Properties.                               */
    GATM_SECURITY_PROPERTIES_UNAUTHENTICATED_ENCRYPTION_WRITE,

    /* Characteristic UUID.                                              */
    { 0x6A, 0xFE, 0x84, 0x56, 0x6C, 0x52, 0x10, 0x14, 0x8B, 0x7A, 0x95, 0x02, 0x01, 0xB8, 0x65, 0xBC  },

    /* Value is allocated dynamically.                                   */
    FALSE,

    /* Maximum value length.                                             */
    494,

    /* Current value length.                                             */
    0,

    /* Current value.                                                    */
    (Byte_t *)NULL
};

static DescriptorInfo_t Srv5Attr6=
{
    /* Descriptor Properties.                                            */
    GATM_DESCRIPTOR_PROPERTIES_READ,

    /* Descriptor Security Properties.                                   */
    GATM_SECURITY_PROPERTIES_NO_SECURITY,

    /* Descriptor UUID.                                                  */
    { 0x6A, 0xFE, 0x85, 0x21, 0x6C, 0x52, 0x10, 0x14, 0x8B, 0x7A, 0x95, 0x02, 0x01, 0xB8, 0x65, 0xBC  },

    /* Value is allocated dynamically.                                   */
    FALSE,

    /* Maximum value length.                                             */
    417,

    /* Current value length.                                             */
    349,

    /* Current value.                                                    */
    (Byte_t *)"ZLyNTJKNaxZcH~!%^&*()-_+={}[]|:;.<>?/#EEIealwBbtwOnKTCRafurXYsYTnhaBeUWgqmeIcFdcoQRFDKsLdOmq~!%^&*()-_+={}[]|:;.<>?/#NQEDYGgBuJrftgIHUXJNrISkqOJkjEYwqxGfJUVVuQBqSgDwiJzCJSptJykLhVBffIJXqaBEgZggyioVUYJhCJrjXtGNCWirdaAefsirCxyFiAjfxfCZSEVdXewuByBoUFWvIw~!%^&*()-_+={}[]|:;.<>?/#oyjZNkHouTeuwRkzTsUY~!%^&*()-_+={}[]|:;.<>?/#zAUBeqLLSUMYvMh~!%^&*()-_+={}[]|:;.<>?/#ihbCdBBLYkbVFLNZfpYfTBNLPFIxKwuFTKxQINtabCfRWiqEufdurxVaXCDb~!%^&*()-_+={}[]|:;.<>?/#goQypeNfKntfqMLrxfZCvYknOefYToAGvqLqVcwFZGMAJBd"
};

static CharacteristicInfo_t Srv5Attr7=
{
    /* Characteristic Properties.                                        */
    GATM_CHARACTERISTIC_PROPERTIES_READ | GATM_CHARACTERISTIC_PROPERTIES_WRITE_WO_RESP,

    /* Characteristic Security Properties.                               */
    GATM_SECURITY_PROPERTIES_AUTHENTICATED_ENCRYPTION_READ,

    /* Characteristic UUID.                                              */
    { 0x6A, 0xFE, 0x8A, 0x37, 0x6C, 0x52, 0x10, 0x14, 0x8B, 0x7A, 0x95, 0x02, 0x01, 0xB8, 0x65, 0xBC  },

    /* Value is allocated dynamically.                                   */
    FALSE,

    /* Maximum value length.                                             */
    374,

    /* Current value length.                                             */
    10,

    /* Current value.                                                    */
    (Byte_t *)"HelloWorld"
};

static CharacteristicInfo_t Srv5Attr8=
{
    /* Characteristic Properties.                                        */
    GATM_CHARACTERISTIC_PROPERTIES_READ | GATM_CHARACTERISTIC_PROPERTIES_WRITE_WO_RESP,

    /* Characteristic Security Properties.                               */
    GATM_SECURITY_PROPERTIES_UNAUTHENTICATED_ENCRYPTION_WRITE,

    /* Characteristic UUID.                                              */
    { 0x6A, 0xFE, 0x8A, 0xFF, 0x6C, 0x52, 0x10, 0x14, 0x8B, 0x7A, 0x95, 0x02, 0x01, 0xB8, 0x65, 0xBC  },

    /* Value is allocated dynamically.                                   */
    FALSE,

    /* Maximum value length.                                             */
    213,

    /* Current value length.                                             */
    10,

    /* Current value.                                                    */
    (Byte_t *)"HelloWorld"
};

static CharacteristicInfo_t Srv5Attr9=
{
    /* Characteristic Properties.                                        */
    GATM_CHARACTERISTIC_PROPERTIES_READ | GATM_CHARACTERISTIC_PROPERTIES_WRITE_WO_RESP | GATM_CHARACTERISTIC_PROPERTIES_WRITE,

    /* Characteristic Security Properties.                               */
    GATM_SECURITY_PROPERTIES_AUTHENTICATED_ENCRYPTION_WRITE,

    /* Characteristic UUID.                                              */
    { 0x6A, 0xFE, 0x8B, 0xCD, 0x6C, 0x52, 0x10, 0x14, 0x8B, 0x7A, 0x95, 0x02, 0x01, 0xB8, 0x65, 0xBC  },

    /* Value is allocated dynamically.                                   */
    FALSE,

    /* Maximum value length.                                             */
    418,

    /* Current value length.                                             */
    0,

    /* Current value.                                                    */
    (Byte_t *)NULL
};


/* The following array is the array containing attributes that are   */
/* registered by this service.                                       */
static AttributeInfo_t SrvTable5[] =
{
    { atCharacteristic, 1, (void *)&Srv5Attr1 },
    { atCharacteristic, 3, (void *)&Srv5Attr2 },
    { atCharacteristic, 5, (void *)&Srv5Attr3 },
    { atCharacteristic, 7, (void *)&Srv5Attr4 },
    { atCharacteristic, 9, (void *)&Srv5Attr5 },
    { atDescriptor, 11, (void *)&Srv5Attr6 },
    { atCharacteristic, 12, (void *)&Srv5Attr7 },
    { atCharacteristic, 14, (void *)&Srv5Attr8 },
    { atCharacteristic, 16, (void *)&Srv5Attr9 }
};

/*********************************************************************/
/* End of Service Declaration.                                       */
/*********************************************************************/

/*********************************************************************/
/* Service Declaration.                                              */
/*********************************************************************/

static CharacteristicInfo_t Srv6Attr2=
{
    /* Characteristic Properties.                                        */
    GATM_CHARACTERISTIC_PROPERTIES_WRITE,

    /* Characteristic Security Properties.                               */
    GATM_SECURITY_PROPERTIES_NO_SECURITY,

    /* Characteristic UUID.                                              */
    { 0x6A, 0xFE, 0x8D, 0xCB, 0x6C, 0x52, 0x10, 0x14, 0x8B, 0x7A, 0x95, 0x02, 0x01, 0xB8, 0x65, 0xBC  },

    /* Value is allocated dynamically.                                   */
    FALSE,

    /* Maximum value length.                                             */
    374,

    /* Current value length.                                             */
    0,

    /* Current value.                                                    */
    (Byte_t *)NULL
};

static CharacteristicInfo_t Srv6Attr3=
{
    /* Characteristic Properties.                                        */
    GATM_CHARACTERISTIC_PROPERTIES_READ | GATM_CHARACTERISTIC_PROPERTIES_WRITE_WO_RESP,

    /* Characteristic Security Properties.                               */
    GATM_SECURITY_PROPERTIES_UNAUTHENTICATED_ENCRYPTION_WRITE,

    /* Characteristic UUID.                                              */
    { 0x6A, 0xFE, 0x8E, 0xB5, 0x6C, 0x52, 0x10, 0x14, 0x8B, 0x7A, 0x95, 0x02, 0x01, 0xB8, 0x65, 0xBC  },

    /* Value is allocated dynamically.                                   */
    FALSE,

    /* Maximum value length.                                             */
    255,

    /* Current value length.                                             */
    255,

    /* Current value.                                                    */
    (Byte_t *)"JHRgqzMWRhsbGOsxWrszFcyPpOzv~!%^&*()-_+={}[]|:;.<>?/#gJQbSFvpLKbBktISgdCfOgAgDcAOEMMsrEajXMvKkAPBxxoKTIFRAnWfPpegGfCzFDjRgZlyskytEIQDihRKbzPGJzXkwinWguoxkdQOLoJqbjjgjcRdzBeewuZKkEhZjrGpINilcPBxNhepqxHeJnFfotxgNcxnmnYuJTc~!%^&*()-_+={}[]|:;.<>?/#muyF~!%^&*()-_+={}[]|:;.<>?/#LmriVMHhxlGLoaIJnoGXAxCcokMphszDikMuGW~!%^&*()-_+={}[]|:;.<>?/#YXASuzLtfXImfc"
};


/* The following array is the array containing attributes that are   */
/* registered by this service.                                       */
static AttributeInfo_t SrvTable6[] =
{
    { atInclude, 1, NULL },
    { atCharacteristic, 2, (void *)&Srv6Attr2 },
    { atCharacteristic, 4, (void *)&Srv6Attr3 }
};

/*********************************************************************/
/* End of Service Declaration.                                       */
/*********************************************************************/

/* The following array is the array containing attributes that are   */
/* registered by this service.                                       */
static ServiceInfo_t ServiceTable[] =
{
    {
        /* Service Flags.                                                 */
        0,

        /* Service ID.                                                    */
        0,

        /* PersistentUID.                                                 */
        0,

        /* Service UUID.
        9760d077-a234-4686-9e00-fcbbee3373f7 */
        MAKEUUID16(97,60,D0,77,A2,34,46,86,9E,00,FC,BB,EE,33,73,F7),

        /* Service Handle Range.                                          */
        {0, 0},

        /* Number of Service Table Entries.                               */
        PAIRING_SERVICE_TABLE_NUM_ENTRIES,

        /* Service Attribute List.                                        */
        SrvTable0
    },
    {
        /* Service Flags.                                                 */
        0,

        /* Service ID.                                                    */
        0,

        /* PersistentUID.                                                 */
        0,

        /* Service UUID.                                                  */
        { 0x6A, 0xFE, 0x54, 0x81, 0x6C, 0x52, 0x10, 0x14, 0x8B, 0x7A, 0x95, 0x02, 0x01, 0xB8, 0x65, 0xBC  },

        /* Service Handle Range.                                          */
        {0, 0},

        /* Number of Service Table Entries.                               */
        3,

        /* Service Attribute List.                                        */
        SrvTable1
    },
    {
        /* Service Flags.                                                 */
        SERVICE_TABLE_FLAGS_USE_PERSISTENT_UID,

        /* Service ID.                                                    */
        0,

        /* PersistentUID.                                                 */
        0,

        /* Service UUID.                                                  */
        { 0x6A, 0xFE, 0x68, 0x65, 0x6C, 0x52, 0x10, 0x14, 0x8B, 0x7A, 0x95, 0x02, 0x01, 0xB8, 0x65, 0xBC  },

        /* Service Handle Range.                                          */
        {0, 0},

        /* Number of Service Table Entries.                               */
        9,

        /* Service Attribute List.                                        */
        SrvTable2
    },
    {
        /* Service Flags.                                                 */
        SERVICE_TABLE_FLAGS_USE_PERSISTENT_UID,

        /* Service ID.                                                    */
        0,

        /* PersistentUID.                                                 */
        0,

        /* Service UUID.                                                  */
        { 0x6A, 0xFE, 0x6E, 0x1B, 0x6C, 0x52, 0x10, 0x14, 0x8B, 0x7A, 0x95, 0x02, 0x01, 0xB8, 0x65, 0xBC  },

        /* Service Handle Range.                                          */
        {0, 0},

        /* Number of Service Table Entries.                               */
        5,

        /* Service Attribute List.                                        */
        SrvTable3
    },
    {
        /* Service Flags.                                                 */
        0,

        /* Service ID.                                                    */
        0,

        /* PersistentUID.                                                 */
        0,

        /* Service UUID.                                                  */
        { 0x6A, 0xFE, 0x79, 0xCE, 0x6C, 0x52, 0x10, 0x14, 0x8B, 0x7A, 0x95, 0x02, 0x01, 0xB8, 0x65, 0xBC  },

        /* Service Handle Range.                                          */
        {0, 0},

        /* Number of Service Table Entries.                               */
        7,

        /* Service Attribute List.                                        */
        SrvTable4
    },
    {
        /* Service Flags.                                                 */
        SERVICE_TABLE_FLAGS_SECONDARY_SERVICE | SERVICE_TABLE_FLAGS_USE_PERSISTENT_UID,

        /* Service ID.                                                    */
        0,

        /* PersistentUID.                                                 */
        0,

        /* Service UUID.                                                  */
        { 0x6A, 0xFE, 0x8C, 0xB1, 0x6C, 0x52, 0x10, 0x14, 0x8B, 0x7A, 0x95, 0x02, 0x01, 0xB8, 0x65, 0xBC  },

        /* Service Handle Range.                                          */
        {0, 0},

        /* Number of Service Table Entries.                               */
        9,

        /* Service Attribute List.                                        */
        SrvTable5
    },
    {
        /* Service Flags.                                                 */
        SERVICE_TABLE_FLAGS_USE_PERSISTENT_UID,

        /* Service ID.                                                    */
        0,

        /* PersistentUID.                                                 */
        0,

        /* Service UUID.                                                  */
        { 0x6A, 0xFE, 0x92, 0x3A, 0x6C, 0x52, 0x10, 0x14, 0x8B, 0x7A, 0x95, 0x02, 0x01, 0xB8, 0x65, 0xBC  },

        /* Service Handle Range.                                          */
        {0, 0},

        /* Number of Service Table Entries.                               */
        3,

        /* Service Attribute List.                                        */
        SrvTable6
    }
};
#define PREDEFINED_SERVICES_COUNT                                     (sizeof(ServiceTable)/sizeof(ServiceInfo_t))
#endif
