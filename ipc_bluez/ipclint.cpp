#include "icommon.h"

// static vars, status
static bool gbPower = false;
static bool gbSession = false;
static bool gbAdvertisement = false;
static uint8_t giSessionId = 0;

static Msg_Type_t msg_list[] = {
    MSG_SESSION,        // open
    MSG_POWER,          // on
    MSG_CONFIG,
    MSG_ADD_SERVICE,
    MSG_ADD_ATTRIBUTE,
    MSG_ADVERTISEMENT,  // start
    // ending session
    MSG_ADVERTISEMENT,  // stop
    MSG_POWER,          // off
    MSG_SESSION         // close
};

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

void format_message_payload(Msg_Type_t type, Comm_Msg_t &msg)
{
    msg.hdr.size = sizeof(Common_Header_t);
    msg.hdr.error = NO_ERROR;
    msg.hdr.session_id = giSessionId;
    msg.hdr.type = type;

    switch (type)
    {
    case MSG_SESSION:
        gbSession = !gbSession;
        msg.data.session.on_off = gbSession?1:0;
        msg.hdr.size += sizeof(Session_t);
        break;

    case MSG_POWER:
        gbPower = !gbPower;
        msg.data.power.on_off = gbPower?1:0;
        msg.hdr.size += sizeof(Power_t);
        break;

    case MSG_CONFIG:
        msg.data.config.device_class = 0x000430;
        strcpy(msg.data.config.device_name, "IpClint-24");
        strcpy(msg.data.config.mac_address, "AA:AA:BB:BB:CC:CC");
        msg.hdr.size += sizeof(Config_t);
        break;

    case MSG_ADVERTISEMENT:
        gbAdvertisement = !gbAdvertisement;
        msg.data.advertisement.on_off = gbAdvertisement?1:0;
        msg.hdr.size += sizeof(Advertisement_t);
        break;

    case MSG_ADD_SERVICE:
        msg.data.add_service.desc = service;
        msg.hdr.size += sizeof(Add_Service_t);
        break;

    case MSG_ADD_ATTRIBUTE:
        msg.hdr.size += sizeof(Add_Attribute_t);
        break;

    case MSG_UPDATE_ATTRIBUTE:
        msg.hdr.size += sizeof(Update_Attribute_t);
        break;

    case MSG_NOTIFY_CONNECT_STATUS:
    case MSG_NOTIFY_DATA_READ:
    case MSG_NOTIFY_DATA_WRITE:
        printf("WARNING! Wrong handler - Notification are sent from server!\n");
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
        printf("giSessionId = %d\n", giSessionId);
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
            // on connected
        }
        else
        {
            // on disconnect - reenable advertisement if needed
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

    for (uint32_t i = 0; i < sizeof(msg_list)/sizeof(Msg_Type_t); i++)
    {
        format_message_payload(msg_list[i], msg);

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

    shut_comm(CLIENT);
    return 0;
}
