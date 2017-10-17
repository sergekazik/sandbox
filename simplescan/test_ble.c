/*---------------------------------------------------------------------------
 *                     ------------------      -----------------            *
 *                     | Ring App Setup |      |   test_ble    |            *
 *                     ------------------      -----------------            *
 *                                |                |        |               *
 *                                |   --------------------  |               *
 *                                |   |gatt_src_test.cpp |  |               *
 *                                |   --------------------  |               *
 *                                |          |              |               *
 *                  --------------------------------        |               *
 *                  |   RingBleApi.cpp abstract    |        |               *
 *                  --------------------------------        |               *
 *                  | RingGattSrv.cpp | ?(BCM).cpp |        |               *
 *                  --------------------------------        |               *
 *                        |                                 |               *
 *       ------------------------                    ---------------        *
 *       |      TIBT lib        |                    | hcitools.c  |        *
 *       ------------------------                    ---------------        *
 *                  |                                       |               *
 *       ------------------------                    ---------------        *
 *       | TI WiLink18xx BlueTP |                    |   BlueZ     |        *
 *       ------------------------                    ---------------        *
 *--------------------------------------------------------------------------*/
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/param.h>
#include <sys/ioctl.h>
#include <sys/socket.h>

#include <bluetooth/bluetooth.h>
#include <bluetooth/hci.h>
#include <bluetooth/hci_lib.h>
#include <bluetooth/rfcomm.h>

#include "version.h"
#include "hcitools.h"

#if defined(hpcam2) || defined(Linux_x86_64)
#include "gatt_srv_test.h"
#endif

typedef enum
{
    eConfig_UP,
    eConfig_DOWN,
    eConfig_PISCAN,
    eConfig_NOSCAN,
    eConfig_LEADV,
    eConfig_NOLEADV,
    eConfig_CLASS,

    // ------- combos
    eConfig_ALLUP,
    eConfig_ALLDOWN,
} eConfig_cmd_t;

static int execute_cmd(eConfig_cmd_t aCmd);

#define DEFAULT_TXT "hello ring 123!"
#define COMM_BUF_LEN    1024

#ifdef BLUEZ_TOOLS_SUPPORT
int my_listen(void)
{
    struct sockaddr_rc loc_addr = { 0 }, rem_addr = { 0 };
    char buf[COMM_BUF_LEN] = { 0 };
    int s, client, bytes_read;
    socklen_t opt = sizeof(rem_addr);
    bdaddr_t bdaddr_any = {{0, 0, 0, 0, 0, 0}};

    // allocate socket
    s = socket(AF_BLUETOOTH, SOCK_STREAM, BTPROTO_RFCOMM);

    // bind socket to port 1 of the first available
    // local bluetooth adapter
    loc_addr.rc_family = AF_BLUETOOTH;
    loc_addr.rc_bdaddr = bdaddr_any;
    loc_addr.rc_channel = (uint8_t) 1;

    bind(s, (struct sockaddr *)&loc_addr, sizeof(loc_addr));

    // put socket into listening mode
    listen(s, 1);

    // accept one connection
    client = accept(s, (struct sockaddr *)&rem_addr, &opt);

    ba2str( &rem_addr.rc_bdaddr, buf );
    fprintf(stderr, "accepted connection from %s\n", buf);
    memset(buf, 0, sizeof(buf));

    // read data from the client
    bytes_read = read(client, buf, sizeof(buf));
    if( bytes_read > 0 ) {
        printf("received [%s]\n", buf);

        // ACK
        sprintf(&buf[strlen(buf)], " - acknowledged");
        int len = strlen(buf);
        printf("sending ACK \"%s\"\n", buf);
        int status = write(client, buf, len);
        printf("sent %d byte, status %d\n", len, status);
    }

    // close connection
    close(client);
    close(s);
    return 0;
}

int my_connect(char *dest, const char *data, int nLen, int nRepeat)
{
    char buf[COMM_BUF_LEN] = { 0 };
    struct sockaddr_rc addr = { 0 };
    int s, status;
    // e.g. char dest[18] = "01:23:45:67:89:AB";
    const char *data_stored = data;
    int nLen_stored = nLen;

    printf("connecting to %s\n", dest);
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
            printf("sending to %s:\n%s\n", dest, data);
            status = write(s, data, len);
            printf("sent %d byte, status %d\n", len, status);
        }

        if ( status < 0 )
        {
            perror("uh oh");
            break;
        }
        else
        {
            int bytes_read = read(s, buf, sizeof(buf));
            printf("// read ACK data from the server = %d bytes\n", bytes_read);

            if( bytes_read > 0 ) {
                printf("received %d bytes from GATT(?):\n", bytes_read);
                printf("ASCII: [%s]\n", buf);
                if ((int) strlen(buf) == bytes_read)
                {
                    for (int i = 0; i < bytes_read; i++)
                    {
                        printf("%c", (buf[i] >= ' ') ? buf[i] : ' ');
                    }
                }
                printf("\n");

                for (int i = 0; i < bytes_read; i++)
                {
                    printf("%02X ", buf[i]);
                }
                printf("\n");

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

int my_scan(void)
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
        printf("%s  %s\n", addr, name);
    }

    free( ii );
    close( sock );
    return 0;
}
#endif // BLUEZ_TOOLS_SUPPORT

void print_help(void)
{
    printf("********************************************************\n");
    printf("* BT/LE test tool (date %s)\n", version_date);
    printf("********************************************************\n");
#ifdef BLUEZ_TOOLS_SUPPORT
    printf("--scan                  scan for bluetooth device\n");
    printf("--listen                listen for incoming connection, read once\n");
    printf("--conn <dev_addr>       connect to device MAC address, write once\n");
    printf("--send <dev_addr> [\"txt] connect and send custom text once\n");
    printf("------------------------------------------------\n");
#endif
#if defined(s2lm_ironman) || defined(Linux_x86_64)
    printf("--up                    hciconfig hci0 up\n");
    printf("--down                  hciconfig hci0 down\n");
    printf("--piscan                hciconfig hci0 piscan\n");
    printf("--noscan                hciconfig hci0 noscan\n");
    printf("--leadv                 hciconfig hci0 leadv\n");
    printf("--noleadv               hciconfig hci0 noleadv\n");
    printf("--class                 hciconfig hci0 class 0x280430\n");
    printf("--hciinit               up, piscan, class 0x280430, leadv\n");
    printf("--hcishutdown           noleadv, noscan, down\n");
    printf("------------------------------------------------\n");
#endif  // not "else if"!
#if defined(hpcam2) || defined(Linux_x86_64)
    printf("--gattauto              run Bluetopia GATT Server sample\n");
    printf("--gatt                  load Bluetopia GATT Server sample\n");
    printf("------------------------------------------------\n");
#endif

}

int main(int argc, char **argv)
{
    int arg_idx, ret = 0;
    char *dev_addr __attribute__ ((unused)) = NULL;

    for (arg_idx = argc-1; arg_idx > 0; arg_idx--)
    {
        if (0) {} // plaseholder for following "else if"

#ifdef BLUEZ_TOOLS_SUPPORT
        else if (!strcmp(argv[arg_idx], "--scan"))
        {
            ret = my_scan();
            break;
        }
        else if (!strcmp(argv[arg_idx], "--listen"))
        {
            ret = my_listen();
            break;
        }
        else if (!strcmp(argv[arg_idx], "--conn"))
        {
            if (arg_idx + 1 < argc)
            {
                dev_addr = argv[arg_idx+1];
                ret = my_connect(dev_addr, DEFAULT_TXT, 0, 1);
            }
            else
            {
                printf("--conn missing <dev_addr>...\n");
                ret = -100;
            }
            break;
        }
        else if (!strcmp(argv[arg_idx], "--send"))
        {
            if (arg_idx + 1 < argc)
            {
                char *dt = DEFAULT_TXT;

                char mac_addr[32];
                char command[COMM_BUF_LEN];
                int repeat = 1;
                int rx = 0;
                int ll = 0;

                dev_addr = argv[arg_idx+1];
                if (!strchr(dev_addr, ':')) // if not : separated add it
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
                    if (argv[arg_idx + 2][0] == '"')
                    {
                        dt = &argv[arg_idx + 2][1];
                    }
                    else if (argv[arg_idx + 2][0] == '\'')
                    {
                        //Frame 4: 7 bytes on wire (56 bits), 7 bytes captured (56 bits) on interface 0
                        //Bluetooth
                        //    [Source: controller]
                        //    [Destination: host]
                        //Bluetooth HCI H4
                        //    [Direction: Rcvd (0x01)]
                        command[0] = 0x04; //    HCI Packet Type: HCI Event (0x04)
                        // Bluetooth HCI Event - Command Status
                        command[1] = 0x0f; //     Event Code: Command Status (0x0f)
                        command[2] = 0x04; //     Parameter Total Length: 4
                        command[3] = 0x00; //    Status: Pending (0x00)
                        command[4] = 0x01; //    Number of Allowed Command Packets: 1
                        command[5] = 0x1b; //    Command Opcode: Read Remote Supported Features (0x041b)
                        command[6] = 0x04; //    Command Opcode: Read Remote Supported Features (0x041b)
                        dt = command;
                        ll = 7;
                    }
                    else if (1 == sscanf(argv[arg_idx + 2], "%d", &rx))
                    {
                        repeat = rx;
                    }
                }
                ret = my_connect(dev_addr, (const char*) dt, ll, repeat);
            }
            else
            {
                printf("--conn missing <dev_addr>...\n");
                ret = -100;
            }
            break;
        }
#endif // BLUEZ_TOOLS_SUPPORT


#if defined(hpcam2) || defined(Linux_x86_64)
        else if (!strcmp(argv[arg_idx], "--gattauto"))
        {
            ret = gatt_server_start("--autoinit");
        }
        else if (!strcmp(argv[arg_idx], "--gatt"))
        {
            ret = gatt_server_start(NULL);
        }
#endif

        //------------------------------------------------
        else if (!strcmp(argv[arg_idx], "--up"))
        {
            execute_cmd(eConfig_UP); break;
        }
        else if (!strcmp(argv[arg_idx], "--down"))
        {
            execute_cmd(eConfig_DOWN); break;
        }
        else if (!strcmp(argv[arg_idx], "--piscan"))
        {
            execute_cmd(eConfig_PISCAN); break;
        }
        else if (!strcmp(argv[arg_idx], "--noscan"))
        {
            execute_cmd(eConfig_NOSCAN); break;
        }
        else if (!strcmp(argv[arg_idx], "--leadv"))
        {
            execute_cmd(eConfig_LEADV); break;
        }
        else if (!strcmp(argv[arg_idx], "--noleadv"))
        {
            execute_cmd(eConfig_NOLEADV); break;
        }
        else if (!strcmp(argv[arg_idx], "--class"))
        {
            execute_cmd(eConfig_CLASS); break;
        }
        else if (!strcmp(argv[arg_idx], "--hciinit"))
        {
            execute_cmd(eConfig_ALLUP); break;
        }
        else if (!strcmp(argv[arg_idx], "--hcishutdown"))
        {
            execute_cmd(eConfig_ALLDOWN); break;
        }
    }

    if (!arg_idx)
    {
        print_help();
    }

    printf("%s ----- done; ret val=%d\n", argv[0], ret);
    return ret;
}

static int execute_cmd(eConfig_cmd_t aCmd __attribute__ ((unused)) )
{
#ifdef  BLUEZ_TOOLS_SUPPORT
#if defined(s2lm_ironman) // || defined(Linux_x86_64)
    int ctl;
    static struct hci_dev_info di;

    /* Open HCI socket  */
    if ((ctl = socket(AF_BLUETOOTH, SOCK_RAW, BTPROTO_HCI)) < 0) {
        perror("Can't open HCI socket.");
        return errno;
    }

    if (ioctl(ctl, HCIGETDEVINFO, (void *) &di))
    {
        perror("Can't get device info");
        return errno;
    }

    di.dev_id = 0;

    if (hci_test_bit(HCI_RAW, &di.flags) && !bacmp(&di.bdaddr, BDADDR_ANY)) {
        int dd = hci_open_dev(di.dev_id);
        hci_read_bd_addr(dd, &di.bdaddr, 1000);
        hci_close_dev(dd);
    }

    switch (aCmd)
    {
        case eConfig_UP: hcitool_up(ctl, di.dev_id); break;
        case eConfig_DOWN: hcitool_down(ctl, di.dev_id); break;
        case eConfig_PISCAN: hcitool_scan(ctl, di.dev_id, "piscan"); break;
        case eConfig_NOSCAN: hcitool_scan(ctl, di.dev_id, "noscan"); break;
        case eConfig_LEADV: hcitool_le_adv(di.dev_id, NULL); break;
        case eConfig_NOLEADV: hcitool_no_le_adv(di.dev_id); break;
        case eConfig_CLASS: hcitool_class(di.dev_id, "0x280430"); break;

        case eConfig_ALLUP:
            hcitool_up(ctl, di.dev_id);
            hcitool_scan(ctl, di.dev_id, "piscan");
            hcitool_class(di.dev_id, "0x280430");
            hcitool_le_adv(di.dev_id, NULL);
            break;

        case eConfig_ALLDOWN:
            hcitool_no_le_adv(di.dev_id);
            hcitool_scan(ctl, di.dev_id, "noscan");
            hcitool_down(ctl, di.dev_id);
            break;
    }

    close(ctl);
#else
    printf("hci commands are not available with this target. Abort\n");
#endif
#else
    printf("hci commands are not available without -lbluetooth (Bluez). Abort\n");
#endif //  BLUEZ_TOOLS_SUPPORT


    return 0;
}

