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
    SOCKET_CREATE_FAILED,
    SOCKET_BIND_FAILED,
    SOCKET_SET_TIMEOUT_FAILED,
    SOCKET_SEND_FAILED,
    SOCKET_RECV_FAILED,
    SOCKET_RECV_TIMEOUT,
    OPEN_SESSION_FAILED,
    POWER_COMMAND_FAILED,
} Rmnp_Error_t;

Rmnp_Error_t rmnp_init(int tm_ms = 0, int port = 0, const char* ip = NULL);
Rmnp_Error_t rmnp_shutdown();

Rmnp_Error_t rmnp_config(uint32_t dev_class = 0, char *name = NULL, char *mac = NULL);

typedef void (*data_access_cb) (int attr_idx, void* data, int size);
Rmnp_Error_t rmnp_register_callback(data_access_cb cb = NULL);

Rmnp_Error_t rmnp_add_service(uint8_t *uuid, uint32_t number_of_attr);
Rmnp_Error_t rmnp_add_attribute(uint8_t *uuid, char* name, int max_len, int size, uint8_t type, uint8_t prop, void* value = NULL);
Rmnp_Error_t rmnp_update_attribute(int attr_idx, int size, void *value);

Rmnp_Error_t rmnp_start_advertisement();
Rmnp_Error_t rmnp_stop_advertisement();




