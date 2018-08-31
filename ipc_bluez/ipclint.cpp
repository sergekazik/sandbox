#include "icommon.h"

// static vars, status
static bool gbPower = false;
static bool gbSession = false;

static Msg_Type_t msg_list[] = {
    MSG_SESSION,
    MSG_POWER,
    MSG_ADD_SERVICE,
    MSG_ADD_ATTRIBUTE,
    MSG_ADVERTISEMENT,
    MSG_POWER,
    MSG_SESSION
};

int main(int argc, char** argv )
{
    Comm_Msg_t msg;

    if (Ble::Error::NONE != parse_command_line(argc, argv))
    {
        die("invalid parameters");
    }

    if (Ble::Error::NONE != init_comm(CLIENT))
    {
        die("failed to init communication");
    }

    for (uint32_t i = 0; i < sizeof(msg_list)/sizeof(Msg_Type_t); i++)
    {
        msg.error = NO_ERROR;
        msg.session_id = 0;
        msg.type = msg_list[i];

        // handle message payload
        if (msg_list[i] == MSG_POWER)
        {
            gbPower = !gbPower;
            msg.data.power.on_off = gbPower?1:0;
        }
        else if (msg_list[i] == MSG_SESSION)
        {
            gbSession = !gbSession;
            msg.data.session.on_off = gbSession?1:0;
        }

        if (Ble::Error::NONE != send_comm(TO_SERVER, &msg, sizeof(msg)))
        {
            die("msg send to server failed");
        }
        else
        {
            printf ("Message Sent to server, type %d %s\n", msg.type, get_msg_name(&msg));
            if (Ble::Error::NONE != recv_comm(FROM_SERVER, &msg))
            {
                die("failed recv from server");
            }
            printf ("got rsp: msg type %d, %s error =  %d\n", msg.type, get_msg_name(&msg), msg.error);
        }
    }

    shut_comm(CLIENT);
    return 0;
}
