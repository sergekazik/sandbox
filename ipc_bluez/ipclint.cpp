#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "icommon.h"

void die(const char *s)
{
  perror(s);
  exit(1);
}

Msg_Type_t msg_list[] = {
    MSG_OPEN_SESSION,
    MSG_POWER,
    MSG_ADD_SERVICE,
    MSG_ADD_ATTRIBUTE,
    MSG_START_ADVERTISEMENT,
    MSG_STOP_ADVERTISEMENT,
    MSG_POWER,
    MSG_CLOSE_SESSION
};

int main()
{
    int msqid, rsp_msg_id;
    key_t key;
    Comm_Msgbuf_t sbuf;
    size_t buflen;
    int msgflg = 0666;
    bool bPower = false;

    key = SHARED_KEY;
    if ((msqid = msgget(key, msgflg )) < 0)   //Get the message queue ID for the given key
    {
      die("msgget");
    }

    key = NOTIFY_KEY;
    if ((rsp_msg_id = msgget(key, msgflg )) < 0)   //Get the message queue ID for the given key
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

        if (msg_list[i] == MSG_POWER)
        {
            bPower = !bPower;
            sbuf.msg.data.power.on_off = bPower?1:0;
        }

        if (msgsnd(msqid, &sbuf, buflen, IPC_NOWAIT) < 0)
        {
            printf ("%d, %ld, %d, %d \n", msqid, sbuf.mtype, sbuf.msg.type, (int)buflen);
            die("msgsnd");
        }
        else
        {
            printf ("Message Sent to %d, %ld, type %d, len %d \n", msqid, sbuf.mtype, sbuf.msg.type, (int)buflen);
            if (msgrcv(rsp_msg_id, &sbuf, buflen, 1, 0) < 0)
            {
              die("msgrcv");
            }
            printf ("ACK-> %d, %ld, msg type %d, len %d error =  %d\n", rsp_msg_id, sbuf.mtype, sbuf.msg.type, (int)buflen, sbuf.msg.error);
        }
    }

    return 0;
}
