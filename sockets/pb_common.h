#ifndef PB_COMMON
#define PB_COMMON
#include <stdint.h>

#define UNUSED(_var) (void)_var
#define UNUSED2(_var1, _var2) (void)_var1, (void)_var2

#ifdef __x86_64__
static const char* test_addr = "192.168.1.3";
#else
static const char* test_addr = "10.0.1.15";
#endif

#ifndef INVALID_SOCKET
#define INVALID_SOCKET (-1)
#endif

#define MSG_LENGHT  0xFF
#define BUFSIZE     1024

// #if defined(__x86_64__) && defined(PB_CLIENT)
#define SERVER_PORT     1100
#define CLIENT_PORT     1101

typedef enum command_list
{ // command     |value  |type |    format     | response
    ERRNO       =   0,  // all | ERRNO         | ERRCODE - last error code
    OPEN_UDP    =   1,  // all | OPEN_UDP,PORT | ERRCODE,SOCK
    OPEN_TCP    =   2,  // all | OPEN_TCP,PORT | ERRCODE,SOCK
    RECVFOMR    =   3,  // UDP | RECVFOMR,SOCK | ERRCODE
    SENDTO      =   4,  // UDP | SENDTO,SOCK   | ERRCODE
    LISTEN      =   5,  // TCP | LISTEN,SOCK   | ERRCODE
    CONNECT     =   6,  // TCP | CONNECT,SOCK  | ERRCODE
    STATS       =   7,  // all | STATS,SOCK,   | ERRCODE,RCVPACK,BYTE,SNDPACK,BYTE
    CLOSE       =   8,  // all | CLOSE,SOCK    | ERRCODE
    SESSION_END =   9,  // all | END           |
} command_list_t;

typedef enum comm_lenght
{ // binary req/rsp in bytes
    ERRNO_REQ_LEN       = 1,
    OPEN_UDP_REQ_LEN    = 3,
    OPEN_TCP_REQ_LEN    = 3,
    RECVFOMR_REQ_LEN    = 3,
    SENDTO_REQ_LEN      = 3,
    LISTEN_REQ_LEN      = 3,
    CONNECT_REQ_LEN     = 3,
    STATS_REQ_LEN       = 3,
    CLOSE_REQ_LEN       = 3,
    SESSION_END_REQ_LEN = 1,

    ERRNO_RSP_LEN    = 1,
    OPEN_UDP_RSP_LEN = 3,
    OPEN_TCP_RSP_LEN = 3,
    RECVFOMR_RSP_LEN = 1,
    SENDTO_RSP_LEN   = 1,
    LISTEN_RSP_LEN   = 1,
    CONNECT_RSP_LEN  = 1,
    STATS_RSP_LEN    = 5,
    CLOSE_RSP_LEN    = 1
} comm_lenght_t;

typedef enum error_code
{
    NO_ERROR        = 0,
    INVALID_REQUEST,
    INVALID_ARGUMENT,
    NOT_AVAILABLE,
    SOCKET_ERROR,
    CONNECT_ERROR,
    SEND_ERROR,
    RECV_ERROR,
    TIMEOUT_ERROR,
    GENERAL_ERROR,
} error_code_t;

typedef struct control_channel_info
{
    union {
        uint8_t command;    // 8 bit command id as defined in command_list_t
        uint8_t err_code;   // 8 bit error code as defined by error_code_t
    };
    uint8_t data[4];        // req/rsp related data
    union
    {                       // additional fields for processing, not to be sent
        uint16_t port;
        uint16_t sock;
        struct {
            uint8_t rcv_packets;
            uint8_t rcv_bytes;
            uint8_t snd_packets;
            uint8_t snd_butes;
        } stats;
    };
    uint8_t len;
} control_channel_info_t;

#define PACK_DATA_PORT(ptr_ctrl_chan_info_arg)    \
    (ptr_ctrl_chan_info_arg)->data[0] = (uint8_t) ((ptr_ctrl_chan_info_arg)->port >> 8);   \
    (ptr_ctrl_chan_info_arg)->data[1] = (uint8_t) ((ptr_ctrl_chan_info_arg)->port & 0xFF);

#define PACK_DATA_SOCK(ptr_ctrl_chan_info_arg)    \
    (ptr_ctrl_chan_info_arg)->data[0] = (uint8_t) ((ptr_ctrl_chan_info_arg)->sock >> 8);   \
    (ptr_ctrl_chan_info_arg)->data[1] = (uint8_t) ((ptr_ctrl_chan_info_arg)->sock & 0xFF);

#define UNPACK_DATA_PORT(ptr_ctrl_chan_info_arg)                        \
    (ptr_ctrl_chan_info_arg)->port = (uint16_t) ((ptr_ctrl_chan_info_arg)->data[0] << 8) | (ptr_ctrl_chan_info_arg)->data[1];

#define UNPACK_DATA_SOCK(ptr_ctrl_chan_info_arg)                        \
    (ptr_ctrl_chan_info_arg)->sock = (uint16_t) ((ptr_ctrl_chan_info_arg)->data[0] << 8) | (ptr_ctrl_chan_info_arg)->data[1];


#endif // PB_COMMON

