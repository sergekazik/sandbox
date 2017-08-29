#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/param.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <bluetooth/bluetooth.h>
#include <bluetooth/hci.h>
#include <bluetooth/hci_lib.h>

#include <stdio.h>
#include <unistd.h>
#include <sys/socket.h>
#include <bluetooth/bluetooth.h>
#include <bluetooth/rfcomm.h>

#include "hcitools.h"

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

static struct hci_dev_info di;
static int execute_cmd(eConfig_cmd_t aCmd);

#define DEFAULT_TXT "hello ring 123!"

int my_listen(void)
{
    struct sockaddr_rc loc_addr = { 0 }, rem_addr = { 0 };
    char buf[1024] = { 0 };
    int s, client, bytes_read;
    socklen_t opt = sizeof(rem_addr);

    // allocate socket
    s = socket(AF_BLUETOOTH, SOCK_STREAM, BTPROTO_RFCOMM);

    // bind socket to port 1 of the first available
    // local bluetooth adapter
    loc_addr.rc_family = AF_BLUETOOTH;
    loc_addr.rc_bdaddr = *BDADDR_ANY;
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
    }

    // close connection
    close(client);
    close(s);
    return 0;
}

int my_connect(char *dest, const char *data)
{
    struct sockaddr_rc addr = { 0 };
    int s, status;
    // e.g. char dest[18] = "01:23:45:67:89:AB";

    printf("connecting to %s\n", dest);
    // allocate a socket
    s = socket(AF_BLUETOOTH, SOCK_STREAM, BTPROTO_RFCOMM);

    // set the connection parameters (who to connect to)
    addr.rc_family = AF_BLUETOOTH;
    addr.rc_channel = (uint8_t) 1;
    str2ba( dest, &addr.rc_bdaddr );

    // connect to server
    status = connect(s, (struct sockaddr *)&addr, sizeof(addr));

    // send a message
    if( status == 0 ) {
        int len = strlen(data);
        printf("sending \"%s\" to %s\n", data, dest);
        status = write(s, data, len);
        printf("send %d byte, status %d\n", len, status);
    }

    if ( status < 0 )
    {
        perror("uh oh");
    }

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

void print_help(void)
{
    printf("------------------------------------------------\n");
    printf("--scan                  scan for bluetooth device\n");
    printf("--listen                listen for incoming connection, read once\n");
    printf("--conn <dev_addr>       connect to device MAC address, write once\n");
    printf("--send <dev_addr> [\"txt] connect and send custom text once\n");
    printf("------------------------------------------------\n");
    printf("--up                    hciconfig hci0 up\n");
    printf("--down                  hciconfig hci0 down\n");
    printf("--piscan                hciconfig hci0 piscan\n");
    printf("--noscan                hciconfig hci0 noscan\n");
    printf("--leadv                 hciconfig hci0 leadv\n");
    printf("--noleadv               hciconfig hci0 leadv\n");
    printf("--class                 hciconfig hci0 class 0x280430\n");
    printf("--hciinit               up, piscan, class 0x280430, leadv\n");
    printf("--hcishutdown           noleadv, noscan, down\n");
    printf("------------------------------------------------\n");

}

int main(int argc, char **argv)
{
    int arg_idx, ret = 0;
    char *dev_addr = NULL;

    for (arg_idx = argc-1; arg_idx > 0; arg_idx--)
    {
        if (!strcmp(argv[arg_idx], "--scan"))
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
                ret = my_connect(dev_addr, DEFAULT_TXT);
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
                dev_addr = argv[arg_idx+1];
                char *dt = DEFAULT_TXT;

                if (arg_idx + 2 < argc)
                {
                    if (argv[arg_idx + 2][0] == '"')
                    {
                        dt = &argv[arg_idx + 2][1];
                    }
                }
                ret = my_connect(dev_addr, (const char*) dt);
            }
            else
            {
                printf("--conn missing <dev_addr>...\n");
                ret = -100;
            }
            break;
        }
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

static int execute_cmd(eConfig_cmd_t aCmd)
{
	int ctl;

	/* Open HCI socket  */
	if ((ctl = socket(AF_BLUETOOTH, SOCK_RAW, BTPROTO_HCI)) < 0) {
		perror("Can't open HCI socket.");
		exit(1);
	}

	if (ioctl(ctl, HCIGETDEVINFO, (void *) &di))
    {
		perror("Can't get device info");
		exit(1);
	}

    di.dev_id = 0;

	if (hci_test_bit(HCI_RAW, &di.flags) && !bacmp(&di.bdaddr, BDADDR_ANY)) {
		int dd = hci_open_dev(di.dev_id);
		hci_read_bd_addr(dd, &di.bdaddr, 1000);
		hci_close_dev(dd);
	}

    switch (aCmd)
    {
        case eConfig_UP: hcitool_up(ctl, di.dev_id, NULL); break;
        case eConfig_DOWN: hcitool_down(ctl, di.dev_id, NULL); break;
        case eConfig_PISCAN: hcitool_scan(ctl, di.dev_id, "piscan"); break;
        case eConfig_NOSCAN: hcitool_scan(ctl, di.dev_id, "noscan"); break;
        case eConfig_LEADV: hcitool_le_adv(ctl, di.dev_id, NULL); break;
        case eConfig_NOLEADV: hcitool_no_le_adv(ctl, di.dev_id, NULL); break;
        case eConfig_CLASS: hcitool_class(ctl, di.dev_id, "0x280430"); break;

        case eConfig_ALLUP:
            hcitool_up(ctl, di.dev_id, NULL);
            hcitool_scan(ctl, di.dev_id, "piscan");
            hcitool_class(ctl, di.dev_id, "0x280430");
            hcitool_le_adv(ctl, di.dev_id, NULL);
            break;

        case eConfig_ALLDOWN:
            hcitool_no_le_adv(ctl, di.dev_id, NULL);
            hcitool_scan(ctl, di.dev_id, "noscan");
            hcitool_down(ctl, di.dev_id, NULL);
            break;
    }

	close(ctl);
	return 0;
}

