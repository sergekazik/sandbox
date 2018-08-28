#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "icommon.h"

void die(char *s)
{
  perror(s);
  exit(1);
}

Msg_Type_t msg_list[] = {
    MSG_OPEN_SESSION,
    MSG_POWER_ON,
    MSG_ADD_SERVICE,
    MSG_ADD_ATTRIBUTE,
    MSG_START_ADVERTISEMENT,
    MSG_STOP_ADVERTISEMENT,
    MSG_POWER_OFF,
    MSG_CLOSE_SESSION
};


int main()
{
    int msqid;
    key_t key;
    Comm_Msgbuf_t sbuf;
    size_t buflen;
    int msgflg = 0666;

    key = SHARED_KEY;

    if ((msqid = msgget(key, msgflg )) < 0)   //Get the message queue ID for the given key
    {
      die("msgget");
    }

    //Message Type
    sbuf.mtype = 1;

    for (uint32_t i = 0; i < sizeof(msg_list)/sizeof(Msg_Type_t); i++)
    {
        sbuf.msg.error = NO_ERROR;
        sbuf.msg.session_id = 0;
        sbuf.msg.type = msg_list[i];
        buflen = sizeof(Comm_Msg_t) + 1;

        if (msgsnd(msqid, &sbuf, buflen, IPC_NOWAIT) < 0)
        {
            printf ("%d, %ld, %d, %d \n", msqid, sbuf.mtype, sbuf.msg.type, (int)buflen);
            die("msgsnd");
        }
        else
        {
            printf("Message %d Sent\n", i+1);
            if (msgrcv(msqid, &sbuf, MAXSIZE, 1, 0) < 0)
            {
              die("msgrcv");
            }
            printf ("ACK-> %d, %ld, %d, %d \n", msqid, sbuf.mtype, sbuf.msg.type, (int)buflen);
        }
    }

    return 0;
}
