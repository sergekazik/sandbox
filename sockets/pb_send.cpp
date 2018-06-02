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
    struct sockaddr_in myaddr;
    char message[100];

    struct sockaddr_in server_addr;
    socklen_t addrlen = sizeof(server_addr);

    //create socket
    socketfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (socketfd == -1) {
        printf("Could not create socket");
    }

    //assign local values
    myaddr.sin_addr.s_addr = inet_addr(test_addr);
    myaddr.sin_family = AF_INET;
    myaddr.sin_port = htons( MY_PORT );

    printf("Binding to %s port %d\n", test_addr, MY_PORT);
    //binds connection
    if (bind(socketfd, (struct sockaddr *)&myaddr, sizeof(myaddr)) < 0) {
        perror("bind error");
        goto done;
    }

    printf("Input Message: ");
    fgets(message, 100, stdin);

    //assign server values
    server_addr.sin_addr.s_addr = inet_addr(test_addr);
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons( SERVER_PORT );

    //sends message
    if (sendto(socketfd, message, strlen(message), 0, (struct sockaddr *) &server_addr, addrlen) <0) {
        perror("Send failed");
        goto done;
    }
    puts("Message Sent");

    //receives message back
    if (recvfrom(socketfd, message, sizeof(message), 0, (struct sockaddr *) &server_addr, &addrlen) < 0) {
        puts("Received failed");
        goto done;
    }
    puts("Message received");
    puts(message);

done:
    puts("done");
    if (socketfd)
        close(socketfd);
}
