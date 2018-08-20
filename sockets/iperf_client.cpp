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

typedef enum error_code
{
    NO_ERROR        = 0,
    INVALID_REQUEST,
    INVALID_ARGUMENT,
    NOT_AVAILABLE,
    SOCKET_ERROR,
    CONNECT_ERROR,
    TIMEOUT_ERROR,
    GENERAL_ERROR,
    NOT_INITIALIZED,
    COMMAND_SKIPPED,
    FAILED_OPEN_CONFIG
} error_code_t;

static const char* config_name = "./iperf.config";

int main(int argc, char *argv[])
{
    // do argument handling here
    (void) argc, (void) argv;

    char line[0Xff];
    char sline[0xff];
    error_code_t ret_val = NO_ERROR;
    FILE *fin = fopen(config_name, "rt");
    if (!fin)
    {
        perror("failed to open file\n");
        ret_val = FAILED_OPEN_CONFIG;
        goto done;
    }

    system("/bin/rm iperf.out");
    for (int i = 0; (ret_val == NO_ERROR) && (fgets(line, 0Xff, fin) != NULL); i++)
    {
        printf("\n%d) %s", i+1, line);
        fflush(stdout);

        while (strlen(line) && (line[strlen(line)-1]=='\n' || line[strlen(line)-1]=='\r'))
        {
            line[strlen(line)-1]='\0';
        }

        sprintf(sline, "%s >> iperf.out 2>&1", line);
        system(sline);
        system("/bin/cat iperf.out");
    }

done:
    if (fin)
    {
        fclose(fin);
    }

    return ret_val;
}
