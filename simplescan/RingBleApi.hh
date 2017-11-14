#ifndef _RING_BLE_API_H_
#define _RING_BLE_API_H_

#include <stdio.h>
#include <stdlib.h>
#include <stdlib.h>
#include <errno.h>
#include <signal.h>
#include <unistd.h>

namespace Ring { namespace Ble {

#define MAX_SUPPORTED_COMMANDS                     (75)  /* Denotes the       */
                                                         /* maximum number of */
                                                         /* User Commands that*/
                                                         /* are supported by  */
                                                         /* this application. */

#define MAX_COMMAND_LENGTH                        (128)  /* Denotes the max   */
                                                         /* buffer size used  */
                                                         /* for user commands */
                                                         /* input via the     */
                                                         /* User Interface.   */

#define MAX_NUM_OF_PARAMETERS                      (10)  /* Denotes the max   */
                                                         /* number of         */
                                                         /* parameters a      */
                                                         /* command can have. */

#define DEFAULT_IO_CAPABILITY          (icDisplayYesNo)  /* Denotes the       */
                                                         /* default I/O       */
                                                         /* Capability that is*/
                                                         /* used with Secure  */
                                                         /* Simple Pairing.   */

#define DEFAULT_LE_IO_CAPABILITY      (licDisplayYesNo)  /* Denotes the       */
                                                         /* default I/O       */
                                                         /* Capability that is*/
                                                         /* used with LE      */
                                                         /* Pairing.          */

#define DEFAULT_LE_MITM_PROTECTION               (TRUE)  /* Denotes the       */
                                                         /* default value used*/
                                                         /* for Man in the    */
                                                         /* Middle (MITM)     */
                                                         /* protection used   */
                                                         /* with LE Pairing.  */

#define DEFAULT_LE_BONDING_TYPE	           (lbtBonding)  /* Denotes the       */
                                                         /* default value used*/
                                                         /* for Bonding Type. */

#define DEFAULT_SC_PROTECTION                    (TRUE)	 /* Denotes the       */
                                                         /* default value used*/
                                                         /* for LE pairing	  */
                                                         /* method SC or	  */
                                                         /* Legacy    		  */

#define DEFAULT_KEYPRESS      			         (TRUE)  /* Denotes the       */
                                                         /* default value used*/
                                                         /* for keypress	  */
                                                         /* notifications     */

#define DEFAULT_P256_DEBUG_ENABLE	            (FALSE)  /* Denotes the       */
                                                         /* default value used*/
                                                         /* for P256 debug	  */
                                                         /* mode.This is	  */
                                                         /* relevant only	  */
                                                         /* for LE SC 		  */

#define DEFAULT_MITM_PROTECTION                  (TRUE)  /* Denotes the       */
                                                         /* default value used*/
                                                         /* for Man in the    */
                                                         /* Middle (MITM)     */
                                                         /* protection used   */
                                                         /* with Secure Simple*/
                                                         /* Pairing.          */

#define INDENT_LENGTH                                3   /* Denotes the number*/
                                                         /* of character      */
                                                         /* spaces to be used */
                                                         /* for indenting when*/
                                                         /* displaying SDP    */
                                                         /* Data Elements.    */

#define DIRECT_CONNECTABLE_MODE                (0x0200)  /* Denotes that the  */
                                                         /* connectabillity   */
                                                         /* mode is Directed  */
                                                         /* Connectable.      */

#define LOW_DUTY_CYCLE_DIRECT_CONNECTABLE      (0x0400)  /* Denotes that the  */
                                                         /* connectabillity   */
                                                         /* mode is Low Duty  */
                                                         /* Cycle Directed    */
                                                         /* Connectable.      */
#define DEV_CLASS_LEN                               16   /* device class bitmask as a string */
#define DEV_NAME_LEN                                64   /* text string = device name */
#define DEBUG_STRING_MAX_LEN                        64   /* text string for debug */

/* The following MACRO is used to convert an ASCII character into the*/
/* equivalent decimal value.  The MACRO converts lower case          */
/* characters to upper case before the conversion.                   */
#define ToInt(_x) (((_x) > 0x39)?(((_x) & ~0x20)-0x37):((_x)-0x30))

#ifndef __BTTYPESH_INC__
typedef unsigned char Byte_t;
typedef char Boolean_t;
typedef unsigned int DWord_t;                       /* Generic 32 bit Container.  */
typedef unsigned short Word_t;                      /* Generic 16 bit Container.  */

typedef struct _tagUUID_16_t
{
   Byte_t UUID_Byte0;
   Byte_t UUID_Byte1;
} UUID_16_t;

typedef struct _tagUUID_32_t
{
   Byte_t UUID_Byte0;
   Byte_t UUID_Byte1;
   Byte_t UUID_Byte2;
   Byte_t UUID_Byte3;
} UUID_32_t;

#endif

#if !defined(__BNEPTYPEH__) && !defined(__BTBTYPESH__)
typedef struct _tagUUID_128_t
{
   Byte_t UUID_Byte0;
   Byte_t UUID_Byte1;
   Byte_t UUID_Byte2;
   Byte_t UUID_Byte3;
   Byte_t UUID_Byte4;
   Byte_t UUID_Byte5;
   Byte_t UUID_Byte6;
   Byte_t UUID_Byte7;
   Byte_t UUID_Byte8;
   Byte_t UUID_Byte9;
   Byte_t UUID_Byte10;
   Byte_t UUID_Byte11;
   Byte_t UUID_Byte12;
   Byte_t UUID_Byte13;
   Byte_t UUID_Byte14;
   Byte_t UUID_Byte15;
} UUID_128_t;

typedef struct _tagBD_ADDR_t
{
   Byte_t BD_ADDR0;
   Byte_t BD_ADDR1;
   Byte_t BD_ADDR2;
   Byte_t BD_ADDR3;
   Byte_t BD_ADDR4;
   Byte_t BD_ADDR5;
} BD_ADDR_t;
#endif

#ifndef __GATMAPIH__
typedef enum
{
   /* GATM Connection Events.                                           */
   getGATTConnected,
   getGATTDisconnected,
   getGATTConnectionMTUUpdate,
   getGATTHandleValueData,

   /* GATM Client Events.                                               */
   getGATTReadResponse,
   getGATTWriteResponse,
   getGATTErrorResponse,

   /* GATM Server Events.                                               */
   getGATTWriteRequest,
   getGATTSignedWrite,
   getGATTReadRequest,
   getGATTPrepareWriteRequest,
   getGATTCommitPrepareWrite,
   getGATTHandleValueConfirmation
} GATM_Event_Type_t;
#endif

#ifndef __GATTAPIH__
typedef void * SDP_UUID_Entry_t;

typedef enum
{
   gctLE,
   gctBR_EDR
} GATT_Connection_Type_t;

typedef struct _tagGATT_Attribute_Handle_Group_t
{
   Word_t Starting_Handle;
   Word_t Ending_Handle;
} GATT_Attribute_Handle_Group_t;

typedef enum
{
   guUUID_16,
   guUUID_128,
   guUUID_32
} GATT_UUID_Type_t;

typedef struct _tagGATT_UUID_t
{
   GATT_UUID_Type_t UUID_Type;
   union
   {
      UUID_16_t  UUID_16;
      UUID_32_t  UUID_32;
      UUID_128_t UUID_128;
   } UUID;
} GATT_UUID_t;
#endif


/* The following type definition represents the structure which holds*/
/* all information about the parameter, in particular the parameter  */
/* as a string and the parameter as an unsigned int.                 */
typedef struct _tagParameter_t
{
    char         *strParam;
    unsigned int  intParam;
} Parameter_t;

/* The following type definition represents the structure which holds*/
/* a list of parameters that are to be associated with a command The */
/* NumberofParameters variable holds the value of the number of      */
/* parameters in the list.                                           */
typedef struct _tagParameterList_t
{
    int         NumberofParameters;
    Parameter_t Params[MAX_NUM_OF_PARAMETERS];
} ParameterList_t;

/* The following type definition represents the structure which holds*/
/* the command and parameters to be executed.                        */
typedef struct _tagUserCommand_t
{
    char            *Command;
    ParameterList_t  Parameters;
} UserCommand_t;

/* The following type definition represents the generic function     */
/* pointer to be used by all commands that can be executed by the    */
/* test program.                                                     */
typedef int (*CommandFunction_t)(ParameterList_t *aParams __attribute__ ((unused)));

/* The following type definition represents the structure which holds*/
/* information used in the interpretation and execution of Commands. */
typedef struct _tagCommandTable_t
{
    char              *CommandName;
    CommandFunction_t  CommandFunction;
} CommandTable_t;

enum ConfigTag
{
    Config_EOL                  = 0, // End of l ist
    Config_ServiceTable,
    Config_LocalDeviceName,
    Config_LocalClassOfDevice,
    Config_Discoverable,
    Config_Connectable,
    Config_Pairable,
    Config_RemoteDeviceLinkActive,
    Config_LocalDeviceAppearance,
    Config_AdvertisingInterval,
    Config_AndUpdateConnectionAndScanBLEParameters,
    Config_AuthenticatedPayloadTimeout,
};

typedef struct _tagDeviceConfig_t
{
    ConfigTag       tag;
    ParameterList_t params;
} DeviceConfig_t;

/*********************************************************************/
/* Service Table Structures.                                         */
/*********************************************************************/

#define SERVICE_TABLE_FLAGS_USE_PERSISTENT_UID                 0x00000001
#define SERVICE_TABLE_FLAGS_SECONDARY_SERVICE                  0x00000002

/*********************************************************************/
/* Service Tables.                                                   */
/*********************************************************************/
/* * NOTE * For simplicity this application will not include Client  */
/*          Characteristic Configuration Descriptors (CCCD) for      */
/*          characteristics that are indicatable/notifiable.  This is*/
/*          because the CCCD is a per client value that is stored    */
/*          persistently for bonded devices.  This application, whose*/
/*          only purpose is showing the usage of the APIs, does not  */
/*          store per client values and also does not store values   */
/*          persistently.                                            */
/* * NOTE * To Calculate the AttributeOffset apply the following     */
/*          formula:                                                 */
/*                                                                   */
/*             AttributeOffset = 1 + (NumPrevIncludes * 1) +         */
/*                               (NumPrevCharacteristics * 2) +      */
/*                               (NumPrevDescriptors * 1)            */
/*                                                                   */
/*          where:                                                   */
/*                                                                   */
/*             NumPrevIncludes = The number of previous Include      */
/*                               Definition that exist in the        */
/*                               service table prior to the attribute*/
/*                               (Include, Characteristic or         */
/*                               Descriptor) that is being added.    */
/*                                                                   */
/*             NumPrevCharacteristics = The number of previous       */
/*                               Characteristics that exist in the   */
/*                               service table prior to the attribute*/
/*                               (Include, Characteristic or         */
/*                               Descriptor) that is being added.    */
/*                                                                    */
/*             NumPrevDescriptors = The number of previous           */
/*                               Descriptors that exist in the       */
/*                               service table prior to the attribute*/
/*                               (Include, Characteristic or         */
/*                               Descriptor) that is being added.    */
/*-------------------------------------------------------------------*/
/* The following type defintion represents the structure which holds */
/* information on a pending prepare write queue entry.               */
typedef struct _tagPrepareWriteEntry_t
{
    GATT_Connection_Type_t          ConnectionType;
    BD_ADDR_t                       RemoteDeviceAddress;
    unsigned int                    ServiceID;
    unsigned int                    AttributeOffset;
    unsigned int                    AttributeValueOffset;
    unsigned int                    MaximumValueLength;
    unsigned int                    ValueLength;
    Byte_t                         *Value;
    struct _tagPrepareWriteEntry_t *NextPrepareWriteEntryPtr;
} PrepareWriteEntry_t;


/* The following type definition represents the container structure  */
/* for a Characteristic Attribute.                                   */
typedef struct _tagCharacteristicInfo_t
{
    unsigned long  CharacteristicPropertiesMask;
    unsigned long  SecurityPropertiesMask;
    UUID_128_t     CharacteristicUUID;
    Boolean_t      AllocatedValue; // Note: don't replace to bool: used in C and C++ (different size) to access by pointer
    unsigned int   MaximumValueLength;
    unsigned int   ValueLength;
    Byte_t        *Value;
} CharacteristicInfo_t;

/* The following type definition represents the container structure  */
/* for a Descriptor Attribute.                                       */
typedef struct _tagDescriptorInfo_t
{
    unsigned long  DescriptorPropertiesMask;
    unsigned long  SecurityPropertiesMask;
    UUID_128_t     CharacteristicUUID;
    Boolean_t      AllocatedValue; // Note: don't replace to bool: used in C and C++ (different size) to access by pointer
    unsigned int   MaximumValueLength;
    unsigned int   ValueLength;
    Byte_t        *Value;
} DescriptorInfo_t;

/* The following enumeration represents all of the different         */
/* attributes that may be added in a service table.                  */
typedef enum
{
    atInclude,
    atCharacteristic,
    atDescriptor
} AttributeType_t;

/* The following type definition represents the container structure  */
/* for a Service Table.                                              */
/* * NOTE * For an AttributeType of atInclude the AttributeParameter */
/*          is not used (the include will automatically reference the*/
/*          previous service that was registered).                   */
typedef struct _tagAttributeInfo_t
{
    AttributeType_t  AttributeType;
    unsigned int     AttributeOffset;
    void            *Attribute;
    char            *AttributeName;
} AttributeInfo_t;

/* The following type definition represents the container structure  */
/* for an dynamically allocated service.                             */
typedef struct _tagServiceInfo_t
{
    unsigned long                  Flags;
    unsigned int                   ServiceID;
    DWord_t                        PersistentUID;
    UUID_128_t                     ServiceUUID;
    GATT_Attribute_Handle_Group_t  ServiceHandleRange;
    unsigned int                   NumberAttributes;
    AttributeInfo_t               *AttributeList;
    char                          *ServiceName;
} ServiceInfo_t;

namespace Error { enum Error {
    NONE                =  0, // NO ERROR, SUCCESS
    PARSER              = -1, // Denotes that no command was specified to the parser.
    INVALID_COMMAND     = -2, // Denotes that the Command does not exist for processing.
    EXIT_CODE           = -3, // Denotes that the Command specified was the Exit Command.
    FUNCTION            = -4, // Denotes that an error occurred in execution of the Command Function.
    TOO_MANY_PARAMS     = -5, // Denotes that there are more parameters then will fit in the UserCommand.
    INVALID_PARAMETERS  = -6, // Denotes that an error occurred due to the fact that one or more of the required parameters were invalid.
    NOT_INITIALIZED     = -7, // Denotes that an error occurred due to the fact that the Platform Manager has not been initialized.
    UNDEFINED           = -8, // Not initialized value; denotes that not all paths of the function modify return value
    NOT_IMPLEMENTED     = -9, // Not yet implemented or not supported for this target
    NOT_FOUND           = -10,// Search not found
    INVALID_STATE       = -11,// Already set or single use error
    NOT_REGISTERED      = -12,// Callback is not registered
};}

namespace Power { enum Power {
    Off = 0,
    On = 1,
};}

namespace Characteristic { enum Access {
    Read,
    Write,
    Confirmed,
};}


class BleApi
{
public:
    // some Bluetooth Appearance values
    // https://www.bluetooth.com/specifications/gatt/viewer?attributeXmlFile=org.bluetooth.characteristic.gap.appearance.xml
    enum Appearance {
        BLE_APPEARANCE_UNKNOWN                  = 0,
        BLE_APPEARANCE_GENERIC_PHONE            = 64,
        BLE_APPEARANCE_GENERIC_COMPUTER         = 128,
        BLE_APPEARANCE_GENERIC_WATCH            = 192,
        BLE_APPEARANCE_GENERIC_SENSOR           = 1344,
        BLE_APPEARANCE_MOTION_SENSOR            = 1345,
    };

    typedef void (*onCharacteristicAccessCallback) (int aServiceIdx, int aAttribueIdx, Characteristic::Access aAccessType);

    ~BleApi();

protected:
    BleApi();
    bool          mInitialized;      // initialization state

public:
    virtual int Initialize() = 0;
    virtual int SetDevicePower(bool aPowerOn) = 0;
    virtual int ShutdownService() = 0;
    virtual int Shutdown() = 0;

    // local device state queries
    virtual int QueryDevicePower() = 0;
    virtual int QueryLocalRemoteDebugZoneMask(ParameterList_t *aParams __attribute__ ((unused))) = 0;
    virtual int QueryLocalDeviceProperties(ParameterList_t *aParams __attribute__ ((unused))) = 0;

    // configuration
    virtual int Configure(DeviceConfig_t* aConfig) = 0;

    virtual int RegisterEventCallback(ParameterList_t *aParams __attribute__ ((unused))) = 0;
    virtual int UnRegisterEventCallback(ParameterList_t *aParams __attribute__ ((unused))) = 0;
    virtual int RegisterCharacteristicAccessCallback(onCharacteristicAccessCallback aCb) = 0;
    virtual int UnregisterCharacteristicAccessCallback(onCharacteristicAccessCallback aCb) = 0;

    // discovery
    virtual int StartDeviceDiscovery(ParameterList_t *aParams __attribute__ ((unused))) = 0;
    virtual int StopDeviceDiscovery(ParameterList_t *aParams __attribute__ ((unused))) = 0;

    // remote device operations
    virtual int QueryRemoteDeviceList(ParameterList_t *aParams __attribute__ ((unused))) = 0;
    virtual int QueryRemoteDeviceProperties(ParameterList_t *aParams __attribute__ ((unused))) = 0;
    virtual int AddRemoteDevice(ParameterList_t *aParams __attribute__ ((unused))) = 0;
    virtual int DeleteRemoteDevice(ParameterList_t *aParams __attribute__ ((unused))) = 0;
    virtual int UpdateRemoteDeviceApplicationData(ParameterList_t *aParams __attribute__ ((unused))) = 0;
    virtual int DeleteRemoteDevices(ParameterList_t *aParams __attribute__ ((unused))) = 0;
    virtual int PairWithRemoteDevice(ParameterList_t *aParams __attribute__ ((unused))) = 0;
    virtual int CancelPairWithRemoteDevice(ParameterList_t *aParams __attribute__ ((unused))) = 0;
    virtual int UnPairRemoteDevice(ParameterList_t *aParams __attribute__ ((unused))) = 0;
    virtual int QueryRemoteDeviceServices(ParameterList_t *aParams __attribute__ ((unused))) = 0;
    virtual int QueryRemoteDeviceServiceSupported(ParameterList_t *aParams __attribute__ ((unused))) = 0;
    virtual int QueryRemoteDevicesForService(ParameterList_t *aParams __attribute__ ((unused))) = 0;
    virtual int QueryRemoteDeviceServiceClasses(ParameterList_t *aParams __attribute__ ((unused))) = 0;

    // connection and security
    virtual int EnableSCOnly(ParameterList_t *aParams __attribute__ ((unused))) = 0;
    virtual int RegenerateP256LocalKeys(ParameterList_t *aParams __attribute__ ((unused))) = 0;
    virtual int OOBGenerateParameters(ParameterList_t *aParams __attribute__ ((unused))) = 0;
    virtual int AuthenticateRemoteDevice(ParameterList_t *aParams __attribute__ ((unused))) = 0;
    virtual int ConnectWithRemoteDevice(ParameterList_t *aParams __attribute__ ((unused))) = 0;
    virtual int DisconnectRemoteDevice(ParameterList_t *aParams __attribute__ ((unused))) = 0;
    virtual int EncryptRemoteDevice(ParameterList_t *aParams __attribute__ ((unused))) = 0;
    virtual int SetRemoteDeviceLinkActive(ParameterList_t *aParams __attribute__ ((unused))) = 0;

    // session description protocol
    virtual int CreateSDPRecord(ParameterList_t *aParams __attribute__ ((unused))) = 0;
    virtual int DeleteSDPRecord(ParameterList_t *aParams __attribute__ ((unused))) = 0;
    virtual int AddSDPAttribute(ParameterList_t *aParams __attribute__ ((unused))) = 0;
    virtual int DeleteSDPAttribute(ParameterList_t *aParams __attribute__ ((unused))) = 0;

    virtual int RegisterAuthentication(ParameterList_t *aParams __attribute__ ((unused))) = 0;
    virtual int UnRegisterAuthentication(ParameterList_t *aParams __attribute__ ((unused))) = 0;

    virtual int PINCodeResponse(ParameterList_t *aParams __attribute__ ((unused))) = 0;
    virtual int PassKeyResponse(ParameterList_t *aParams __attribute__ ((unused))) = 0;
    virtual int UserConfirmationResponse(ParameterList_t *aParams __attribute__ ((unused))) = 0;
    virtual int ChangeSimplePairingParameters(ParameterList_t *aParams __attribute__ ((unused))) = 0;
    virtual int ChangeLEPairingParameters(ParameterList_t *aParams __attribute__ ((unused))) = 0;

    // GATT
    virtual int RegisterGATMEventCallback(ParameterList_t *aParams __attribute__ ((unused))) = 0;
    virtual int UnRegisterGATMEventCallback(ParameterList_t *aParams __attribute__ ((unused))) = 0;
    virtual int GATTQueryConnectedDevices(ParameterList_t *aParams __attribute__ ((unused))) = 0;
    virtual int GATTRegisterService(ParameterList_t *aParams __attribute__ ((unused))) = 0;
    virtual int GATTUnRegisterService(ParameterList_t *aParams __attribute__ ((unused))) = 0;
    virtual int GATTUpdateCharacteristic(unsigned int aServiceID, int aAttrOffset, Byte_t *aAttrData, int aAttrLen) = 0;
    virtual int GATTIndicateCharacteristic(ParameterList_t *aParams __attribute__ ((unused))) = 0;
    virtual int GATTNotifyCharacteristic(ParameterList_t *aParams __attribute__ ((unused))) = 0;
    virtual int NotifyCharacteristic(int aServiceIdx, int aAttributeIdx, const char* aStrPayload) = 0;
    virtual int ListCharacteristics(ParameterList_t *aParams __attribute__ ((unused))) = 0;
    virtual int ListDescriptors(ParameterList_t *aParams __attribute__ ((unused))) = 0;
    virtual int GATTQueryPublishedServices(ParameterList_t *aParams __attribute__ ((unused))) = 0;

    // Advertising
    virtual int SetAdvertisingInterval(ParameterList_t *aParams __attribute__ ((unused))) = 0;
    virtual int StartAdvertising(ParameterList_t *aParams __attribute__ ((unused))) = 0;
    virtual int StopAdvertising(ParameterList_t *aParams __attribute__ ((unused))) = 0;

    virtual int SetAuthenticatedPayloadTimeout(ParameterList_t *aParams __attribute__ ((unused))) = 0;
    virtual int QueryAuthenticatedPayloadTimeout(ParameterList_t *aParams __attribute__ ((unused))) = 0;
    virtual int SetAndUpdateConnectionAndScanBLEParameters(ParameterList_t *aParams __attribute__ ((unused))) = 0;

    // debug
    virtual int EnableBluetoothDebug(ParameterList_t *aParams __attribute__ ((unused))) = 0;

    // debug / display functions and helper functions
    virtual void DisplayGATTUUID(GATT_UUID_t *UUID, const char *Prefix, unsigned int Level) = 0;
    virtual void DisplayAttributeValue(unsigned int aServiceIdx, unsigned int aAttributeIdx) = 0;

    virtual void BD_ADDRToStr(BD_ADDR_t Board_Address, char *BoardStr) = 0;
    virtual void StrToBD_ADDR(char *BoardStr, BD_ADDR_t *Board_Address) = 0;
    virtual void StrToUUIDEntry(char *UUIDStr, SDP_UUID_Entry_t *UUIDEntry) = 0;
    virtual void DumpData(Boolean_t String, unsigned int Length, Byte_t *Data) = 0;
    virtual char *GetServiceNameById(unsigned int ServiceID) = 0;
    virtual int GetServiceIndexById(unsigned int ServiceID) = 0;
    virtual AttributeInfo_t *SearchServiceListByOffset(unsigned int ServiceID, unsigned int AttributeOffset) = 0;
    virtual int GetAttributeIdxByOffset(unsigned int ServiceID, unsigned int AttributeOffset) = 0;
    virtual int ProcessRegisteredCallback(GATM_Event_Type_t aEventType, int aServiceID, int aAttrOffset) = 0;
    virtual void SaveRemoteDeviceAddress(BD_ADDR_t aConnectedMACAddress) = 0;
};

} } /* namespace Ring::Ble */


#endif // _RING_BLE_API_H_
