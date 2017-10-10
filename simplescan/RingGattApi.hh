#ifndef _RING_GATT_API_H_
#define _RING_GATT_API_H_

#include <stdio.h>
#include <stdlib.h>

#include "RingBleApi.hh"
extern "C" {

#include "SS1BTPM.h"
#include "BTPMMAPI.h"

}   // extern "C"

#include "RingBleApi.hh"

namespace Ring { namespace Ble {

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
/*                                                                   */
/*             NumPrevDescriptors = The number of previous           */
/*                               Descriptors that exist in the       */
/*                               service table prior to the attribute*/
/*                               (Include, Characteristic or         */
/*                               Descriptor) that is being added.    */

/* The following type definition represents the container structure  */
/* for a Characteristic Attribute.                                   */
typedef struct _tagCharacteristicInfo_t
{
    unsigned long  CharacteristicPropertiesMask;
    unsigned long  SecurityPropertiesMask;
    UUID_128_t     CharacteristicUUID;
    bool      AllocatedValue;
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
    bool      AllocatedValue;
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
} ServiceInfo_t;

class GattSrv : BleApi
{
public:
    static BleApi* getInstance();
    static void BTPSAPI ServerUnRegistrationCallback(void *CallbackParameter __attribute__ ((unused)));

private:
    static GattSrv* instance;
    GattSrv();

public:
    int Initialize(ParameterList_t *aParams __attribute__ ((unused)));
    int SetDevicePower(ParameterList_t *aParams __attribute__ ((unused)));
    int QueryDevicePower(ParameterList_t *aParams __attribute__ ((unused)));
    int SetLocalDeviceName(ParameterList_t *aParams __attribute__ ((unused)));
    int SetLocalClassOfDevice(ParameterList_t *aParams __attribute__ ((unused)));
    int SetDiscoverable(ParameterList_t *aParams __attribute__ ((unused)));
    int SetConnectable(ParameterList_t *aParams __attribute__ ((unused)));
    int SetPairable(ParameterList_t *aParams __attribute__ ((unused)));
    int ShutdownService(ParameterList_t *aParams __attribute__ ((unused)));
    int Cleanup(ParameterList_t *aParams __attribute__ ((unused)));

    int RegisterEventCallback(ParameterList_t *aParams __attribute__ ((unused)));
    int UnRegisterEventCallback(ParameterList_t *aParams __attribute__ ((unused)));

    int SetLocalRemoteDebugZoneMask(ParameterList_t *aParams __attribute__ ((unused)));
    int QueryLocalRemoteDebugZoneMask(ParameterList_t *aParams __attribute__ ((unused)));
    int SetDebugZoneMaskPID(ParameterList_t *aParams __attribute__ ((unused)));
    int QueryLocalDeviceProperties(ParameterList_t *aParams __attribute__ ((unused)));
    int SetLocalDeviceAppearance(ParameterList_t *aParams __attribute__ ((unused)));

    // discovery
    int StartDeviceDiscovery(ParameterList_t *aParams __attribute__ ((unused)));
    int StopDeviceDiscovery(ParameterList_t *aParams __attribute__ ((unused)));

    // remote device operations
    int QueryRemoteDeviceList(ParameterList_t *aParams __attribute__ ((unused)));
    int QueryRemoteDeviceProperties(ParameterList_t *aParams __attribute__ ((unused)));
    int AddRemoteDevice(ParameterList_t *aParams __attribute__ ((unused)));
    int DeleteRemoteDevice(ParameterList_t *aParams __attribute__ ((unused)));
    int UpdateRemoteDeviceApplicationData(ParameterList_t *aParams __attribute__ ((unused)));
    int DeleteRemoteDevices(ParameterList_t *aParams __attribute__ ((unused)));
    int PairWithRemoteDevice(ParameterList_t *aParams __attribute__ ((unused)));
    int CancelPairWithRemoteDevice(ParameterList_t *aParams __attribute__ ((unused)));
    int UnPairRemoteDevice(ParameterList_t *aParams __attribute__ ((unused)));
    int QueryRemoteDeviceServices(ParameterList_t *aParams __attribute__ ((unused)));
    int QueryRemoteDeviceServiceSupported(ParameterList_t *aParams __attribute__ ((unused)));
    int QueryRemoteDevicesForService(ParameterList_t *aParams __attribute__ ((unused)));
    int QueryRemoteDeviceServiceClasses(ParameterList_t *aParams __attribute__ ((unused)));

    // connection and security
    int EnableSCOnly(ParameterList_t *aParams __attribute__ ((unused)));
    int RegenerateP256LocalKeys(ParameterList_t *aParams __attribute__ ((unused)));
    int OOBGenerateParameters(ParameterList_t *aParams __attribute__ ((unused)));
    int AuthenticateRemoteDevice(ParameterList_t *aParams __attribute__ ((unused)));
    int ConnectWithRemoteDevice(ParameterList_t *aParams __attribute__ ((unused)));
    int DisconnectRemoteDevice(ParameterList_t *aParams __attribute__ ((unused)));
    int EncryptRemoteDevice(ParameterList_t *aParams __attribute__ ((unused)));
    int SetRemoteDeviceLinkActive(ParameterList_t *aParams __attribute__ ((unused)));

    // Service Discovery Protocol (SDP)
    int CreateSDPRecord(ParameterList_t *aParams __attribute__ ((unused)));
    int DeleteSDPRecord(ParameterList_t *aParams __attribute__ ((unused)));
    int AddSDPAttribute(ParameterList_t *aParams __attribute__ ((unused)));
    int DeleteSDPAttribute(ParameterList_t *aParams __attribute__ ((unused)));

    int RegisterAuthentication(ParameterList_t *aParams __attribute__ ((unused)));
    int UnRegisterAuthentication(ParameterList_t *aParams __attribute__ ((unused)));

    int PINCodeResponse(ParameterList_t *aParams __attribute__ ((unused)));
    int PassKeyResponse(ParameterList_t *aParams __attribute__ ((unused)));
    int UserConfirmationResponse(ParameterList_t *aParams __attribute__ ((unused)));
    int ChangeSimplePairingParameters(ParameterList_t *aParams __attribute__ ((unused)));
    int ChangeLEPairingParameters(ParameterList_t *aParams __attribute__ ((unused)));

    // GATT
    int RegisterGATMEventCallback(ParameterList_t *aParams __attribute__ ((unused)));
    int UnRegisterGATMEventCallback(ParameterList_t *aParams __attribute__ ((unused)));
    int GATTQueryConnectedDevices(ParameterList_t *aParams __attribute__ ((unused)));
    int GATTRegisterService(ParameterList_t *aParams __attribute__ ((unused)));
    int GATTUnRegisterService(ParameterList_t *aParams __attribute__ ((unused)));
    int GATTIndicateCharacteristic(ParameterList_t *aParams __attribute__ ((unused)));
    int GATTNotifyCharacteristic(ParameterList_t *aParams __attribute__ ((unused)));
    int ListCharacteristics(ParameterList_t *aParams __attribute__ ((unused)));
    int ListDescriptors(ParameterList_t *aParams __attribute__ ((unused)));
    int GATTQueryPublishedServices(ParameterList_t *aParams __attribute__ ((unused)));

    // Advertising
    int SetAdvertisingInterval(ParameterList_t *aParams __attribute__ ((unused)));
    int StartAdvertising(ParameterList_t *aParams __attribute__ ((unused)));
    int StopAdvertising(ParameterList_t *aParams __attribute__ ((unused)));

    int SetAuthenticatedPayloadTimeout(ParameterList_t *aParams __attribute__ ((unused)));
    int QueryAuthenticatedPayloadTimeout(ParameterList_t *aParams __attribute__ ((unused)));
    int SetAndUpdateConnectionAndScanBLEParameters(ParameterList_t *aParams __attribute__ ((unused)));

    // debug
    int EnableBluetoothDebug(ParameterList_t *aParams __attribute__ ((unused)));

    // configuration and get/set functions
    void SetCurrentRemoteBD_ADDR(BD_ADDR_t aCurrentRemoteBD_ADDR);
    BD_ADDR_t GetCurrentRemoteBD_ADDR();
    void SetCurrentLowEnergy(bool aCurrentLowEnergy);
    bool GetCurrentLowEnergy();
    unsigned int GetDEVMCallbackID();
    unsigned int GetGATMCallbackID();
    unsigned int GetAuthenticationCallbackID();
    GAP_IO_Capability_t GetIOCapability();
    GAP_LE_IO_Capability_t GetLEIOCapability();
    GAP_LE_Bonding_Type_t GetBondingType();
    bool GetOOBSupport();
    bool GetSC();
    bool GetKeypress();
    bool GetMITMProtection();
    bool GetLEMITMProtection();
    bool GetP256DebugMode();

    void CopyRandomValues();
    void CopyConfirmValues();
    void CopyRandomValues(void * dst);
    void CopyConfirmValues(void * dst);

    // GATT processing functions
    void ProcessReadRequestEvent(GATM_Read_Request_Event_Data_t *ReadRequestData);
    void ProcessWriteRequestEvent(GATM_Write_Request_Event_Data_t *WriteRequestData);
    void ProcessSignedWriteEvent(GATM_Signed_Write_Event_Data_t *SignedWriteData);
    void ProcessPrepareWriteRequestEvent(GATM_Prepare_Write_Request_Event_Data_t *PrepareWriteRequestData);
    void ProcessCommitPrepareWriteEvent(GATM_Commit_Prepare_Write_Event_Data_t *CommitPrepareWriteData);

    // debug / display functions
    void DisplayGATTUUID(GATT_UUID_t *UUID, const char *Prefix, unsigned int Level);
    void DisplayParsedGATTServiceData(DEVM_Parsed_Services_Data_t *ParsedGATTData);
    void DisplayParsedSDPServiceData(DEVM_Parsed_SDP_Data_t *ParsedSDPData);
    void DisplaySDPAttributeResponse(SDP_Service_Attribute_Response_Data_t *SDPServiceAttributeResponse, unsigned int InitLevel);
    void DisplayDataElement(SDP_Data_Element_t *SDPDataElement, unsigned int Level);
    void DisplayLocalDeviceProperties(unsigned long UpdateMask, DEVM_Local_Device_Properties_t *LocalDeviceProperties);
    void DisplayRemoteDeviceProperties(unsigned long UpdateMask, DEVM_Remote_Device_Properties_t *RemoteDeviceProperties);
    void BD_ADDRToStr(BD_ADDR_t Board_Address, char *BoardStr);
    void StrToBD_ADDR(char *BoardStr, BD_ADDR_t *Board_Address);
    void StrToUUIDEntry(char *UUIDStr, SDP_UUID_Entry_t *UUIDEntry);
    void DumpData(bool String, unsigned int Length, Byte_t *Data);

private:
    unsigned int   mDEVMCallbackID;            // callback ID of the currently registered Device Manager
    unsigned int   mGATMCallbackID;            // Callback ID of the currently registered Generic Attribute Profile Manager Event Callback.
    unsigned int   mAuthenticationCallbackID;  // current Authentication Callback ID that is assigned from the Device Manager when the local
    unsigned int   mServiceCount;              // the current number of services passed to register
    ServiceInfo_t  *mServiceTable;             // pointer to populated service tbl
                                               // client registers for Authentication.
    bool           mOOBSupport;                // whether or not Out of Band Secure Simple Pairing exchange is supported.
    bool           mSC;                        // whether or not SC protection is to be requested during a Pairing procedure.
    BD_ADDR_t      mCurrentRemoteBD_ADDR;      // current BD_ADDR of the device which is currently pairing or authenticating.
    bool           mCurrentLowEnergy;          // current LE state of the device  which is currently pairing or authenticating.
    GAP_IO_Capability_t mIOCapability;         // current I/O Capabilities that are to be used for Secure SimplePairing.
    GAP_LE_IO_Capability_t	mLEIOCapability;   // current I/O Capabilities that are to be used for LE pairing
    GAP_LE_Bonding_Type_t	mBondingType;      // Variable which holds the currentBonding Type that is going to   be used for LE pairing
    bool           mKeypress;                  // Variable which flags whether or not sending and recieving Keypress notification during pairing is supported.
    bool           mMITMProtection;            // Variable which flags whether or not Man in the Middle (MITM) protection is to be requested during a Secure Simple Pairing procedure.
    bool           mLEMITMProtection;          // Variable which flags whether or not Man in the Middle (MITM) protection is to be requested during a Simple Pairing procedure.
    bool           mP256DebugMode;             // Variable which flags whether or not to use P-256 debug key during a SC Pairing procedure.
    Mutex_t        mServiceMutex = NULL;       // Mutex which guards access to theService List.
    PrepareWriteEntry_t *mPrepareWriteList = NULL;   // Pointer to head of list containing all currently pendingprepared writes.

    // private helper functions
    AttributeInfo_t *SearchServiceListByOffset(unsigned int ServiceID, unsigned int AttributeOffset);
    void CleanupServiceList(void);
    void FreeServerList();

    unsigned int CalculateNumberAttributes(ServiceInfo_t *ServiceInfo);
    int RegisterService(unsigned int ServiceIndex);
    int UnRegisterService(unsigned int ServiceIndex);

    void FreePrepareWriteEntryEntryMemory(PrepareWriteEntry_t *EntryToFree);
    void FreePrepareWriteEntryList(PrepareWriteEntry_t **ListHead);
    PrepareWriteEntry_t *CombinePrepareWrite(PrepareWriteEntry_t **ListHead, GATM_Prepare_Write_Request_Event_Data_t *PrepareWriteRequestData);
    bool AddPrepareWriteEntry(PrepareWriteEntry_t **ListHead, PrepareWriteEntry_t *EntryToAdd);
    PrepareWriteEntry_t *DeletePrepareWriteEntry(PrepareWriteEntry_t **ListHead, GATT_Connection_Type_t ConnectionType, BD_ADDR_t RemoteDeviceAddress, unsigned int ServiceID);
    PrepareWriteEntry_t *DeletePrepareWriteEntryByPtr(PrepareWriteEntry_t **ListHead, PrepareWriteEntry_t *PrepareWriteEntry);

    // for LE SC pairing, when using the OOB association method
    SM_Random_Value_t  mOOBLocalRandom;
    SM_Random_Value_t  mOOBRemoteRandom;
    SM_Confirm_Value_t mOOBLocalConfirmation;
    SM_Confirm_Value_t mOOBRemoteConfirmation;
};

} } /* namespace Ring::Ble */


#endif // _RING_GATT_API_H_
