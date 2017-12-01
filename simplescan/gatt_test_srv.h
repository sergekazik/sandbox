#ifndef _GATT_SERVER_TEST_H_
#define _GATT_SERVER_TEST_H_

#ifdef __cplusplus
extern "C" {
#endif

typedef enum
{
    eConfig_UP,
    eConfig_DOWN,
    eConfig_PISCAN,
    eConfig_NOSCAN,
    eConfig_LEADV,
    eConfig_NOLEADV,
    eConfig_CLASS,

    // ------- combos
    eConfig_ALLUP,
    eConfig_ALLDOWN,
} eConfig_cmd_t;

///
/// \brief gatt_server_start function for tests
/// \param char* arguments, NULL or "--autoinit"
/// \return errno
///
int gatt_server_start(const char* arguments);

///
/// \brief execute_hci_cmd
/// \param aCmd
/// \return
///
int execute_hci_cmd(eConfig_cmd_t aCmd);

#ifdef __cplusplus
}
#endif

#endif // _GATT_SERVER_TEST_H_

