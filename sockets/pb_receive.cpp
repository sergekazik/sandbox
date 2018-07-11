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

#include "pb_common.h"
typedef char *tokarr[8];

int split(char* str, tokarr &tarr)
{

  char * pch;
  printf ("Splitting string \"%s\" into tokens:\n",str);
  pch = strtok (str,","); // strtok (str," ,.-");
  int count = 0;
  while (pch != NULL)
  {
    printf ("%s\n",pch);
    tarr[count]=pch;
    count++;
    pch = strtok(NULL, ",");

  }
  return count;

//  example
//        char str[] ="udp,1001,17";
//        tokarr tarr;
//        int num = split(str, tarr);
//        for (int i = 0; i < num; i++)
//            printf ("%s\n",tarr[i]);
//        return 0;
}

int run_ctrl_chan_server();

int main(int argc, char *argv[])
{
    if (argc > 2 && strstr(argv[1], "--ip")) // ex: test_server_portblock --ip 192.168.1.3
    {
        test_addr = argv[2];
    }
    else if (argc > 1 && strstr(argv[1], "--ctrlchan")) // ex: test_server_portblock --ctrlchan
    {
        return run_ctrl_chan_server();
    }

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
    server.sin_addr.s_addr = inet_addr(test_addr);
    server.sin_family = AF_INET;
    server.sin_port = htons( SERVER_PORT );

    //checks connection
    if (bind(socketfd, (struct sockaddr *)&server, sizeof(server)) < 0)
    {
        perror("Connection error");
        goto done;
    }
    printf("Bind to %s port %d\n", test_addr, SERVER_PORT);

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

void exec_recvfrom(int test_sock)
{
    struct sockaddr src_addr;
    socklen_t addrlen = sizeof(src_addr);
    char buf[BUFSIZE]; /* message buffer */
    int nb = 0;
    printf("listening on UDP sock %d\n", test_sock);
    if ((nb = recvfrom(test_sock, buf, BUFSIZE, 0, &src_addr, &addrlen)) <= 0)
    {
        // TODO: add error handling
    }
    else
    {
        printf("test_sock recvfrom %d bytes\n", nb);
        sendto(test_sock, buf, strlen(buf), 0, &src_addr, addrlen);
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
    char buf[BUFSIZE]; /* message buffer */
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
    serveraddr.sin_addr.s_addr = inet_addr(test_addr); // htonl(INADDR_ANY);
    serveraddr.sin_port = htons((unsigned short)portno);

    // bind: associate the parent socket with a port
    if (bind(parentfd, (struct sockaddr *) &serveraddr, sizeof(serveraddr)) < 0)
        perror("ERROR on binding");

    // listen: make this socket ready to accept connection requests
    if (listen(parentfd, 5) < 0) /* allow 5 requests to queue up */
        perror("ERROR on listen");

    // main loop: wait for a connection request, echo input line, then close connection.
    printf("starting main loop on %s port %d\n", test_addr, portno);
    clientlen = sizeof(clientaddr);
    bool done = false;
    while (!done) {
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

        while (!done)
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
            printf("server received %d bytes:", n);
            if ( n == 0)
                break;
            for (int i = 0; i < n; i++)
                printf(" %02X", (uint8_t) buf[i]);
            printf("\n");

            // here do action, generate reply and write to socket
            control_channel_info_t *cci = (control_channel_info_t*) buf;
            switch (cci->command)
            {
            case OPEN_UDP:
                if (test_sock != INVALID_SOCKET)
                {
                    close(test_sock);
                }

                UNPACK_DATA_SOCK(cci);
                cci->err_code = SOCKET_ERROR;
                cci->len = 1;

                test_sock = socket(AF_INET, SOCK_DGRAM, 0);

                if (test_sock != INVALID_SOCKET)
                {
                    struct sockaddr_in server;
                    server.sin_addr.s_addr = inet_addr(test_addr);
                    server.sin_family = AF_INET;
                    server.sin_port = htons(cci->port);

                    printf("binding UDP to port %d\n", cci->port);
                    if (bind(test_sock, (struct sockaddr *)&server, sizeof(server) == 0))
                    {
                        cci->err_code = NO_ERROR;
                        cci->len = 3;
                        cci->sock = test_sock;
                        PACK_DATA_SOCK(cci);
                    }
                }
                break;

            case RECVFROM:
                UNPACK_DATA_SOCK(cci);
                cci->len = 1;
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
                    fut = std::async(std::launch::async, exec_recvfrom, test_sock);
                    cci->err_code = NO_ERROR;
                }
                break;

            case SESSION_END:
                printf("end of session command - done!\n");
                close(childfd);
                done = true;
                break;
            }

            if (!done)
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
