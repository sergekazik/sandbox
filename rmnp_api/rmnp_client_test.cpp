#include "rmnp_api.h"

#define MAKE_UUID_128(_h, _g, _f, _d, _s, _a, _p, _o, _i, _u, _y, _t, _r, _e, _w, _q) \
    {0x##_h, 0x##_g, 0x##_f, 0x##_d, 0x##_s, 0x##_a, 0x##_p, 0x##_o, 0x##_i, 0x##_u, 0x##_y, 0x##_t, 0x##_r, 0x##_e, 0x##_w, 0x##_q}
#define SERVICE_UUID   MAKE_UUID_128(97,60,AB,BA,A2,34,46,86,9E,00,FC,BB,EE,33,73,F7)
#define MAKE_UUID(_attr_idx) MAKE_UUID_128(97,60,AB,BA,A2,34,46,86,9E,20,D0,87,33,3C,2C,_attr_idx)
#define CCCDESC_UUID()       MAKE_UUID_128(00,00,29,02,00,00,10,00,80,00,00,80,5F,9B,34,FB)

#define GATT_PROPERTY_READ          0x02
#define GATT_PROPERTY_WRITE         0x08
#define GATT_PROPERTY_NOTIFY        0x10
#define GATT_TYPE_CHARACTERISTIC    0x00
#define GATT_TYPE_DESCRIPTOR        0x01

enum Permission
{
    R__     = GATT_PROPERTY_READ,
    _W_     = GATT_PROPERTY_WRITE,
    RW_     = GATT_PROPERTY_READ | GATT_PROPERTY_WRITE,
    __N     = GATT_PROPERTY_NOTIFY,
    R_N     = GATT_PROPERTY_READ | GATT_PROPERTY_NOTIFY,
    _WN     = GATT_PROPERTY_WRITE | GATT_PROPERTY_NOTIFY,
    RWN     = GATT_PROPERTY_READ | GATT_PROPERTY_WRITE | GATT_PROPERTY_NOTIFY,
};

static void my_callback(int attr_idx, void* data, int size)
{
    (void)attr_idx, (void)data, (void)size;
}

int main()
{
    Rmnp_Error_t ret = NO_ERROR;
    uint8_t uuid[16] = SERVICE_UUID;
    uint8_t attr_uuid[3][16] = {MAKE_UUID(01), MAKE_UUID(02), CCCDESC_UUID()};

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
    else if (NO_ERROR != (ret = rmnp_add_attribute(attr_uuid[0],"attr-1", 64, 6, 0, RWN, "value1")))
    {
        printf("failed to add attr, err = %d\n", ret);
    }
    else if (NO_ERROR != (ret = rmnp_add_attribute(attr_uuid[1],"attr-2", 64, 6, 0, RWN, "value2")))
    {
        printf("failed to add attr, err = %d\n", ret);
    }
    else if (NO_ERROR != (ret = rmnp_add_attribute(attr_uuid[2],"desc", 32, 2, 1, RW_, "\x01\00")))
    {
        printf("failed to add attr, err = %d\n", ret);
    }





    return 0;
}
