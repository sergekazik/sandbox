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
    const char * client_local = NULL;
    const char * msg = NULL;
    bool bAuto = false;

    if (argc > 1 && strstr(argv[1], "--auto"))
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
    //initialize socket and structure
    int socketfd, rcv;
    struct sockaddr_in myaddr;
    char message[MSG_LENGHT] = "";

    struct sockaddr_in server_addr;
    socklen_t addrlen = sizeof(server_addr);

    //create socket
    socketfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (socketfd == -1) {
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
