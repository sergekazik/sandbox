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

#include "pb_common.h"

int exec_script(const char* script_name);
void handle_recvfrom_command(int test_sock, int port);

int main(int argc, char *argv[])
{
    const char *client_local = NULL;
    const char *msg = NULL;
    bool bAuto = false;
    char *script_name = NULL;

    if (argc > 2 && strstr(argv[1], "-f"))
    {
        script_name = argv[2];
        bAuto = true;
    }
    else if (argc > 1 && strstr(argv[1], "--auto"))
    {
        bAuto = true;
    }
    else
    {
        if (argc > 1)
            client_local = argv[1];
        if (argc > 2)
            msg = argv[2];
    }

    if (bAuto && script_name)
    {
//        system("./test_server_portblock --ctrlchan");
//        sleep(1);
        exec_script(script_name);
        return 0;
    }

    //initialize socket and structure
    int socketfd, rcv;
    struct sockaddr_in myaddr;
    char message[MSG_LENGHT] = "";

    struct sockaddr_in server_addr;
    socklen_t addrlen = sizeof(server_addr);

    //create socket
    socketfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (socketfd == INVALID_SOCKET) {
        printf("Could not create socket");
    }

    //assign local values
    myaddr.sin_addr.s_addr = client_local ? inet_addr(client_local) : INADDR_ANY;
    myaddr.sin_family = AF_INET;
    myaddr.sin_port = htons( CLIENT_PORT );

    printf("Binding to %s port %d\n", inet_ntoa(myaddr.sin_addr), CLIENT_PORT);
    //binds connection
    if (bind(socketfd, (struct sockaddr *)&myaddr, sizeof(myaddr)) < 0) {
        perror("bind error");
        goto done;
    }

    if (bAuto)
    {
        strcpy(message, "start port test");
    }
    else if (msg == NULL)
    {
        printf("Input Message: ");
        fgets(message, MSG_LENGHT, stdin);
    }
    else
    {
        strcpy(message, msg);
    }
    if (strlen(message) && ((message[strlen(message)-1] == '\n') || (message[strlen(message)-1] == '\r')))
        message[strlen(message)-1] = '\0';

    //assign server values
    server_addr.sin_addr.s_addr = inet_addr(test_addr);
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons( SERVER_PORT );

    //sends message
    if (sendto(socketfd, message, strlen(message), 0, (struct sockaddr *) &server_addr, addrlen) <0) {
        perror("Send failed");
        goto done;
    }
    printf("Message Sent to %s port %d\n", inet_ntoa(server_addr.sin_addr), SERVER_PORT);

    //receives message back
    if ((rcv = recvfrom(socketfd, message, sizeof(message), 0, (struct sockaddr *) &server_addr, &addrlen)) < 0) {
        puts("Received failed");
        goto done;
    }
    message[rcv]='\0';
    puts("Message received");
    puts(message);

done:
    puts("done");
    if (socketfd)
        close(socketfd);
}

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
    PARSE_CMD_GEN(RECVFROM),
    PARSE_CMD_GEN(SENDTO  ),
    PARSE_CMD_GEN(LISTEN  ),
    PARSE_CMD_GEN(CONNECT ),
    PARSE_CMD_GEN(STATS   ),
    PARSE_CMD_GEN(CLOSE   ),
    PARSE_CMD_GEN(SESSION_END)
};

error_code_t parse_script_command(const char *line, control_channel_info_t *cci)
{
    if (!line || !cci)
    {
        return INVALID_ARGUMENT;
    }

    memset(cci, 0, sizeof(control_channel_info_t));

    for (unsigned int i = 0; i < sizeof(cmd_translated)/sizeof(parse_cmd_list); i++)
    {
        if (0 == strncmp(line, cmd_translated[i].cmd_name, strlen(cmd_translated[i].cmd_name)))
        {
            cci->command = cmd_translated[i].cmd_val;
            cci->len = cmd_translated[i].cmd_len;

            switch (cci->command)
            {
            case OPEN_UDP:
            case OPEN_TCP:
                {
                    int port;
                    if (1 != sscanf(strstr(line, ",")+1, "%d", &port))
                    {
                        return INVALID_ARGUMENT;
                    }
                    cci->port = port;
                    PACK_DATA_PORT(cci);
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

bool check_socket(int &sock, int type)
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


int exec_script(const char* script_name)
{
    error_code_t ret_val = NO_ERROR;

    FILE *fin = fopen(script_name, "rt");
    if (!fin)
    {
        perror("failed to open file\n");
        return GENERAL_ERROR;
    }

    int ctrl_sock = INVALID_SOCKET;
    int test_sock = INVALID_SOCKET;
    int test_port = 0;
    uint16_t sock_hndl = 0;
    char line[0Xff];

    // here to connect to TCP server with ctrl_sock
    // the first line in script should contain valid ip address and port in format 127.0.0.1,1234
    ret_val = INVALID_ARGUMENT;
    if (fgets(line, 0Xff, fin) != NULL)
    {
        char *p_start =  strchr(line, ',');
        if (p_start)
        {
            *p_start = '\0';
            char *server_ip = line;
            int server_port = atoi(p_start+1);
            if (server_port > 0)
            {
                ctrl_sock = socket(AF_INET, SOCK_STREAM, 0);
                if (ctrl_sock > INVALID_SOCKET)
                {
                    //assign server values
                    struct sockaddr_in server_addr;
                    server_addr.sin_addr.s_addr = inet_addr(server_ip);
                    server_addr.sin_family = AF_INET;
                    server_addr.sin_port = htons(server_port);

                    if (connect(ctrl_sock, (struct sockaddr *) &server_addr, sizeof(server_addr)) < 0)
                    {
                        ret_val = SOCKET_ERROR;
                        perror("failed connect to server\n");
                    }
                    else
                    {
                        ret_val = NO_ERROR;
                    }
                }
            }
        }
    }

    while ((ret_val == NO_ERROR) && (fgets(line, 0Xff, fin) != NULL))
    {
        printf("->%s", line);
        if (line[strlen(line)-1]=='\n' || line[strlen(line)-1]=='\r')
        {
            line[strlen(line)-1]='\0';
        }

        control_channel_info_t cci;

        if (NO_ERROR != (ret_val = parse_script_command(line, &cci)))
        {
            printf("error parsing ret = %d, abort\n", ret_val);
            break;
        }

        switch (cci.command)
        {
        case OPEN_TCP:
        case OPEN_UDP:
            test_port = cci.port;
            break;

        case SENDTO:
        case RECVFROM: // request server to recvfrom client
            if (sock_hndl && check_socket(test_sock, SOCK_DGRAM))
            {
                cci.sock = sock_hndl;
                PACK_DATA_SOCK(&cci);
            }
            else
            {
                ret_val = SOCKET_ERROR;
            }
            break;

        case LISTEN:
        case CONNECT:
            if (sock_hndl && check_socket(test_sock, SOCK_STREAM))
            {
                cci.sock = sock_hndl;
                PACK_DATA_SOCK(&cci);
            }
            else
            {
                ret_val = SOCKET_ERROR;
            }
            break;

        case CLOSE:
            if (test_sock != INVALID_SOCKET)
            {
                close(test_sock);
                test_sock = INVALID_SOCKET;
            }
            // no break intentionally
        case STATS:
            if (sock_hndl)
            {
                cci.sock = sock_hndl;
                PACK_DATA_SOCK(&cci);
            }
            else
            {
                ret_val = SOCKET_ERROR;
            }
            break;
        }

        if (ret_val != NO_ERROR)
        {
            printf("error processing command \"%s\" ret = %d, abort\n", line, ret_val);
            break;
        }

        // sending command to server --------------------------------------------------------
        printf("sending command \"%s\" %02X %02X %02X %02X %02X len %d\n",
               line, cci.command, cci.data[0], cci.data[1], cci.data[2], cci.data[3], cci.len);
        if (cci.len != send(ctrl_sock, &cci, cci.len, 0))
        {
            perror("control channel communication failed");
            ret_val = SOCKET_ERROR;
            break;
        }
        if (cci.command == SESSION_END)
        {
            // end of the session - no need ACK
            break;
        }

        // getting response and processing section -------------------------------------------
        char buf[BUFSIZE];
        bzero(buf, BUFSIZE);
        int n = read(ctrl_sock, buf, BUFSIZE);
        if (n < 0)
        {
            perror("ERROR reading from socket");
            ret_val = RECV_ERROR;
        }
        else
        {
            printf("received %d bytes:", n);
            for (int i = 0; i < n; i++)
                printf(" %02X", (uint8_t) buf[i]);
            printf("\n");

            control_channel_info_t *rsp_cci = (control_channel_info_t *)buf;
            switch (cci.command)
            {
            case OPEN_UDP:
                if (rsp_cci->err_code == NO_ERROR)
                {
                    UNPACK_DATA_SOCK(rsp_cci);
                    sock_hndl = rsp_cci->sock;
                }
                break;

            case RECVFROM:
                handle_recvfrom_command(test_sock, test_port);
                break;
            }
        }
    }

    if (fin)
    {
        fclose(fin);
    }
    if (test_sock != INVALID_SOCKET)
    {
        close(test_sock);
        test_sock = INVALID_SOCKET;
    }

    return ret_val;
}

void handle_recvfrom_command(int test_sock, int port)
{
    struct sockaddr_in server_addr;
    socklen_t addrlen = sizeof(server_addr);

    server_addr.sin_addr.s_addr = inet_addr(test_addr);
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);

    char buf[BUFSIZE] = "test-ping-beep";
    int nb = 0;

    printf("testing UDP sock %d, to server %s port %d\n", test_sock, test_addr, port);
    sleep(3);
    if ((nb = sendto(test_sock, buf, strlen(buf), 0, (struct sockaddr *) &server_addr, addrlen)) > 0)
    {
        printf("testing UDP sock sent %d bytes, listening...\n", nb);
        if ((nb = recvfrom(test_sock, buf, BUFSIZE, 0, (struct sockaddr *) &server_addr, &addrlen)) <= 0)
        {
            // TODO: add error handling
        }
        else
        {
            printf("UDP test_sock recvfrom %d bytes\n", nb);
        }
    }
    else
    {
        // TODO: error handling
    }
}
