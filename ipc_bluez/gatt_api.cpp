#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <errno.h>
#include <getopt.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <sys/param.h>
#include <sys/socket.h>
#include <unistd.h>
#include <pthread.h>

#include "icommon.h"

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
    #include "src/shared/gatt-db.h"
}

using namespace Ble;

GattSrv* GattSrv::instance = NULL;
GattServerInfo_t GattSrv::mServer =
{
    .fd = -1,
    .hci_socket = -1,
    .hci_thread_id = 0,
    .sref = NULL,
};

GattSrv* GattSrv::getInstance()
{
    if (instance == NULL)
        instance = new GattSrv();
    return instance;
}

GattSrv::GattSrv() :
    mInitialized(false)
    ,mServiceTable(NULL)
    ,mOnCharCb(NULL)
{
    strcpy(mDeviceName, "ClientDevice");
    strcpy(mMacAddress, "AA:BB:CC:DD:EE:FF");
}

// static callback definitions
typedef int (*cb_on_accept) (int fd);
static void* start_L2CAP_listener(void *data);
static int get_connections(int flag, long arg);

int GattSrv::Initialize()
{
    int ctl, ret = Error::NONE;
    if (!mInitialized)
    {
        DEBUG_PRINTF("GattSrv::Initialize");
        if (0 <= (ctl = OpenSocket(di)))
        {
            HCIreset(ctl, di.dev_id);
            sleep(1);
            HCIscan(ctl, di.dev_id, (char*) "piscan");
            sleep(1);

            // set simple secure pairing mode
            HCIssp_mode(di.dev_id, (char*) "1");

            close(ctl);
            mInitialized = true;
        }
        else
            ret = Error::FAILED_INITIALIZE;
    }
    return ret;
}

///
/// \brief GattSrv::Shutdown
/// \return error code
///
int GattSrv::Shutdown()
{
    int ctl, ret = Error::NONE;
    if (mInitialized)
    {
        DEBUG_PRINTF("GattSrv::Shutdown");
        mainloop_quit();

        if (0 <= (ctl = OpenSocket(di)))
        {
            HCIdown(ctl, di.dev_id);

            close(ctl);
            mInitialized = false;
        }
        else
            ret = Error::FAILED_INITIALIZE;
    }
    return ret;
}

int GattSrv::Configure(Ble::Config::DeviceConfig_t* aConfig)
{
    int ret_val = Error::NONE;

    while (aConfig != NULL && aConfig->tag != Ble::Config::EOL)
    {
        switch (aConfig->tag)
        {
        case Ble::Config::ServiceTable:
            if ((aConfig->params.count > 0) && (aConfig->params.strParam != NULL))
            {
                mServiceTable = (ServiceInfo_t *) aConfig->params.strParam;
                DEBUG_PRINTF("Service Table configure succeeded, attr count = %d", mServiceTable->NumberAttributes);
            }
            else
            {
                /* One or more of the necessary parameters is/are invalid.     */
                DEBUG_PRINTF("Usage: Ble::Config::ServiceTable: ServiceInfo_t *ptr, int numOfSvc");
            }
            break;

        case Ble::Config::LocalDeviceName:
            ret_val = SetLocalDeviceName(&aConfig->params);
            break;

        case Ble::Config::LocalClassOfDevice:
            ret_val = SetLocalClassOfDevice(&aConfig->params);
            break;

        case Ble::Config::MACAddress:
            strncpy(mMacAddress, aConfig->params.strParam, DEV_MAC_ADDR_LEN);
            DEBUG_PRINTF("Device Config: stored MAC address = %s", mMacAddress);
            break;

        default:
            DEBUG_PRINTF("Device Config: unknown tag = %d", aConfig->tag);
            ret_val = Error::INVALID_PARAMETER;
            break;
        }
        aConfig++;
    }
    return ret_val;
}

///
/// \brief GattSrv::RegisterCharacteristicAccessCallback
/// \param aCb
/// \return
///
int GattSrv::RegisterCharacteristicAccessCallback(onCharacteristicAccessCallback aCb)
{
    if (!aCb)
        return Error::INVALID_PARAMETER;
    if (!mInitialized)
        return Error::NOT_INITIALIZED;
    if (mOnCharCb)
        return Error::INVALID_STATE;

    mOnCharCb = aCb;
    return Error::NONE;
}

///
/// \brief GattSrv::UnregisterCharacteristicAccessCallback
/// \param aCb
/// \return
///
int GattSrv::UnregisterCharacteristicAccessCallback(onCharacteristicAccessCallback aCb)
{
    if (!aCb)
        return Error::INVALID_PARAMETER;
    if (!mInitialized)
        return Error::NOT_INITIALIZED;
    if (!mOnCharCb || mOnCharCb != aCb)
        return Error::INVALID_STATE;

    mOnCharCb = NULL;
    return Error::NONE;
}

///
/// \brief GattSrv::SetLocalDeviceName
/// \param aParams
/// \return error code
///
int GattSrv::SetLocalDeviceName(Ble::Config::Parameter_t *aParams __attribute__ ((unused)))
{
    int ret_val = Error::UNDEFINED;
    if (mInitialized)
    {
        if ((aParams) && (aParams->count) && (aParams->strParam))
        {
            strncpy(mDeviceName, aParams->strParam, DEV_NAME_LEN);
        }

        DEBUG_PRINTF("Attempting to set Device Name to: \"%s\".", mDeviceName);
        HCIname(di.dev_id, mDeviceName);
        ret_val = Error::NONE;
    }
    else
    {
        DEBUG_PRINTF("GattSrv has not been Initialized.");
        ret_val = Error::NOT_INITIALIZED;
    }
    return ret_val;
}

///
/// \brief GattSrv::SetLocalClassOfDevice
/// \param aParams
/// \return error code
///
int GattSrv::SetLocalClassOfDevice(Ble::Config::Parameter_t *aParams __attribute__ ((unused)))
{
    int ret_val = Error::UNDEFINED;
    if (mInitialized)
    {
        char device_class[DEV_CLASS_LEN] = "0x1F00"; // Uncategorized, specific device code not specified
        if ((aParams) && (aParams->count) && (aParams->intParam))
        {
            sprintf(device_class, "%06X", aParams->intParam);
        }

        DEBUG_PRINTF("setting Device Class to: \"%s\".", device_class);
        HCIclass(di.dev_id, device_class);
        ret_val = Error::NONE;
    }
    else
    {
        DEBUG_PRINTF("GattSrv has not been Initialized.");
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
int GattSrv::StartAdvertising(Ble::Config::Parameter_t *aParams __attribute__ ((unused)))
{
    int ret_val = Error::UNDEFINED;
    if (mInitialized && mServiceTable && mServiceTable->NumberAttributes)
    {
        DEBUG_PRINTF("Starting LE adv");

        /* First determine if any device is connected         */
        if (get_connections(HCI_UP, di.dev_id) > 0)
        {
            // don't restart advertising if any device is currently connected
            return Ble::Error::RESOURCE_UNAVAILABLE;
        }

        WriteBdaddr(di.dev_id, mMacAddress);
        HCIle_adv(di.dev_id, NULL);

        ret_val = pthread_create(&mServer.hci_thread_id, NULL, start_L2CAP_listener, NULL);
        if (ret_val != Error::NONE)
        {
            DEBUG_PRINTF("failed to create pthread %s (%d)", strerror(errno), errno);
            mServer.hci_thread_id = 0;
            ret_val = Error::PTHREAD_ERROR;
        }
    }
    else
    {
        DEBUG_PRINTF("GattSrv has not been Initialized.");
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
int GattSrv::StopAdvertising(Ble::Config::Parameter_t *aParams __attribute__ ((unused)))
{
    int ret_val = Error::UNDEFINED;
    if (mInitialized)
    {
        DEBUG_PRINTF("Stopping LE adv");
        HCIno_le_adv(di.dev_id);

        mainloop_quit();
        CleanupServiceList();

        ret_val = Error::NONE;
        if (mServer.hci_socket >= 0)
        {
            shutdown(mServer.hci_socket, SHUT_RDWR);
            mServer.hci_socket = -1;
        }

        if (mServer.hci_thread_id > 0)
        {
            // not sure if this is needed - sometimes it hangs on this call
            // ret_val = pthread_cancel(mServer.hci_thread_id);
            // DEBUG_PRINTF("cancel GATT pthread ret_val %d, Errno: %s (%d)", ret_val, strerror(errno), errno);
            mServer.hci_thread_id = 0;
            ret_val = Error::NONE;
        }
    }
    else
    {
        DEBUG_PRINTF("GattSrv has not been Initialized.");
        ret_val = Error::NOT_INITIALIZED;
    }
    return ret_val;
}

///
/// \brief GattSrv::NotifyCharacteristic
/// \param aAttributeIdx
/// \param aPayload
/// \param len
/// \return
///
int GattSrv::NotifyCharacteristic(int aAttributeIdx, const char* aPayload, int len)
{
    int ret_val = Error::INVALID_PARAMETER;
    if (mInitialized && mServiceTable && mServiceTable->NumberAttributes && GattSrv::mServer.sref)
    {
        if (aAttributeIdx < (int) mServiceTable->NumberAttributes)
        {
            DEBUG_PRINTF("sending %s Notify \"%s\"", mServiceTable->AttributeList[aAttributeIdx].AttributeName, aPayload);
            bt_gatt_server_send_notification(GattSrv::mServer.sref->gatt, GattSrv::mServer.client_attr_handle[aAttributeIdx], (const uint8_t*) aPayload, len?len:strlen(aPayload));
            ret_val = Error::NONE;
        }
        else
        {
            DEBUG_PRINTF("invalid attribute index %d.", aAttributeIdx);
            ret_val = Error::INVALID_STATE;
        }
    }
    else if (!GattSrv::mServer.sref)
    {
        DEBUG_PRINTF("GattSrv is not connected.");
        ret_val = Error::INVALID_STATE;
    }
    else
    {
        DEBUG_PRINTF("GattSrv has not been Initialized.");
        ret_val = Error::NOT_INITIALIZED;
    }
    return ret_val;
}

///
/// \brief GattSrv::GATTUpdateCharacteristic
/// \param attr_idx
/// \param str_data
/// \param len
/// \return
///
int GattSrv::GATTUpdateCharacteristic(int attr_idx, const char *str_data, int len)
{
    int ret_val = Error::UNDEFINED;
    if (mServiceTable == NULL)
        ret_val = Error::NOT_INITIALIZED;
    else if (attr_idx >= (int) mServiceTable->NumberAttributes)
        ret_val = Error::INVALID_PARAMETER;
    else
    {
        AttributeInfo_t *attr = (AttributeInfo_t*) &mServiceTable->AttributeList[attr_idx];
        if (len > (int) attr->MaximumValueLength)
            ret_val = Error::INVALID_PARAMETER;
        else
        {
            if (attr->Value && ((int) attr->ValueLength == len) && str_data && !memcmp(attr->Value, str_data, len))
            {
                // the same - skip
                ret_val = Error::NONE;
            }
            else
            {
                if (attr->Value && attr->AllocatedValue)
                {
                    free(attr->Value);
                    attr->AllocatedValue = 0;
                }
                attr->Value = NULL;
                attr->ValueLength = 0;

                if (str_data && len)
                {
                    attr->Value = (char*) malloc(len);
                    if (attr->Value)
                    {
                        attr->AllocatedValue = 1;
                        memcpy(attr->Value, str_data, attr->ValueLength = len);
                    }
                }
                ret_val = Error::NONE;
            }
        }
    }
    return ret_val;
}

///
/// \brief GattSrv::ProcessRegisteredCallback
/// \param aEventType
/// \param aAttrIdx
/// \return
///
int GattSrv::ProcessRegisteredCallback(Ble::Property::Access aEventType, int aAttrIdx)
{
    if (mOnCharCb)
        (*(mOnCharCb))(aAttrIdx,  aEventType);
    return Error::NONE;
}

///
/// \brief QueryLocalDeviceProperties
/// \param aParams - not used
/// \return
///
int GattSrv::QueryLocalDeviceProperties(Ble::Config::Parameter_t *aParams __attribute__ ((unused)))
{
    PrintDeviceHeader(&di);
    HCIname(di.dev_id, NULL);

    DEBUG_PRINTF("Connections:\n");
    DEBUG_PRINTF("%sconnected", (get_connections(HCI_UP, di.dev_id) > 0) ? "":"dis");

    return Error::NONE;
}

///
/// \brief GattSrv::CleanupServiceList
/// Loop through the attribute list for this service and free
/// any attributes with dynamically allocated buffers.
///
void GattSrv::CleanupServiceList(void)
{
    for (unsigned int idx=0; (mServiceTable != NULL) && (idx < mServiceTable->NumberAttributes); idx++)
    {
        AttributeInfo_t *charin;
        if (NULL != (charin = (AttributeInfo_t *)&mServiceTable->AttributeList[idx]))
        {
            // DEBUG_PRINTF("Cleanup: attr idx %d, AllocatedValue = %d", idx, charin->AllocatedValue);
            if ((charin->AllocatedValue) && (charin->Value != NULL))
            {
                free(charin->Value);
                charin->Value          = NULL;
                charin->ValueLength    = 0;
                charin->AllocatedValue = 0;
            }
        }
    }
    memset(mServer.client_attr_handle, 0, sizeof(mServer.client_attr_handle));
    memset(&mServer.client_svc_handle, 0, sizeof(mServer.client_svc_handle));
}

/**********************************************************
 * Helper functions
 *********************************************************/
///
/// \brief GattSrv::OpenSocket
/// \param di
/// \return
///
int GattSrv::OpenSocket(struct hci_dev_info &di)
{
    int ctl = -1;
    (void) di;

//    bdaddr_t  _BDADDR_ANY = {{0, 0, 0, 0, 0, 0}};

    /* Open HCI socket  */
    if ((ctl = socket(AF_BLUETOOTH, SOCK_RAW, BTPROTO_HCI)) < 0)
    {
        DEBUG_PRINTF("Can't open HCI socket. %s (%d)", strerror(errno), errno);
        return Error::FAILED_INITIALIZE;
    }

//    if (ioctl(ctl, HCIGETDEVINFO, (void *) &di))
//    {
//        DEBUG_PRINTF("Can't get device info. %s (%d)", strerror(errno), errno);
//        return Error::FAILED_INITIALIZE;
//    }

#ifdef TEST_HCI1_DEVICE
    di.dev_id = TEST_HCI1_DEVICE;
#else
    di.dev_id = 0;
#endif

//    if (hci_test_bit(HCI_RAW, &di.flags) && !bacmp(&di.bdaddr, &_BDADDR_ANY)) {
//        int dd = hci_open_dev(di.dev_id);
//        hci_read_bd_addr(dd, &di.bdaddr, 1000);
//        hci_close_dev(dd);
//    }
    return ctl;
}

///
/// \brief conn_list
/// \param s
/// \param dev_id
/// \param arg
/// \return
///
static int conn_list(int s, int dev_id, long arg)
{
    struct hci_conn_list_req *cl;
    struct hci_conn_info *ci;
    int id = arg;
    int i;

    if (id != -1 && dev_id != id)
        return 0;

    if (!(cl = (hci_conn_list_req *) malloc(10 * sizeof(*ci) + sizeof(*cl))))
    {
        DEBUG_PRINTF("Can't allocate memory");
        return 0;
    }
    cl->dev_id = dev_id;
    cl->conn_num = 10;
    ci = cl->conn_info;

    if (ioctl(s, HCIGETCONNLIST, (void *) cl))
    {
        DEBUG_PRINTF("Can't get connection list");
        return 0;
    }

    for (i = 0; i < cl->conn_num; i++, ci++)
    {
        char addr[18];
        char *str;
        ba2str(&ci->bdaddr, addr);
        str = hci_lmtostr(ci->link_mode);
        DEBUG_PRINTF("\t%s type:%d %s handle %d state %d lm %s\n",
                     ci->out ? "<" : ">", ci->type,
                     addr, ci->handle, ci->state, str);
        bt_free(str);
    }

    i = cl->conn_num;
    free(cl);
    return i;
}

///
/// \brief get_connections
/// \param flag
/// \param arg
/// \return
///
static int get_connections(int flag, long arg)
{
    struct hci_dev_list_req *dl;
    struct hci_dev_req *dr;
    int conns = 0;
    int i, sk;

    sk = socket(AF_BLUETOOTH, SOCK_RAW | SOCK_CLOEXEC, BTPROTO_HCI);
    if (sk < 0)
        return -1;

    dl = (hci_dev_list_req *) malloc(HCI_MAX_DEV * sizeof(*dr) + sizeof(*dl));
    if (!dl)
    {
        DEBUG_PRINTF("malloc(HCI_MAX_DEV failed");
        goto done;
    }

    memset(dl, 0, HCI_MAX_DEV * sizeof(*dr) + sizeof(*dl));

    dl->dev_num = HCI_MAX_DEV;
    dr = dl->dev_req;

    if (ioctl(sk, HCIGETDEVLIST, (void *) dl) < 0)
    {
        DEBUG_PRINTF("HCIGETDEVLIST failed");
        goto free;
    }

    for (i = 0; i < dl->dev_num; i++, dr++)
    {
        if (hci_test_bit(flag, &dr->dev_opt))
            if ((conns = conn_list(sk, dr->dev_id, arg)))
                break;
    }

free:
    free(dl);

done:
    close(sk);
    return conns;
}

///
/// \brief GattSrv::HCIssp_mode
/// \param hdev
/// \param opt
///
void GattSrv::HCIssp_mode(int hdev, char *opt)
{
    int dd;
    dd = hci_open_dev(hdev);
    if (dd < 0)
    {
        DEBUG_PRINTF("ERROR: can't open device hci%d: %s (%d)", hdev, strerror(errno), errno);
        return;
    }
    if (opt)
    {
        uint8_t mode = atoi(opt);
        if (hci_write_simple_pairing_mode(dd, mode, 2000) < 0)
        {
            DEBUG_PRINTF("WARNING: can't set simple pairing mode on hci%d: %s (%d)", hdev, strerror(errno), errno);
            return;
        }
    }
}

///
/// \brief GattSrv::HCIscan
/// \param ctl
/// \param hdev
/// \param opt
///
void GattSrv::HCIscan(int ctl, int hdev, char *opt)
{
    struct hci_dev_req dr;
    dr.dev_id  = hdev;
    dr.dev_opt = SCAN_DISABLED;
    if (!strcmp(opt, "iscan"))
        dr.dev_opt = SCAN_INQUIRY;
    else if (!strcmp(opt, "pscan"))
        dr.dev_opt = SCAN_PAGE;
    else if (!strcmp(opt, "piscan"))
        dr.dev_opt = SCAN_PAGE | SCAN_INQUIRY;
    if (ioctl(ctl, HCISETSCAN, (unsigned long) &dr) < 0)
    {
        DEBUG_PRINTF("Can't set scan mode on hci%d: %s (%d)", hdev, strerror(errno), errno);
        return;
    }
}

///
/// \brief GattSrv::HCIup
/// \param ctl
/// \param hdev
///
void GattSrv::HCIup(int ctl, int hdev)
{
    /* Start HCI device */
    if (ioctl(ctl, HCIDEVUP, hdev) < 0)
    {
        if (errno == EALREADY)
            return;
        DEBUG_PRINTF("Can't init device hci%d: %s (%d)", hdev, strerror(errno), errno);
        return;
    }
}

///
/// \brief GattSrv::HCIdown
/// \param ctl
/// \param hdev
///
void GattSrv::HCIdown(int ctl, int hdev)
{
    /* Stop HCI device */
    if (ioctl(ctl, HCIDEVDOWN, hdev) < 0)
    {
        DEBUG_PRINTF("Can't down device hci%d: %s (%d)", hdev, strerror(errno), errno);
        return;
    }
}

///
/// \brief GattSrv::HCIreset
/// \param ctl
/// \param hdev
///
void GattSrv::HCIreset(int ctl, int hdev)
{
    /* Reset HCI device */
    GattSrv::HCIdown(ctl, hdev);
    GattSrv::HCIup(ctl, hdev);
}
///
/// \brief GattSrv::HCIno_le_adv
/// \param hdev
///
void GattSrv::HCIno_le_adv(int hdev)
{
    struct hci_request rq;
    le_set_advertise_enable_cp advertise_cp;
    uint8_t status;
    int dd, ret;
    if (hdev < 0)
        hdev = hci_get_route(NULL);
    dd = hci_open_dev(hdev);
    if (dd < 0)
    {
        DEBUG_PRINTF("Could not open device");
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
    if (ret < 0)
    {
        DEBUG_PRINTF("Can't stop advertise mode on hci%d: %s (%d)", hdev, strerror(errno), errno);
        return;
    }
    if (status)
    {
        DEBUG_PRINTF("LE set advertise disable on hci%d returned status %d", hdev, status);
        return;
    }
}

///
/// \brief GattSrv::WriteBdaddr
/// \param hdev
/// \param opt
///
void GattSrv::WriteBdaddr(int hdev, char *opt)
{
    /* Vendor specific commands from lib/hci.h */
    #define OGF_VENDOR_CMD              0x3f
    // from tools/bdaddr.c
    #define OCF_BCM_WRITE_BD_ADDR       0x0001
    typedef struct
    {
        bdaddr_t    bdaddr;
    } __attribute__ ((packed)) bcm_write_bd_addr_cp;

    struct hci_request rq;
    bcm_write_bd_addr_cp cp;
    uint8_t status;
    int dd, err, ret;
    if (!opt)
        return;
    if (hdev < 0)
        hdev = hci_get_route(NULL);
    dd = hci_open_dev(hdev);
    if (dd < 0)
    {
        err = -errno;
        DEBUG_PRINTF("Could not open device: %s(%d)", strerror(-err), -err);
        return;
    }
    memset(&cp, 0, sizeof(cp));
    str2ba(opt, &cp.bdaddr);
    memset(&rq, 0, sizeof(rq));
    rq.ogf    = OGF_VENDOR_CMD;
    rq.ocf    = OCF_BCM_WRITE_BD_ADDR;
    rq.cparam = &cp;
    rq.clen = sizeof(cp);
    rq.rparam = &status;
    rq.rlen = 1;
    ret = hci_send_req(dd, &rq, 1000);
    if (status || ret < 0)
    {
        err = -errno;
        DEBUG_PRINTF("Can't write BD address for hci%d: " "%s (%d)", hdev, strerror(-err), -err);
    }
    else
        DEBUG_PRINTF("Set BD address %s OK for hci%d ok", opt, hdev);

    hci_close_dev(dd);
}

///
/// \brief GattSrv::HCIle_adv
/// \param hdev
/// \param opt
///
void GattSrv::HCIle_adv(int hdev, char *opt)
{
    struct hci_request rq;
    le_set_advertise_enable_cp advertise_cp;
    le_set_advertising_parameters_cp adv_params_cp;
    uint8_t status;
    int dd, ret;
    if (hdev < 0)
        hdev = hci_get_route(NULL);
    dd = hci_open_dev(hdev);
    if (dd < 0)
    {
        DEBUG_PRINTF("Could not open device");
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

    if (ret < 0)
        goto done;
    else
    {   // disable BR/EDR and add device name
        // hcitool -i hci0 cmd 0x08 0x0008 11 02 01 06 0D 09 52 69 6e 67 53 65 74 75 70 2d 61 63
        le_set_advertising_data_cp adv_data_cp;
        memset(&adv_data_cp, 0, sizeof(adv_data_cp));

        int name_len = strlen(mDeviceName);
        const uint8_t flags[] = {0x02, 0x01, 0x06};
        const int flags_len = sizeof(flags)/sizeof(uint8_t);

        int offset = 0;
        // adv_data_cp.data[offset++] = flags_len + name_len + 1 /*segment len = name_len+type*/ + 1; /*type = 0x09*/

        DEBUG_PRINTF("setting ads flags 0x%02x 0x%02x 0x%02x", flags[0], flags[1], flags[2]);
        memcpy(&adv_data_cp.data[offset], flags, flags_len); offset += flags_len;

        DEBUG_PRINTF("setting ads name %s in ads data", mDeviceName);
        adv_data_cp.data[offset++] = (name_len + 1); // name len + type
        adv_data_cp.data[offset++] = 0x09;           // device name
        memcpy(&adv_data_cp.data[offset], mDeviceName, name_len); offset += name_len;

        adv_data_cp.length = offset; // adv_data_cp.data[0] +1;

        memset(&rq, 0, sizeof(rq));
        rq.ogf = OGF_LE_CTL;
        rq.ocf = OCF_LE_SET_ADVERTISING_DATA;
        rq.cparam = &adv_data_cp;
        rq.clen = LE_SET_ADVERTISING_DATA_CP_SIZE;
        rq.rparam = &status;
        rq.rlen = 1;

        ret = hci_send_req(dd, &rq, 1000);
        if(ret < 0)
        {
            DEBUG_PRINTF("failed OCF_LE_SET_ADVERTISING_DATA %s (%d)", strerror(errno), errno);
            goto done;
        }
        else if (mServiceTable != NULL)
        {   // set scan response with UUID
            // hci cmd 0x08 0x0009 12 11 07 FB 34 9B 5F 80 00 00 80 00 10 00 00 CE FA 00 00
            le_set_scan_response_data_cp scn_data_cp;
            memset(&scn_data_cp, 0, sizeof(scn_data_cp));
            scn_data_cp.data[0] = 17;
            scn_data_cp.data[1] = 0x07;
            memcpy(&scn_data_cp.data[2], &mServiceTable->ServiceUUID, sizeof(UUID_128_t));
            scn_data_cp.length = 18;

            memset(&rq, 0, sizeof(rq));
            rq.ogf = OGF_LE_CTL;
            rq.ocf = OCF_LE_SET_SCAN_RESPONSE_DATA;
            rq.cparam = &scn_data_cp;
            rq.clen = LE_SET_SCAN_RESPONSE_DATA_CP_SIZE;
            rq.rparam = &status;
            rq.rlen = 1;

            ret = hci_send_req(dd, &rq, 1000);
            if(ret < 0)
            {
                DEBUG_PRINTF("failed OCF_LE_SET_SCAN_RESPONSE_DATA %s (%d)", strerror(errno), errno);
                goto done;
            }
        }
    }

done:
    hci_close_dev(dd);
    if (ret < 0)
    {
        DEBUG_PRINTF("Can't set advertise mode on hci%d: %s (%d)", hdev, strerror(errno), errno);
        return;
    }
    if (status)
    {
        DEBUG_PRINTF("LE set advertise enable on hci%d returned status %d", hdev, status);
        return;
    }
}

///
/// \brief GattSrv::HCIclass
/// \param hdev
/// \param opt
///
void GattSrv::HCIclass(int hdev, char *opt)
{
    int s = hci_open_dev(hdev);
    if (s < 0)
    {
        DEBUG_PRINTF("Can't open device hci%d: %s (%d)", hdev, strerror(errno), errno);
        return;
    }
    if (opt)
    {
        uint32_t cod = strtoul(opt, NULL, 16);
        if (hci_write_class_of_dev(s, cod, 2000) < 0)
        {
            DEBUG_PRINTF("Can't write local class of device on hci%d: %s (%d)", hdev, strerror(errno), errno);
            return;
        }
    }
}

///
/// \brief GattSrv::HCIname
/// \param hdev
/// \param opt
///
void GattSrv::HCIname(int hdev, char *opt)
{
    int dd;
    dd = hci_open_dev(hdev);
    if (dd < 0)
    {
        DEBUG_PRINTF("Can't open device hci%d: %s (%d)", hdev, strerror(errno), errno);
        return;
    }
    if (opt)
    {
        if (hci_write_local_name(dd, opt, 2000) < 0)
        {
            DEBUG_PRINTF("Can't change local name on hci%d: %s (%d)", hdev, strerror(errno), errno);
            return;
        }
    }
    else
    {
        char name[249];
        int i;
        if (hci_read_local_name(dd, sizeof(name), name, 1000) < 0)
        {
            DEBUG_PRINTF("Can't read local name on hci%d: %s (%d)", hdev, strerror(errno), errno);
            return;
        }
        for (i = 0; i < 248 && name[i]; i++)
        {
            if ((unsigned char) name[i] < 32 || name[i] == 127)
                name[i] = '.';
        }
        name[248] = '\0';
        PrintDeviceHeader(&di);
        DEBUG_PRINTF("\tName: '%s'", name);
    }
    hci_close_dev(dd);
}

///
/// \brief GattSrv::PrintDeviceHeader
/// \param di
///
void GattSrv::PrintDeviceHeader(struct hci_dev_info *di)
{
    static int hdr = -1;
    char addr[18];
    if (hdr == di->dev_id)
        return;
    hdr = di->dev_id;
    ba2str(&di->bdaddr, addr);
    DEBUG_PRINTF("%s:\tType: %s  Bus: %s", di->name,
                     hci_typetostr((di->type & 0x30) >> 4),
                     hci_bustostr(di->type & 0x0f));
    DEBUG_PRINTF("\tBD Address: %s  ACL MTU: %d:%d  SCO MTU: %d:%d",
                     addr, di->acl_mtu, di->acl_pkts,
                     di->sco_mtu, di->sco_pkts);
}

////
/// \brief GattSrv::GetAttributeIdxByOffset
/// \param ServiceID - not used here
/// \param AttributeOffset
/// \return index in service table or -1 if not found
///
int GattSrv::GetAttributeIdxByOffset(unsigned int AttributeOffset)
{
    for (unsigned int i = 0; mServiceTable && (i < mServiceTable->NumberAttributes); i++)
    {
        if (mServiceTable->AttributeList[i].AttributeOffset == AttributeOffset)
        {
            return i;
        }
    }
    return -1;
}

///
/// \brief gatt_characteristic_read_cb
/// \param attrib
/// \param id
/// \param offset
/// \param opcode
/// \param att
/// \param user_data
///
/// Note: this callback is serving ALL client READ requests
/// returning data from mServiceTable as is"
/// This callback invokes OnAttributeAccessCallback callback via BleApi class
///
static void gatt_characteristic_read_cb(struct gatt_db_attribute *attrib,
                    unsigned int id, uint16_t offset,
                    uint8_t opcode, struct bt_att *att,
                    void *user_data)
{
    (void) att;
    AttributeInfo_t *attr_info = (AttributeInfo_t *) user_data;
    GattSrv * gatt = GattSrv::getInstance();

    DEBUG_PRINTF("read_cb: AttributeOffset = %d", attr_info->AttributeOffset);
    int attr_index = gatt->GetAttributeIdxByOffset(attr_info->AttributeOffset);

    if (attr_index >= 0 )
    {
        AttributeInfo_t *ch_info = attr_info;
        int remain_len = ch_info->ValueLength >= offset ? ch_info->ValueLength - offset : 0;
        DEBUG_PRINTF("read_cb: opcode %d, offset = %d, remlen = %d, attr_idx = %d [%s]\n", opcode, offset, remain_len, attr_index, attr_info->AttributeName);
        gatt_db_attribute_read_result(attrib, id, 0, (const uint8_t *) (remain_len ? &ch_info->Value[offset] : NULL), remain_len);


        // gatt_characteristic_read_cb serves "long read" so callback is needed to be called
        // only when the full payload read
        if (!remain_len || (remain_len <= BLE_READ_PACKET_MAX))
            gatt->ProcessRegisteredCallback(Ble::Property::Read, attr_index);
    }
    else
        DEBUG_PRINTF("read_cb: attr_index is not found for attr_offset = %d", attr_info->AttributeOffset);
}

///
/// \brief gatt_characteristic_write_cb
/// \param attrib
/// \param id
/// \param offset
/// \param value
/// \param len
/// \param opcode
/// \param att
/// \param user_data
///
/// Note: this callback is service ALL client WRITE requests
/// store data "as is" in the mServiceTable
/// This callback invokes OnAttributeAccessCallback callback
///
static void gatt_characteristic_write_cb(struct gatt_db_attribute *attrib,
                    unsigned int id, uint16_t offset,
                    const uint8_t *value, size_t len,
                    uint8_t opcode, struct bt_att *att,
                    void *user_data)
{
    (void) id, (void) offset, (void) opcode, (void) att,(void)user_data, (void) value, (void)len;
    uint8_t ecode = 0;
    GattSrv * gatt = GattSrv::getInstance();

    AttributeInfo_t *attr_info = (AttributeInfo_t *) user_data;

    DEBUG_PRINTF("write_cb: AttributeOffset = %d", attr_info->AttributeOffset);
    int attr_index = gatt->GetAttributeIdxByOffset(attr_info->AttributeOffset);

    if (attr_index >= 0 )
    {
        DEBUG_PRINTF("write_cb: id [%d] offset [%d] attr_index = %d, len = %d, called for %s\n", id, offset, attr_index, (int) len, attr_info->AttributeName);

        gatt_db_attribute_write_result(attrib, id, ecode);

        // update in the mServiceTable value "as is" - if encrypted by client - then it will be encrypted
        gatt->GATTUpdateCharacteristic(attr_index, (const char*) value, len);
        gatt->ProcessRegisteredCallback(Ble::Property::Write, attr_index);
    }
    else
        DEBUG_PRINTF("write_cb: attr_index is not found for attr_offset = %d", attr_info->AttributeOffset);
}

///
/// \brief populate_gatt_service
/// \return
///
static int populate_gatt_service()
{
    // populate_gap_service();
    const ServiceInfo_t* svc = GattSrv::getInstance()->mServiceTable;
    if (!svc)
        return Error::NOT_INITIALIZED;

    bt_uuid_t uuid;
    struct gatt_db_attribute *client_svc;
    bool primary = true;

    /* Add the Client service*/
    int number_of_handlers = GATT_SVC_NUM_OF_HANDLERS_MAX;
    int max_attr = (int) svc->NumberAttributes;

    uint128_t uuid128;
    memcpy(&uuid128.data, &svc->ServiceUUID, sizeof(UUID_128_t));
    bt_uuid128_create(&uuid, uuid128);

    client_svc = gatt_db_add_service(GattSrv::mServer.sref->db, &uuid, primary, number_of_handlers);

    if (NULL == client_svc)
    {
        DEBUG_PRINTF("gatt_db_add_service failed, Abort");
        return Error::FAILED_INITIALIZE;
    }

    GattSrv::mServer.client_svc_handle = gatt_db_attribute_get_handle(client_svc);
    DEBUG_PRINTF("gatt_db_add_service added %llu hndl %d", (unsigned long long) client_svc, GattSrv::mServer.client_svc_handle);

    for (int attr_index = 0; attr_index < max_attr; attr_index++)
    {
        AttributeInfo_t *ch_info = NULL;
        struct gatt_db_attribute *attr_new = NULL;

        if (NULL != (ch_info = (AttributeInfo_t *)&svc->AttributeList[attr_index]))
        {
            memcpy(&uuid128.data, &ch_info->CharacteristicUUID, sizeof(UUID_128_t));
            bt_uuid128_create(&uuid, uuid128); //

            if (svc->AttributeList[attr_index].AttributeType == atCharacteristic)
            {
                if (NULL != (attr_new =  gatt_db_service_add_characteristic(client_svc, &uuid,
                              BT_ATT_PERM_READ | BT_ATT_PERM_WRITE,
                              BT_GATT_CHRC_PROP_READ | BT_GATT_CHRC_PROP_WRITE | BT_GATT_CHRC_PROP_NOTIFY,
                              gatt_characteristic_read_cb, gatt_characteristic_write_cb,
                              // user_data to be passed to every callback - here is the pointer to AttributeInfo_t
                              (void*) (unsigned long) &svc->AttributeList[attr_index])))
                {
                    GattSrv::mServer.client_attr_handle[attr_index] = gatt_db_attribute_get_handle(attr_new);
                    DEBUG_PRINTF("+ attr %d %s, hndl %d", attr_index, svc->AttributeList[attr_index].AttributeName, GattSrv::mServer.client_attr_handle[attr_index]);
                }
            }
            else if (svc->AttributeList[attr_index].AttributeType == atDescriptor)
            {
                if (NULL != (attr_new = gatt_db_service_add_descriptor(client_svc, &uuid, BT_ATT_PERM_READ | BT_ATT_PERM_WRITE, NULL, NULL, NULL)))
                {
                    DEBUG_PRINTF("+ desc %d %s, hndl %d", attr_index, svc->AttributeList[attr_index].AttributeName, 0);
                }
            }

            if (NULL == attr_new)
            {
                DEBUG_PRINTF("gatt_db_service_add_characteristic failed, Abort - attr[%d] %s", attr_index, svc->AttributeList[attr_index].AttributeName);
                return Error::FAILED_INITIALIZE;
            }
        }
    }

    bool ret = gatt_db_service_set_active(client_svc, true);
    DEBUG_PRINTF("populate_gatt_service gatt_db_service_set_active ret %d\n-----------------------------------------------", ret);
    return Error::NONE;
}

///
/// \brief att_disconnect_cb
/// \param err
/// \param user_data
///
static void att_disconnect_cb(int err, void *user_data)
{
    (void) user_data;
    DEBUG_PRINTF("Device disconnected: %s; exiting GATT Server loop", strerror(err));
    GattSrv::getInstance()->ProcessRegisteredCallback(Ble::Property::Disconnected, -1);
    mainloop_quit();
}

///
/// \brief att_debug_cb
/// \param str
/// \param user_data
///
static void att_debug_cb(const char *str, void *user_data)
{
    const char *prefix = (const char *) user_data;
    DEBUG_PRINTF("%s %s", prefix, str);
}

///
/// \brief gatt_debug_cb
/// \param str
/// \param user_data
///
static void gatt_debug_cb(const char *str, void *user_data)
{
    const char *prefix = (const char *) user_data;
    DEBUG_PRINTF("%s %s", prefix, str);
}

///
/// \brief server_destroy
/// \param server
///
static void server_destroy(struct server_ref *server)
{
    bt_gatt_server_unref(server->gatt);
    gatt_db_unref(server->db);
}

///
/// \brief server_create
/// \return
///
int server_create()
{
    mainloop_init();

    uint16_t mtu = 0;
    struct server_ref *server = new0(struct server_ref, 1);
    if (!server)
    {
        DEBUG_PRINTF("Failed to allocate memory for server reference");
        goto fail;
    }

    server->att = bt_att_new(GattSrv::mServer.fd, false);
    if (!server->att)
    {
        DEBUG_PRINTF("Failed to initialze ATT transport layer");
        goto fail;
    }

    if (!bt_att_set_close_on_unref(server->att, true))
    {
        DEBUG_PRINTF("Failed to set up ATT transport layer");
        goto fail;
    }

    if (!bt_att_register_disconnect(server->att, att_disconnect_cb, NULL, NULL))
    {
        DEBUG_PRINTF("Failed to set ATT disconnect handler");
        goto fail;
    }

    server->db = gatt_db_new();
    if (!server->db)
    {
        DEBUG_PRINTF("Failed to create GATT database");
        goto fail;
    }

    server->gatt = bt_gatt_server_new(server->db, server->att, mtu);
    if (!server->gatt)
    {
        DEBUG_PRINTF("Failed to create GATT server");
        goto fail;
    }

    GattSrv::mServer.sref = server;
    // enable debug
#ifdef DEBUG_ENABLED
        bt_att_set_debug(server->att, att_debug_cb, (void*) "att: ", NULL);
        bt_gatt_server_set_debug(server->gatt, gatt_debug_cb, (void*) "server: ", NULL);
#endif

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

///
/// \brief signal_cb
/// \param signum
/// \param user_data
///
static void signal_cb(int signum, void *user_data)
{
    (void) user_data;
    switch (signum)
    {
    case SIGINT:
    case SIGTERM:
        mainloop_quit();
        break;
    default:
        break;
    }
}

///
/// \brief start_L2CAP_listener
/// \param data
/// \return
///
static void* start_L2CAP_listener(void *data __attribute__ ((unused)))
{
    #define ATT_CID 4
    int nsk;
    struct sockaddr_l2 srcaddr, addr;
    socklen_t optlen;
    struct bt_security btsec;
    char ba[18];
    bdaddr_t  _BDADDR_ANY = {{0, 0, 0, 0, 0, 0}};
    GattSrv *gatt = GattSrv::getInstance();

    bdaddr_t src_addr;
    int dev_id = -1;
    int sec = BT_SECURITY_LOW;
    uint8_t src_type = BDADDR_LE_PUBLIC;

    if (dev_id == -1)
        bacpy(&src_addr, &_BDADDR_ANY);
    else if (hci_devba(dev_id, &src_addr) < 0)
    {
        DEBUG_PRINTF("Adapter not available %s (%d)", strerror(errno), errno);
        return (void*) EXIT_FAILURE;
    }

    GattSrv::mServer.hci_socket = socket(PF_BLUETOOTH, SOCK_SEQPACKET, BTPROTO_L2CAP);
    if (GattSrv::mServer.hci_socket < 0)
    {
        DEBUG_PRINTF("Failed to create L2CAP socket %s (%d)", strerror(errno), errno);
        goto fail;    }

    /* Set up source address */
    memset(&srcaddr, 0, sizeof(srcaddr));
    srcaddr.l2_family = AF_BLUETOOTH;
    srcaddr.l2_cid = htobs(ATT_CID);
    srcaddr.l2_bdaddr_type = src_type;
    bacpy(&srcaddr.l2_bdaddr, &src_addr);

    if (bind(GattSrv::mServer.hci_socket, (struct sockaddr *) &srcaddr, sizeof(srcaddr)) < 0)
    {
        DEBUG_PRINTF("Failed to bind L2CAP socket %s (%d)", strerror(errno), errno);
        goto fail;
    }

    /* Set the security level */
    memset(&btsec, 0, sizeof(btsec));
    btsec.level = sec;
    if (setsockopt(GattSrv::mServer.hci_socket, SOL_BLUETOOTH, BT_SECURITY, &btsec, sizeof(btsec)) != 0)
    {
        DEBUG_PRINTF("Failed to set L2CAP security level");
        goto fail;
    }

    if (listen(GattSrv::mServer.hci_socket, 10) < 0)
    {
        DEBUG_PRINTF("Listening on socket failed %s (%d)", strerror(errno), errno);
        goto fail;
    }

    DEBUG_PRINTF("Started listening on ATT channel. Waiting for connections");

    memset(&addr, 0, sizeof(addr));
    optlen = sizeof(addr);
    nsk = accept(GattSrv::mServer.hci_socket, (struct sockaddr *) &addr, &optlen);
    if (nsk < 0)
    {
        DEBUG_PRINTF("Accept failed %s (%d)", strerror(errno), errno);
        goto fail;
    }

    ba2str(&addr.l2_bdaddr, ba);
    DEBUG_PRINTF("Accepted connect from %s, nsk=%d", ba, nsk);
    close(GattSrv::mServer.hci_socket);
    GattSrv::mServer.hci_socket = -1;
    DEBUG_PRINTF("GattSrv: listening hci_socket released");

    GattSrv::mServer.fd = nsk;
    if (Error::NONE == server_create())
    {
        DEBUG_PRINTF("Running GATT server");
    }
    else
    {
        DEBUG_PRINTF("GATT server init failed\n\n\n");
    }

    gatt->ProcessRegisteredCallback(Ble::Property::Connected, -1);

    sigset_t mask;
    sigemptyset(&mask);
    sigaddset(&mask, SIGINT);
    sigaddset(&mask, SIGTERM);

    mainloop_set_signal(&mask, signal_cb, NULL, NULL);
    mainloop_run();

    DEBUG_PRINTF("GATT server exited mainloop, releasing...");
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

