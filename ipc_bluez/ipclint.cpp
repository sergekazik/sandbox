#include "icommon.h"

// static vars, status
static uint8_t giSessionId = 0;

// sample attribute definitions
//typedef struct characteristicinfo
//{
//    AttributeType_t AttributeType;
//    unsigned int    AttributeOffset;
//    char            AttributeName[ATTR_NAME_LEN];
//    unsigned long   CharacteristicPropertiesMask;
//    unsigned long   SecurityPropertiesMask;
//    UUID_128_t      CharacteristicUUID;
//    Boolean_t       AllocatedValue;
//    unsigned int    MaximumValueLength;
//    unsigned int    ValueLength;
//    Byte_t         *Value;
//} AttributeInfo_t;

#define MAKE_UUID_128(_h, _g, _f, _d, _s, _a, _p, _o, _i, _u, _y, _t, _r, _e, _w, _q) {0x##_h, 0x##_g, 0x##_f, 0x##_d, 0x##_s, 0x##_a, 0x##_p, 0x##_o, 0x##_i, 0x##_u, 0x##_y, 0x##_t, 0x##_r, 0x##_e, 0x##_w, 0x##_q}
#define MAKE_UUID(_attr_idx) MAKE_UUID_128(97,60,AB,BA,A2,34,46,86,9E,20,D0,87,33,3C,2C,_attr_idx)
#define CCCDESC_UUID() MAKE_UUID_128(00,00,29,02,00,00,10,00,80,00,00,80,5F,9B,34,FB)
#define MAKE_PAYLOAD(_pld) (unsigned int) strlen(_pld), (unsigned char *) _pld

// sample Attribute Table definition
static Ble::AttributeInfo_t attr_table[] =
{
    {Ble::atCharacteristic, 1, "SAMPLE_ATTR-1", Ble::Property::RW_, GATM_SECURITY_PROPERTIES_NO_SECURITY, MAKE_UUID(01), 0, 160, MAKE_PAYLOAD("Char1")},
    {Ble::atCharacteristic, 3, "SAMPLE_ATTR-2", Ble::Property::RWN, GATM_SECURITY_PROPERTIES_NO_SECURITY, MAKE_UUID(02), 0, 160, MAKE_PAYLOAD("Char2")},
    {Ble::atDescriptor,     5, "SAMPLE_DESC-1", Ble::Property::RW_, GATM_SECURITY_PROPERTIES_NO_SECURITY, CCCDESC_UUID(),0, 16,  MAKE_PAYLOAD("\x01\x00")}
};

// sample service definitions
//typedef struct serviceinfo
//{
//    unsigned int                   ServiceID;
//    UUID_128_t                     ServiceUUID;
//    unsigned int                   NumberAttributes;
//    AttributeInfo_t               *AttributeList;
//} ServiceInfo_t;
#define SERVICE_UUID   MAKE_UUID_128(97,60,AB,BA,A2,34,46,86,9E,00,FC,BB,EE,33,73,F7)
static Ble::ServiceInfo_t service = {0, SERVICE_UUID, sizeof(attr_table)/sizeof(Ble::AttributeInfo_t), attr_table};

// sample of the configuration
typedef struct {
    Msg_Type_t type;
    void* data;
} SampleStruct_t;
SampleStruct_t msg_list[] = {
    { MSG_SESSION,        /* open */    (void*)1 },
    { MSG_POWER,          /* on */      (void*)1 },
    { MSG_CONFIG,         /*..*/         NULL },
    { MSG_ADD_SERVICE,    /*..*/ (void*) &service },
    { MSG_ADD_ATTRIBUTE,  /*..*/ (void*) &attr_table[0] },
    { MSG_ADD_ATTRIBUTE,  /*..*/ (void*) &attr_table[1] },
    { MSG_ADD_ATTRIBUTE,  /*..*/ (void*) &attr_table[2] },
    { MSG_ADVERTISEMENT,  /* start */   (void*)1 },
    { CMD_PAUSE_GETCHAR,  /* pause */    NULL },
    { MSG_ADVERTISEMENT,  /* stop */    (void*)0 },
    { MSG_POWER,          /* off */     (void*)0 },
    { MSG_SESSION,        /* close */   (void*)0 },
};


void format_message_payload(Msg_Type_t type, Comm_Msg_t &msg, void* data)
{
    msg.hdr.size = sizeof(Common_Header_t);
    msg.hdr.error = NO_ERROR;
    msg.hdr.session_id = giSessionId;
    msg.hdr.type = type;

    switch (type)
    {
    case MSG_SESSION:
        msg.hdr.size += sizeof(Session_t);
        msg.data.session.on_off = (uint8_t)(unsigned long)data;
        break;

    case MSG_POWER:
        msg.hdr.size += sizeof(Power_t);
        msg.data.power.on_off = (uint8_t)(unsigned long)data;
        break;

    case MSG_CONFIG:
        msg.hdr.size += sizeof(Config_t);
        msg.data.config.device_class = 0x000430;
        strcpy(msg.data.config.device_name, "IpClint-24");
        strcpy(msg.data.config.mac_address, "AA:AA:BB:BB:CC:CC");
        break;

    case MSG_ADVERTISEMENT:
        msg.hdr.size += sizeof(Advertisement_t);
        msg.data.advertisement.on_off = (uint8_t)(unsigned long)data;
        break;

    case MSG_ADD_SERVICE:
        msg.hdr.size += sizeof(Add_Service_t);
        msg.data.add_service.desc = *((Ble::ServiceInfo_t*)data);
        break;

    case MSG_ADD_ATTRIBUTE:
        msg.hdr.size += sizeof(Add_Attribute_t);
        msg.data.add_attribute = *((Add_Attribute_t*)data);
        break;

    case MSG_UPDATE_ATTRIBUTE:
        msg.hdr.size += sizeof(Update_Attribute_t);
        msg.data.update_attribute = *((Update_Attribute_t*)data);
        break;

    case MSG_NOTIFY_CONNECT_STATUS:
    case MSG_NOTIFY_DATA_READ:
    case MSG_NOTIFY_DATA_WRITE:
    default:
        printf("WARNING! Wrong handler - Notification and commands are not processed here\n");
        break;
    }
}

int handle_response_message(Comm_Msg_t &msg)
{
    int ret = NO_ERROR;

    if (msg.hdr.error != NO_ERROR)
    {
        printf("ERROR in response: %d\n", msg.hdr.error);
        ret = msg.hdr.error;
    }
    else switch (msg.hdr.type)
    {
    // server responses
    case MSG_SESSION:
        giSessionId = msg.hdr.session_id;
        printf("giSessionId = %d %s\n", giSessionId, msg.data.session.on_off?"opened":"closed");
        break;

    case MSG_POWER:
    case MSG_CONFIG:
    case MSG_ADVERTISEMENT:
    case MSG_ADD_SERVICE:
    case MSG_ADD_ATTRIBUTE:
    case MSG_UPDATE_ATTRIBUTE:
        break;

    // server notifications
    case MSG_NOTIFY_CONNECT_STATUS:
        if (msg.data.notify_connect.on_off) // connected
        {
            // on connect
        }
        else
        {
            // on disconnect
        }
        break;

    case MSG_NOTIFY_DATA_READ:
        break;

    case MSG_NOTIFY_DATA_WRITE:
        break;
    }
    return ret;
}

int main(int argc, char** argv )
{
    Comm_Msg_t msg;
    int ret;

    if (Ble::Error::NONE != (ret = parse_command_line(argc, argv)))
    {
        die("invalid parameters", ret);
    }

    if (Ble::Error::NONE != (ret = init_comm(CLIENT)))
    {
        die("failed to init communication", ret);
    }

    for (uint32_t i = 0; i < sizeof(msg_list)/sizeof(SampleStruct_t); i++)
    {
        if (CMD_PAUSE_GETCHAR == msg_list[i].type)
        {
            printf("\nCMD_PAUSE_GETCHAR\\>");
            getchar();
        }
        else
        {
            format_message_payload(msg_list[i].type, msg, msg_list[i].data);

            if (Ble::Error::NONE != (ret = send_comm(TO_SERVER, &msg, msg.hdr.size)))
            {
                shut_comm(CLIENT);
                die("msg send to server failed", ret);
            }
            else
            {
                printf ("Message Sent to server, type %d %s, size = %d\n", msg.hdr.type, get_msg_name(&msg), msg.hdr.size);
                if (Ble::Error::NONE != (ret = recv_comm(FROM_SERVER, &msg)))
                {
                    shut_comm(CLIENT);
                    die("failed recv from server", ret);
                }
                printf ("got rsp: msg type %d, %s error =  %d\n", msg.hdr.type, get_msg_name(&msg), msg.hdr.error);
                handle_response_message(msg);
            }
        }
    }

    shut_comm(CLIENT);
    return 0;
}
