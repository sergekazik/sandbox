#ifndef PB_COMMON
#define PB_COMMON

#define UNUSED(_var) (void)_var
#define UNUSED2(_var1, _var2) (void)_var1, (void)_var2

// static const char* test_addr = "192.168.1.3";
static const char* test_addr = "10.0.1.15";
#define MSG_LENGHT  0xFF

// #if defined(__x86_64__) && defined(PB_CLIENT)
#define SERVER_PORT     1100
#define CLIENT_PORT     1101

#endif // PB_COMMON

