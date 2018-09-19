#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

typedef enum _rmnp_error
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
} Rmnp_Error_t;

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
Rmnp_Error_t rmnp_config(uint32_t dev_class = 0, char *name = NULL, char *mac = NULL);

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
Rmnp_Error_t rmnp_add_service(uint8_t *uuid, uint32_t number_of_attr);

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
Rmnp_Error_t rmnp_add_attribute(uint8_t *uuid, char* name, int max_len, int size, uint8_t type, uint8_t prop, const void* value = NULL);

typedef struct Attr_Define
{
    uint8_t uuid[16];
    char* name;
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




