#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>

#include <mutex>
#include <iostream>
#include <vector>
#include <algorithm>
#include <numeric>
#include <future>

#include <sys/types.h>       // For data types
#include <sys/socket.h>      // For socket(), connect(), send(), and recv()
#include <netdb.h>           // For gethostbyname()
#include <arpa/inet.h>       // For inet_addr()
#include <unistd.h>          // For close()
#include <netinet/in.h>      // For sockaddr_in

#include "RingIPDetector.hh"
using namespace Ring;

#define PARSE_CMD_GEN(cmd_to_parse_arg) { #cmd_to_parse_arg, cmd_to_parse_arg, cmd_to_parse_arg##_RSP_LEN}
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

int run_ctrl_chan_server();

int main(int argc, char *argv[])
{
    if ((argc == 1) || ((argc > 1) && strstr(argv[1], "--ctrlchan"))) // ex: test_server_portblock --ctrlchan
    {
        return run_ctrl_chan_server();
    }
    // else if (argc > 1 && strstr(argv[1], "--test"))

    //initialize socket and structure
    int socketfd;
    struct sockaddr_in server;
    char message[MSG_LENGHT];
    struct sockaddr src_addr;
    socklen_t addrlen = sizeof(src_addr);

    //create socket
    socketfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (socketfd == -1)
    {
        printf("Could not create socket");
    }

    //assign values
    server.sin_addr.s_addr = inet_addr(TEST_ADDR);
    server.sin_family = AF_INET;
    server.sin_port = htons( SERVER_PORT );

    //checks connection
    if (bind(socketfd, (struct sockaddr *)&server, sizeof(server)) < 0)
    {
        perror("Connection error");
        goto done;
    }
    printf("Bind to %s port %d\n", TEST_ADDR, SERVER_PORT);

    //Receive an incoming message
    if (recvfrom(socketfd, message, sizeof(message), 0, &src_addr, &addrlen) < 0)
    {
        puts("Received failed");
        goto done;
    }

    printf("Message received: \"%s\"\n", message);
    {
        char const *ack = ",ACK";
        if (strlen(ack) +  strlen(message) < MSG_LENGHT)
            strcpy(&message[strlen(message)], ack);
    }
    printf("Sending message \"%s\" back to client\n", message);

    if (sendto(socketfd, message, strlen(message), 0, &src_addr, addrlen) <0)
    {
        perror("Send failed");
        goto done;
    }
    puts("Message Sent");

done:
    puts("done");
    if (socketfd)
        close(socketfd);
}

void exec_test_udp(int test_sock)
{
    //initialize socket and structure
    int socketfd = test_sock;
    char message[MSG_LENGHT];
    struct sockaddr src_addr;
    socklen_t addrlen = sizeof(src_addr);
    int nb = 0;

    //Receive an incoming message
    printf("|UDP| listening on UDP sock %d\n", test_sock);
    if ((nb = recvfrom(socketfd, message, sizeof(message), 0, &src_addr, &addrlen)) < 0)
    {
        perror("// TODO: add error handling");
    }
    else
    {
        printf("|UDP| test_sock recvfrom %d bytes \"%s\"\n", nb, message);
        nb = sendto(socketfd, message, strlen(message), 0,  (struct sockaddr *) &src_addr, addrlen);
        printf("|UDP| sent %d byte on UDP socket back\n", nb);
    }
}

int run_ctrl_chan_server()
{
    int parentfd; /* parent socket */
    int childfd; /* child socket */
    int portno; /* port to listen on */
    socklen_t clientlen; /* byte size of client's address */
    struct sockaddr_in serveraddr; /* server's addr */
    struct sockaddr_in clientaddr; /* client addr */
    struct hostent *hostp; /* client host info */
    unsigned char buf[BUFSIZE]; /* message buffer */
    char *hostaddrp; /* dotted decimal host addr string */
    int optval; /* flag value for setsockopt */
    int n; /* message byte size */

    int test_sock = INVALID_SOCKET;

    // check command line arguments
    portno = SERVER_PORT; // atoi(argv[1]);

    // socket: create the parent socket
    parentfd = socket(AF_INET, SOCK_STREAM, 0);
    if (parentfd < 0)
        perror("ERROR opening socket");

    /* setsockopt: Handy debugging trick that lets
     * us rerun the server immediately after we kill it;
     * otherwise we have to wait about 20 secs.
     * Eliminates "ERROR on binding: Address already in use" error.
     */
    optval = 1;
    setsockopt(parentfd, SOL_SOCKET, SO_REUSEADDR, (const void *)&optval , sizeof(int));

    // build the server's Internet address
    bzero((char *) &serveraddr, sizeof(serveraddr));

    serveraddr.sin_family = AF_INET;
    serveraddr.sin_addr.s_addr = inet_addr(TEST_ADDR); // htonl(INADDR_ANY);
    serveraddr.sin_port = htons((unsigned short)portno);

    // bind: associate the parent socket with a port
    if (bind(parentfd, (struct sockaddr *) &serveraddr, sizeof(serveraddr)) < 0)
        perror("ERROR on binding");

    // listen: make this socket ready to accept connection requests
    if (listen(parentfd, 5) < 0) /* allow 5 requests to queue up */
        perror("ERROR on listen");

    // main loop: wait for a connection request, echo input line, then close connection.
    printf("starting main loop on %s port %d\n", TEST_ADDR, portno);
    clientlen = sizeof(clientaddr);
    bool done = false;
    while (!done) {
        bool session_done  = false;
        printf("server is listening on ctrl_chan for connections...\n");

        // accept: wait for a connection request
        childfd = accept(parentfd, (struct sockaddr *) &clientaddr, &clientlen);
        if (childfd < 0)
            perror("ERROR on accept");

        // gethostbyaddr: determine who sent the message
        hostp = gethostbyaddr((const char *)&clientaddr.sin_addr.s_addr, sizeof(clientaddr.sin_addr.s_addr), AF_INET);
        if (hostp == NULL)
            perror("ERROR on gethostbyaddr");
        hostaddrp = inet_ntoa(clientaddr.sin_addr);
        if (hostaddrp == NULL)
            perror("ERROR on inet_ntoa\n");
        printf("server established connection with %s (%s)\n", hostp->h_name, hostaddrp);

        while (!session_done)
        {
            std::future<void> fut;
            // read: read input string from the client
            bzero(buf, BUFSIZE);
            n = read(childfd, buf, BUFSIZE);
            if (n < 0)
            {
                perror("ERROR reading from socket");
                break; // from this loop while
            }
            printf("server received %s - %d bytes:", n?cmd_translated[buf[0]].cmd_name:"", n);
            for (int i = 0; i < n; i++)
                printf(" %02X", (uint8_t) buf[i]);
            printf("\n");
            if ( n == 0)
                break;

            // here do action, generate reply and write to socket
            control_channel_info_t *cci = (control_channel_info_t*) buf;
            cci->len = cmd_translated[cci->command].cmd_len;
            switch (cci->command)
            {
            case OPEN_UDP:
                if (test_sock != INVALID_SOCKET)
                {
                    close(test_sock);
                }

                UNPACK_DATA_SOCK(cci);
                cci->err_code = SOCKET_ERROR;

                test_sock = socket(AF_INET, SOCK_DGRAM, 0);
                if (test_sock != INVALID_SOCKET)
                {
                    //assign values
                    int port = cci->port;
                    struct sockaddr_in server;
                    server.sin_addr.s_addr = inet_addr(TEST_ADDR);
                    server.sin_family = AF_INET;
                    server.sin_port = htons(port);

                    //checks connection
                    if (bind(test_sock, (struct sockaddr *)&server, sizeof(server)) < 0)
                    {
                        perror("Connection error");
                        close(test_sock);
                        test_sock = INVALID_SOCKET;
                    }
                    else
                    {
                        cci->err_code = NO_ERROR;
                        cci->sock = test_sock;
                        PACK_DATA_SOCK(cci);
                    }
                    printf("binding UDP socket %d to port %d\n", test_sock, cci->port);
                }
                break;

            case TEST_UDP:
                UNPACK_DATA_SOCK(cci);
                if (test_sock == INVALID_SOCKET)
                {
                    cci->err_code = NOT_AVAILABLE;
                }
                else if (test_sock != cci->sock)
                {
                    cci->err_code = INVALID_REQUEST;
                }
                else
                {
                    fut = std::async(std::launch::async, exec_test_udp, test_sock);
                    cci->err_code = NO_ERROR;
                }
                break;

            case CLOSE:
                if (test_sock != INVALID_SOCKET)
                {
                    close(test_sock);
                }
                cci->err_code = NO_ERROR;
                break;

            case STATS:
                cci->err_code = NO_ERROR;
                // mocking - TODO:
                cci->stats.rcv_packets = 1;
                cci->stats.rcv_bytes = 15;
                cci->stats.snd_packets = 1;
                cci->stats.snd_butes = 15;
                break;

            case SESSION_END:
                printf("end of session command - done!\n\n");
                close(childfd);
                session_done = true;
                break;
            }

            if (!session_done)
            {
                printf("sending %d bytes:", cci->len);
                for (int i = 0; i < cci->len; i++)
                    printf(" %02X", (uint8_t) buf[i]);
                printf("\n");

                n = send(childfd, buf, cci->len, 0);
                if (n < 0)
                {
                    perror("ERROR writing to socket");
                    done = true;
                }
            }
        }
    }
    return 0;
}
