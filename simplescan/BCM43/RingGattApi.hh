#ifndef _RING_GATT_API_H_
#define _RING_GATT_API_H_

#ifndef BCM43
#error WRONG RingGattSrv platform-related file included into build
#endif

#include <stdio.h>
#include <stdlib.h>

#include <bluetooth/bluetooth.h>
#include <bluetooth/hci.h>
#include <bluetooth/hci_lib.h>

#include "RingBleApi.hh"

namespace Ring { namespace Ble {

class GattSrv : BleApi
{
public:
    static BleApi* getInstance();
    static const int IOCAPABILITIESSTRINGS_SIZE;
    static const char *IOCapabilitiesStrings[];

private:
    static GattSrv* instance;
    GattSrv();

public:
    int Initialize();
    int Configure(DeviceConfig_t* aConfig);

    int QueryDevicePower() { return NOT_IMPLEMENTED_ERROR; }
    int SetDevicePower(bool aPowerOn) { (void) aPowerOn; return NOT_IMPLEMENTED_ERROR; }

    int ShutdownService() { return NOT_IMPLEMENTED_ERROR; }
    int Shutdown();

    int Initialize(ParameterList_t *aParams __attribute__ ((unused))) { return NOT_IMPLEMENTED_ERROR; }
    int SetDevicePower(ParameterList_t *aParams __attribute__ ((unused))) { return NOT_IMPLEMENTED_ERROR; }
    int QueryDevicePower(ParameterList_t *aParams __attribute__ ((unused))) { return NOT_IMPLEMENTED_ERROR; }
    int SetLocalDeviceName(ParameterList_t *aParams __attribute__ ((unused))) { return NOT_IMPLEMENTED_ERROR; }
    int SetLocalClassOfDevice(ParameterList_t *aParams __attribute__ ((unused))) { return NOT_IMPLEMENTED_ERROR; }
    int SetDiscoverable(ParameterList_t *aParams __attribute__ ((unused))) { return NOT_IMPLEMENTED_ERROR; }
    int SetConnectable(ParameterList_t *aParams __attribute__ ((unused))) { return NOT_IMPLEMENTED_ERROR; }
    int SetPairable(ParameterList_t *aParams __attribute__ ((unused))) { return NOT_IMPLEMENTED_ERROR; }
    int ShutdownService(ParameterList_t *aParams __attribute__ ((unused))) { return NOT_IMPLEMENTED_ERROR; }
    int Shutdown(ParameterList_t *aParams __attribute__ ((unused))) { return NOT_IMPLEMENTED_ERROR; }

    int RegisterEventCallback(ParameterList_t *aParams __attribute__ ((unused))) { return NOT_IMPLEMENTED_ERROR; }
    int UnRegisterEventCallback(ParameterList_t *aParams __attribute__ ((unused))) { return NOT_IMPLEMENTED_ERROR; }
    int RegisterCharacteristicAccessCallback(onCharacteristicAccessCallback aCb) { (void) aCb; return NOT_IMPLEMENTED_ERROR; }
    int UnregisterCharacteristicAccessCallback(onCharacteristicAccessCallback aCb) { (void) aCb; return NOT_IMPLEMENTED_ERROR; }

    int SetLocalRemoteDebugZoneMask(ParameterList_t *aParams __attribute__ ((unused))) { return NOT_IMPLEMENTED_ERROR; }
    int QueryLocalRemoteDebugZoneMask(ParameterList_t *aParams __attribute__ ((unused))) { return NOT_IMPLEMENTED_ERROR; }
    int SetDebugZoneMaskPID(ParameterList_t *aParams __attribute__ ((unused))) { return NOT_IMPLEMENTED_ERROR; }
    int QueryLocalDeviceProperties(ParameterList_t *aParams __attribute__ ((unused))) { return NOT_IMPLEMENTED_ERROR; }
    int SetLocalDeviceAppearance(ParameterList_t *aParams __attribute__ ((unused))) { return NOT_IMPLEMENTED_ERROR; }

    // discovery
    int StartDeviceDiscovery(ParameterList_t *aParams __attribute__ ((unused))) { return NOT_IMPLEMENTED_ERROR; }
    int StopDeviceDiscovery(ParameterList_t *aParams __attribute__ ((unused))) { return NOT_IMPLEMENTED_ERROR; }

    // remote device operations
    int QueryRemoteDeviceList(ParameterList_t *aParams __attribute__ ((unused))) { return NOT_IMPLEMENTED_ERROR; }
    int QueryRemoteDeviceProperties(ParameterList_t *aParams __attribute__ ((unused))) { return NOT_IMPLEMENTED_ERROR; }
    int AddRemoteDevice(ParameterList_t *aParams __attribute__ ((unused))) { return NOT_IMPLEMENTED_ERROR; }
    int DeleteRemoteDevice(ParameterList_t *aParams __attribute__ ((unused))) { return NOT_IMPLEMENTED_ERROR; }
    int UpdateRemoteDeviceApplicationData(ParameterList_t *aParams __attribute__ ((unused))) { return NOT_IMPLEMENTED_ERROR; }
    int DeleteRemoteDevices(ParameterList_t *aParams __attribute__ ((unused))) { return NOT_IMPLEMENTED_ERROR; }
    int PairWithRemoteDevice(ParameterList_t *aParams __attribute__ ((unused))) { return NOT_IMPLEMENTED_ERROR; }
    int CancelPairWithRemoteDevice(ParameterList_t *aParams __attribute__ ((unused))) { return NOT_IMPLEMENTED_ERROR; }
    int UnPairRemoteDevice(ParameterList_t *aParams __attribute__ ((unused))) { return NOT_IMPLEMENTED_ERROR; }
    int QueryRemoteDeviceServices(ParameterList_t *aParams __attribute__ ((unused))) { return NOT_IMPLEMENTED_ERROR; }
    int QueryRemoteDeviceServiceSupported(ParameterList_t *aParams __attribute__ ((unused))) { return NOT_IMPLEMENTED_ERROR; }
    int QueryRemoteDevicesForService(ParameterList_t *aParams __attribute__ ((unused))) { return NOT_IMPLEMENTED_ERROR; }
    int QueryRemoteDeviceServiceClasses(ParameterList_t *aParams __attribute__ ((unused))) { return NOT_IMPLEMENTED_ERROR; }

    // connection and security
    int EnableSCOnly(ParameterList_t *aParams __attribute__ ((unused))) { return NOT_IMPLEMENTED_ERROR; }
    int RegenerateP256LocalKeys(ParameterList_t *aParams __attribute__ ((unused))) { return NOT_IMPLEMENTED_ERROR; }
    int OOBGenerateParameters(ParameterList_t *aParams __attribute__ ((unused))) { return NOT_IMPLEMENTED_ERROR; }
    int AuthenticateRemoteDevice(ParameterList_t *aParams __attribute__ ((unused))) { return NOT_IMPLEMENTED_ERROR; }
    int ConnectWithRemoteDevice(ParameterList_t *aParams __attribute__ ((unused))) { return NOT_IMPLEMENTED_ERROR; }
    int DisconnectRemoteDevice(ParameterList_t *aParams __attribute__ ((unused))) { return NOT_IMPLEMENTED_ERROR; }
    int EncryptRemoteDevice(ParameterList_t *aParams __attribute__ ((unused))) { return NOT_IMPLEMENTED_ERROR; }
    int SetRemoteDeviceLinkActive(ParameterList_t *aParams __attribute__ ((unused))) { return NOT_IMPLEMENTED_ERROR; }

    // Service Discovery Protocol (SDP)
    int CreateSDPRecord(ParameterList_t *aParams __attribute__ ((unused))) { return NOT_IMPLEMENTED_ERROR; }
    int DeleteSDPRecord(ParameterList_t *aParams __attribute__ ((unused))) { return NOT_IMPLEMENTED_ERROR; }
    int AddSDPAttribute(ParameterList_t *aParams __attribute__ ((unused))) { return NOT_IMPLEMENTED_ERROR; }
    int DeleteSDPAttribute(ParameterList_t *aParams __attribute__ ((unused))) { return NOT_IMPLEMENTED_ERROR; }

    int RegisterAuthentication(ParameterList_t *aParams __attribute__ ((unused))) { return NOT_IMPLEMENTED_ERROR; }
    int UnRegisterAuthentication(ParameterList_t *aParams __attribute__ ((unused))) { return NOT_IMPLEMENTED_ERROR; }

    int PINCodeResponse(ParameterList_t *aParams __attribute__ ((unused))) { return NOT_IMPLEMENTED_ERROR; }
    int PassKeyResponse(ParameterList_t *aParams __attribute__ ((unused))) { return NOT_IMPLEMENTED_ERROR; }
    int UserConfirmationResponse(ParameterList_t *aParams __attribute__ ((unused))) { return NOT_IMPLEMENTED_ERROR; }
    int ChangeSimplePairingParameters(ParameterList_t *aParams __attribute__ ((unused))) { return NOT_IMPLEMENTED_ERROR; }
    int ChangeLEPairingParameters(ParameterList_t *aParams __attribute__ ((unused))) { return NOT_IMPLEMENTED_ERROR; }

    // GATT
    int RegisterGATMEventCallback(ParameterList_t *aParams __attribute__ ((unused))) { return NOT_IMPLEMENTED_ERROR; }
    int UnRegisterGATMEventCallback(ParameterList_t *aParams __attribute__ ((unused))) { return NOT_IMPLEMENTED_ERROR; }
    int GATTQueryConnectedDevices(ParameterList_t *aParams __attribute__ ((unused))) { return NOT_IMPLEMENTED_ERROR; }
    int GATTRegisterService(ParameterList_t *aParams __attribute__ ((unused))) { return NOT_IMPLEMENTED_ERROR; }
    int GATTUnRegisterService(ParameterList_t *aParams __attribute__ ((unused))) { return NOT_IMPLEMENTED_ERROR; }
    int GATTIndicateCharacteristic(ParameterList_t *aParams __attribute__ ((unused))) { return NOT_IMPLEMENTED_ERROR; }
    int GATTNotifyCharacteristic(ParameterList_t *aParams __attribute__ ((unused))) { return NOT_IMPLEMENTED_ERROR; }
    int ListCharacteristics(ParameterList_t *aParams __attribute__ ((unused))) { return NOT_IMPLEMENTED_ERROR; }
    int ListDescriptors(ParameterList_t *aParams __attribute__ ((unused))) { return NOT_IMPLEMENTED_ERROR; }
    int GATTQueryPublishedServices(ParameterList_t *aParams __attribute__ ((unused))) { return NOT_IMPLEMENTED_ERROR; }

    // Advertising
    int SetAdvertisingInterval(ParameterList_t *aParams __attribute__ ((unused))) { return NOT_IMPLEMENTED_ERROR; }
    int StartAdvertising(ParameterList_t *aParams __attribute__ ((unused))) { return NOT_IMPLEMENTED_ERROR; }
    int StopAdvertising(ParameterList_t *aParams __attribute__ ((unused))) { return NOT_IMPLEMENTED_ERROR; }

    int SetAuthenticatedPayloadTimeout(ParameterList_t *aParams __attribute__ ((unused))) { return NOT_IMPLEMENTED_ERROR; }
    int QueryAuthenticatedPayloadTimeout(ParameterList_t *aParams __attribute__ ((unused))) { return NOT_IMPLEMENTED_ERROR; }
    int SetAndUpdateConnectionAndScanBLEParameters(ParameterList_t *aParams __attribute__ ((unused))) { return NOT_IMPLEMENTED_ERROR; }

    // debug
    int EnableBluetoothDebug(ParameterList_t *aParams __attribute__ ((unused))) { return NOT_IMPLEMENTED_ERROR; }

private:
    struct hci_dev_info di;
    char mDeviceClass[DEV_CLASS_LEN];
    char mDeviceName[DEV_NAME_LEN];

    int open_socket(struct hci_dev_info &di);
    void print_dev_hdr(struct hci_dev_info *di);
    void print_pkt_type(struct hci_dev_info *di);
    void print_link_policy(struct hci_dev_info *di);
    void print_link_mode(struct hci_dev_info *di);
    void print_dev_features(struct hci_dev_info *di, int format);
    void print_le_states(uint64_t states);
    char *get_minor_device_name(int major, int minor);

public:
    // HCI methods
    void HCIrstat(int ctl, int hdev);
    void HCIscan(int ctl, int hdev, char *opt);
    void HCIle_addr(int hdev, char *opt);
    void HCIle_adv(int hdev, char *opt);
    void HCIno_le_adv(int hdev);
    void HCIle_states(int hdev);
    void HCIiac(int hdev, char *opt);
    void HCIauth(int ctl, int hdev, char *opt);
    void HCIencrypt(int ctl, int hdev, char *opt);
    void HCIup(int ctl, int hdev);
    void HCIdown(int ctl, int hdev);
    void HCIreset(int ctl, int hdev);
    void HCIptype(int ctl, int hdev, char *opt);
    void HCIlp(int ctl, int hdev, char *opt);
    void HCIlm(int ctl, int hdev, char *opt);
    void HCIaclmtu(int ctl, int hdev, char *opt);
    void HCIscomtu(int ctl, int hdev, char *opt);
    void HCIfeatures(int hdev);
    void HCIname(int hdev, char *opt);
    void HCIclass(int hdev, char *opt);
    void HCIvoice(int hdev, char *opt);
    void HCIdelkey(int hdev, char *opt);
    void HCIoob_data(int hdev);
    void HCIcommands(int hdev);
    void HCIversion(int hdev);
    void HCIinq_tpl(int hdev, char *opt);
    void HCIinq_mode(int hdev, char *opt);
    void HCIinq_data(int hdev, char *opt);
    void HCIinq_type(int hdev, char *opt);
    void HCIinq_parms(int hdev, char *opt);
    void HCIpage_parms(int hdev, char *opt);
    void HCIpage_to(int hdev, char *opt);
    void HCIafh_mode(int hdev, char *opt);
    void HCIssp_mode(int hdev, char *opt);
    void HCIrevision(int hdev);
    void HCIblock(int hdev, char *opt);
    void HCIunblock(int hdev, char *opt);
};

} } /* namespace Ring::Ble */


#endif // _RING_GATT_API_H_
