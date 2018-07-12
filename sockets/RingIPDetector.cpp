#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <mutex>

#include <sys/types.h>       // For data types
#include <sys/socket.h>      // For socket(), connect(), send(), and recv()
#include <netdb.h>           // For gethostbyname()
#include <arpa/inet.h>       // For inet_addr()
#include <unistd.h>          // For close()
#include <netinet/in.h>      // For sockaddr_in

#include "RingIPDetector.hh"

using namespace Ring;

#define PARSE_CMD_GEN(cmd_to_parse_arg) { #cmd_to_parse_arg, cmd_to_parse_arg, cmd_to_parse_arg##_REQ_LEN}
struct parse_cmd_list
{
    char *cmd_name;
    uint8_t cmd_val;
    uint8_t cmd_len;
} cmd_translated[] = {
    PARSE_CMD_GEN(ERRNO   ),
    PARSE_CMD_GEN(OPEN_UDP),
    PARSE_CMD_GEN(OPEN_TCP),
    PARSE_CMD_GEN(TEST_UDP),
    PARSE_CMD_GEN(LISTEN  ),
    PARSE_CMD_GEN(CONNECT ),
    PARSE_CMD_GEN(STATS   ),
    PARSE_CMD_GEN(CLOSE   ),
    PARSE_CMD_GEN(SESSION_END)
};

ClientIPDetector::ClientIPDetector()
{
}

ClientIPDetector::~ClientIPDetector()
{
    CLOSE_SOCKET(m_ctrl_sock);
    CLOSE_SOCKET(m_test_sock);

}

error_code_t ClientIPDetector::parse_script_command(const char *line)
{
    if (line == NULL)
    {
        return INVALID_ARGUMENT;
    }

    memset(&m_cci, 0, sizeof(control_channel_info_t));

    for (unsigned int i = 0; i < sizeof(cmd_translated)/sizeof(parse_cmd_list); i++)
    {
        if (0 == strncmp(line, cmd_translated[i].cmd_name, strlen(cmd_translated[i].cmd_name)))
        {
            m_cci.command = cmd_translated[i].cmd_val;
            m_cci.len = cmd_translated[i].cmd_len;

            switch (m_cci.command)
            {
            case OPEN_UDP:
            case OPEN_TCP:
                {
                    int port;
                    if (1 != sscanf(strstr(line, ",")+1, "%d", &port))
                    {
                        return INVALID_ARGUMENT;
                    }
                    m_cci.port = port;
                }
                break;
            default:
                break;
            }
            return NO_ERROR;
        }
    }
    return INVALID_REQUEST;
}

bool ClientIPDetector::check_socket(int &sock, int type)
{
    if (sock == INVALID_SOCKET)
    {
        sock = socket(AF_INET, type, 0);
        if (sock == INVALID_SOCKET)
        {
            perror("socket create failed");
            return false;
        }
    }
    return true;
}

error_code_t ClientIPDetector::preprocess_script_command()
{
    error_code_t ret_val = SOCKET_ERROR;

    switch (m_cci.command)
    {
    case OPEN_TCP:
    case OPEN_UDP:
        m_test_port = m_cci.port;
        PACK_DATA_PORT(&m_cci);
        ret_val = NO_ERROR;
        break;

    case TEST_UDP: // request server to listen UDP from client
        if (m_sock_hndl && check_socket(m_test_sock, SOCK_DGRAM))
        {
            m_cci.sock = m_sock_hndl;
            PACK_DATA_SOCK(&m_cci);
            ret_val = NO_ERROR;
        }
        break;

    case LISTEN:
    case CONNECT:
        if (m_sock_hndl && check_socket(m_test_sock, SOCK_STREAM))
        {
            m_cci.sock = m_sock_hndl;
            PACK_DATA_SOCK(&m_cci);
            ret_val = NO_ERROR;
        }
        break;

    case CLOSE:
        if (m_test_sock != INVALID_SOCKET)
        {
            close(m_test_sock);
            m_test_sock = INVALID_SOCKET;
        }
        // no break intentionally
    case STATS:
        if (m_sock_hndl)
        {
            m_cci.sock = m_sock_hndl;
            PACK_DATA_SOCK(&m_cci);
            ret_val = NO_ERROR;
        }
        break;
    case SESSION_END:
    case ERRNO:
        ret_val = NO_ERROR;
    }
    return ret_val;
}

///
/// \brief connect_tcp_server
/// \param line - should contain valid ip address and port in format "127.0.0.1,1234"
/// \param &m_ctrl_sock - ref.to fill socket desc
/// \return error_code
///
error_code_t ClientIPDetector::connect_tcp_server(char *line)
{
    char *p_start =  strchr(line, ',');
    if (p_start)
    {
        *p_start = '\0';
        strcpy(m_server_ip, line);
        int server_port = atoi(p_start+1);
        if (server_port > 0)
        {
            m_ctrl_sock = socket(AF_INET, SOCK_STREAM, 0);
            if (m_ctrl_sock > INVALID_SOCKET)
            {
                //assign server values
                struct sockaddr_in server_addr;
                server_addr.sin_addr.s_addr = inet_addr(m_server_ip);
                server_addr.sin_family = AF_INET;
                server_addr.sin_port = htons(server_port);

                if (connect(m_ctrl_sock, (struct sockaddr *) &server_addr, sizeof(server_addr)) < 0)
                {
                    perror("failed connect to server\n");
                    close(m_ctrl_sock);
                    m_ctrl_sock = INVALID_SOCKET;
                }
                else
                {
                    return NO_ERROR;
                }
            }
            return SOCKET_ERROR;
        }
    }
    return INVALID_ARGUMENT;
}

error_code_t ClientIPDetector::process_server_response()
{
    if (!m_rsp_cci)
    {
       return NOT_INITIALIZED;
    }
    else if (m_rsp_cci->err_code == NO_ERROR)
    {
        switch (m_cci.command)
        {
        case OPEN_UDP:
            UNPACK_DATA_SOCK(m_rsp_cci);
            m_sock_hndl = m_rsp_cci->sock;
            break;

        case TEST_UDP:
            handle_test_udp_command();
            break;
        }
        return NO_ERROR;
    }
    return (error_code_t) m_rsp_cci->err_code;
}

void ClientIPDetector::handle_test_udp_command()
{
    struct sockaddr_in server_addr;
    socklen_t addrlen = sizeof(server_addr);

    server_addr.sin_addr.s_addr = inet_addr(m_server_ip);
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(m_test_port);

    char buf[BUFSIZE] = "test-ping-beep";
    int nb = 0;

    printf("testing UDP sock %d, sendto \"%s\" to server %s port %d\n", m_test_sock, buf, m_server_ip, m_test_port);
    sendto(m_test_sock, buf, strlen(buf), 0, (struct sockaddr *) &server_addr, addrlen);
    if ((nb = sendto(m_test_sock, buf, strlen(buf), 0, (struct sockaddr *) &server_addr, addrlen)) > 0)
    {
        printf("testing UDP sock sent %d bytes, listening...\n", nb);
        if ((nb = recvfrom(m_test_sock, buf, BUFSIZE, 0, (struct sockaddr *) &server_addr, &addrlen)) <= 0)
        {
            // TODO: add error handling
        }
        else
        {
            printf("UDP m_test_sock recvfrom %d bytes\n", nb);
        }
    }
    else
    {
        // TODO: error handling
    }
}

error_code_t ClientIPDetector::init_response_info(const char *buffer)
{
    if (!buffer)
    {
        return NOT_INITIALIZED;
    }
    m_rsp_cci = (control_channel_info_t *) buffer;
    return NO_ERROR;
}

void ClientIPDetector::printf_cci_info(const char* command)
{
    printf("sending command \"%s\" %02X %02X %02X %02X %02X len %d\n",
           command, m_cci.command, m_cci.data[0], m_cci.data[1], m_cci.data[2], m_cci.data[3], m_cci.len);

}
