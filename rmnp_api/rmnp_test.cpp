#include <unistd.h>
#include "rmnp_api.h"
#include <string>

//https://www.bluetooth.com/specifications/assigned-numbers/company-identifiers
//369 0x0171 Amazon Fulfillment Service
#define AMAZON_BLE_SERVICE  0x0171

static volatile bool done = false;
static void my_callback(Rmnp_Callback_Type_t type, int attr_idx, void* data, int size)
{
    printf("%s waked up with type = %d\n", __FUNCTION__, type);
    switch (type)
    {
    case Server_disconnected:
        printf("callback for Server_disconnected\n");
        done = true;
        break;
    case Server_connected:
        printf("callback for Server_connected\n");
        break;
    case Attr_data_read:
        printf("callback for Attr_data_read attr idx %d\n", attr_idx);
        break;
    case Attr_data_write:
    {
        std::string val((char*) data, size);
        printf("callback for Attr_data_write attr idx %d size %d val %s\n", attr_idx, size, val.c_str());
    }
        break;
    default:
        printf("unexpected type, ignored\n");
        break;
    }
}

int main()
{
    Rmnp_Error_t ret = NO_ERROR;
    Attr_Define_t attr_list[] = {
        {{0}, "attr-1", 0xFACC, 64, 6, CHAR, RWN, "value_1"},
        {{0}, "attr-1", 0xFACD, 64, 6, CHAR, RWN, "value_2"},
        {{0}, "attr-2", 0xFACE, 64, 6, CHAR, RWN, "value_3"},
        {{0}, "CCCD",   0x2902, 16, 2, DESC, RW_, "\x01\x01"}
    };

    if (NO_ERROR != (ret = rmnp_init()))
        printf("failed to init, err = %d\n", (int16_t) ret);
    else if (NO_ERROR != (ret = rmnp_config(0, "SimpleSample", "34:45:56:67:78:8B")))
        printf("failed to config, err = %d\n", (int16_t) ret);
    else if (NO_ERROR != (ret = rmnp_register_callback(my_callback)))
        printf("failed to register callback, err = %d\n", (int16_t) ret);
    else if (NO_ERROR != (ret = rmnp_add_service(AMAZON_BLE_SERVICE, sizeof(attr_list)/sizeof(Attr_Define_t))))
        printf("failed to add service, err = %d\n", (int16_t) ret);
    else if (NO_ERROR != (ret = rmnp_add_attribute(attr_list[0])))
        printf("failed to add attr1, err = %d\n", (int16_t) ret);
    else if (NO_ERROR != (ret = rmnp_add_attribute(attr_list[1])))
        printf("failed to add attr2, err = %d\n", (int16_t) ret);
    else if (NO_ERROR != (ret = rmnp_add_attribute(attr_list[2])))
        printf("failed to add attr3, err = %d\n", (int16_t) ret);
    else if (NO_ERROR != (ret = rmnp_add_attribute(attr_list[3])))
        printf("failed to add desc, err = %d\n", (int16_t) ret);
    else if (NO_ERROR != (ret = rmnp_start_advertisement()))
        printf("failed to shutdown, err = %d\n", (int16_t) ret);

    printf("waiting for callback events.....\n");
    while (!ret && !done) {
        sleep(3);
    }

    printf("continue operations - exiting...\n");
    if (NO_ERROR != (ret = rmnp_stop_advertisement()))
        printf("failed to shutdown, err = %d\n", (int16_t) ret);
    else if (NO_ERROR != (ret = rmnp_shutdown()))
        printf("failed to shutdown, err = %d\n", (int16_t) ret);
    return ret;
}
