#ifndef _GATT_SERVER_TEST_H_
#define _GATT_SERVER_TEST_H_

#ifdef __cplusplus
extern "C" {
#endif

#define NUMBER_OF_SERVICES_MAX                                     (sizeof(ServiceTable)/sizeof(ServiceInfo_t))

///
/// \brief gatt_server_start function for tests
/// \param char* arguments, NULL or "--autoinit"
/// \return errno
///
int gatt_server_start(const char* arguments);

#ifdef __cplusplus
}
#endif

#endif // _GATT_SERVER_TEST_H_

