#ifndef _PORT_BLOCKING_DETECT_H
#define _PORT_BLOCKING_DETECT_H

#include <stdint.h>

namespace Ring {

#define UNUSED(_var) (void)_var
#define UNUSED2(_var1, _var2) (void)_var1, (void)_var2

#ifndef INVALID_SOCKET
#define INVALID_SOCKET (-1)
#endif
#define CLOSE_SOCKET(socket_desc_arg) { if (socket_desc_arg != INVALID_SOCKET) { close (socket_desc_arg); socket_desc_arg = INVALID_SOCKET; } }

#ifdef __x86_64__
#define TEST_ADDR "192.168.1.3"
#else
#define TEST_ADDR "10.0.1.15"
#endif

#define MSG_LENGHT  0xFF
#define BUFSIZE     1024

#define SERVER_PORT     1100
#define CLIENT_PORT     1101

typedef enum command_list
{ // command      |type  |    format     | response
    ERRNO    = 0, // all | ERRNO         | ERRCODE - last error code
    OPEN_UDP    , // all | OPEN_UDP,PORT | ERRCODE,SOCK
    OPEN_TCP    , // all | OPEN_TCP,PORT | ERRCODE,SOCK
    TEST_UDP    , // UDP | TEST_UDP,SOCK | ERRCODE
    TEST_TCP    , // TCP | TEST_TCP,SOCK | ERRCODE
    STATS       , // all | STATS,SOCK,   | ERRCODE,RCVPACK,BYTE,SNDPACK,BYTE
    CLOSE       , // all | CLOSE,SOCK    | ERRCODE
    EOS         , // all | END           |
} command_list_t;

typedef enum comm_lenght
{ // binary req/rsp in bytes
    ERRNO_REQ_LEN       = 1,
    OPEN_UDP_REQ_LEN    = 3,
    OPEN_TCP_REQ_LEN    = 3,
    TEST_UDP_REQ_LEN    = 3,
    TEST_TCP_REQ_LEN    = 3,
    STATS_REQ_LEN       = 3,
    CLOSE_REQ_LEN       = 3,
    EOS_REQ_LEN = 1,

    ERRNO_RSP_LEN       = 1,
    OPEN_UDP_RSP_LEN    = 3,
    OPEN_TCP_RSP_LEN    = 3,
    TEST_UDP_RSP_LEN    = 1,
    TEST_TCP_RSP_LEN    = 1,
    STATS_RSP_LEN       = 5,
    CLOSE_RSP_LEN       = 1,
    EOS_RSP_LEN = 0,
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
    NOT_INITIALIZED,
    STATS_MISMATCH,
    COMMAND_SKIPPED,
} error_code_t;

typedef struct stats_info
{
    uint8_t recv_packets;
    uint8_t recv_bytes;
    uint8_t sent_packets;
    uint8_t sent_bytes;
} stats_info_t;

typedef struct control_channel_info
{
    union {
        uint8_t command;    // 8 bit command id as defined in command_list_t
        uint8_t err_code;   // 8 bit error code as defined by error_code_t
    };
    union
    {
        uint8_t data[4];    // req/rsp related data
        stats_info_t stats;
    };
    union
    {                       // additional fields for processing, not to be sent
        uint16_t     port;
        uint16_t     sock;
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


class PortBlockDetect
{
public:
    PortBlockDetect();
    ~PortBlockDetect();

    error_code_t parse_script_command(const char *line);
    error_code_t preprocess_script_command();
    error_code_t connect_tcp_server(char *line);
    error_code_t send_tcp_server();
    error_code_t recv_tcp_server();
    error_code_t process_server_response();

    void handle_sendto_command();
    void handle_test_udp_command();
    void print_cci_info(const char *command);
    void dump_recv_buffer(int max);
    void skip_failed_test() { m_skip_failed_test = true; }

    control_channel_info_t  m_cci;
    control_channel_info_t *m_rsp_cci = (control_channel_info_t *) m_buffer;
    char m_server_ip[16] = "127.0.0.1";
    int m_ctrl_sock = INVALID_SOCKET;
    int m_test_sock = INVALID_SOCKET;
    int m_test_port = 0;
    int m_sock_hndl = 0;

private:
    void dump_buffer(const uint8_t *src, int bytes, int max, const char *msg = nullptr);

    bool check_socket(int &sock, int type);
    char m_buffer[BUFSIZE];
    stats_info_t m_stat = {0,0,0,0};
    int m_bytes_read = 0;
    bool m_skip_failed_test = false;

};

} // namespace Ring

#endif // _PORT_BLOCKING_DETECT_H
