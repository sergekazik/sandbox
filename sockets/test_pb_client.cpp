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

int exec_script(const char* script_name);

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
    server_addr.sin_addr.s_addr = inet_addr(TEST_ADDR);
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

int exec_script(const char* script_name)
{
    error_code_t ret_val = GENERAL_ERROR;

    FILE *fin = fopen(script_name, "rt");
    if (!fin)
    {
        perror("failed to open file\n");
        return ret_val;
    }

    char line[0Xff];
    Ring::ClientIPDetector client_control;

    // here to connect to TCP server with client_control.m_ctrl_sock
    // the first line in script should contain valid ip address and port in format 127.0.0.1,1234
    if (fgets(line, 0Xff, fin) != NULL)
    {
        ret_val = client_control.connect_tcp_server(line);
    }

    while ((ret_val == NO_ERROR) && (fgets(line, 0Xff, fin) != NULL))
    {
        printf("\n->%s", line);
        if (line[strlen(line)-1]=='\n' || line[strlen(line)-1]=='\r')
        {
            line[strlen(line)-1]='\0';
        }

        if (NO_ERROR != (ret_val = client_control.parse_script_command(line)))
        {
            printf("error parsing ret = %d, abort\n", ret_val);
            break;
        }

        ret_val = client_control.preprocess_script_command();

        if (ret_val != NO_ERROR)
        {
            printf("error processing command \"%s\" ret = %d, abort\n", line, ret_val);
            break;
        }

        // sending
        client_control.print_cci_info(line);
        if (NO_ERROR != (ret_val = client_control.send_tcp_server()))
        {
            perror("control channel communication failed");
            break;
        }

        if (client_control.m_cci.command == SESSION_END)
        {   // end of the session - no need ACK
            break;
        }

        // getting rsp
        if (NO_ERROR != (ret_val = client_control.recv_tcp_server()))
        {
            perror("control channel communication failed");
            break;
        }

        client_control.dump_recv_buffer(16);

        ret_val = client_control.process_server_response();
    }

    if (fin)
    {
        fclose(fin);
    }

    return ret_val;
}
