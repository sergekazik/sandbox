#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include "icommon.h"

#define QUEUE_KEY_DEFAULT 0x52696e67
#define SOCK_PORT_DEFAULT 2407
#define LOCAL_IP   ((char*) "127.0.0.1")
#define fdServerTx fdClientRx
#define fdClientTx fdServerRx


static bool gbIpc = true;
static uint32_t guiKey = QUEUE_KEY_DEFAULT;
static int giPort = SOCK_PORT_DEFAULT;

static int fdServerRx = -1;
static int fdClientRx = -1;
static struct sockaddr_in gClient_addr;
static char *gsServerAdd = LOCAL_IP;
static bool gbInitialized = false;

#ifdef DEBUG_ENABLED
static const char *debug_msg[] =
{
    "MSG_SESSION",

    // controller / addapter
    "MSG_POWER",
    "MSG_CONFIG",

    // advertisement
    "MSG_ADVERTISEMENT",

    // GATT attribute operations
    "MSG_ADD_SERVICE",
    "MSG_ADD_ATTRIBUTE",
    "MSG_UPDATE_ATTRIBUTE",

    // Server notifications to Client
    "MSG_NOTIFY_STATUS_CHANGE",
    "MSG_NOTIFY_DATA_READ",
    "MSG_NOTIFY_DATA_WRITE",
};
#endif


typedef struct comm_msgbuf
{
    __syscall_slong_t mtype;	/* type of received/sent message */
    Comm_Msg_t msg;
} Comm_Msgbuf_t;

///
/// \brief die
/// \param s
///
void die(const char *s)
{
  perror(s);
  exit(1);
}

///
/// \brief get_msg_name
/// \param cm
///
const char* get_msg_name(Comm_Msg_t *cm)
{
#ifdef DEBUG_ENABLED
    return debug_msg[cm->type];
#endif
    return "";
}

///
/// \brief parse_command_line
/// \param argc
/// \param argv
/// \return
///
int parse_command_line(int argc, char** argv)
{
    for (int i = 1; i < argc; i++)
    {
        if (!strcmp(argv[i], "-k")) // hey for queue
        {
            int key = i+1<argc?atoi(argv[++i]):0;
            if (key > 0)
            {
                gbIpc = true;
                guiKey = key;
                printf("set key %d\n", guiKey);
            }
        }
        else if (!strcmp(argv[i], "-p")) // port for UDP
        {
            int port = i+1<argc?atoi(argv[++i]):0;
            if (port > 0)
            {
                gbIpc = false;
                giPort = port;
                printf("set port %d\n", giPort);
            }
        }
        else if (!strcmp(argv[i], "-ip")) // Server IP for Client to connect
        {
            gbIpc = false;
            gsServerAdd = i+1<argc?argv[++i]:LOCAL_IP;
            if (gsServerAdd && strlen(gsServerAdd))
            {
                printf("set server ip %s\n", gsServerAdd);
            }
        }
        else
        {
            printf("invalid argument %s \nusage [-k key][-p port]\n", argv[i]);
            return Ble::Error::INVALID_PARAMETERS;
        }
    }
    printf("configured to use %s %d [0x%08x]\n", gbIpc?"queue key =":gsServerAdd, gbIpc?guiKey:giPort, gbIpc?guiKey:giPort);
    return 0;
}

///
/// \brief init_comm
/// \param bServer
/// \return
///
int init_comm(bool bServer)
{
    if (!gbInitialized)
    {
        if (gbIpc)
        {
            int msgflg = bServer ? (IPC_CREAT | 0666) : 0666;
            if (((fdServerRx = msgget(guiKey, msgflg )) < 0)   || // Get the message queues IDs for the given key
                ((fdClientRx = msgget(guiKey+1, msgflg )) < 0))
            {
              return Ble::Error::FAILED_INITIALIZE;
            }
        }
        else
        {
            struct sockaddr_in rx_addr;

            // Creating listening socket file descriptor
            int *sockfd = bServer?&fdServerRx:&fdClientRx;
            if ( (*sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0 )
            {
                return Ble::Error::FAILED_INITIALIZE;
            }

            memset(&rx_addr, 0, sizeof(rx_addr));

            // Binding to dedicated port
            int binding_port = giPort + (bServer?0:1);
            rx_addr.sin_family    = AF_INET; // IPv4
            rx_addr.sin_addr.s_addr = INADDR_ANY;
            rx_addr.sin_port = htons(binding_port);

            if ( bind(*sockfd, (const struct sockaddr *)&rx_addr, sizeof(rx_addr)) < 0 )
            {
                close(*sockfd);
                return Ble::Error::FAILED_INITIALIZE;
            }

            // create sending socket file descriptor
            sockfd = bServer?&fdServerTx:&fdClientTx;
            if ( (*sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0 )
            {
                return Ble::Error::FAILED_INITIALIZE;
            }
        }
        gbInitialized = true;
    }
    return Ble::Error::NONE;
}

///
/// \brief send_comm
/// \param bServer
/// \param msg
/// \param size
/// \return
///
int send_comm(bool bServer, Comm_Msg_t *msg, int size)
{
    if (!gbInitialized)
        return Ble::Error::NOT_INITIALIZED;

    if (gbIpc)
    {
        Comm_Msgbuf_t sbuf;
        sbuf.mtype = 1;
        sbuf.msg = *msg;

        if (msgsnd(bServer?fdClientRx:fdServerRx, &sbuf, size+1, IPC_NOWAIT) < 0)
        {
            return Ble::Error::OPERATION_FAILED;
        }
    }
    else
    {
        struct sockaddr_in rx_addr;
        int dest_port = giPort + (bServer?1:0);
        int sockfd = bServer?fdServerTx:fdClientTx;

        rx_addr.sin_family = AF_INET;
        rx_addr.sin_port = htons(dest_port);
        rx_addr.sin_addr.s_addr = !bServer ? inet_addr(gsServerAdd) : gClient_addr.sin_addr.s_addr;

        int bsent = sendto(sockfd, (const char *)msg, sizeof(Comm_Msg_t), MSG_CONFIRM, (const struct sockaddr *) &rx_addr, sizeof(rx_addr));
        if (bsent != sizeof(Comm_Msg_t))
        {
            return Ble::Error::OPERATION_FAILED;
        }
    }
    return Ble::Error::NONE;
}

///
/// \brief recv_comm
/// \param bServer
/// \param msg
/// \return
///
int recv_comm(bool bServer, Comm_Msg_t *msg)
{
    if (!gbInitialized)
        return Ble::Error::NOT_INITIALIZED;

    if (gbIpc)
    {
        Comm_Msgbuf_t sbuf;
        if (msgrcv(bServer?fdServerRx:fdClientRx, &sbuf, sizeof(Comm_Msgbuf_t), 1, 0) < 0)
        {
            return Ble::Error::OPERATION_FAILED;
        }
        *msg = sbuf.msg;
    }
    else
    {
        socklen_t len = 0;
        struct sockaddr_in addr;

        memset(&addr, 0, sizeof(addr));
        int sockfd = bServer?fdServerRx:fdClientRx;

        int brecv = recvfrom(sockfd, (char *)msg, sizeof(Comm_Msg_t), MSG_WAITALL, (struct sockaddr *) &addr, &len);
        if (brecv != sizeof(Comm_Msg_t))
        {
            return Ble::Error::OPERATION_FAILED;
        }

        // save client addres for Server to respond
        if (bServer)
        {
            gClient_addr = addr;
        }
    }
    return Ble::Error::NONE;
}

///
/// \brief shut_comm
/// \param bServer
/// \return
///
int shut_comm(bool bServer)
{
    if (gbInitialized)
    {
        if (gbIpc)
        {
            if (bServer)
            {
                msgctl(fdClientRx, IPC_RMID, 0);
                msgctl(fdServerRx, IPC_RMID, 0);
            }
        }
        else
        {
            close(fdClientRx);
            close(fdServerRx);
        }
        fdClientRx = fdServerRx = -1;
        gbInitialized = false;
    }
    return Ble::Error::NONE;
}
