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

int main(int argc, char *argv[])
{
    if (argc > 1)
        test_addr = argv[1];

    //initialize socket and structure
    int socketfd;
    struct sockaddr_in server;
    char message[MSG_LENGHT];
    struct sockaddr src_addr;
    socklen_t addrlen = sizeof(src_addr);

    //create socket
    socketfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (socketfd == -1) {
        printf("Could not create socket");
    }

    //assign values
    server.sin_addr.s_addr = inet_addr(test_addr);
    server.sin_family = AF_INET;
    server.sin_port = htons( MY_PORT );

    //checks connection
    if (bind(socketfd, (struct sockaddr *)&server, sizeof(server)) < 0) {
        perror("Connection error");
        goto done;
    }
    printf("Bind to %s port %d\n", test_addr, MY_PORT);

    //Receive an incoming message
    if (recvfrom(socketfd, message, sizeof(message), 0, &src_addr, &addrlen) < 0) {
        puts("Received failed");
        goto done;
    }

    printf("Message received: \"%s\"\n", message);
    {
        char const *ack = " - acknowledged";
        if (strlen(ack) +  strlen(message) < MSG_LENGHT)
            strcpy(&message[strlen(message)], ack);
    }
    printf("Sending message \"%s\" back to client\n", message);

    if (sendto(socketfd, message, strlen(message), 0, &src_addr, addrlen) <0) {
        perror("Send failed");
        goto done;
    }
    puts("Message Sent");

done:
    puts("done");
    if (socketfd)
        close(socketfd);
}

