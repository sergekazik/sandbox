#ifndef _RING_BLE_API_H_
#define _RING_BLE_API_H_

#include <stdio.h>
#include <stdlib.h>
#include <stdlib.h>
#include <errno.h>
#include <signal.h>
#include <unistd.h>

namespace Ring { namespace Ble {

#define MAX_SUPPORTED_COMMANDS                     (75)  /* Denotes the       */
                                                         /* maximum number of */
                                                         /* User Commands that*/
                                                         /* are supported by  */
                                                         /* this application. */

#define MAX_COMMAND_LENGTH                        (128)  /* Denotes the max   */
                                                         /* buffer size used  */
                                                         /* for user commands */
                                                         /* input via the     */
                                                         /* User Interface.   */

#define MAX_NUM_OF_PARAMETERS                      (10)  /* Denotes the max   */
                                                         /* number of         */
                                                         /* parameters a      */
                                                         /* command can have. */

#define DEFAULT_IO_CAPABILITY          (icDisplayYesNo)  /* Denotes the       */
                                                         /* default I/O       */
                                                         /* Capability that is*/
                                                         /* used with Secure  */
                                                         /* Simple Pairing.   */

#define DEFAULT_LE_IO_CAPABILITY      (licDisplayYesNo)  /* Denotes the       */
                                                         /* default I/O       */
                                                         /* Capability that is*/
                                                         /* used with LE      */
                                                         /* Pairing.          */

#define DEFAULT_LE_MITM_PROTECTION               (TRUE)  /* Denotes the       */
                                                         /* default value used*/
                                                         /* for Man in the    */
                                                         /* Middle (MITM)     */
                                                         /* protection used   */
                                                         /* with LE Pairing.  */

#define DEFAULT_LE_BONDING_TYPE	           (lbtBonding)  /* Denotes the       */
                                                         /* default value used*/
                                                         /* for Bonding Type. */

#define DEFAULT_SC_PROTECTION                    (TRUE)	 /* Denotes the       */
                                                         /* default value used*/
                                                         /* for LE pairing	  */
                                                         /* method SC or	  */
                                                         /* Legacy    		  */

#define DEFAULT_KEYPRESS      			         (TRUE)  /* Denotes the       */
                                                         /* default value used*/
                                                         /* for keypress	  */
                                                         /* notifications     */

#define DEFAULT_P256_DEBUG_ENABLE	            (FALSE)  /* Denotes the       */
                                                         /* default value used*/
                                                         /* for P256 debug	  */
                                                         /* mode.This is	  */
                                                         /* relevant only	  */
                                                         /* for LE SC 		  */

#define DEFAULT_MITM_PROTECTION                  (TRUE)  /* Denotes the       */
                                                         /* default value used*/
                                                         /* for Man in the    */
                                                         /* Middle (MITM)     */
                                                         /* protection used   */
                                                         /* with Secure Simple*/
                                                         /* Pairing.          */

#define INDENT_LENGTH                                3   /* Denotes the number*/
                                                         /* of character      */
                                                         /* spaces to be used */
                                                         /* for indenting when*/
                                                         /* displaying SDP    */
                                                         /* Data Elements.    */

#define DIRECT_CONNECTABLE_MODE                (0x0200)  /* Denotes that the  */
                                                         /* connectabillity   */
                                                         /* mode is Directed  */
                                                         /* Connectable.      */

#define LOW_DUTY_CYCLE_DIRECT_CONNECTABLE      (0x0400)  /* Denotes that the  */
                                                         /* connectabillity   */
                                                         /* mode is Low Duty  */
                                                         /* Cycle Directed    */
                                                         /* Connectable.      */

/* The following MACRO is used to convert an ASCII character into the*/
/* equivalent decimal value.  The MACRO converts lower case          */
/* characters to upper case before the conversion.                   */
#define ToInt(_x) (((_x) > 0x39)?(((_x) & ~0x20)-0x37):((_x)-0x30))

/* The following type definition represents the structure which holds*/
/* all information about the parameter, in particular the parameter  */
/* as a string and the parameter as an unsigned int.                 */
typedef struct _tagParameter_t
{
    char         *strParam;
    unsigned int  intParam;
} Parameter_t;

/* The following type definition represents the structure which holds*/
/* a list of parameters that are to be associated with a command The */
/* NumberofParameters variable holds the value of the number of      */
/* parameters in the list.                                           */
typedef struct _tagParameterList_t
{
    int         NumberofParameters;
    Parameter_t Params[MAX_NUM_OF_PARAMETERS];
} ParameterList_t;

/* The following type definition represents the structure which holds*/
/* the command and parameters to be executed.                        */
typedef struct _tagUserCommand_t
{
    char            *Command;
    ParameterList_t  Parameters;
} UserCommand_t;

/* The following type definition represents the generic function     */
/* pointer to be used by all commands that can be executed by the    */
/* test program.                                                     */
typedef int (*CommandFunction_t)(ParameterList_t *aParams __attribute__ ((unused)));

/* The following type definition represents the structure which holds*/
/* information used in the interpretation and execution of Commands. */
typedef struct _tagCommandTable_t
{
    char              *CommandName;
    CommandFunction_t  CommandFunction;
} CommandTable_t;

class BleApi
{
public:
    enum Error {
        NO_ERROR                                  =  0,
        PARSER_ERROR                              = -1, // Denotes that no command was specified to the parser.
        INVALID_COMMAND_ERROR                     = -2, // Denotes that the Command does not exist for processing.
        EXIT_CODE                                 = -3, // Denotes that the Command specified was the Exit Command.
        FUNCTION_ERROR                            = -4, // Denotes that an error occurred in execution of the Command Function.
        TO_MANY_PARAMS                            = -5, // Denotes that there are more parameters then will fit in the UserCommand.
        INVALID_PARAMETERS_ERROR                  = -6, // Denotes that an error occurred due to the fact that one or more of the required parameters were invalid.
        NOT_INITIALIZED_ERROR                     = -7, // Denotes that an error occurred due to the fact that the Platform Manager has not been initialized.
    };

    BleApi();
    static BleApi* getInstance();

    ~BleApi();

protected:
    static BleApi* instance;
    bool          mInitialized;      // initialization state

public:
    // configuration
    virtual int Initialize(ParameterList_t *aParams __attribute__ ((unused))) = 0;
    virtual int SetDevicePower(ParameterList_t *aParams __attribute__ ((unused))) = 0;
    virtual int QueryDevicePower(ParameterList_t *aParams __attribute__ ((unused))) = 0;
    virtual int SetLocalDeviceName(ParameterList_t *aParams __attribute__ ((unused))) = 0;
    virtual int SetLocalClassOfDevice(ParameterList_t *aParams __attribute__ ((unused))) = 0;
    virtual int SetDiscoverable(ParameterList_t *aParams __attribute__ ((unused))) = 0;
    virtual int SetConnectable(ParameterList_t *aParams __attribute__ ((unused))) = 0;
    virtual int SetPairable(ParameterList_t *aParams __attribute__ ((unused))) = 0;
    virtual int ShutdownService(ParameterList_t *aParams __attribute__ ((unused))) = 0;
    virtual int Cleanup(ParameterList_t *aParams __attribute__ ((unused))) = 0;

    virtual int RegisterEventCallback(ParameterList_t *aParams __attribute__ ((unused))) = 0;
    virtual int UnRegisterEventCallback(ParameterList_t *aParams __attribute__ ((unused))) = 0;

    virtual int SetLocalRemoteDebugZoneMask(ParameterList_t *aParams __attribute__ ((unused))) = 0;
    virtual int QueryLocalRemoteDebugZoneMask(ParameterList_t *aParams __attribute__ ((unused))) = 0;
    virtual int SetDebugZoneMaskPID(ParameterList_t *aParams __attribute__ ((unused))) = 0;
    virtual int QueryLocalDeviceProperties(ParameterList_t *aParams __attribute__ ((unused))) = 0;
    virtual int SetLocalDeviceAppearance(ParameterList_t *aParams __attribute__ ((unused))) = 0;

    // discovery
    virtual int StartDeviceDiscovery(ParameterList_t *aParams __attribute__ ((unused))) = 0;
    virtual int StopDeviceDiscovery(ParameterList_t *aParams __attribute__ ((unused))) = 0;

    // remote device operations
    virtual int QueryRemoteDeviceList(ParameterList_t *aParams __attribute__ ((unused))) = 0;
    virtual int QueryRemoteDeviceProperties(ParameterList_t *aParams __attribute__ ((unused))) = 0;
    virtual int AddRemoteDevice(ParameterList_t *aParams __attribute__ ((unused))) = 0;
    virtual int DeleteRemoteDevice(ParameterList_t *aParams __attribute__ ((unused))) = 0;
    virtual int UpdateRemoteDeviceApplicationData(ParameterList_t *aParams __attribute__ ((unused))) = 0;
    virtual int DeleteRemoteDevices(ParameterList_t *aParams __attribute__ ((unused))) = 0;
    virtual int PairWithRemoteDevice(ParameterList_t *aParams __attribute__ ((unused))) = 0;
    virtual int CancelPairWithRemoteDevice(ParameterList_t *aParams __attribute__ ((unused))) = 0;
    virtual int UnPairRemoteDevice(ParameterList_t *aParams __attribute__ ((unused))) = 0;
    virtual int QueryRemoteDeviceServices(ParameterList_t *aParams __attribute__ ((unused))) = 0;
    virtual int QueryRemoteDeviceServiceSupported(ParameterList_t *aParams __attribute__ ((unused))) = 0;
    virtual int QueryRemoteDevicesForService(ParameterList_t *aParams __attribute__ ((unused))) = 0;
    virtual int QueryRemoteDeviceServiceClasses(ParameterList_t *aParams __attribute__ ((unused))) = 0;

    // connection and security
    virtual int EnableSCOnly(ParameterList_t *aParams __attribute__ ((unused))) = 0;
    virtual int RegenerateP256LocalKeys(ParameterList_t *aParams __attribute__ ((unused))) = 0;
    virtual int OOBGenerateParameters(ParameterList_t *aParams __attribute__ ((unused))) = 0;
    virtual int AuthenticateRemoteDevice(ParameterList_t *aParams __attribute__ ((unused))) = 0;
    virtual int ConnectWithRemoteDevice(ParameterList_t *aParams __attribute__ ((unused))) = 0;
    virtual int DisconnectRemoteDevice(ParameterList_t *aParams __attribute__ ((unused))) = 0;
    virtual int EncryptRemoteDevice(ParameterList_t *aParams __attribute__ ((unused))) = 0;
    virtual int SetRemoteDeviceLinkActive(ParameterList_t *aParams __attribute__ ((unused))) = 0;

    // session description protocol
    virtual int CreateSDPRecord(ParameterList_t *aParams __attribute__ ((unused))) = 0;
    virtual int DeleteSDPRecord(ParameterList_t *aParams __attribute__ ((unused))) = 0;
    virtual int AddSDPAttribute(ParameterList_t *aParams __attribute__ ((unused))) = 0;
    virtual int DeleteSDPAttribute(ParameterList_t *aParams __attribute__ ((unused))) = 0;

    virtual int RegisterAuthentication(ParameterList_t *aParams __attribute__ ((unused))) = 0;
    virtual int UnRegisterAuthentication(ParameterList_t *aParams __attribute__ ((unused))) = 0;

    virtual int PINCodeResponse(ParameterList_t *aParams __attribute__ ((unused))) = 0;
    virtual int PassKeyResponse(ParameterList_t *aParams __attribute__ ((unused))) = 0;
    virtual int UserConfirmationResponse(ParameterList_t *aParams __attribute__ ((unused))) = 0;
    virtual int ChangeSimplePairingParameters(ParameterList_t *aParams __attribute__ ((unused))) = 0;
    virtual int ChangeLEPairingParameters(ParameterList_t *aParams __attribute__ ((unused))) = 0;

    // GATT
    virtual int RegisterGATMEventCallback(ParameterList_t *aParams __attribute__ ((unused))) = 0;
    virtual int UnRegisterGATMEventCallback(ParameterList_t *aParams __attribute__ ((unused))) = 0;
    virtual int GATTQueryConnectedDevices(ParameterList_t *aParams __attribute__ ((unused))) = 0;
    virtual int GATTRegisterService(ParameterList_t *aParams __attribute__ ((unused))) = 0;
    virtual int GATTUnRegisterService(ParameterList_t *aParams __attribute__ ((unused))) = 0;
    virtual int GATTIndicateCharacteristic(ParameterList_t *aParams __attribute__ ((unused))) = 0;
    virtual int GATTNotifyCharacteristic(ParameterList_t *aParams __attribute__ ((unused))) = 0;
    virtual int ListCharacteristics(ParameterList_t *aParams __attribute__ ((unused))) = 0;
    virtual int ListDescriptors(ParameterList_t *aParams __attribute__ ((unused))) = 0;
    virtual int GATTQueryPublishedServices(ParameterList_t *aParams __attribute__ ((unused))) = 0;

    // Advertising
    virtual int SetAdvertisingInterval(ParameterList_t *aParams __attribute__ ((unused))) = 0;
    virtual int StartAdvertising(ParameterList_t *aParams __attribute__ ((unused))) = 0;
    virtual int StopAdvertising(ParameterList_t *aParams __attribute__ ((unused))) = 0;

    virtual int SetAuthenticatedPayloadTimeout(ParameterList_t *aParams __attribute__ ((unused))) = 0;
    virtual int QueryAuthenticatedPayloadTimeout(ParameterList_t *aParams __attribute__ ((unused))) = 0;
    virtual int SetAndUpdateConnectionAndScanBLEParameters(ParameterList_t *aParams __attribute__ ((unused))) = 0;

    // debug
    virtual int EnableBluetoothDebug(ParameterList_t *aParams __attribute__ ((unused))) = 0;
};

} } /* namespace Ring::Ble */


#endif // _RING_BLE_API_H_
