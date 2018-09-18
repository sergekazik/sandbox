#include "rmnp_api.h"

typedef enum
{
    CHR     = 0x00,
    DSC     = 0x01,
    R__     = 0x02,
    _W_     = 0x08,
    RW_     = 0x02 | 0x08,
    __N     = 0x10,
    R_N     = 0x02 | 0x10,
    _WN     = 0x08 | 0x10,
    RWN     = 0x02 | 0x08 | 0x10,
} Properties_t;

static void my_callback(int attr_idx, void* data, int size)
{
    (void)attr_idx, (void)data, (void)size;
}

int main()
{
    Rmnp_Error_t ret = NO_ERROR;
    uint8_t uuid[16] = {0x97,0x60,0xAB,0xBA,0xA2,0x34,0x46,0x86,0x9E,0x00,0xFC,0xBB,0xEE,0x33,0x73,0xF7};
    uint8_t attr_uuid[3][16] = {
        {0x97,0x60,0xAB,0xBA,0xA2,0x34,0x46,0x86,0x9E,0x20,0xD0,0x87,0x33,0x3C,0x2C,0x01},
        {0x97,0x60,0xAB,0xBA,0xA2,0x34,0x46,0x86,0x9E,0x20,0xD0,0x87,0x33,0x3C,0x2C,0x02},
        {0x00,0x00,0x29,0x02,0x00,0x00,0x10,0x00,0x80,0x00,0x00,0x80,0x5F,0x9B,0x34,0xFB}
    };

    if (NO_ERROR != (ret = rmnp_init()))
    {
        printf("failed to init, err = %d\n", ret);
    }
    else if (NO_ERROR != (ret = rmnp_config(0, "Sample")))
    {
        printf("failed to config, err = %d\n", ret);
    }
    else if (NO_ERROR != (ret = rmnp_register_callback(my_callback)))
    {
        printf("failed to register callback, err = %d\n", ret);
    }
    else if (NO_ERROR != (ret = rmnp_add_service(uuid,3)))
    {
        printf("failed to add service, err = %d\n", ret);
    }
    else if (NO_ERROR != (ret = rmnp_add_attribute(attr_uuid[0],"attr-1", 64, 6, CHR, RWN, "value1")))
    {
        printf("failed to add attr, err = %d\n", ret);
    }
    else if (NO_ERROR != (ret = rmnp_add_attribute(attr_uuid[1],"attr-2", 64, 6, CHR, RWN, "value2")))
    {
        printf("failed to add attr, err = %d\n", ret);
    }
    else if (NO_ERROR != (ret = rmnp_add_attribute(attr_uuid[2],"desc", 32, 2, DSC, RW_, "\x01\00")))
    {
        printf("failed to add attr, err = %d\n", ret);
    }
    else if (NO_ERROR != (ret = rmnp_start_advertisement()))
    {
        printf("failed to shutdown, err = %d\n", ret);
    }


    // do real things in callback now


    if (NO_ERROR != (ret = rmnp_stop_advertisement()))
    {
        printf("failed to shutdown, err = %d\n", ret);
    }
    else if (NO_ERROR != (ret = rmnp_shutdown()))
    {
        printf("failed to shutdown, err = %d\n", ret);
    }
    return ret;
}
