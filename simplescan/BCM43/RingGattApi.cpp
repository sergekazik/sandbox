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
#ifndef BCM43
#error WRONG RingGattSrv platform-related file included into build
#endif
#if !defined(BLUEZ_TOOLS_SUPPORT)
#error REQUIRED CONFIG_AMBARELLA_BLUEZ_TOOLS_SUPPORT and CONFIG_AMBARELLA_BLUETOOTH5_TOOL_SUPPORT
#endif

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <errno.h>
#include <getopt.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/param.h>
#include <sys/socket.h>
#include <unistd.h>
#include <pthread.h>

#include "Bot_Notifier.h"
#include "RingGattApi.hh"

extern "C" {
    #include "lib/bluetooth.h"
    #include "lib/hci.h"
    #include "lib/hci_lib.h"
    #include "lib/l2cap.h"
    #include "lib/uuid.h"

    #include "src/shared/mainloop.h"
    #include "src/shared/util.h"
    #include "src/shared/att.h"
    #include "src/shared/queue.h"
    #include "src/shared/timeout.h"
    #include "src/shared/gatt-server.h"

    #include "gatt_db.h"
}

using namespace Ring;
using namespace Ring::Ble;

GattSrv* GattSrv::instance = NULL;
GattServerInfo_t GattSrv::mServer =
{
    .fd = -1,
    .hci_socket = -1,
    .ring_svc_handle = 0,
    .hci_thread_id = 0,
    .sref = NULL,
};

BleApi* GattSrv::getInstance() {
    if (instance == NULL)
        instance = new GattSrv();
    return (BleApi*) instance;
}

GattSrv::GattSrv()
{
}

// static callback definitions
typedef int (*cb_on_accept) (int fd);
static void* l2cap_le_att_listen_and_accept(void *data);
static void confirm_write(struct gatt_db_attribute *attr, int err, void *user_data);

int GattSrv::Initialize()
{
    int ctl, ret = Error::NONE;
    if (!mInitialized)
    {
        BOT_NOTIFY_DEBUG("GattSrv::Initialize");
        if (0 <= (ctl = open_socket(di)))
        {
            HCIup(ctl, di.dev_id);
            HCIscan(ctl, di.dev_id, (char*) "piscan");

            close(ctl);
            mInitialized = true;
        }
        else
            ret = Error::FAILED_INITIALIZE;
    }
    return ret;
}

int GattSrv::Shutdown()
{
    int ctl, ret = Error::NONE;
    if (mInitialized)
    {
        mainloop_quit();

        if (0 <= (ctl = open_socket(di)))
        {
            HCIscan(ctl, di.dev_id, (char*) "noscan");
            HCIdown(ctl, di.dev_id);

            close(ctl);
            mInitialized = false;
        }
        else
            ret = Error::FAILED_INITIALIZE;
    }
    return ret;
}
int GattSrv::SetLocalDeviceName(ParameterList_t *aParams __attribute__ ((unused)))
{
    int ret_val = Error::UNDEFINED;
    if (mInitialized)
    {
        if ((aParams) && (aParams->NumberofParameters))
        {
            strncpy(mDeviceName, aParams->Params[0].strParam, DEV_NAME_LEN);
        }

        BOT_NOTIFY_DEBUG("Attempting to set Device Name to: \"%s\".", mDeviceName);
        HCIname(di.dev_id, mDeviceName);
        ret_val = Error::NONE;
    }
    else
    {
        BOT_NOTIFY_ERROR("Platform Manager has not been Initialized.");
        ret_val = Error::NOT_INITIALIZED;
    }
    return ret_val;
}

int GattSrv::SetLocalClassOfDevice(ParameterList_t *aParams __attribute__ ((unused)))
{
    int ret_val = Error::UNDEFINED;
    if (mInitialized)
    {
        if ((aParams) && (aParams->NumberofParameters))
        {
            sprintf(mDeviceClass, "%06X", aParams->Params[0].intParam);
        }

        BOT_NOTIFY_DEBUG("Attempting to set Class of Device to: \"%s\".", mDeviceClass);
        HCIclass(di.dev_id, mDeviceClass);
        ret_val = Error::NONE;
    }
    else
    {
        BOT_NOTIFY_ERROR("Platform Manager has not been Initialized.");
        ret_val = Error::NOT_INITIALIZED;
    }
    return ret_val;
}

///
/// \brief GattSrv::StartAdvertising
/// \param aParams
/// \return error code
/// Multifunctional:
///     -start LE adv,
///     - launch listeninng pthread on HCI socket for incoming connections
///
int GattSrv::StartAdvertising(ParameterList_t *aParams __attribute__ ((unused)))
{
    int ret_val = Error::UNDEFINED;
    if (mInitialized && mServiceTable && mServiceTable->NumberAttributes)
    {
        BOT_NOTIFY_DEBUG("Starting LE adv");
        HCIle_adv(di.dev_id, NULL);

        ret_val = pthread_create(&mServer.hci_thread_id, NULL, l2cap_le_att_listen_and_accept, NULL);
        if (ret_val != Error::NONE)
        {
            BOT_NOTIFY_ERROR("failed to create pthread %s (%d)", strerror(errno), errno);
            mServer.hci_thread_id = 0;
            ret_val = Error::PTHREAD_ERROR;
        }
    }
    else
    {
        BOT_NOTIFY_ERROR("Platform Manager has not been Initialized.");
        ret_val = Error::NOT_INITIALIZED;
    }
    return ret_val;
}

///
/// \brief GattSrv::StopAdvertising
/// \param aParams
/// \return error code
/// Multifunctional:
///     - stop LE adv
///     - stop listenning pthread if still listen on the socket
///
int GattSrv::StopAdvertising(ParameterList_t *aParams __attribute__ ((unused)))
{
    int ret_val = Error::UNDEFINED;
    if (mInitialized)
    {
        BOT_NOTIFY_DEBUG("Stopping LE adv");
        HCIno_le_adv(di.dev_id);

        ret_val = Error::NONE;
        if (mServer.hci_socket >= 0)
        {
            shutdown(mServer.hci_socket, SHUT_RDWR);
            mServer.hci_socket = -1;
        }

        if (mServer.hci_thread_id > 0)
        {
            mainloop_quit();
            ret_val = pthread_cancel(mServer.hci_thread_id);
            mServer.hci_thread_id = 0;
            BOT_NOTIFY_ERROR("cancel GATT pthread ret_val %d, Errno: %s (%d)", ret_val, strerror(errno), errno);
            ret_val = Error::NONE;
        }
    }
    else
    {
        BOT_NOTIFY_ERROR("Platform Manager has not been Initialized.");
        ret_val = Error::NOT_INITIALIZED;
    }
    return ret_val;
}

///
/// \brief GattSrv::NotifyCharacteristic
/// \param aServiceIdx
/// \param aAttributeIdx
/// \param aStrPayload
/// \return
///
int GattSrv::NotifyCharacteristic(int aServiceIdx, int aAttributeIdx, const char* aStrPayload)
{
    int ret_val = Error::INVALID_PARAMETERS;
    if (mInitialized && mServiceTable && mServiceTable->NumberAttributes && GattSrv::mServer.sref)
    {
        if (aServiceIdx == RING_PAIRING_SVC_IDX)
        {
            if (aAttributeIdx < (int) mServiceTable->NumberAttributes)
            {
                BOT_NOTIFY_DEBUG("sending %s Notify \"%s\"", mServiceTable->AttributeList[aAttributeIdx].AttributeName, aStrPayload);
                bt_gatt_server_send_notification(GattSrv::mServer.sref->gatt, GattSrv::mServer.ring_attr_handle[aAttributeIdx], (const uint8_t*) aStrPayload, strlen(aStrPayload));
                ret_val = Error::NONE;
            }
        }
    }
    else
    {
        BOT_NOTIFY_ERROR("Platform Manager has not been Initialized.");
        ret_val = Error::NOT_INITIALIZED;
    }
    return ret_val;
}

///
/// \brief GattSrv::GATTUpdateCharacteristic
/// \param aServiceID
/// \param aAttrOffset
/// \param aAttrData
/// \param aAttrLen
/// \return
///
int GattSrv::GATTUpdateCharacteristic(unsigned int aServiceID, int aAttrOffset, Byte_t *aAttrData, int aAttrLen)
{
    int ret_val = Error::NOT_INITIALIZED;
    if (mInitialized && mServiceTable && mServiceTable->NumberAttributes && GattSrv::mServer.sref)
    {
        if ((aServiceID == mServiceTable->ServiceID) && aAttrData && aAttrLen)
        {
            ret_val = Error::NOT_FOUND;
            // find attribute idx by offset
            for (unsigned int attributeIdx = 0; attributeIdx < mServiceTable->NumberAttributes; attributeIdx++)
            {
                // BOT_NOTIFY_TRACE("mServiceTable->AttributeList[%d].AttributeOffset = %d == %d ?", attributeIdx, mServiceTable->AttributeList[attributeIdx].AttributeOffset, aAttrOffset);
                if ((int) mServiceTable->AttributeList[attributeIdx].AttributeOffset == aAttrOffset)
                {
                    struct gatt_db_attribute * attr = gatt_db_get_attribute(GattSrv::mServer.sref->db, GattSrv::mServer.ring_attr_handle[attributeIdx]);
                    if (attr)
                    {
                        gatt_db_attribute_write(attr, 0,
                                                (const uint8_t *) aAttrData, aAttrLen,
                                                BT_ATT_OP_WRITE_REQ, NULL,
                                                confirm_write, (void*) mServiceTable->AttributeList[attributeIdx].AttributeName);
                        ret_val = Error::NONE;
                    }
                    else
                    {
                        BOT_NOTIFY_ERROR("Failed to access aAttrOffset %d, %s", aAttrOffset, mServiceTable->AttributeList[attributeIdx].AttributeName);
                    }
                    break;
                }
            }
        }
        else
        {
            BOT_NOTIFY_ERROR("GATTUpdateCharacteristic INVALID_PARAMETERS, %d, %d, %08X, %d", aServiceID, aAttrOffset, (int) (unsigned long) aAttrData, aAttrLen);
            ret_val = Error::INVALID_PARAMETERS;
        }
    }
    else
    {
        BOT_NOTIFY_ERROR("Platform Manager has not been Initialized.");
    }
    return ret_val;
}

///
/// \brief GattSrv::ProcessRegisteredCallback
/// \param aEventType
/// \param aServiceID
/// \param aAttrOffset - ! here in BCM43 is used as aAttrIdx
/// \return
///
int GattSrv::ProcessRegisteredCallback(GATM_Event_Type_t aEventType, int aServiceID, int aAttrOffset)
{
    if (mOnCharCb)
        (*(mOnCharCb))(aServiceID, aAttrOffset, (Ble::Property::Access) aEventType);
    return Error::NONE;
}

///
/// \brief QueryLocalDeviceProperties
/// \param aParams - not used
/// \return
///
int GattSrv::QueryLocalDeviceProperties(ParameterList_t *aParams __attribute__ ((unused)))
{
    print_dev_hdr(&di);
    HCIname(di.dev_id, NULL);
    HCIclass(di.dev_id, NULL);
    return Error::NONE;
}

int GattSrv::SetDiscoverable(ParameterList_t *aParams __attribute__ ((unused)))
{   // nothing todo at this time
    return Error::NONE;
}

int GattSrv::SetConnectable(ParameterList_t *aParams __attribute__ ((unused)))
{   // nothing todo at this time
    return Error::NONE;
}

int GattSrv::SetPairable(ParameterList_t *aParams __attribute__ ((unused)))
{   // nothing todo at this time
    return Error::NONE;
}

int GattSrv::SetLocalDeviceAppearance(ParameterList_t *aParams __attribute__ ((unused)))
{   // nothing todo at this time
    return Error::NONE;
}

int GattSrv::SetRemoteDeviceLinkActive(ParameterList_t *aParams __attribute__ ((unused)))
{   // nothing todo at this time
    return Error::NONE;
}

int GattSrv::SetAdvertisingInterval(ParameterList_t *aParams __attribute__ ((unused)))
{   // nothing todo at this time
    return Error::NONE;
}

int GattSrv::RegisterAuthentication(ParameterList_t *aParams __attribute__ ((unused)))
{   // nothing todo at this time
    return Error::NONE;
}

int GattSrv::ChangeSimplePairingParameters(ParameterList_t *aParams __attribute__ ((unused)))
{   // nothing todo at this time
    return Error::NONE;
}

int GattSrv::RegisterGATMEventCallback(ParameterList_t *aParams __attribute__ ((unused)))
{   // nothing todo at this time
    return Error::NONE;
}

int GattSrv::GATTRegisterService(ParameterList_t *aParams __attribute__ ((unused)))
{   // nothing todo at this time
    return Error::NONE;
}

int GattSrv::SetAuthenticatedPayloadTimeout(ParameterList_t *aParams __attribute__ ((unused)))
{   // nothing todo at this time
    return Error::NONE;
}

int GattSrv::SetAndUpdateConnectionAndScanBLEParameters(ParameterList_t *aParams __attribute__ ((unused)))
{   // nothing todo at this time
    return Error::NONE;
}

int GattSrv::EnableBluetoothDebug(ParameterList_t *aParams __attribute__ ((unused)))
{   // nothing todo at this time
    return Error::NONE;
}

int GattSrv::SetDevicePower(Ble::ConfigArgument::Arg aOnOff)
{   // nothing todo at this time
    (void) aOnOff;
    return Error::NONE;
}

/**********************************************************
 * Helper functions
 *********************************************************/

int GattSrv::open_socket(struct hci_dev_info &di) {
    int ctl = -1;
    (void) di;

//    bdaddr_t  _BDADDR_ANY = {{0, 0, 0, 0, 0, 0}};

    /* Open HCI socket  */
    if ((ctl = socket(AF_BLUETOOTH, SOCK_RAW, BTPROTO_HCI)) < 0) {
        BOT_NOTIFY_ERROR("Can't open HCI socket. %s (%d)", strerror(errno), errno);
        return Error::FAILED_INITIALIZE;
    }

//    if (ioctl(ctl, HCIGETDEVINFO, (void *) &di))
//    {
//        BOT_NOTIFY_ERROR("Can't get device info. %s (%d)", strerror(errno), errno);
//        return Error::FAILED_INITIALIZE;
//    }

      di.dev_id = 0;

//    if (hci_test_bit(HCI_RAW, &di.flags) && !bacmp(&di.bdaddr, &_BDADDR_ANY)) {
//        int dd = hci_open_dev(di.dev_id);
//        hci_read_bd_addr(dd, &di.bdaddr, 1000);
//        hci_close_dev(dd);
//    }
    return ctl;
}


/**************************** HCI ******************************/
/**************************** HCI ******************************/
/**************************** HCI ******************************/
void GattSrv::print_pkt_type(struct hci_dev_info *di) {
    char *str;
    str = hci_ptypetostr(di->pkt_type);
    BOT_NOTIFY_DEBUG("\tPacket type: %s", str);
    bt_free(str);
}

void GattSrv::print_link_policy(struct hci_dev_info *di) {
    BOT_NOTIFY_DEBUG("\tLink policy: %s", hci_lptostr(di->link_policy));
}

void GattSrv::print_link_mode(struct hci_dev_info *di) {
    char *str;
    str =  hci_lmtostr(di->link_mode);
    BOT_NOTIFY_DEBUG("\tLink mode: %s", str);
    bt_free(str);
}

void GattSrv::print_dev_features(struct hci_dev_info *di, int format) {
    BOT_NOTIFY_DEBUG("\tFeatures: 0x%2.2x 0x%2.2x 0x%2.2x 0x%2.2x "
                     "0x%2.2x 0x%2.2x 0x%2.2x 0x%2.2x",
                     di->features[0], di->features[1], di->features[2],
            di->features[3], di->features[4], di->features[5],
            di->features[6], di->features[7]);

    if (format) {
        char *tmp = (char*) lmp_featurestostr(di->features, (char *) "\t\t", 63);
        BOT_NOTIFY_DEBUG("%s", tmp);
        bt_free(tmp);
    }
}

void GattSrv::print_le_states(uint64_t states) {
    int i;
    const char *le_states[] = {
        "Non-connectable Advertising State" ,
        "Scannable Advertising State",
        "Connectable Advertising State",
        "Directed Advertising State",
        "Passive Scanning State",
        "Active Scanning State",
        "Initiating State/Connection State in Master Role",
        "Connection State in the Slave Role",
        "Non-connectable Advertising State and Passive Scanning State combination",
        "Scannable Advertising State and Passive Scanning State combination",
        "Connectable Advertising State and Passive Scanning State combination",
        "Directed Advertising State and Passive Scanning State combination",
        "Non-connectable Advertising State and Active Scanning State combination",
        "Scannable Advertising State and Active Scanning State combination",
        "Connectable Advertising State and Active Scanning State combination",
        "Directed Advertising State and Active Scanning State combination",
        "Non-connectable Advertising State and Initiating State combination",
        "Scannable Advertising State and Initiating State combination",
        "Non-connectable Advertising State and Master Role combination",
        "Scannable Advertising State and Master Role combination",
        "Non-connectable Advertising State and Slave Role combination",
        "Scannable Advertising State and Slave Role combination",
        "Passive Scanning State and Initiating State combination",
        "Active Scanning State and Initiating State combination",
        "Passive Scanning State and Master Role combination",
        "Active Scanning State and Master Role combination",
        "Passive Scanning State and Slave Role combination",
        "Active Scanning State and Slave Role combination",
        "Initiating State and Master Role combination/Master Role and Master Role combination",
        NULL
    };

    BOT_NOTIFY_DEBUG("Supported link layer states:");
    for (i = 0; le_states[i]; i++) {
        const char *status;
        status = states & (1 << i) ? "YES" : "NO ";
        BOT_NOTIFY_DEBUG("\t%s %s", status, le_states[i]);
    }
}

void GattSrv::HCIrstat(int ctl, int hdev) {
    /* Reset HCI device stat counters */
    if (ioctl(ctl, HCIDEVRESTAT, hdev) < 0) {
        BOT_NOTIFY_ERROR("Can't reset stats counters hci%d: %s (%d)", hdev, strerror(errno), errno);
        return;
    }
}

void GattSrv::HCIscan(int ctl, int hdev, char *opt) {
    struct hci_dev_req dr;
    dr.dev_id  = hdev;
    dr.dev_opt = SCAN_DISABLED;
    if (!strcmp(opt, "iscan"))
        dr.dev_opt = SCAN_INQUIRY;
    else if (!strcmp(opt, "pscan"))
        dr.dev_opt = SCAN_PAGE;
    else if (!strcmp(opt, "piscan"))
        dr.dev_opt = SCAN_PAGE | SCAN_INQUIRY;
    if (ioctl(ctl, HCISETSCAN, (unsigned long) &dr) < 0) {
        BOT_NOTIFY_ERROR("Can't set scan mode on hci%d: %s (%d)", hdev, strerror(errno), errno);
        return;
    }
}

void GattSrv::HCIle_addr(int hdev, char *opt) {
    struct hci_request rq;
    le_set_random_address_cp cp;
    uint8_t status;
    int dd, err, ret;
    if (!opt)
        return;
    if (hdev < 0)
        hdev = hci_get_route(NULL);
    dd = hci_open_dev(hdev);
    if (dd < 0) {
        err = -errno;
        BOT_NOTIFY_ERROR("Could not open device: %s(%d)", strerror(-err), -err);
        return;
    }
    memset(&cp, 0, sizeof(cp));
    str2ba(opt, &cp.bdaddr);
    memset(&rq, 0, sizeof(rq));
    rq.ogf = OGF_LE_CTL;
    rq.ocf = OCF_LE_SET_RANDOM_ADDRESS;
    rq.cparam = &cp;
    rq.clen = LE_SET_RANDOM_ADDRESS_CP_SIZE;
    rq.rparam = &status;
    rq.rlen = 1;
    ret = hci_send_req(dd, &rq, 1000);
    if (status || ret < 0) {
        err = -errno;
        BOT_NOTIFY_ERROR("Can't set random address for hci%d: " "%s (%d)", hdev, strerror(-err), -err);
    }
    hci_close_dev(dd);
}

void GattSrv::HCIle_adv(int hdev, char *opt) {
    struct hci_request rq;
    le_set_advertise_enable_cp advertise_cp;
    le_set_advertising_parameters_cp adv_params_cp;
    uint8_t status;
    int dd, ret;
    if (hdev < 0)
        hdev = hci_get_route(NULL);
    dd = hci_open_dev(hdev);
    if (dd < 0) {
        BOT_NOTIFY_ERROR("Could not open device");
        return;
    }
    memset(&adv_params_cp, 0, sizeof(adv_params_cp));
    adv_params_cp.min_interval = htobs(0x0800);
    adv_params_cp.max_interval = htobs(0x0800);
    if (opt)
        adv_params_cp.advtype = atoi(opt);
    adv_params_cp.chan_map = 7;
    memset(&rq, 0, sizeof(rq));
    rq.ogf = OGF_LE_CTL;
    rq.ocf = OCF_LE_SET_ADVERTISING_PARAMETERS;
    rq.cparam = &adv_params_cp;
    rq.clen = LE_SET_ADVERTISING_PARAMETERS_CP_SIZE;
    rq.rparam = &status;
    rq.rlen = 1;
    ret = hci_send_req(dd, &rq, 1000);
    if (ret < 0)
        goto done;
    memset(&advertise_cp, 0, sizeof(advertise_cp));
    advertise_cp.enable = 0x01;
    memset(&rq, 0, sizeof(rq));
    rq.ogf = OGF_LE_CTL;
    rq.ocf = OCF_LE_SET_ADVERTISE_ENABLE;
    rq.cparam = &advertise_cp;
    rq.clen = LE_SET_ADVERTISE_ENABLE_CP_SIZE;
    rq.rparam = &status;
    rq.rlen = 1;
    ret = hci_send_req(dd, &rq, 1000);
done:
    hci_close_dev(dd);
    if (ret < 0) {
        BOT_NOTIFY_ERROR("Can't set advertise mode on hci%d: %s (%d)", hdev, strerror(errno), errno);
        return;
    }
    if (status) {
        BOT_NOTIFY_ERROR("LE set advertise enable on hci%d returned status %d", hdev, status);
        return;
    }
}

void GattSrv::HCIno_le_adv(int hdev) {
    struct hci_request rq;
    le_set_advertise_enable_cp advertise_cp;
    uint8_t status;
    int dd, ret;
    if (hdev < 0)
        hdev = hci_get_route(NULL);
    dd = hci_open_dev(hdev);
    if (dd < 0) {
        BOT_NOTIFY_ERROR("Could not open device");
        return;
    }
    memset(&advertise_cp, 0, sizeof(advertise_cp));
    memset(&rq, 0, sizeof(rq));
    rq.ogf = OGF_LE_CTL;
    rq.ocf = OCF_LE_SET_ADVERTISE_ENABLE;
    rq.cparam = &advertise_cp;
    rq.clen = LE_SET_ADVERTISE_ENABLE_CP_SIZE;
    rq.rparam = &status;
    rq.rlen = 1;
    ret = hci_send_req(dd, &rq, 1000);
    hci_close_dev(dd);
    if (ret < 0) {
        BOT_NOTIFY_ERROR("Can't set advertise mode on hci%d: %s (%d)", hdev, strerror(errno), errno);
        return;
    }
    if (status) {
        BOT_NOTIFY_ERROR("LE set advertise enable on hci%d returned status %d", hdev, status);
        return;
    }
}

void GattSrv::HCIle_states(int hdev) {
    le_read_supported_states_rp rp;
    struct hci_request rq;
    int err, dd;
    if (hdev < 0)
        hdev = hci_get_route(NULL);
    dd = hci_open_dev(hdev);
    if (dd < 0) {
        BOT_NOTIFY_ERROR("Can't open device hci%d: %s (%d)", hdev, strerror(errno), errno);
        return;
    }
    memset(&rp, 0, sizeof(rp));
    memset(&rq, 0, sizeof(rq));
    rq.ogf    = OGF_LE_CTL;
    rq.ocf    = OCF_LE_READ_SUPPORTED_STATES;
    rq.rparam = &rp;
    rq.rlen   = LE_READ_SUPPORTED_STATES_RP_SIZE;
    err = hci_send_req(dd, &rq, 1000);
    hci_close_dev(dd);
    if (err < 0) {
        BOT_NOTIFY_ERROR("Can't read LE supported states on hci%d:" " %s(%d)", hdev, strerror(errno), errno);
        return;
    }
    if (rp.status) {
        BOT_NOTIFY_ERROR("Read LE supported states on hci%d" " returned status %d", hdev, rp.status);
        return;
    }
    print_le_states(rp.states);
}

void GattSrv::HCIiac(int hdev, char *opt) {
    int s = hci_open_dev(hdev);
    if (s < 0) {
        BOT_NOTIFY_ERROR("Can't open device hci%d: %s (%d)", hdev, strerror(errno), errno);
        return;
    }
    if (opt) {
        int l = strtoul(opt, 0, 16);
        uint8_t lap[3];
        if (!strcasecmp(opt, "giac")) {
            l = 0x9e8b33;
        } else if (!strcasecmp(opt, "liac")) {
            l = 0x9e8b00;
        } else if (l < 0x9e8b00 || l > 0x9e8b3f) {
            BOT_NOTIFY_DEBUG("Invalid access code 0x%x", l);
            return;
        }
        lap[0] = (l & 0xff);
        lap[1] = (l >> 8) & 0xff;
        lap[2] = (l >> 16) & 0xff;
        if (hci_write_current_iac_lap(s, 1, lap, 1000) < 0) {
            BOT_NOTIFY_DEBUG("Failed to set IAC on hci%d: %s", hdev, strerror(errno));
            return;
        }
    } else {
        uint8_t lap[3 * MAX_IAC_LAP];
        int i, j;
        uint8_t n;
        if (hci_read_current_iac_lap(s, &n, lap, 1000) < 0) {
            BOT_NOTIFY_DEBUG("Failed to read IAC from hci%d: %s", hdev, strerror(errno));
            return;
        }
        print_dev_hdr(&di);
        BOT_NOTIFY_DEBUG("\tIAC: ");
        for (i = 0; i < n; i++) {
            BOT_NOTIFY_DEBUG("0x");
            for (j = 3; j--; )
                BOT_NOTIFY_DEBUG("%02x", lap[j + 3 * i]);
            if (i < n - 1)
                BOT_NOTIFY_DEBUG(", ");
        }
        // BOT_NOTIFY_DEBUG("");
    }
    close(s);
}

void GattSrv::HCIauth(int ctl, int hdev, char *opt) {
    struct hci_dev_req dr;
    dr.dev_id = hdev;
    if (!strcmp(opt, "auth"))
        dr.dev_opt = AUTH_ENABLED;
    else
        dr.dev_opt = AUTH_DISABLED;
    if (ioctl(ctl, HCISETAUTH, (unsigned long) &dr) < 0) {
        BOT_NOTIFY_ERROR("Can't set auth on hci%d: %s (%d)", hdev, strerror(errno), errno);
        return;
    }
}

void GattSrv::HCIencrypt(int ctl, int hdev, char *opt) {
    struct hci_dev_req dr;
    dr.dev_id = hdev;
    if (!strcmp(opt, "encrypt"))
        dr.dev_opt = ENCRYPT_P2P;
    else
        dr.dev_opt = ENCRYPT_DISABLED;
    if (ioctl(ctl, HCISETENCRYPT, (unsigned long) &dr) < 0) {
        BOT_NOTIFY_ERROR("Can't set encrypt on hci%d: %s (%d)", hdev, strerror(errno), errno);
        return;
    }
}

void GattSrv::HCIup(int ctl, int hdev) {
    /* Start HCI device */
    if (ioctl(ctl, HCIDEVUP, hdev) < 0) {
        if (errno == EALREADY)
            return;
        BOT_NOTIFY_ERROR("Can't init device hci%d: %s (%d)", hdev, strerror(errno), errno);
        return;
    }
}

void GattSrv::HCIdown(int ctl, int hdev) {
    /* Stop HCI device */
    if (ioctl(ctl, HCIDEVDOWN, hdev) < 0) {
        BOT_NOTIFY_ERROR("Can't down device hci%d: %s (%d)", hdev, strerror(errno), errno);
        return;
    }
}

void GattSrv::HCIreset(int ctl, int hdev) {
    /* Reset HCI device */
#if 0
    if (ioctl(ctl, HCIDEVRESET, hdev) < 0 ) {
        BOT_NOTIFY_ERROR("Reset failed for device hci%d: %s (%d)", hdev, strerror(errno), errno);
        return;
    }
#endif
    GattSrv::HCIdown(ctl, hdev);
    GattSrv::HCIup(ctl, hdev);
}

void GattSrv::HCIptype(int ctl, int hdev, char *opt) {
    struct hci_dev_req dr;
    dr.dev_id = hdev;
    if (hci_strtoptype(opt, &dr.dev_opt)) {
        if (ioctl(ctl, HCISETPTYPE, (unsigned long) &dr) < 0) {
            BOT_NOTIFY_ERROR("Can't set pkttype on hci%d: %s (%d)", hdev, strerror(errno), errno);
            return;
        }
    } else {
        print_dev_hdr(&di);
        print_pkt_type(&di);
    }
}

void GattSrv::HCIlp(int ctl, int hdev, char *opt) {
    struct hci_dev_req dr;
    dr.dev_id = hdev;
    if (hci_strtolp(opt, &dr.dev_opt)) {
        if (ioctl(ctl, HCISETLINKPOL, (unsigned long) &dr) < 0) {
            BOT_NOTIFY_ERROR("Can't set link policy on hci%d: %s (%d)", hdev, strerror(errno), errno);
            return;
        }
    } else {
        print_dev_hdr(&di);
        print_link_policy(&di);
    }
}

void GattSrv::HCIlm(int ctl, int hdev, char *opt) {
    struct hci_dev_req dr;
    dr.dev_id = hdev;
    if (hci_strtolm(opt, &dr.dev_opt)) {
        if (ioctl(ctl, HCISETLINKMODE, (unsigned long) &dr) < 0) {
            BOT_NOTIFY_ERROR("Can't set default link mode on hci%d: %s (%d)", hdev, strerror(errno), errno);
            return;
        }
    } else {
        print_dev_hdr(&di);
        print_link_mode(&di);
    }
}

void GattSrv::HCIaclmtu(int ctl, int hdev, char *opt) {
    struct hci_dev_req dr = { .dev_id = (uint16_t) hdev, .dev_opt = 0 };
    uint16_t mtu, mpkt;
    if (!opt)
        return;
    if (sscanf(opt, "%4hu:%4hu", &mtu, &mpkt) != 2)
        return;
    dr.dev_opt = htobl(htobs(mpkt) | (htobs(mtu) << 16));
    if (ioctl(ctl, HCISETACLMTU, (unsigned long) &dr) < 0) {
        BOT_NOTIFY_ERROR("Can't set ACL mtu on hci%d: %s(%d)", hdev, strerror(errno), errno);
        return;
    }
}

void GattSrv::HCIscomtu(int ctl, int hdev, char *opt) {
    struct hci_dev_req dr = { .dev_id = (uint16_t) hdev, .dev_opt = 0 };
    uint16_t mtu, mpkt;
    if (!opt)
        return;
    if (sscanf(opt, "%4hu:%4hu", &mtu, &mpkt) != 2)
        return;
    dr.dev_opt = htobl(htobs(mpkt) | (htobs(mtu) << 16));
    if (ioctl(ctl, HCISETSCOMTU, (unsigned long) &dr) < 0) {
        BOT_NOTIFY_ERROR("Can't set SCO mtu on hci%d: %s (%d)", hdev, strerror(errno), errno);
        return;
    }
}

void GattSrv::HCIfeatures(int hdev) {
    uint8_t features[8], max_page = 0;
    char *tmp;
    int i, dd;
    if (!(di.features[7] & LMP_EXT_FEAT)) {
        print_dev_hdr(&di);
        print_dev_features(&di, 1);
        return;
    }
    dd = hci_open_dev(hdev);
    if (dd < 0) {
        BOT_NOTIFY_ERROR("Can't open device hci%d: %s (%d)", hdev, strerror(errno), errno);
        return;
    }
    if (hci_read_local_ext_features(dd, 0, &max_page, features, 1000) < 0) {
        BOT_NOTIFY_ERROR("Can't read extended features hci%d: %s (%d)", hdev, strerror(errno), errno);
        return;
    }
    print_dev_hdr(&di);
    BOT_NOTIFY_DEBUG("\tFeatures%s: 0x%2.2x 0x%2.2x 0x%2.2x 0x%2.2x "
                     "0x%2.2x 0x%2.2x 0x%2.2x 0x%2.2x",
                     (max_page > 0) ? " page 0" : "",
                     features[0], features[1], features[2], features[3],
            features[4], features[5], features[6], features[7]);
    tmp = (char*) lmp_featurestostr(di.features, (char *) "\t\t", 63);
    BOT_NOTIFY_DEBUG("%s", tmp);
    bt_free(tmp);
    for (i = 1; i <= max_page; i++) {
        if (hci_read_local_ext_features(dd, i, NULL,
                                        features, 1000) < 0)
            continue;
        BOT_NOTIFY_DEBUG("\tFeatures page %d: 0x%2.2x 0x%2.2x 0x%2.2x 0x%2.2x "
                         "0x%2.2x 0x%2.2x 0x%2.2x 0x%2.2x", i,
                         features[0], features[1], features[2], features[3],
                features[4], features[5], features[6], features[7]);
    }
    hci_close_dev(dd);
}

void GattSrv::HCIname(int hdev, char *opt) {
    int dd;
    dd = hci_open_dev(hdev);
    if (dd < 0) {
        BOT_NOTIFY_ERROR("Can't open device hci%d: %s (%d)", hdev, strerror(errno), errno);
        return;
    }
    if (opt) {
        if (hci_write_local_name(dd, opt, 2000) < 0) {
            BOT_NOTIFY_ERROR("Can't change local name on hci%d: %s (%d)", hdev, strerror(errno), errno);
            return;
        }
    } else {
        char name[249];
        int i;
        if (hci_read_local_name(dd, sizeof(name), name, 1000) < 0) {
            BOT_NOTIFY_ERROR("Can't read local name on hci%d: %s (%d)", hdev, strerror(errno), errno);
            return;
        }
        for (i = 0; i < 248 && name[i]; i++) {
            if ((unsigned char) name[i] < 32 || name[i] == 127)
                name[i] = '.';
        }
        name[248] = '\0';
        print_dev_hdr(&di);
        BOT_NOTIFY_DEBUG("\tName: '%s'", name);
    }
    hci_close_dev(dd);
}

const char *GattSrv::get_minor_device_name(int major, int minor) {
    switch (major) {
    case 0:	/* misc */
        return "";
    case 1:	/* computer */
        switch (minor) {
        case 0:
            return "Uncategorized";
        case 1:
            return "Desktop workstation";
        case 2:
            return "Server";
        case 3:
            return "Laptop";
        case 4:
            return "Handheld";
        case 5:
            return "Palm";
        case 6:
            return "Wearable";
        }
        break;
    case 2:	/* phone */
        switch (minor) {
        case 0:
            return "Uncategorized";
        case 1:
            return "Cellular";
        case 2:
            return "Cordless";
        case 3:
            return "Smart phone";
        case 4:
            return "Wired modem or voice gateway";
        case 5:
            return "Common ISDN Access";
        case 6:
            return "Sim Card Reader";
        }
        break;
    case 3:	/* lan access */
        if (minor == 0)
            return "Uncategorized";
        switch (minor / 8) {
        case 0:
            return "Fully available";
        case 1:
            return "1-17% utilized";
        case 2:
            return "17-33% utilized";
        case 3:
            return "33-50% utilized";
        case 4:
            return "50-67% utilized";
        case 5:
            return "67-83% utilized";
        case 6:
            return "83-99% utilized";
        case 7:
            return "No service available";
        }
        break;
    case 4:	/* audio/video */
        switch (minor) {
        case 0:
            return "Uncategorized";
        case 1:
            return "Device conforms to the Headset profile";
        case 2:
            return "Hands-free";
            /* 3 is reserved */
        case 4:
            return "Microphone";
        case 5:
            return "Loudspeaker";
        case 6:
            return "Headphones";
        case 7:
            return "Portable Audio";
        case 8:
            return "Car Audio";
        case 9:
            return "Set-top box";
        case 10:
            return "HiFi Audio Device";
        case 11:
            return "VCR";
        case 12:
            return "Video Camera";
        case 13:
            return "Camcorder";
        case 14:
            return "Video Monitor";
        case 15:
            return "Video Display and Loudspeaker";
        case 16:
            return "Video Conferencing";
            /* 17 is reserved */
        case 18:
            return "Gaming/Toy";
        }
        break;
    case 5: {	/* peripheral */
        static char cls_str[48];
        cls_str[0] = '\0';
        switch (minor & 48) {
        case 16:
            strncpy(cls_str, "Keyboard", sizeof(cls_str));
            break;
        case 32:
            strncpy(cls_str, "Pointing device", sizeof(cls_str));
            break;
        case 48:
            strncpy(cls_str, "Combo keyboard/pointing device", sizeof(cls_str));
            break;
        }
        if ((minor & 15) && (strlen(cls_str) > 0))
            strcat(cls_str, "/");
        switch (minor & 15) {
        case 0:
            break;
        case 1:
            strncat(cls_str, "Joystick", sizeof(cls_str) - strlen(cls_str));
            break;
        case 2:
            strncat(cls_str, "Gamepad", sizeof(cls_str) - strlen(cls_str));
            break;
        case 3:
            strncat(cls_str, "Remote control", sizeof(cls_str) - strlen(cls_str));
            break;
        case 4:
            strncat(cls_str, "Sensing device", sizeof(cls_str) - strlen(cls_str));
            break;
        case 5:
            strncat(cls_str, "Digitizer tablet", sizeof(cls_str) - strlen(cls_str));
            break;
        case 6:
            strncat(cls_str, "Card reader", sizeof(cls_str) - strlen(cls_str));
            break;
        default:
            strncat(cls_str, "(reserved)", sizeof(cls_str) - strlen(cls_str));
            break;
        }
        if (strlen(cls_str) > 0)
            return cls_str;
    }
    case 6:	/* imaging */
        if (minor & 4)
            return "Display";
        if (minor & 8)
            return "Camera";
        if (minor & 16)
            return "Scanner";
        if (minor & 32)
            return "Printer";
        break;
    case 7: /* wearable */
        switch (minor) {
        case 1:
            return "Wrist Watch";
        case 2:
            return "Pager";
        case 3:
            return "Jacket";
        case 4:
            return "Helmet";
        case 5:
            return "Glasses";
        }
        break;
    case 8: /* toy */
        switch (minor) {
        case 1:
            return "Robot";
        case 2:
            return "Vehicle";
        case 3:
            return "Doll / Action Figure";
        case 4:
            return "Controller";
        case 5:
            return "Game";
        }
        break;
    case 63:	/* uncategorised */
        return "";
    }
    return "Unknown (reserved) minor device class";
}

void GattSrv::HCIclass(int hdev, char *opt) {
    static const char *services[] = { "Positioning",
                                      "Networking",
                                      "Rendering",
                                      "Capturing",
                                      "Object Transfer",
                                      "Audio",
                                      "Telephony",
                                      "Information"
                                    };
    static const char *major_devices[] = { "Miscellaneous",
                                           "Computer",
                                           "Phone",
                                           "LAN Access",
                                           "Audio/Video",
                                           "Peripheral",
                                           "Imaging",
                                           "Uncategorized"
                                         };
    int s = hci_open_dev(hdev);
    if (s < 0) {
        BOT_NOTIFY_ERROR("Can't open device hci%d: %s (%d)", hdev, strerror(errno), errno);
        return;
    }
    if (opt) {
        uint32_t cod = strtoul(opt, NULL, 16);
        if (hci_write_class_of_dev(s, cod, 2000) < 0) {
            BOT_NOTIFY_ERROR("Can't write local class of device on hci%d: %s (%d)", hdev, strerror(errno), errno);
            return;
        }
    } else {
        uint8_t cls[3];
        if (hci_read_class_of_dev(s, cls, 1000) < 0) {
            BOT_NOTIFY_ERROR("Can't read class of device on hci%d: %s (%d)", hdev, strerror(errno), errno);
            return;
        }
        print_dev_hdr(&di);
        BOT_NOTIFY_DEBUG("\tClass: 0x%02x%02x%02x", cls[2], cls[1], cls[0]);
        char strtmp[123] = "";
        if (cls[2]) {
            unsigned int i, offset = 0;
            int first = 1;
            for (i = 0; i < (sizeof(services) / sizeof(*services)); i++)
                if (cls[2] & (1 << i)) {
                    if (!first)
                        offset += sprintf(&strtmp[offset], ", ");
                    offset += sprintf(&strtmp[offset], "%s", services[i]);
                    first = 0;
                }
            BOT_NOTIFY_DEBUG("\tService Classes: %s", strtmp);
        } else
            BOT_NOTIFY_DEBUG("\tService Classes: %s", "Unspecified");
        if ((cls[1] & 0x1f) >= sizeof(major_devices) / sizeof(*major_devices))
            BOT_NOTIFY_DEBUG("\n\tInvalid Device Class!");
        else
            BOT_NOTIFY_DEBUG("\n\tDevice Class: %s, %s", major_devices[cls[1] & 0x1f],
                    get_minor_device_name(cls[1] & 0x1f, cls[0] >> 2));
    }
}

void GattSrv::HCIvoice(int hdev, char *opt) {
    static char *icf[] = {	(char *) "Linear",
                            (char *) "u-Law",
                            (char *) "A-Law",
                            (char *) "Reserved"
                         };
    static char *idf[] = {	(char *) "1's complement",
                            (char *) "2's complement",
                            (char *) "Sign-Magnitude",
                            (char *) "Reserved"
                         };
    static char *iss[] = {	(char *) "8 bit",
                            (char *) "16 bit"
                         };
    static char *acf[] = {	(char *) "CVSD",
                            (char *) "u-Law",
                            (char *) "A-Law",
                            (char *) "Reserved"
                         };
    int s = hci_open_dev(hdev);
    if (s < 0) {
        BOT_NOTIFY_ERROR("Can't open device hci%d: %s (%d)", hdev, strerror(errno), errno);
        return;
    }
    if (opt) {
        uint16_t vs = htobs(strtoul(opt, NULL, 16));
        if (hci_write_voice_setting(s, vs, 2000) < 0) {
            BOT_NOTIFY_ERROR("Can't write voice setting on hci%d: %s (%d)", hdev, strerror(errno), errno);
            return;
        }
    } else {
        uint16_t vs;
        uint8_t ic;
        if (hci_read_voice_setting(s, &vs, 1000) < 0) {
            BOT_NOTIFY_ERROR("Can't read voice setting on hci%d: %s (%d)", hdev, strerror(errno), errno);
            return;
        }
        vs = htobs(vs);
        ic = (vs & 0x0300) >> 8;
        print_dev_hdr(&di);
        BOT_NOTIFY_DEBUG("\tVoice setting: 0x%04x%s", vs,
                         ((vs & 0x03fc) == 0x0060) ? " (Default Condition)" : "");
        BOT_NOTIFY_DEBUG("\tInput Coding: %s", icf[ic]);
        BOT_NOTIFY_DEBUG("\tInput Data Format: %s", idf[(vs & 0xc0) >> 6]);
        if (!ic) {
            BOT_NOTIFY_DEBUG("\tInput Sample Size: %s",
                             iss[(vs & 0x20) >> 5]);
            BOT_NOTIFY_DEBUG("\t# of bits padding at MSB: %d",
                             (vs & 0x1c) >> 2);
        }
        BOT_NOTIFY_DEBUG("\tAir Coding Format: %s", acf[vs & 0x03]);
    }
}

void GattSrv::HCIdelkey(int hdev, char *opt) {
    bdaddr_t bdaddr;
    bdaddr_t  _BDADDR_ANY = {{0, 0, 0, 0, 0, 0}};
    uint8_t all;
    int dd;
    if (!opt)
        return;
    dd = hci_open_dev(hdev);
    if (dd < 0) {
        BOT_NOTIFY_ERROR("Can't open device hci%d: %s (%d)", hdev, strerror(errno), errno);
        return;
    }
    if (!strcasecmp(opt, "all")) {
        bacpy(&bdaddr, &_BDADDR_ANY);
        all = 1;
    } else {
        str2ba(opt, &bdaddr);
        all = 0;
    }
    if (hci_delete_stored_link_key(dd, &bdaddr, all, 1000) < 0) {
        BOT_NOTIFY_ERROR("Can't delete stored link key on hci%d: %s (%d)", hdev, strerror(errno), errno);
        return;
    }
    hci_close_dev(dd);
}

void GattSrv::HCIoob_data(int hdev) {
    uint8_t hash[16], randomizer[16];
    int i, dd;
    dd = hci_open_dev(hdev);
    if (dd < 0) {
        BOT_NOTIFY_ERROR("Can't open device hci%d: %s (%d)", hdev, strerror(errno), errno);
        return;
    }
    if (hci_read_local_oob_data(dd, hash, randomizer, 1000) < 0) {
        BOT_NOTIFY_ERROR("Can't read local OOB data on hci%d: %s (%d)", hdev, strerror(errno), errno);
        return;
    }
    print_dev_hdr(&di);
    BOT_NOTIFY_DEBUG("\tOOB Hash:  ");
    for (i = 0; i < 16; i++)
        BOT_NOTIFY_DEBUG(" %02x", hash[i]);
    BOT_NOTIFY_DEBUG("\n\tRandomizer:");
    for (i = 0; i < 16; i++)
        BOT_NOTIFY_DEBUG(" %02x", randomizer[i]);
    // BOT_NOTIFY_DEBUG("");
    hci_close_dev(dd);
}

void GattSrv::HCIcommands(int hdev) {
    uint8_t cmds[64];
    char *str;
    int i, n, dd;
    dd = hci_open_dev(hdev);
    if (dd < 0) {
        BOT_NOTIFY_ERROR("Can't open device hci%d: %s (%d)", hdev, strerror(errno), errno);
        return;
    }
    if (hci_read_local_commands(dd, cmds, 1000) < 0) {
        BOT_NOTIFY_ERROR("Can't read support commands on hci%d: %s (%d)", hdev, strerror(errno), errno);
        return;
    }
    print_dev_hdr(&di);
    for (i = 0; i < 64; i++) {
        if (!cmds[i])
            continue;
        BOT_NOTIFY_DEBUG("%s Octet %-2d = 0x%02x (Bit",
                         i ? "\t\t ": "\tCommands:", i, cmds[i]);
        for (n = 0; n < 8; n++)
            if (cmds[i] & (1 << n))
                BOT_NOTIFY_DEBUG(" %d", n);
        BOT_NOTIFY_DEBUG(")");
    }
    str = (char*) hci_commandstostr(cmds, (char *) "\t", 71);
    BOT_NOTIFY_DEBUG("%s", str);
    bt_free(str);
    hci_close_dev(dd);
}

void GattSrv::HCIversion(int hdev) {
    struct hci_version ver;
    char *hciver, *lmpver;
    int dd;
    dd = hci_open_dev(hdev);
    if (dd < 0) {
        BOT_NOTIFY_ERROR("Can't open device hci%d: %s (%d)", hdev, strerror(errno), errno);
        return;
    }
    if (hci_read_local_version(dd, &ver, 1000) < 0) {
        BOT_NOTIFY_ERROR("Can't read version info hci%d: %s (%d)", hdev, strerror(errno), errno);
        return;
    }
    hciver = hci_vertostr(ver.hci_ver);
    if (((di.type & 0x30) >> 4) == HCI_BREDR)
        lmpver = lmp_vertostr(ver.lmp_ver);
    else
        lmpver = pal_vertostr(ver.lmp_ver);
    print_dev_hdr(&di);
    BOT_NOTIFY_DEBUG("\tHCI Version: %s (0x%x)  Revision: 0x%x"
                     "\t%s Version: %s (0x%x)  Subversion: 0x%x"
                     "\tManufacturer: %s (%d)",
                     hciver ? hciver : "n/a", ver.hci_ver, ver.hci_rev,
                     (((di.type & 0x30) >> 4) == HCI_BREDR) ? "LMP" : "PAL",
                     lmpver ? lmpver : "n/a", ver.lmp_ver, ver.lmp_subver,
                     bt_compidtostr(ver.manufacturer), ver.manufacturer);
    if (hciver)
        bt_free(hciver);
    if (lmpver)
        bt_free(lmpver);
    hci_close_dev(dd);
}

void GattSrv::HCIinq_tpl(int hdev, char *opt) {
    int dd;
    dd = hci_open_dev(hdev);
    if (dd < 0) {
        BOT_NOTIFY_ERROR("Can't open device hci%d: %s (%d)", hdev, strerror(errno), errno);
        return;
    }
    if (opt) {
        int8_t level = atoi(opt);
        if (hci_write_inquiry_transmit_power_level(dd, level, 2000) < 0) {
            BOT_NOTIFY_ERROR("Can't set inquiry transmit power level on hci%d: %s (%d)", hdev, strerror(errno), errno);
            return;
        }
    } else {
        int8_t level;
        if (hci_read_inq_response_tx_power_level(dd, &level, 1000) < 0) {
            BOT_NOTIFY_ERROR("Can't read inquiry transmit power level on hci%d: %s (%d)", hdev, strerror(errno), errno);
            return;
        }
        print_dev_hdr(&di);
        BOT_NOTIFY_DEBUG("\tInquiry transmit power level: %d", level);
    }
    hci_close_dev(dd);
}

void GattSrv::HCIinq_mode(int hdev, char *opt) {
    int dd;
    dd = hci_open_dev(hdev);
    if (dd < 0) {
        BOT_NOTIFY_ERROR("Can't open device hci%d: %s (%d)", hdev, strerror(errno), errno);
        return;
    }
    if (opt) {
        uint8_t mode = atoi(opt);
        if (hci_write_inquiry_mode(dd, mode, 2000) < 0) {
            BOT_NOTIFY_ERROR("Can't set inquiry mode on hci%d: %s (%d)", hdev, strerror(errno), errno);
            return;
        }
    } else {
        uint8_t mode;
        if (hci_read_inquiry_mode(dd, &mode, 1000) < 0) {
            BOT_NOTIFY_ERROR("Can't read inquiry mode on hci%d: %s (%d)", hdev, strerror(errno), errno);
            return;
        }
        print_dev_hdr(&di);
        BOT_NOTIFY_DEBUG("\tInquiry mode: ");
        switch (mode) {
        case 0:
            BOT_NOTIFY_DEBUG("Standard Inquiry");
            break;
        case 1:
            BOT_NOTIFY_DEBUG("Inquiry with RSSI");
            break;
        case 2:
            BOT_NOTIFY_DEBUG("Inquiry with RSSI or Extended Inquiry");
            break;
        default:
            BOT_NOTIFY_DEBUG("Unknown (0x%02x)", mode);
            break;
        }
    }
    hci_close_dev(dd);
}

void GattSrv::HCIinq_data(int hdev, char *opt) {
    int i, dd;
    dd = hci_open_dev(hdev);
    if (dd < 0) {
        BOT_NOTIFY_ERROR("Can't open device hci%d: %s (%d)", hdev, strerror(errno), errno);
        return;
    }
    if (opt) {
        uint8_t fec = 0, data[HCI_MAX_EIR_LENGTH];
        char tmp[3];
        int i, size;
        memset(data, 0, sizeof(data));
        memset(tmp, 0, sizeof(tmp));
        size = (strlen(opt) + 1) / 2;
        if (size > HCI_MAX_EIR_LENGTH)
            size = HCI_MAX_EIR_LENGTH;
        for (i = 0; i < size; i++) {
            memcpy(tmp, opt + (i * 2), 2);
            data[i] = strtol(tmp, NULL, 16);
        }
        if (hci_write_ext_inquiry_response(dd, fec, data, 2000) < 0) {
            BOT_NOTIFY_ERROR("Can't set extended inquiry response on hci%d: %s (%d)", hdev, strerror(errno), errno);
            return;
        }
    } else {
        uint8_t fec, data[HCI_MAX_EIR_LENGTH], len, type, *ptr;
        char *str;
        if (hci_read_ext_inquiry_response(dd, &fec, data, 1000) < 0) {
            BOT_NOTIFY_ERROR("Can't read extended inquiry response on hci%d: %s (%d)", hdev, strerror(errno), errno);
            return;
        }
        print_dev_hdr(&di);
        BOT_NOTIFY_DEBUG("\tFEC %s\n\t\t", fec ? "enabled" : "disabled");
        for (i = 0; i < HCI_MAX_EIR_LENGTH; i++)
            BOT_NOTIFY_DEBUG("%02x%s%s", data[i], (i + 1) % 8 ? "" : " ",
                             (i + 1) % 16 ? " " : (i < 239 ? "\n\t\t" : ""));
        ptr = data;
        while (*ptr) {
            len = *ptr++;
            type = *ptr++;
            switch (type) {
            case 0x01:
                BOT_NOTIFY_DEBUG("\tFlags:");
                for (i = 0; i < len - 1; i++)
                    BOT_NOTIFY_DEBUG(" 0x%2.2x", *((uint8_t *) (ptr + i)));
                // BOT_NOTIFY_DEBUG("");
                break;
            case 0x02:
            case 0x03:
                BOT_NOTIFY_DEBUG("\t%s service classes:",
                                 type == 0x02 ? "Shortened" : "Complete");
                for (i = 0; i < (len - 1) / 2; i++) {
                    uint16_t val = bt_get_le16((ptr + (i * 2)));
                    BOT_NOTIFY_DEBUG(" 0x%4.4x", val);
                }
                // BOT_NOTIFY_DEBUG("");
                break;
            case 0x08:
            case 0x09:
                str = (char*) malloc(len);
                if (str) {
                    snprintf(str, len, "%s", ptr);
                    for (i = 0; i < len - 1; i++) {
                        if ((unsigned char) str[i] < 32 || str[i] == 127)
                            str[i] = '.';
                    }
                    BOT_NOTIFY_DEBUG("\t%s local name: \'%s\'",
                                     type == 0x08 ? "Shortened" : "Complete", str);
                    free(str);
                }
                break;
            case 0x0a:
                BOT_NOTIFY_DEBUG("\tTX power level: %d", *((int8_t *) ptr));
                break;
            case 0x10:
                BOT_NOTIFY_DEBUG("\tDevice ID with %d bytes data",
                                 len - 1);
                break;
            default:
                BOT_NOTIFY_DEBUG("\tUnknown type 0x%02x with %d bytes data",
                                 type, len - 1);
                break;
            }
            ptr += (len - 1);
        }
        // BOT_NOTIFY_DEBUG("");
    }
    hci_close_dev(dd);
}

void GattSrv::HCIinq_type(int hdev, char *opt) {
    int dd;
    dd = hci_open_dev(hdev);
    if (dd < 0) {
        BOT_NOTIFY_ERROR("Can't open device hci%d: %s (%d)", hdev, strerror(errno), errno);
        return;
    }
    if (opt) {
        uint8_t type = atoi(opt);
        if (hci_write_inquiry_scan_type(dd, type, 2000) < 0) {
            BOT_NOTIFY_ERROR("Can't set inquiry scan type on hci%d: %s (%d)", hdev, strerror(errno), errno);
            return;
        }
    } else {
        uint8_t type;
        if (hci_read_inquiry_scan_type(dd, &type, 1000) < 0) {
            BOT_NOTIFY_ERROR("Can't read inquiry scan type on hci%d: %s (%d)", hdev, strerror(errno), errno);
            return;
        }
        print_dev_hdr(&di);
        BOT_NOTIFY_DEBUG("\tInquiry scan type: %s",
                         type == 1 ? "Interlaced Inquiry Scan" : "Standard Inquiry Scan");
    }
    hci_close_dev(dd);
}

void GattSrv::HCIinq_parms(int hdev, char *opt) {
    struct hci_request rq;
    int s;
    if ((s = hci_open_dev(hdev)) < 0) {
        BOT_NOTIFY_ERROR("Can't open device hci%d: %s (%d)", hdev, strerror(errno), errno);
        return;
    }
    memset(&rq, 0, sizeof(rq));
    if (opt) {
        unsigned int window, interval;
        write_inq_activity_cp cp;
        if (sscanf(opt,"%4u:%4u", &window, &interval) != 2) {
            BOT_NOTIFY_DEBUG("Invalid argument format");
            return;
        }
        rq.ogf = OGF_HOST_CTL;
        rq.ocf = OCF_WRITE_INQ_ACTIVITY;
        rq.cparam = &cp;
        rq.clen = WRITE_INQ_ACTIVITY_CP_SIZE;
        cp.window = htobs((uint16_t) window);
        cp.interval = htobs((uint16_t) interval);
        if (window < 0x12 || window > 0x1000)
            BOT_NOTIFY_DEBUG("Warning: inquiry window out of range!");
        if (interval < 0x12 || interval > 0x1000)
            BOT_NOTIFY_DEBUG("Warning: inquiry interval out of range!");
        if (hci_send_req(s, &rq, 2000) < 0) {
            BOT_NOTIFY_ERROR("Can't set inquiry parameters name on hci%d: %s (%d)", hdev, strerror(errno), errno);
            return;
        }
    } else {
        uint16_t window, interval;
        read_inq_activity_rp rp;
        rq.ogf = OGF_HOST_CTL;
        rq.ocf = OCF_READ_INQ_ACTIVITY;
        rq.rparam = &rp;
        rq.rlen = READ_INQ_ACTIVITY_RP_SIZE;
        if (hci_send_req(s, &rq, 1000) < 0) {
            BOT_NOTIFY_ERROR("Can't read inquiry parameters on hci%d: %s (%d)", hdev, strerror(errno), errno);
            return;
        }
        if (rp.status) {
            BOT_NOTIFY_DEBUG("Read inquiry parameters on hci%d returned status %d",
                             hdev, rp.status);
            return;
        }
        print_dev_hdr(&di);
        window   = btohs(rp.window);
        interval = btohs(rp.interval);
        BOT_NOTIFY_DEBUG("\tInquiry interval: %u slots (%.2f ms), window: %u slots (%.2f ms)",
                         interval, (float)interval * 0.625, window, (float)window * 0.625);
    }
}

void GattSrv::HCIpage_parms(int hdev, char *opt) {
    struct hci_request rq;
    int s;
    if ((s = hci_open_dev(hdev)) < 0) {
        BOT_NOTIFY_ERROR("Can't open device hci%d: %s (%d)", hdev, strerror(errno), errno);
        return;
    }
    memset(&rq, 0, sizeof(rq));
    if (opt) {
        unsigned int window, interval;
        write_page_activity_cp cp;
        if (sscanf(opt,"%4u:%4u", &window, &interval) != 2) {
            BOT_NOTIFY_DEBUG("Invalid argument format");
            return;
        }
        rq.ogf = OGF_HOST_CTL;
        rq.ocf = OCF_WRITE_PAGE_ACTIVITY;
        rq.cparam = &cp;
        rq.clen = WRITE_PAGE_ACTIVITY_CP_SIZE;
        cp.window = htobs((uint16_t) window);
        cp.interval = htobs((uint16_t) interval);
        if (window < 0x12 || window > 0x1000)
            BOT_NOTIFY_DEBUG("Warning: page window out of range!");
        if (interval < 0x12 || interval > 0x1000)
            BOT_NOTIFY_DEBUG("Warning: page interval out of range!");
        if (hci_send_req(s, &rq, 2000) < 0) {
            BOT_NOTIFY_ERROR("Can't set page parameters name on hci%d: %s (%d)", hdev, strerror(errno), errno);
            return;
        }
    } else {
        uint16_t window, interval;
        read_page_activity_rp rp;
        rq.ogf = OGF_HOST_CTL;
        rq.ocf = OCF_READ_PAGE_ACTIVITY;
        rq.rparam = &rp;
        rq.rlen = READ_PAGE_ACTIVITY_RP_SIZE;
        if (hci_send_req(s, &rq, 1000) < 0) {
            BOT_NOTIFY_ERROR("Can't read page parameters on hci%d: %s (%d)", hdev, strerror(errno), errno);
            return;
        }
        if (rp.status) {
            BOT_NOTIFY_DEBUG("Read page parameters on hci%d returned status %d",
                             hdev, rp.status);
            return;
        }
        print_dev_hdr(&di);
        window   = btohs(rp.window);
        interval = btohs(rp.interval);
        BOT_NOTIFY_DEBUG("\tPage interval: %u slots (%.2f ms), "
                         "window: %u slots (%.2f ms)",
                         interval, (float)interval * 0.625,
                         window, (float)window * 0.625);
    }
}

void GattSrv::HCIpage_to(int hdev, char *opt) {
    struct hci_request rq;
    int s;
    if ((s = hci_open_dev(hdev)) < 0) {
        BOT_NOTIFY_ERROR("Can't open device hci%d: %s (%d)", hdev, strerror(errno), errno);
        return;
    }
    memset(&rq, 0, sizeof(rq));
    if (opt) {
        unsigned int timeout;
        write_page_timeout_cp cp;
        if (sscanf(opt,"%5u", &timeout) != 1) {
            BOT_NOTIFY_DEBUG("Invalid argument format");
            return;
        }
        rq.ogf = OGF_HOST_CTL;
        rq.ocf = OCF_WRITE_PAGE_TIMEOUT;
        rq.cparam = &cp;
        rq.clen = WRITE_PAGE_TIMEOUT_CP_SIZE;
        cp.timeout = htobs((uint16_t) timeout);
        if (timeout < 0x01 || timeout > 0xFFFF)
            BOT_NOTIFY_DEBUG("Warning: page timeout out of range!");
        if (hci_send_req(s, &rq, 2000) < 0) {
            BOT_NOTIFY_ERROR("Can't set page timeout on hci%d: %s (%d)", hdev, strerror(errno), errno);
            return;
        }
    } else {
        uint16_t timeout;
        read_page_timeout_rp rp;
        rq.ogf = OGF_HOST_CTL;
        rq.ocf = OCF_READ_PAGE_TIMEOUT;
        rq.rparam = &rp;
        rq.rlen = READ_PAGE_TIMEOUT_RP_SIZE;
        if (hci_send_req(s, &rq, 1000) < 0) {
            BOT_NOTIFY_ERROR("Can't read page timeout on hci%d: %s (%d)", hdev, strerror(errno), errno);
            return;
        }
        if (rp.status) {
            BOT_NOTIFY_DEBUG("Read page timeout on hci%d returned status %d",
                             hdev, rp.status);
            return;
        }
        print_dev_hdr(&di);
        timeout = btohs(rp.timeout);
        BOT_NOTIFY_DEBUG("\tPage timeout: %u slots (%.2f ms)",
                         timeout, (float)timeout * 0.625);
    }
}

void GattSrv::HCIafh_mode(int hdev, char *opt) {
    int dd;
    dd = hci_open_dev(hdev);
    if (dd < 0) {
        BOT_NOTIFY_ERROR("Can't open device hci%d: %s (%d)", hdev, strerror(errno), errno);
        return;
    }
    if (opt) {
        uint8_t mode = atoi(opt);
        if (hci_write_afh_mode(dd, mode, 2000) < 0) {
            BOT_NOTIFY_ERROR("Can't set AFH mode on hci%d: %s (%d)", hdev, strerror(errno), errno);
            return;
        }
    } else {
        uint8_t mode;
        if (hci_read_afh_mode(dd, &mode, 1000) < 0) {
            BOT_NOTIFY_ERROR("Can't read AFH mode on hci%d: %s (%d)", hdev, strerror(errno), errno);
            return;
        }
        print_dev_hdr(&di);
        BOT_NOTIFY_DEBUG("\tAFH mode: %s", mode == 1 ? "Enabled" : "Disabled");
    }
}

void GattSrv::HCIssp_mode(int hdev, char *opt) {
    int dd;
    dd = hci_open_dev(hdev);
    if (dd < 0) {
        BOT_NOTIFY_ERROR("Can't open device hci%d: %s (%d)", hdev, strerror(errno), errno);
        return;
    }
    if (opt) {
        uint8_t mode = atoi(opt);
        if (hci_write_simple_pairing_mode(dd, mode, 2000) < 0) {
            BOT_NOTIFY_ERROR("Can't set Simple Pairing mode on hci%d: %s (%d)", hdev, strerror(errno), errno);
            return;
        }
    } else {
        uint8_t mode;
        if (hci_read_simple_pairing_mode(dd, &mode, 1000) < 0) {
            BOT_NOTIFY_ERROR("Can't read Simple Pairing mode on hci%d: %s (%d)", hdev, strerror(errno), errno);
            return;
        }
        print_dev_hdr(&di);
        BOT_NOTIFY_DEBUG("\tSimple Pairing mode: %s",
                         mode == 1 ? "Enabled" : "Disabled");
    }
}

void GattSrv::HCIrevision(int hdev) {
    struct hci_version ver;
    int dd;
    dd = hci_open_dev(hdev);
    if (dd < 0) {
        BOT_NOTIFY_ERROR("Can't open device hci%d: %s (%d)", hdev, strerror(errno), errno);
        return;
    }
    if (hci_read_local_version(dd, &ver, 1000) < 0) {
        BOT_NOTIFY_ERROR("Can't read version info for hci%d: %s (%d)", hdev, strerror(errno), errno);
        return;
    }
    return;
}

void GattSrv::HCIblock(int hdev, char *opt) {
    bdaddr_t bdaddr;
    int dd;
    if (!opt)
        return;
    dd = hci_open_dev(hdev);
    if (dd < 0) {
        BOT_NOTIFY_ERROR("Can't open device hci%d: %s (%d)", hdev, strerror(errno), errno);
        return;
    }
    str2ba(opt, &bdaddr);
    if (ioctl(dd, HCIBLOCKADDR, &bdaddr) < 0) {
        BOT_NOTIFY_ERROR("ioctl(HCIBLOCKADDR)");
        return;
    }
    hci_close_dev(dd);
}

void GattSrv::HCIunblock(int hdev, char *opt) {
    bdaddr_t bdaddr = {{0, 0, 0, 0, 0, 0}};
    bdaddr_t  _BDADDR_ANY = {{0, 0, 0, 0, 0, 0}};

    int dd;
    if (!opt)
        return;
    dd = hci_open_dev(hdev);
    if (dd < 0) {
        BOT_NOTIFY_ERROR("Can't open device hci%d: %s (%d)", hdev, strerror(errno), errno);
        return;
    }
    if (!strcasecmp(opt, "all"))
        bacpy(&bdaddr, &_BDADDR_ANY);
    else
        str2ba(opt, &bdaddr);
    if (ioctl(dd, HCIUNBLOCKADDR, &bdaddr) < 0) {
        BOT_NOTIFY_ERROR("ioctl(HCIUNBLOCKADDR)");
        return;
    }
    hci_close_dev(dd);
}

void GattSrv::print_dev_hdr(struct hci_dev_info *di) {
    static int hdr = -1;
    char addr[18];
    if (hdr == di->dev_id)
        return;
    hdr = di->dev_id;
    ba2str(&di->bdaddr, addr);
    BOT_NOTIFY_DEBUG("%s:\tType: %s  Bus: %s", di->name,
                     hci_typetostr((di->type & 0x30) >> 4),
                     hci_bustostr(di->type & 0x0f));
    BOT_NOTIFY_DEBUG("\tBD Address: %s  ACL MTU: %d:%d  SCO MTU: %d:%d",
                     addr, di->acl_mtu, di->acl_pkts,
                     di->sco_mtu, di->sco_pkts);
}

/**************************************************************
 * static functions and callbacks implementation
 * ***********************************************************/
static uint16_t find_attr_idx_by_handle(gatt_db_attribute *attrib)
{
    uint16_t idx = RING_CHARACTERISTICS_MAX;
    const ServiceInfo_t* svc = BlePairing::getInstance()->GetServiceTable();
    if (svc)
    {
        uint16_t handle = gatt_db_attribute_get_handle(attrib);
        for (idx = 0; idx < svc->NumberAttributes; idx++)
        {
            if (GattSrv::mServer.ring_attr_handle[idx] == handle)
                break;
        }
    }
    return idx;
}

static void gatt_attr_read_cb(struct gatt_db_attribute *attrib,
                    unsigned int id, uint16_t offset,
                    uint8_t opcode, struct bt_att *att,
                    void *user_data)
{
    (void) offset;
    (void) opcode;
    (void) att;
    (void) user_data;

    const ServiceInfo_t* svc = BlePairing::getInstance()->GetServiceTable();
    uint16_t AttribueIdx = find_attr_idx_by_handle(attrib);

    if (AttribueIdx < svc->NumberAttributes)
    {
        BOT_NOTIFY_DEBUG("gatt_attr_read_cb called for [%s]", svc->AttributeList[AttribueIdx].AttributeName);
        GattSrv *gatt = (GattSrv *) GattSrv::getInstance();
        // note: AttribueIdx is passed as offset on purpose
        gatt->ProcessRegisteredCallback((GATM_Event_Type_t) Ble::Property::Read, RING_PAIRING_SVC_IDX, AttribueIdx);
    }
    else
        BOT_NOTIFY_WARNING("gatt_attr_read_cb UNKNOWN for id %d", id);
}
static void gatt_attr_write_cb(struct gatt_db_attribute *attrib,
                    unsigned int id, uint16_t offset,
                    const uint8_t *value, size_t len,
                    uint8_t opcode, struct bt_att *att,
                    void *user_data)
{
    (void) offset;
    (void) opcode;
    (void) att;
    (void) user_data;

    const ServiceInfo_t* svc = BlePairing::getInstance()->GetServiceTable();
    uint16_t AttribueIdx = find_attr_idx_by_handle(attrib);

    if (AttribueIdx < svc->NumberAttributes)
    {
        BOT_NOTIFY_DEBUG("gatt_attr_write_cb called for [%s] for %d bytes, [%s]", svc->AttributeList[AttribueIdx].AttributeName, (int) len, (char*) value);
        GattSrv *gatt = (GattSrv *) GattSrv::getInstance();
        // note: AttribueIdx is passed as offset on purpose
        gatt->ProcessRegisteredCallback((GATM_Event_Type_t) Ble::Property::Write, RING_PAIRING_SVC_IDX, AttribueIdx);
    }
    else
        BOT_NOTIFY_WARNING("gatt_attr_write_cb UNKNOWN called for id %d with %d bytes, [%s]", id, (int) len, (char*) value);
}

static void confirm_write(struct gatt_db_attribute *attr, int err, void *user_data)
{
    (void) attr;
    if (err != Error::NONE)
        BOT_NOTIFY_ERROR("Error caching/writing attribute %s - err: %d", (char*) user_data, err);
}

static int populate_gatt_service(void)
{
    const ServiceInfo_t* svc = BlePairing::getInstance()->GetServiceTable();
    if (!svc)
        return Error::NOT_INITIALIZED;

    bt_uuid_t uuid;
    struct gatt_db_attribute *ringsvc;
    bool primary = true;

    /* Add the RING service*/
    uint128_t uuid128;
    memcpy(&uuid128.data, &svc->ServiceUUID, sizeof(uint128_t));
    bt_uuid128_create(&uuid, uuid128);
    ringsvc = gatt_db_add_service_ext(GattSrv::mServer.sref->db, &uuid, primary, 32, gatt_attr_read_cb, gatt_attr_write_cb);
    GattSrv::mServer.ring_svc_handle = gatt_db_attribute_get_handle(ringsvc);

    for (int idx = 0; idx < (int) svc->NumberAttributes && idx < RING_CHARACTERISTICS_MAX; idx++)
    {
        CharacteristicInfo_t *ch_info = NULL;
        struct gatt_db_attribute *attr_new = NULL;

        if (NULL != (ch_info = (CharacteristicInfo_t *)svc->AttributeList[idx].Attribute))
        {
            memcpy(&uuid128.data, &ch_info->CharacteristicUUID, sizeof(uint128_t));
            bt_uuid128_create(&uuid, uuid128);

            attr_new =  gatt_db_service_add_characteristic(ringsvc, &uuid,
                          BT_ATT_PERM_READ | BT_ATT_PERM_WRITE,
                          BT_GATT_CHRC_PROP_READ | BT_GATT_CHRC_PROP_WRITE | BT_GATT_CHRC_PROP_NOTIFY,
                          NULL, NULL,   // don't overwrite callbacks for read/write each attr
                          NULL);        // optional user_data to be passed to every callback

            GattSrv::mServer.ring_attr_handle[idx] = gatt_db_attribute_get_handle(attr_new);

            if (ch_info->Value && (ch_info->ValueLength > 0))
            {
                gatt_db_attribute_write(attr_new, 0,
                                        (const uint8_t *) ch_info->Value, ch_info->ValueLength,
                                        BT_ATT_OP_WRITE_REQ, NULL,
                                        confirm_write, (void*) svc->AttributeList[idx].AttributeName);
            }
        }
    }

    bool ret = gatt_db_service_set_active(ringsvc, true);
    BOT_NOTIFY_DEBUG("populate_gatt_service ret %d", ret);
    return Error::NONE;
}

static void att_disconnect_cb(int err, void *user_data)
{
    (void) user_data;
    BOT_NOTIFY_INFO("Device disconnected: %s; exiting GATT Server loop", strerror(err));

    GattSrv * gatt = (GattSrv*) GattSrv::getInstance();
    gatt->ProcessRegisteredCallback((GATM_Event_Type_t)Ble::Property::Disconnected, RING_PAIRING_SVC_IDX, -1);
    mainloop_quit();
}

static void att_debug_cb(const char *str, void *user_data)
{
    const char *prefix = (const char *) user_data;
    BOT_NOTIFY_INFO("%s %s", prefix, str);
}

static void gatt_debug_cb(const char *str, void *user_data)
{
    const char *prefix = (const char *) user_data;
    BOT_NOTIFY_INFO("%s %s", prefix, str);
}

static void server_destroy(struct server_ref *server)
{
    bt_gatt_server_unref(server->gatt);
    gatt_db_unref(server->db);
}

int server_create()
{
    mainloop_init();

    uint16_t mtu = 0;
    struct server_ref *server = new0(struct server_ref, 1);
    if (!server) {
        BOT_NOTIFY_ERROR("Failed to allocate memory for server reference");
        goto fail;
    }

    server->att = bt_att_new(GattSrv::mServer.fd, false);
    if (!server->att) {
        BOT_NOTIFY_ERROR("Failed to initialze ATT transport layer");
        goto fail;
    }

    if (!bt_att_set_close_on_unref(server->att, true)) {
        BOT_NOTIFY_ERROR("Failed to set up ATT transport layer");
        goto fail;
    }

    if (!bt_att_register_disconnect(server->att, att_disconnect_cb, NULL, NULL)) {
        BOT_NOTIFY_ERROR("Failed to set ATT disconnect handler");
        goto fail;
    }

    server->db = gatt_db_new();
    if (!server->db) {
        BOT_NOTIFY_ERROR("Failed to create GATT database");
        goto fail;
    }

    server->gatt = bt_gatt_server_new(server->db, server->att, mtu);
    if (!server->gatt) {
        BOT_NOTIFY_ERROR("Failed to create GATT server");
        goto fail;
    }

    GattSrv::mServer.sref = server;
    // enable debug for levels TRACE DEBUG and INFO
    if (GetLogLevel() > BOT_NOTIFY_WARNING_T) {
        bt_att_set_debug(server->att, att_debug_cb, (void*) "att: ", NULL);
        bt_gatt_server_set_debug(server->gatt, gatt_debug_cb, (void*) "server: ", NULL);
    }

    /* bt_gatt_server already holds a reference */
    return populate_gatt_service();

fail:
    if (server->db)
        gatt_db_unref(server->db);
    if (server->att)
        bt_att_unref(server->att);
    if (GattSrv::mServer.fd >=0 )
    {
        close(GattSrv::mServer.fd);
        GattSrv::mServer.fd = -1;
    }
    return Error::FAILED_INITIALIZE;
}

static void signal_cb(int signum, void *user_data)
{
    (void) user_data;
    switch (signum) {
    case SIGINT:
    case SIGTERM:
        mainloop_quit();
        break;
    default:
        break;
    }
}

///
/// \brief l2cap_le_att_listen_and_accept
/// \param data
/// \return
///
static void* l2cap_le_att_listen_and_accept(void *data __attribute__ ((unused)))
{
    #define ATT_CID 4
    int nsk;
    struct sockaddr_l2 srcaddr, addr;
    socklen_t optlen;
    struct bt_security btsec;
    char ba[18];
    bdaddr_t  _BDADDR_ANY = {{0, 0, 0, 0, 0, 0}};
    GattSrv *gatt = (GattSrv *) GattSrv::getInstance();

    bdaddr_t src_addr;
    int dev_id = -1;
    int sec = BT_SECURITY_LOW;
    uint8_t src_type = BDADDR_LE_PUBLIC;

    if (dev_id == -1)
        bacpy(&src_addr, &_BDADDR_ANY);
    else if (hci_devba(dev_id, &src_addr) < 0) {
        BOT_NOTIFY_ERROR("Adapter not available %s (%d)", strerror(errno), errno);
        return (void*) EXIT_FAILURE;
    }

    GattSrv::mServer.hci_socket = socket(PF_BLUETOOTH, SOCK_SEQPACKET, BTPROTO_L2CAP);
    if (GattSrv::mServer.hci_socket < 0) {
        BOT_NOTIFY_ERROR("Failed to create L2CAP socket %s (%d)", strerror(errno), errno);
        goto fail;
    }

    /* Set up source address */
    memset(&srcaddr, 0, sizeof(srcaddr));
    srcaddr.l2_family = AF_BLUETOOTH;
    srcaddr.l2_cid = htobs(ATT_CID);
    srcaddr.l2_bdaddr_type = src_type;
    bacpy(&srcaddr.l2_bdaddr, &src_addr);

    if (bind(GattSrv::mServer.hci_socket, (struct sockaddr *) &srcaddr, sizeof(srcaddr)) < 0) {
        BOT_NOTIFY_ERROR("Failed to bind L2CAP socket %s (%d)", strerror(errno), errno);
        goto fail;
    }

    /* Set the security level */
    memset(&btsec, 0, sizeof(btsec));
    btsec.level = sec;
    if (setsockopt(GattSrv::mServer.hci_socket, SOL_BLUETOOTH, BT_SECURITY, &btsec, sizeof(btsec)) != 0) {
        BOT_NOTIFY_ERROR("Failed to set L2CAP security level");
        goto fail;
    }

    if (listen(GattSrv::mServer.hci_socket, 10) < 0) {
        BOT_NOTIFY_ERROR("Listening on socket failed %s (%d)", strerror(errno), errno);
        goto fail;
    }

    BOT_NOTIFY_DEBUG("Started listening on ATT channel. Waiting for connections");

    memset(&addr, 0, sizeof(addr));
    optlen = sizeof(addr);
    nsk = accept(GattSrv::mServer.hci_socket, (struct sockaddr *) &addr, &optlen);
    if (nsk < 0) {
        BOT_NOTIFY_ERROR("Accept failed %s (%d)", strerror(errno), errno);
        goto fail;
    }

    ba2str(&addr.l2_bdaddr, ba);
    BOT_NOTIFY_DEBUG("Accepted connect from %s, nsk=%d", ba, nsk);
    close(GattSrv::mServer.hci_socket);
    GattSrv::mServer.hci_socket = -1;
    BOT_NOTIFY_DEBUG("GattSrv: listening hci_socket released");

    GattSrv::mServer.fd = nsk;
    if (Error::NONE == server_create())
        BOT_NOTIFY_DEBUG("Running GATT server");
    else
        BOT_NOTIFY_ERROR("GATT server ini failed");

    gatt->ProcessRegisteredCallback((GATM_Event_Type_t) Ble::Property::Connected, RING_PAIRING_SVC_IDX, -1);

    sigset_t mask;
    sigemptyset(&mask);
    sigaddset(&mask, SIGINT);
    sigaddset(&mask, SIGTERM);

    mainloop_set_signal(&mask, signal_cb, NULL, NULL);
    mainloop_run();

    BOT_NOTIFY_DEBUG("GATT server exited mainloop, releasing...");
    server_destroy(GattSrv::mServer.sref);
    GattSrv::mServer.sref = NULL;
    GattSrv::mServer.hci_thread_id = 0;

    pthread_exit(NULL);
    return (void*) 0;

fail:
    close(GattSrv::mServer.hci_socket);
    pthread_exit(NULL);
    return (void*) -1;
}
