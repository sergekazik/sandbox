#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

typedef enum
{
    NO_ERROR,
    NOT_INITIALIZED,
    REQUEST_RESPONSE_MISMATCH,
    INVALID_PARAMETER,
    PTHREAD_CREATE_FAILED,
    SOCKET_CREATE_FAILED,
    SOCKET_BIND_FAILED,
    SOCKET_SET_TIMEOUT_FAILED,
    SOCKET_SEND_FAILED,
    SOCKET_RECV_FAILED,
    SOCKET_RECV_TIMEOUT,
    OPEN_SESSION_FAILED,
    POWER_COMMAND_FAILED,
    SERVER_ERROR,
} Rmnp_Error_t;

typedef enum
{
    CHAR     = 0x00,
    DESC     = 0x01,
    R__     = 0x02,
    _W_     = 0x08,
    __N     = 0x10,
    RW_     = R__ | _W_,
    R_N     = R__ | __N,
    _WN     = _W_ | __N,
    RWN     = R__ | _W_ | __N,
} Characteristic_Properties_t;

typedef uint8_t uint128_t[16];

///
/// \brief rmnp_init
/// \param tm_ms
/// \param port
/// \param ip
/// \return
///
Rmnp_Error_t rmnp_init(int tm_ms = 0, int port = 0, const char* ip = NULL);

///
/// \brief rmnp_shutdown
/// \return
///
Rmnp_Error_t rmnp_shutdown();

///
/// \brief rmnp_config
/// \param dev_class
/// \param name
/// \param mac
/// \return
///
Rmnp_Error_t rmnp_config(uint32_t dev_class = 0, const char *name = NULL, const char *mac = NULL);

enum Rmnp_Callback_Type_t
{
    Server_disconnected = 0,
    Server_connected    = 1,
    Attr_data_read      = 2,
    Attr_data_write     = 3,
};

typedef void (*data_access_cb) (Rmnp_Callback_Type_t type, int attr_idx, void* data, int size);
///
/// \brief rmnp_register_callback
/// \param cb
/// \return
///
Rmnp_Error_t rmnp_register_callback(data_access_cb cb = NULL);

///
/// \brief rmnp_add_service
/// \param uuid
/// \param number_of_attr
/// \return
///
Rmnp_Error_t rmnp_add_service(uint16_t uuid, uint8_t number_of_attr);

///
/// \brief rmnp_add_service
/// \param uuid
/// \param number_of_attr
/// \return
///
Rmnp_Error_t rmnp_add_service(uint128_t uuid128, uint8_t number_of_attr);

///
/// \brief rmnp_add_attribute
/// \param uuid
/// \param name
/// \param max_len
/// \param size
/// \param type
/// \param prop
/// \param value
/// \return
///
Rmnp_Error_t rmnp_add_attribute(uint128_t uuid128, uint16_t uuid, const char* name, int max_len, int size, uint8_t type, uint8_t prop, const void* value = NULL);

typedef struct Attr_Define
{
    uint128_t uuid128;
    char* name;
    uint16_t uuid;
    int max_len;
    int size;
    uint8_t type;
    uint8_t prop;
    const void* value;
} Attr_Define_t;

///
/// \brief rmnp_add_attribute
/// \param attr
/// \return
///
Rmnp_Error_t rmnp_add_attribute(Attr_Define_t &attr);

///
/// \brief rmnp_update_attribute
/// \param attr_idx
/// \param size
/// \param value
/// \return
///
Rmnp_Error_t rmnp_update_attribute(int attr_idx, int size, const void *value = NULL);

///
/// \brief rmnp_start_advertisement
/// \return
///
Rmnp_Error_t rmnp_start_advertisement();

///
/// \brief rmnp_stop_advertisement
/// \return
///
Rmnp_Error_t rmnp_stop_advertisement();




