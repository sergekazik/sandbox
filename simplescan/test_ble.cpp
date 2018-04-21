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
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/param.h>
#include <sys/ioctl.h>
#include <sys/socket.h>

#if defined(BLUEZ_TOOLS_SUPPORT) && (defined(BCM43) || defined(Linux_x86_64))
#include <bluetooth/bluetooth.h>
#include <bluetooth/hci.h>
#include <bluetooth/hci_lib.h>
#include <bluetooth/rfcomm.h>
#endif

#include "version.h"
#include "RingGattApi.hh"
#include "RingBlePairing.hh"

#define RING_PAIRING_TABLE_ATTR_ENUM
enum GattAttributeIndexByName {
    #include "gatt_svc_defs.h"
    RING_PAIRING_TABLE_ATTR_MAX
};

using namespace Ring;
using namespace Ring::Ble;

#define COMM_BUF_LEN    1024

#if defined(STANDARD_BT_NO_BLE) && defined(BLUEZ_TOOLS_SUPPORT) && (defined(BCM43) || defined(Linux_x86_64))
///
/// \brief raw_test_listen
/// \return
///
int raw_test_listen(void)
{
    struct sockaddr_rc loc_addr = { 0 }, rem_addr = { 0 };
    char buf[COMM_BUF_LEN] = { 0 };
    int s, client, bytes_read;
    socklen_t opt = sizeof(rem_addr);
    bdaddr_t bdaddr_any = {{0, 0, 0, 0, 0, 0}};

    // allocate socket
    s = socket(AF_BLUETOOTH, SOCK_STREAM, BTPROTO_RFCOMM);
    // s = socket(PF_BLUETOOTH, SOCK_RAW | SOCK_CLOEXEC | SOCK_NONBLOCK, BTPROTO_HCI);
    // s = socket(PF_BLUETOOTH, SOCK_STREAM, BTPROTO_HCI);

    // bind socket to port 1 of the first available
    // local bluetooth adapter
    loc_addr.rc_family = AF_BLUETOOTH; // PF_BLUETOOTH; //
    loc_addr.rc_bdaddr = bdaddr_any;
    loc_addr.rc_channel = (uint8_t) 1;

    bind(s, (struct sockaddr *)&loc_addr, sizeof(loc_addr));

    while (1) {
        // put socket into listening mode
        listen(s, 1);

        // accept one connection
        client = accept(s, (struct sockaddr *)&rem_addr, &opt);

        ba2str( &rem_addr.rc_bdaddr, buf );
        fprintf(stderr, "accepted connection from %s\n", buf);
        if (0 == memcmp("00:00:00:00:00:00", buf, 17))
        {
            close(client);
            sleep(1);
            memset(buf, 0, sizeof(buf));
            continue;
        }
        break;
    }

    memset(buf, 0, sizeof(buf));
    // read data from the client
    bytes_read = read(client, buf, sizeof(buf));
    printf("TESTBLE:  read %d bytes_read\n", bytes_read);
    if( bytes_read > 0 ) {
        printf("TESTBLE:  received [%s]\n", buf);

        // ACK
        sprintf(&buf[strlen(buf)], " - acknowledged");
        int len = strlen(buf);
        printf("TESTBLE:  sending ACK \"%s\"\n", buf);
        int status = write(client, buf, len);
        printf("TESTBLE:  sent %d byte, status %d\n", len, status);
    }

    // close connection
    close(client);
    close(s);
    return 0;
}

///
/// \brief raw_test_connect
/// \param dest
/// \param data
/// \param nLen
/// \param nRepeat
/// \return
///
int raw_test_connect(char *dest, const char *data, int nLen, int nRepeat)
{
    char buf[COMM_BUF_LEN] = { 0 };
    struct sockaddr_rc addr = { 0 };
    int s, status;
    // e.g. char dest[18] = "01:23:45:67:89:AB";
    const char *data_stored = data;
    int nLen_stored = nLen;

    printf("TESTBLE:  connecting to %s\n", dest);
    // allocate a socket
    s = socket(AF_BLUETOOTH, SOCK_STREAM, BTPROTO_RFCOMM);

    // set the connection parameters (who to connect to)
    addr.rc_family = AF_BLUETOOTH;
    addr.rc_channel = (uint8_t) 1;
    str2ba( dest, &addr.rc_bdaddr );

    // connect to server
    status = connect(s, (struct sockaddr *)&addr, sizeof(addr));

    while (nRepeat-- > 0)
    {
        // send a message
        if( status == 0 ) {
            int len = nLen ? nLen : (int) strlen(data);
            sprintf(buf, "%d.%s", nRepeat, data);
            data = buf;
            printf("TESTBLE:  sending to %s:\n%s\n", dest, data);
            status = write(s, data, len);
            printf("TESTBLE:  sent %d byte, status %d\n", len, status);
        }

        if ( status < 0 )
        {
            perror("uh oh");
            break;
        }
        else
        {
            int bytes_read = read(s, buf, sizeof(buf));
            printf("TESTBLE:  // read ACK data from the server = %d bytes\n", bytes_read);

            if( bytes_read > 0 ) {
                printf("TESTBLE:  received %d bytes from GATT(?):\n", bytes_read);
                printf("TESTBLE:  ASCII: [%s]\n", buf);
                if ((int) strlen(buf) == bytes_read)
                {
                    for (int i = 0; i < bytes_read; i++)
                    {
                        printf("TESTBLE:  %c", (buf[i] >= ' ') ? buf[i] : ' ');
                    }
                }
                printf("TESTBLE:  \n");

                for (int i = 0; i < bytes_read; i++)
                {
                    printf("TESTBLE:  %02X ", buf[i]);
                }
                printf("TESTBLE:  \n");

//                data = buf;
//                nLen = bytes_read;
                data = data_stored;
                nLen = nLen_stored;
            }
            else
            {
                data = data_stored;
                nLen = nLen_stored;
            }

        }
        status = 0;
    } // rnd of while repeat

    close(s);
    return status >= 0 ? 0 : status;
}

///
/// \brief raw_test_scan
/// \return
///
int raw_test_scan(void)
{
    inquiry_info *ii = NULL;
    int max_rsp, num_rsp;
    int dev_id, sock, len, flags;
    int i;
    char addr[19] = { 0 };
    char name[248] = { 0 };

    dev_id = hci_get_route(NULL);
    sock = hci_open_dev( dev_id );
    if (dev_id < 0 || sock < 0) {
        perror("opening socket");
        exit(1);
    }

    len  = 8;
    max_rsp = 255;
    flags = IREQ_CACHE_FLUSH;
    ii = (inquiry_info*)malloc(max_rsp * sizeof(inquiry_info));

    num_rsp = hci_inquiry(dev_id, len, max_rsp, NULL, &ii, flags);
    if( num_rsp < 0 ) perror("hci_inquiry");

    for (i = 0; i < num_rsp; i++) {
        ba2str(&(ii+i)->bdaddr, addr);
        memset(name, 0, sizeof(name));
        if (hci_read_remote_name(sock, &(ii+i)->bdaddr, sizeof(name),
            name, 0) < 0)
        strcpy(name, "[unknown]");
        printf("TESTBLE:  %s  %s\n", addr, name);
    }

    free( ii );
    close( sock );
    return 0;
}
#endif //  BLUEZ_TOOLS_SUPPORT


///
/// \brief print_help
///
static void print_help(void)
{
    printf("TESTBLE:  ********************************************************\n");
    printf("TESTBLE:  * BT/LE test tool (date %s)\n", version_date);
    printf("TESTBLE:  ********************************************************\n");
#if defined(STANDARD_BT_NO_BLE) && (defined(BLUEZ_TOOLS_SUPPORT) && (defined(BCM43) || defined(Linux_x86_64)))
    printf("TESTBLE:  --scan                  scan for bluetooth device\n");
    printf("TESTBLE:  --listen                listen for incoming connection, read once\n");
    printf("TESTBLE:  --conn <dev_addr>       connect to device MAC address, write once\n");
    printf("TESTBLE:  --send <dev_addr> [\"txt] connect and send custom text once\n");
    printf("TESTBLE:  ------------------------------------------------\n");
#endif
//#if defined(BCM43) || defined(Linux_x86_64)
//    printf("TESTBLE:  --up                    hciconfig hci0 up\n");
//    printf("TESTBLE:  --down                  hciconfig hci0 down\n");
//    printf("TESTBLE:  --piscan                hciconfig hci0 piscan\n");
//    printf("TESTBLE:  --noscan                hciconfig hci0 noscan\n");
//    printf("TESTBLE:  --leadv                 hciconfig hci0 leadv\n");
//    printf("TESTBLE:  --noleadv               hciconfig hci0 noleadv\n");
//    printf("TESTBLE:  --class                 hciconfig hci0 class 0x000430\n");
//    printf("TESTBLE:  --hciinit               up, piscan, class 0x000430, leadv\n");
//    printf("TESTBLE:  --hcishutdown           noleadv, noscan, down\n");
//    printf("TESTBLE:  ------------------------------------------------\n");
//#endif  // not "else if"!
#if defined(WILINK18) || defined(Linux_x86_64) || (defined(BLUEZ_TOOLS_SUPPORT) && defined(BCM43))
    printf("TESTBLE:  --gattauto              run Bluetopia GATT Server sample\n");
    printf("TESTBLE:  --auto                  auto run Bluetopia GATT Server sample\n");
    printf("TESTBLE:  --gatt                  load Bluetopia GATT Server sample\n");
    printf("TESTBLE:  ------------------------------------------------\n");
#endif
}

static void print_subhelp(void)
{
    printf("TESTBLE:  ********************************************************\n");
    printf("TESTBLE:  interactive commands for Pairing API:\n");
    printf("TESTBLE:  ------------------------------------------------\n");
    printf("TESTBLE:  init            Pairing->Initialize()\n");
    printf("TESTBLE:  startad         Pairing->StartAdvertising()\n");
    printf("TESTBLE:  stopad          Pairing->StopAdvertising()\n");
    printf("TESTBLE:  quit            Pairing->Shutdown() and exit.\n");
    printf("TESTBLE:  ********************************************************\n");
}

void debug_print(const char* name, char* data, int len)
{
    printf("%s (%d bytes):\n", name, len);

    const int num_bytes = 32;
    const int ascii_sta = num_bytes*3;

    int i, offset = 0;
    char out[ascii_sta + num_bytes + 4];
    for (i = 0; i < len; i++)
    {
        offset += sprintf(&out[offset], "%02X ", data[i]);
        if (!((i+1) % num_bytes))
        {
            offset += sprintf(&out[offset], " ");
            for (int j = (i+1)-num_bytes; j < (i+1); j++)
                offset += sprintf(&out[offset], "%c", ((0x20 <= data[j]) && (data[j] < 127)) ? data[j]:'.');
            printf("%s\n", out);
            offset = 0;
        }
    }
    if (i % num_bytes)
    {
        int leftover = i;
        for (; ((leftover+1) % num_bytes); leftover++)
            offset += sprintf(&out[offset], "   ");
        offset += sprintf(&out[offset], "  ");
        for (int j = leftover-num_bytes; j < len; j++)
            offset += sprintf(&out[offset], "%c", ((0x20 <= data[j]) && (data[j] < 127)) ? data[j]:'.');
        printf("%s\n", out);
    }

//    for (int i = 0; i < len; i++)
//        printf("%02X%s", data[i], ((i+1) % 32)?" ":"\n");
//    printf("\n");
//    for (int i = 0; i < len; i++)
//        printf("%c%s", ((0x20 <= data[i]) && (data[i] < 127)) ? data[i]:'.', ((i+1) % 64)?"":"\n");
    printf("\n");
}

static int data_read_write_callback(int a, void* data, int len)
{
    int ret_val = Error::NONE;

    BlePairing *Pairing = BlePairing::getInstance();
    if (Pairing == NULL)
    {
        printf("TESTBLE:  data_rw_cb: failed to obtain BlePairing instance. Abort.\n");
    }
    else
    {
        const ServiceInfo_t *svc = Pairing->GetServiceTable();
        if (data && len)
        {
            printf("TESTBLE:  data_rw_cb: Written %d bytes of data for attr idx %d [%s]\n", len, a, !svc?"":svc->AttributeList[a].AttributeName);
            debug_print("data_rw_cb", (char*) data, len);
        }
        else
        {
            printf("TESTBLE:  data_rw_cb: Read attr idx %d [%s]\n", a, !svc?"":svc->AttributeList[a].AttributeName);
        }
        ret_val = (1); // to inform default handler to stop further processing of this notification

        if (svc && Pairing) switch (a)
        {
            case SET_PUBLIC_KEY:
            case GET_PUBLIC_PAYLOAD:
                break;

            default:
                sleep(1);
                Pairing->updateAttribute(a, (const char*) ((CharacteristicInfo_t*) svc->AttributeList[a].Attribute)->Value,
                                            ((CharacteristicInfo_t*) svc->AttributeList[a].Attribute)->ValueLength);
                break;
        }
    }

    return ret_val;
}

///
/// \brief pairing_test_run
/// \param arguments - expected NULL or "--autoinit"
/// \return errno
///
static int pairing_test_run(const char* arguments)
{
    int ret_val = 0;
    bool bDone = false;
    BlePairing *Pairing = BlePairing::getInstance();
    if (Pairing == NULL)
    {
        printf("TESTBLE:  failed to obtain BlePairing instance. Abort.\n");
        return -777;
    }

    if (arguments != NULL && !strcmp(arguments, "--autoinit"))
    {
#if defined(WILINK18)
        printf("TESTBLE:  ---starting in a sec...---\n");
        sleep(2);
#endif

        if (Ble::Error::NONE != (ret_val = Pairing->Initialize((char*) "RingSetup-ac")))
        {
            printf("TESTBLE:  Pairing->Initialize() failed, ret = %d. Abort\n", ret_val);
            goto autodone;
        }
        if (Ble::Error::NONE != (ret_val = Pairing->registerRingDataCallback(data_read_write_callback)))
        {
            printf("TESTBLE:  WARNING: Pairing->registerRingDataCallback() failed, ret = %d\n", ret_val);
        }
        if (Ble::Error::NONE != (ret_val = Pairing->StartAdvertising()))
        {
            printf("TESTBLE:  Pairing->StartAdvertising failed, ret = %d, Abort.\n", ret_val);
            goto autodone;
        }
    }

    print_subhelp();

    while (!bDone)
    {

        static const int command_lineSize = 0xff;
        char command_line[command_lineSize];

        printf("TESTBLE:  TEST> ");
        fflush(stdout);

        /* Read a line from standard input.                               */
        ret_val = read(STDIN_FILENO, command_line, command_lineSize);

        /* Check if the read succeeded.                                   */
        if(ret_val > 0)
        {
            /* The read succeeded, replace the new line character with a   */
            /* null character to delimit the string.                       */
            command_line[ret_val - 1] = '\0';
            command_line[ret_val] = '\0';
            /* Stop the loop.                                              */
        }
        else
        {
            printf("TESTBLE:  read error. abort\n");
            goto autodone;
        }

        if (!strcmp(command_line, "quit"))
        {
            ret_val = Pairing->Shutdown();
            printf("TESTBLE:  Pairing->Shutdown() ret = %d, Exit.\n", ret_val);
            bDone = true;
        }
        else if (!strcmp(command_line, "init"))
        {
            if (Ble::Error::NONE != (ret_val = Pairing->Initialize()))
            {
                printf("TESTBLE:  Pairing->Initialize() failed, ret = %d", ret_val);
            }
            else
            {
                printf("TESTBLE:  Pairing->Initialize() ret = %d", ret_val);
            }
        }
        else if (strstr(command_line, "sta"))
        {
            if (Ble::Error::NONE != (ret_val = Pairing->StartAdvertising()))
            {
                printf("TESTBLE:  Pairing->StartAdvertising failed, ret = %d, Abort.\n", ret_val);
            }
            else
            {
                printf("TESTBLE:  Pairing->StartAdvertising ret = %d\n", ret_val);
            }
        }
        else if (strstr(command_line, "sto"))
        {
            ret_val = Pairing->StopAdvertising();
            printf("TESTBLE:  Pairing->StopAdvertising ret = %d\n", ret_val);
        }
        else if (!strcmp(command_line, "11"))
        {
            ret_val = Pairing->PrintStatus();
        }
        else if (!strcmp(command_line, "notify"))
        {
//            if (!strcmp("GET_PAIRING_STATE", svc->AttributeList[idx].AttributeName))
//            {
//                uint8_t value[] = {"OK-notify"};
//                BOT_NOTIFY_DEBUG("sending Notify \"%s\"", value);
//                bt_gatt_server_send_notification(GattSrv::mServer.sref->gatt, gatt_db_attribute_get_handle(attrib), value, sizeof(value));

//            }
        }
        else
        {
            Pairing->PrintStatus();
            print_subhelp();
        }
    }

autodone:
    return 0;
}

///
/// \brief main
/// \param argc
/// \param argv
/// \return
///
int main(int argc, char **argv)
{
    int arg_idx, ret = 0;

    for (arg_idx = argc-1; arg_idx > 0; arg_idx--)
    {
        if (0) {;} // plaseholder for following "else if"

#if defined(STANDARD_BT_NO_BLE) && (defined(BLUEZ_TOOLS_SUPPORT) && (defined(BCM43) || defined(Linux_x86_64)))
        else if (!strcmp(argv[arg_idx], "--scan"))
        {
            ret = raw_test_scan();
            break;
        }
        else if (!strcmp(argv[arg_idx], "--listen"))
        {
            ret = raw_test_listen();
            break;
        }
        else if (!strcmp(argv[arg_idx], "--conn"))
        {
            if (arg_idx + 1 < argc)
            {
                char *dev_addr = argv[arg_idx+1];
                ret = raw_test_connect(dev_addr, "hello ring 123!", 0, 1);
            }
            else
            {
                printf("TESTBLE:  --conn missing <dev_addr>...\n");
                ret = -100;
            }
            break;
        }
        else if (!strcmp(argv[arg_idx], "--send"))
        {
            if (arg_idx + 1 < argc)
            {
                char dtdf[] = {"hello ring 123!"};
                char *dt = dtdf;

                char mac_addr[32];
                int repeat = 1;
                int rx = 0;
                int ll = 0;

                char *dev_addr = argv[arg_idx+1];

                // if address is not :-separated add ':'
                if (!strchr(dev_addr, ':'))
                {
                    memset(mac_addr, 0, sizeof(mac_addr));
                    for (int a = 0, b = 0; b < (int) strlen(dev_addr); b++)
                    {
                        mac_addr[a++] = dev_addr[b];
                        if ((b % 2) && (b+1 < (int) strlen(dev_addr)))
                            mac_addr[a++] = ':';
                    }
                    dev_addr = mac_addr;
                }

                if (arg_idx + 2 < argc)
                {
                    // send text payload if the argument is starting \"helloxxx
                    if (argv[arg_idx + 2][0] == '"')
                    {
                        dt = &argv[arg_idx + 2][1];
                    }
                    // repeat n times
                    else if (1 == sscanf(argv[arg_idx + 2], "%d", &rx))
                    {
                        repeat = rx;
                    }
                }
                ret = raw_test_connect(dev_addr, (const char*) dt, ll, repeat);
            }
            else
            {
                printf("TESTBLE:  --conn missing <dev_addr>...\n");
                ret = -100;
            }
            break;
        }
#endif // BLUEZ_TOOLS_SUPPORT

#if defined(WILINK18) || defined(Linux_x86_64) || (defined(BLUEZ_TOOLS_SUPPORT) && defined(BCM43))

        else if (!strcmp(argv[arg_idx], "--gattauto") ||
                 !strcmp(argv[arg_idx], "--auto"))
        {
            ret = pairing_test_run("--autoinit");
        }
        else if (!strcmp(argv[arg_idx], "--gatt"))
        {
            ret = pairing_test_run(NULL);
        }
#endif // defined(WILINK18) || defined(Linux_x86_64)
    }

    if (!arg_idx)
    {
        print_help();
    }

    printf("TESTBLE:  %s ----- done; ret val=%d\n", argv[0], ret);
    return ret;
}

