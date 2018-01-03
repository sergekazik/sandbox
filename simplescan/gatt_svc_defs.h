#ifndef __RING_GATT_SERVICE_DEFINITION_H__
#define __RING_GATT_SERVICE_DEFINITION_H__

// Helper macro
#define MAKEUUID16(_q, _w, _e, _r, _t, _y, _u, _i, _o, _p, _a, _s, _d, _f, _g, _h) {0x##_h, 0x##_g, 0x##_f, 0x##_d, 0x##_s, 0x##_a, 0x##_p, 0x##_o, 0x##_i, 0x##_u, 0x##_y, 0x##_t, 0x##_r, 0x##_e, 0x##_w, 0x##_q}
#define MAKERINGUUID(_attr_idx) MAKEUUID16(00,00,FA,CE,00,00,10,00,80,00,00,80,5F,9B,35,_attr_idx)
#define MAKEPAYLOAD(_pld) (unsigned int) strlen(_pld), (Byte_t *) _pld

#define BLE_PACKET_LEN              18
#define RING_PAIRING_SERVICE_UUID   MAKEUUID16(00,00,FA,CE,00,00,10,00,80,00,00,80,5F,9B,34,FB)

#if DEVELOPMENT_TEST_NO_SECURITY
#define GATM_SECURITY_PROPERTIES_ENCRYPTED (GATM_SECURITY_PROPERTIES_UNAUTHENTICATED_ENCRYPTION_WRITE \
                                            |GATM_SECURITY_PROPERTIES_AUTHENTICATED_ENCRYPTION_WRITE    \
                                            |GATM_SECURITY_PROPERTIES_UNAUTHENTICATED_ENCRYPTION_READ \
                                            |GATM_SECURITY_PROPERTIES_AUTHENTICATED_ENCRYPTION_READ)
#else
#define GATM_SECURITY_PROPERTIES_ENCRYPTED GATM_SECURITY_PROPERTIES_NO_SECURITY
#endif

#endif // __RING_GATT_SERVICE_DEFINITION_H__

#ifdef RING_BLE_PAIRING_SERVICES_DEFINE
#undef RING_BLE_PAIRING_SERVICES_DEFINE
#endif

/*********************************************************************
 *  characteristic def example
 *  static CharacteristicInfo_t Srv1Attr1=
 * {
 *     // Characteristic Properties.
 *     GATM_CHARACTERISTIC_PROPERTIES_READ | GATM_CHARACTERISTIC_PROPERTIES_WRITE | GATM_CHARACTERISTIC_PROPERTIES_INDICATE,
 *
 *     // Characteristic Security Properties.
 *     GATM_SECURITY_PROPERTIES_AUTHENTICATED_ENCRYPTION_WRITE,
 *
 *     // Characteristic UUID.
 *     { 0x6A, 0xFE, 0x4D, 0x41, 0x6C, 0x52, 0x10, 0x14, 0x8B, 0x7A, 0x95, 0x02, 0x01, 0xB8, 0x65, 0xBC  },
 *
 *     // Value is allocated dynamically.
 *     0,
 *
 *     // Maximum value length.
 *     117,
 *
 *     // Current value length.
 *     117,
 *
 *     // Current value.
 *     (Byte_t *)"VUHhfCQDLvB~!%^&*()-_+={}[]|:;.<>?/#uoPXonUEjisFQjbWGlXmAoQsSSqJwexZsugfFtamHcTun~!%^&*()-_+={}[]|:;.<>?/#AfjLKCeuMia"
 * };
*********************************************************************/
#ifdef RING_PAIRING_TABLE_SERVICE_DECL
#undef RING_PAIRING_TABLE_SERVICE_DECL
#define RING_BLE_PAIRING_SERVICES_DEFINE(__arg0, __arg1, __arg2, __arg3, __arg4, __arg5, __argpld)    \
                static CharacteristicInfo_t Srv0Attr_##__arg1 = { __arg2, __arg4, __arg5,  0, __arg3*BLE_PACKET_LEN,  __argpld };
#endif

/*********************************************************************
 * The array containing attributes of the service example
 * static AttributeInfo_t SrvTable1[] =
 * {
 *     { atCharacteristic, 1, (void *)&Srv1Attr1 },
 *     { atCharacteristic, 3, (void *)&Srv1Attr2 },
 *     { atCharacteristic, 5, (void *)&Srv1Attr3 }
 * };
*********************************************************************/
#ifdef RING_PAIRING_TABLE_SERVICE_DEFINE_TABLE
#undef RING_PAIRING_TABLE_SERVICE_DEFINE_TABLE
#define RING_BLE_GATT_SERVER_TEST_DEFINE(__arg1, __arg2)
#define RING_BLE_PAIRING_SERVICES_DEFINE(__arg0, __arg1, __arg2, __arg3, __arg4, __arg5, __argpld) { atCharacteristic, __arg0, (void *) &Srv0Attr_##__arg1, #__arg1 },
#endif

/*********************************************************************
 * characteristic index enumerator example
 * enum GattAttributeIndexByName
 * {
 *     SET_PUBLIC_KEY    ,
 *     GET_PUBLIC_PAYLOAD,
 *     GET_AP_LIST       ,
 *     SET_PROVISION     ,
 *     SET_SECRET_KEY    ,
 *     SET_COUNTRY_CODE  ,
 *     GET_PAIRING_STATE ,
 *     SET_LANGUAGE      ,
 *     SET_ZIPCODE       ,
 *     GET_WIFI_STATUS   ,
 *     GET_SSID_WIFI     ,
 *     GET_SERIAL_NUMBER ,
 *     GET_MAC_ADDRESS   ,
 *     SET_ETHERNET      ,
 *     SET_REG_DOMAIN    ,
 *     GET_NET_INFO      ,
 * };
*********************************************************************/
#ifdef RING_PAIRING_TABLE_ATTR_ENUM
#undef RING_PAIRING_TABLE_ATTR_ENUM
#define RING_BLE_GATT_SERVER_TEST_DEFINE(__arg1, __arg2)
#define RING_BLE_PAIRING_SERVICES_DEFINE(__arg0, __arg1, __arg2, __arg3, __arg4, __arg5, __argpld)   __arg1,
#endif

#ifdef RING_PAIRING_SERVICE_INFO_DEFINE
#undef RING_PAIRING_SERVICE_INFO_DEFINE
static ServiceInfo_t sServiceTable[] =
{
    {
        0,                                            // Flags
        0,                                            // ServiceID (updated by API)
        0,                                            // PersistentUID (used if requested by flags)
        RING_PAIRING_SERVICE_UUID,                    // ServiceUUID
        {0, 0},                                       // ServiceHandleRange (updated by API)
        (sizeof(SrvTable0)/sizeof(AttributeInfo_t)),  // NumberAttributes
        SrvTable0,                                    // AttributeList
        "RING_PAIRING_SVC"                            // ServiceName (for internal use only and not advertised to clients)
    },
};
#define RING_PAIRING_SVC_IDX                                          0
#define RING_GATT_SERVICES_COUNT                                     (sizeof(sServiceTable)/sizeof(ServiceInfo_t))
#endif

#ifndef RING_BLE_PAIRING_SERVICES_DEFINE
#define RING_BLE_PAIRING_SERVICES_DEFINE(__arg0, __arg1, __arg2, __arg3, __arg4, __arg5, __argpld)
#endif

RING_BLE_PAIRING_SERVICES_DEFINE( 1, SET_PUBLIC_KEY    , Ble::Characteristic::_W_, 28, GATM_SECURITY_PROPERTIES_NO_SECURITY, MAKERINGUUID(01), MAKEPAYLOAD(""))
RING_BLE_PAIRING_SERVICES_DEFINE( 3, GET_PUBLIC_PAYLOAD, Ble::Characteristic::RW_, 28, GATM_SECURITY_PROPERTIES_NO_SECURITY, MAKERINGUUID(02), MAKEPAYLOAD("PUBLIC PAYLOAD KEY NONCE PLACEHOLDER"))
RING_BLE_PAIRING_SERVICES_DEFINE( 5, GET_AP_LIST       , Ble::Characteristic::R__,112, GATM_SECURITY_PROPERTIES_ENCRYPTED,   MAKERINGUUID(03), MAKEPAYLOAD("<ap_list></ap_list>"))
RING_BLE_PAIRING_SERVICES_DEFINE( 7, SET_PROVISION     , Ble::Characteristic::_W_, 28, GATM_SECURITY_PROPERTIES_ENCRYPTED,   MAKERINGUUID(04), MAKEPAYLOAD("<network></network>"))
RING_BLE_PAIRING_SERVICES_DEFINE( 9, SET_SECRET_KEY    , Ble::Characteristic::_W_,  2, GATM_SECURITY_PROPERTIES_ENCRYPTED,   MAKERINGUUID(05), MAKEPAYLOAD(""))
RING_BLE_PAIRING_SERVICES_DEFINE(11, SET_COUNTRY_CODE  , Ble::Characteristic::_W_,  2, GATM_SECURITY_PROPERTIES_ENCRYPTED,   MAKERINGUUID(06), MAKEPAYLOAD(""))
RING_BLE_PAIRING_SERVICES_DEFINE(13, GET_PAIRING_STATE , Ble::Characteristic::R_N,  1, GATM_SECURITY_PROPERTIES_ENCRYPTED,   MAKERINGUUID(07), MAKEPAYLOAD("IDLE"))
RING_BLE_PAIRING_SERVICES_DEFINE(15, SET_LANGUAGE      , Ble::Characteristic::_W_,  1, GATM_SECURITY_PROPERTIES_ENCRYPTED,   MAKERINGUUID(08), MAKEPAYLOAD(""))
RING_BLE_PAIRING_SERVICES_DEFINE(17, SET_ZIPCODE       , Ble::Characteristic::_W_,  1, GATM_SECURITY_PROPERTIES_ENCRYPTED,   MAKERINGUUID(09), MAKEPAYLOAD(""))
RING_BLE_PAIRING_SERVICES_DEFINE(19, GET_WIFI_STATUS   , Ble::Characteristic::R__,  1, GATM_SECURITY_PROPERTIES_ENCRYPTED,   MAKERINGUUID(0A), MAKEPAYLOAD("DISCONNECTED"))
RING_BLE_PAIRING_SERVICES_DEFINE(21, GET_SSID_WIFI     , Ble::Characteristic::R__,  2, GATM_SECURITY_PROPERTIES_ENCRYPTED,   MAKERINGUUID(0B), MAKEPAYLOAD("Doorbells"))
RING_BLE_PAIRING_SERVICES_DEFINE(23, GET_SERIAL_NUMBER , Ble::Characteristic::R__,  2, GATM_SECURITY_PROPERTIES_ENCRYPTED,   MAKERINGUUID(0C), MAKEPAYLOAD("BHRC11728CH003122"))
RING_BLE_PAIRING_SERVICES_DEFINE(25, GET_MAC_ADDRESS   , Ble::Characteristic::R__,  1, GATM_SECURITY_PROPERTIES_ENCRYPTED,   MAKERINGUUID(0D), MAKEPAYLOAD("38D269F27E01"))
RING_BLE_PAIRING_SERVICES_DEFINE(27, SET_ETHERNET      , Ble::Characteristic::_W_,  4, GATM_SECURITY_PROPERTIES_ENCRYPTED,   MAKERINGUUID(0E), MAKEPAYLOAD(""))
RING_BLE_PAIRING_SERVICES_DEFINE(29, SET_REG_DOMAIN    , Ble::Characteristic::_W_,  1, GATM_SECURITY_PROPERTIES_ENCRYPTED,   MAKERINGUUID(0F), MAKEPAYLOAD("US"))
RING_BLE_PAIRING_SERVICES_DEFINE(31, GET_NET_INFO      , Ble::Characteristic::R__, 28, GATM_SECURITY_PROPERTIES_ENCRYPTED,   MAKERINGUUID(10), MAKEPAYLOAD("<network></network>"))
