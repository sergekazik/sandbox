#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <stdio.h>
#include <stdlib.h>

#include "icommon.h"

#ifdef DEBUG_ENABLED
static const char *debug_msg[] =
{
    "MSG_OPEN_SESSION",
    "MSG_CLOSE_SESSION",
    "MSG_POWER_ON",
    "MSG_CONFIG",
    "MSG_POWER_OFF",
    "MSG_START_ADVERTISEMENT",
    "MSG_STOP_ADVERTISEMENT",
    "MSG_ADD_SERVICE",
    "MSG_ADD_ATTRIBUTE",
    "MSG_UPDATE_ATTRIBUTE",
    "MSG_REGISTER_ON_CONNECT",
    "MSG_REGISTER_ON_DISCONNECT",
    "MSG_REGISTER_ON_DATA_READ",
    "MSG_REGISTER_ON_DATA_WRITE",
    "MSG_NOTIFY_ON_CONNECT",
    "MSG_NOTIFY_ON_DISCONNECT",
    "MSG_NOTIFY_ON_DATA_READ",
    "MSG_NOTIFY_ON_DATA_WRITE",
};
#endif

void print_out_msg(Comm_Msg_t *cm)
{
#ifdef DEBUG_ENABLED
    printf("ipcluez got: %d, %s\n", cm->type, debug_msg[cm->type]);
#endif
}

void die(char *s)
{
  perror(s);
  exit(1);
}

int main()
{
    int msqid;
    key_t key;
    Comm_Msgbuf_t rcvbuffer;
    int msgflg = IPC_CREAT | 0666;

    key = SHARED_KEY;

    if ((msqid = msgget(key, msgflg )) < 0)
    {
      die("msgget()");
    }

    while (1)
    {
         //Receive an answer of message type 1.
        if (msgrcv(msqid, &rcvbuffer, sizeof(Comm_Msgbuf_t), 1, 0) < 0)
        {
          die("msgrcv");
        }

        print_out_msg(&rcvbuffer.msg);

        // do handling here

        rcvbuffer.msg.error = NO_ERROR;
        int buflen = sizeof(Comm_Msg_t) + 1;

        if (msgsnd(msqid, &rcvbuffer, buflen, IPC_NOWAIT) < 0)
        {
            die("msgrcv");
        }

        if (rcvbuffer.msg.type == MSG_CLOSE_SESSION)
        {
            break;
        }
    }
    return 0;
}

