/*---------------------------------------------------------------------------
 *                     ------------------      -----------------            *
 *                     | Ring App Setup |      |   test_ble    |            *
 *                     ------------------      -----------------            *
 *                                |                |        |               *
 *                                |   --------------------  |               *
 *                                |   |gatt_src_test.cpp |  |               *
 *                                |   --------------------  |               *
 *                                |          |              |               *
 *                  --------------------------------        |               *
 *                  |   RingBleApi.cpp abstract    |        |               *
 *                  --------------------------------        |               *
 *                  | RingGattSrv.cpp | ?(BCM).cpp |        |               *
 *                  --------------------------------        |               *
 *                        |                                 |               *
 *       ------------------------                    ---------------        *
 *       |      TIBT lib        |                    | hcitools.c  |        *
 *       ------------------------                    ---------------        *
 *                  |                                       |               *
 *       ------------------------                    ---------------        *
 *       | TI WiLink18xx BlueTP |                    |   BlueZ     |        *
 *       ------------------------                    ---------------        *
 *--------------------------------------------------------------------------*/
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <signal.h>
#include <unistd.h>

#include "ipc/Bot_Notifier.h"
#include "RingGattApi.hh"

using namespace Ring::Ble;

GattSrv* GattSrv::instance = NULL;

const char * GattSrv::IOCapabilitiesStrings[] =
{
   "Display Only",
   "Display Yes/No",
   "Keyboard Only",
   "No Input/Output",
   "Keyboard/Display"
};
const int GattSrv::IOCAPABILITIESSTRINGS_SIZE = sizeof(GattSrv::IOCapabilitiesStrings)/sizeof(char *);

BleApi* GattSrv::getInstance()
{
    if (instance == NULL)
        instance = new GattSrv();
    return (BleApi*) instance;
}

GattSrv::GattSrv()
{
    mServiceCount = 0;          // the current number of services passed to register
    mServiceTable = NULL;       // pointer to populated service tbl
    mServiceMutex = NULL;       // Mutex which guards access to theService List.
    mPrepareWriteList = NULL;  // Pointer to head of list containing all currently pendingprepared writes.

    /* Initialize the default Secure Simple Pairing parameters.          */
    mIOCapability    = DEFAULT_IO_CAPABILITY;
    mLEIOCapability  = DEFAULT_LE_IO_CAPABILITY;
    mOOBSupport      = FALSE;
    mKeypress	     = DEFAULT_KEYPRESS;
    mMITMProtection  = DEFAULT_MITM_PROTECTION;
    mSC              = DEFAULT_SC_PROTECTION;
    mP256DebugMode   = DEFAULT_P256_DEBUG_ENABLE;
    mBondingType     = DEFAULT_LE_BONDING_TYPE;
}

/* BTPM Local Device Manager Callback function prototype.            */
static void BTPSAPI DEVM_Event_Callback(DEVM_Event_Data_t *EventData, void *CallbackParameter __attribute__ ((unused)));
/* BTPM Local Device Manager Authentication Callback function prototype  */
static void BTPSAPI DEVM_Authentication_Callback(DEVM_Authentication_Information_t *AuthenticationRequestInformation, void *CallbackParameter __attribute__ ((unused)));
/* GATM Manager Callback Function prototype.                         */
static void BTPSAPI GATM_Event_Callback(GATM_Event_Data_t *EventData, void *CallbackParameter __attribute__ ((unused)));

/* Initialization.                 */

/* The following function is responsible for Initializing the        */
/* Bluetopia Platform Manager Framework.  This function returns      */
/* zero if successful and a negative value if an error occurred.     */
int GattSrv::Initialize(ParameterList_t *aParams __attribute__ ((unused)))
{
    int Result;
    int ret_val = UNDEFINED_ERROR;

    /* First, check to make sure that we are not Already Initialized.    */
    if (!mInitialized)
    {
        BOT_NOTIFY_DEBUG("GattSrv::Initialize, %d params\n", aParams->NumberofParameters);
        for (int i = 0; i < aParams->NumberofParameters; i++)
        {
            BOT_NOTIFY_TRACE("aParams[%d] = {0x%08X, 0x%08X [%s]}\n", i, aParams->Params[i].intParam, (unsigned long) aParams->Params[i].strParam, aParams->Params[i].strParam);
        }

        /* Determine if the user would like to Register an Event Callback */
        /* with the calling of this command.                              */
        if ((aParams) && (aParams->NumberofParameters >= 1))
        {
            /* Now actually initialize the Platform Manager Service.       */
            Result = BTPM_Initialize((unsigned long)getpid(), NULL, ServerUnRegistrationCallback, NULL);

            if (!Result)
            {
                /* Initialization successful, go ahead and inform the user  */
                /* that it was successful and flag that the Platform Manager*/
                /* has been Initialized.                                    */
                BOT_NOTIFY_INFO("BTPM_Initialize() Success: %d.\r\n", Result);

                mInitialized = TRUE;
                ret_val = NO_ERROR;

                if (!mServiceMutex)
                {
                    /* Create the mutex which will guard access to the service list.     */
                    mServiceMutex     = BTPS_CreateMutex(FALSE);
                }

                /* If the caller would like to Register an Event Callback   */
                /* then we will do that at this time.                       */
                if (aParams->Params[0].intParam)
                {
                    if ((Result = DEVM_RegisterEventCallback(DEVM_Event_Callback, NULL)) > 0)
                    {
                        BOT_NOTIFY_INFO("DEVM_RegisterEventCallback() Success: %d.\r\n", Result);
                        /* Note the Callback ID and flag success.             */
                        mDEVMCallbackID = (unsigned int)Result;
                    }
                    else
                    {
                        /* Error registering the Callback, inform user and    */
                        /* flag an error.                                     */
                        BOT_NOTIFY_ERROR("DEVM_RegisterEventCallback() Failure: %d, %s.\r\n", Result, ERR_ConvertErrorCodeToString(Result));

                        mInitialized = FALSE;
                        ret_val = FUNCTION_ERROR;
                        /* Since there was an error, go ahead and clean up the*/
                        /* library.                                           */
                        BTPM_Cleanup();
                    }
                }

                /* If the caller want to initialize Service Table *
                 * Params[1] = number of services                   *
                 * Params[2] = pointer to the Service Table         */
                if ((ret_val == NO_ERROR) && (aParams->NumberofParameters > 1) && (aParams->Params[1].strParam != NULL))
                {
                    mServiceCount = aParams->Params[1].intParam;
                    mServiceTable = (ServiceInfo_t *) aParams->Params[1].strParam;
                }
                else
                {
                    BOT_NOTIFY_ERROR("Failed to initialize Service Table, ret=%d\r\n", ret_val);
                }
            }
            else
            {
                /* Error initializing Platform Manager, inform the user.    */
                BOT_NOTIFY_ERROR("BTPM_Initialize() Failure: %d, %s.\r\n", Result, ERR_ConvertErrorCodeToString(Result));
                ret_val = FUNCTION_ERROR;
            }

        }
        else
        {
            /* One or more of the necessary parameters is/are invalid.     */
            BOT_NOTIFY_ERROR("Usage: Initialize [0/1 - Register for Events].\r\n");
            ret_val = INVALID_PARAMETERS_ERROR;
        }
    }
    else
    {
        /* Already Initialized, flag an error.                            */
        BOT_NOTIFY_ERROR("Initialization Failure: Already Initialized.\r\n");

        ret_val = NO_ERROR;
    }

    return ret_val;
}

/* The following function is responsible for Cleaning up/Shutting    */
/* down the Bluetopia Platform Manager Framework.  This function     */
/* returns zero if successful and a negative value if an error       */
/* occurred.                                                         */
int GattSrv::Cleanup(ParameterList_t *aParams __attribute__ ((unused)) __attribute__ ((unused)) )
{
    int ret_val = UNDEFINED_ERROR;

    /* First, check to make sure that we have already been Initialized.  */
    if (mInitialized)
    {
        /* If there was an Event Callback Registered, then we need to     */
        /* un-register it.                                                */
        if (mDEVMCallbackID)
            DEVM_UnRegisterEventCallback(mDEVMCallbackID);

        if (mGATMCallbackID)
            GATM_UnRegisterEventCallback(mGATMCallbackID);

        /* Nothing to do other than to clean up the Bluetopia Platform    */
        /* Manager Service and flag that it is no longer Initialized.     */
        BTPM_Cleanup();

        mInitialized    = FALSE;
        mDEVMCallbackID = 0;
        mServiceCount = 0;
        mServiceTable = NULL;

        /* Cleanup the service list.                                         */
        CleanupServiceList();

        /* Free the Prepare Write List.                                      */
        FreePrepareWriteEntryList(&mPrepareWriteList);
        mPrepareWriteList = NULL;

        /* Wait on the mutex since we are going to free it later.            */
        BTPS_WaitMutex(mServiceMutex, BTPS_INFINITE_WAIT);

        /* Close the Service List Mutex.                                     */
        BTPS_CloseMutex(mServiceMutex);

        mServiceMutex = NULL;

        ret_val = NO_ERROR;
    }
    else
    {
        /* Not Initialized, flag an error.                                */
        BOT_NOTIFY_WARNING("Platform Manager has not been Initialized.\r\n");

        // setting NO_ERROR since it doesn't really affect anything
        // ret_val = NOT_INITIALIZED_ERROR;
        ret_val = NO_ERROR;
    }

    return ret_val;
}

/* The following function is responsible for Registering a Local     */
/* Device Manager Callback with the Bluetopia Platform Manager       */
/* Framework.  This function returns zero if successful and a        */
/* negative value if an error occurred.                              */
int GattSrv::RegisterEventCallback(ParameterList_t *aParams __attribute__ ((unused)))
{
    int Result;
    int ret_val = UNDEFINED_ERROR;

    /* First, check to make sure that we have already been Initialized.  */
    if (mInitialized)
    {
        /* If there is an Event Callback Registered, then we need to flag */
        /* an error.                                                      */
        if (!mDEVMCallbackID)
        {
            /* Callback has not been registered, go ahead and attempt to   */
            /* register it.                                                */
            if ((Result = DEVM_RegisterEventCallback(DEVM_Event_Callback, NULL)) > 0)
            {
                BOT_NOTIFY_INFO("DEVM_RegisterEventCallback() Success: %d.\r\n", Result);

                /* Note the Callback ID and flag success.                   */
                mDEVMCallbackID = (unsigned int)Result;
                ret_val = NO_ERROR;
            }
            else
            {
                /* Error registering the Callback, inform user and flag an  */
                /* error.                                                   */
                BOT_NOTIFY_ERROR("DEVM_RegisterEventCallback() Failure: %d, %s.\r\n", Result, ERR_ConvertErrorCodeToString(Result));
                ret_val = FUNCTION_ERROR;
            }
        }
        else
        {
            /* Callback already registered, go ahead and notify the user.  */
            BOT_NOTIFY_WARNING("Device Manager Event Callback already registered.\r\n");

            // setting NO_ERROR since it doesn't really do anything bad
            // ret_val = FUNCTION_ERROR;
            ret_val = NO_ERROR;
        }
    }
    else
    {
        /* Not Initialized, flag an error.                                */
        BOT_NOTIFY_ERROR("Platform Manager has not been Initialized.\r\n");
        ret_val = NOT_INITIALIZED_ERROR;
    }

    return ret_val;
}

/* The following function is responsible for Un-Registering a Local  */
/* Device Manager Callback that has previously been registered with  */
/* the Bluetopia Platform Manager Framework.  This function returns  */
/* zero if successful and a negative value if an error occurred.     */
int GattSrv::UnRegisterEventCallback(ParameterList_t *aParams __attribute__ ((unused)))
{
    int ret_val = UNDEFINED_ERROR;

    /* First, check to make sure that we have already been Initialized.  */
    if (mInitialized)
    {
        /* Next, check to make sure that there is an Event Callback       */
        /* already registered.                                            */
        if (mDEVMCallbackID)
        {
            /* Callback has been registered, go ahead and attempt to       */
            /* un-register it.                                             */
            DEVM_UnRegisterEventCallback(mDEVMCallbackID);

            BOT_NOTIFY_INFO("DEVM_UnRegisterEventCallback() Success.\r\n");

            /* Flag that there is no longer a Callback registered.         */
            mDEVMCallbackID = 0;

            ret_val = NO_ERROR;
        }
        else
        {
            /* Callback already registered, go ahead and notify the user.  */
            BOT_NOTIFY_WARNING("Device Manager Event Callback is not registered.\r\n");
            ret_val = NO_ERROR; // FUNCTION_ERROR; - no harm
        }
    }
    else
    {
        /* Not Initialized, flag an error.                                */
        BOT_NOTIFY_WARNING("Platform Manager has not been Initialized.\r\n");

        // setting NO_ERROR since it doesn't really do anything bad
        // ret_val = NOT_INITIALIZED_ERROR;
        ret_val = NO_ERROR;
    }

    return ret_val;
}

/* The following function is responsible for Setting the Device Power*/
/* of the Local Device.  This function returns zero if successful and*/
/* a negative value if an error occurred.                            */
int GattSrv::SetDevicePower(ParameterList_t *aParams __attribute__ ((unused)))
{
    int Result;
    int ret_val = UNDEFINED_ERROR;

    /* First, check to make sure that we are not Already Initialized.    */
    if (mInitialized)
    {
        /* Determine if the user would like to Power On or Off the Local  */
        /* Device with the calling of this command.                       */
        if ((aParams) && (aParams->NumberofParameters >= 1))
        {
            /* Now actually Perform the command.                           */
            if (aParams->Params[0].intParam)
            {
                Result = DEVM_QueryDevicePowerState();
                if (Result > 0) // On
                {
                    BOT_NOTIFY_INFO("DEVM_QueryDevicePowerState(): Local Device is already Powered Up\r\n");
                    Result = NO_ERROR;
                }
                else
                {
                    Result = DEVM_PowerOnDevice();
                }
            }
            else
            {
                Result = DEVM_PowerOffDevice();
            }

            if (Result == NO_ERROR)
            {
                /* Device Power request was successful, go ahead and inform the User. */
                BOT_NOTIFY_INFO("DEVM_Power%sDevice() Success: %d.\r\n", aParams->Params[0].intParam?"On":"Off", Result);

                /* Return success to the caller.                            */
                ret_val = NO_ERROR;
            }
            else
            {
                /* Error Powering On/Off the device, inform the user.       */
                BOT_NOTIFY_ERROR("DEVM_Power%sDevice() Failure: %d, %s.\r\n", aParams->Params[0].intParam?"On":"Off", Result, ERR_ConvertErrorCodeToString(Result));
                ret_val = FUNCTION_ERROR;
            }
        }
        else
        {
            BOT_NOTIFY_ERROR("Usage: SetDevicePower [0/1 - Power Off/Power On].\r\n");
            ret_val = INVALID_PARAMETERS_ERROR;
        }
    }
    else
    {
        /* Not Initialized, flag an error.                                */
        BOT_NOTIFY_ERROR("Platform Manager has not been Initialized.\r\n");
        ret_val = NOT_INITIALIZED_ERROR;
    }

    return ret_val;
}

/* The following function is responsible for querying the current    */
/* device power for the local device.  This function returns zero if */
/* successful and a negative value if an error occurred.             */
int GattSrv::QueryDevicePower(ParameterList_t *aParams __attribute__ ((unused)))
{
    int ret_val = UNDEFINED_ERROR;
    int Result;

    /* First, check to make sure that we have already been Initialized.  */
    if (mInitialized)
    {
        /* Framework has been Initialized, go ahead and query the current */
        /* Power state.                                                   */
        Result = DEVM_QueryDevicePowerState();

        if (Result >= 0)
            BOT_NOTIFY_INFO("DEVM_QueryDevicePowerState() Success: %s.\r\n", Result?"On":"Off");
        else
            BOT_NOTIFY_ERROR("DEVM_QueryDevicePowerState() Failure: %d, %s.\r\n", Result, ERR_ConvertErrorCodeToString(Result));

        /* Flag success to the caller.                                    */
        ret_val = NO_ERROR;
    }
    else
    {
        /* Not Initialized, flag an error.                                */
        BOT_NOTIFY_ERROR("Platform Manager has not been Initialized.\r\n");
        ret_val = NOT_INITIALIZED_ERROR;
    }

    return ret_val;
}

/* The following function is responsible for Setting the Debug Zone  */
/* Mask (either Local or Remote).  This function returns zero if     */
/* successful and a negative value if an error occurred.             */
int GattSrv::SetLocalRemoteDebugZoneMask(ParameterList_t *aParams __attribute__ ((unused)))
{
    int Result;
    int ret_val = UNDEFINED_ERROR;

    /* First, check to make sure that we are not Already Initialized.    */
    if (mInitialized)
    {
        /* Determine if the user would like to set the Local Library Debug*/
        /* Mask or the Remote Services Debug Mask.                        */
        if ((aParams) && (aParams->NumberofParameters >= 2))
        {
            /* Now actually Perform the command.                           */
            Result = BTPM_SetDebugZoneMask((Boolean_t)((aParams->Params[0].intParam)?TRUE:FALSE), (unsigned long)aParams->Params[1].intParam);

            if (!Result)
            {
                /* Set Debug Zone Mask request was successful, go ahead and */
                /* inform the User.                                         */
                BOT_NOTIFY_INFO("BTPM_SetDebugZoneMask(%s) Success: 0x%08lX.\r\n", aParams->Params[0].intParam?"Remote":"Local", (unsigned long)aParams->Params[1].intParam);

                /* Return success to the caller.                            */
                ret_val = NO_ERROR;
            }
            else
            {
                /* Error Querying Debug Zone Mask, inform the user.         */
                BOT_NOTIFY_ERROR("BTPM_SetDebugZoneMask(%s) Failure: %d, %s.\r\n", aParams->Params[0].intParam?"Remote":"Local", Result, ERR_ConvertErrorCodeToString(Result));
                ret_val = FUNCTION_ERROR;
            }
        }
        else
        {
            BOT_NOTIFY_ERROR("Usage: SetDebugZoneMask [0/1 - Local/Service] [Debug Zone Mask].\r\n");
            ret_val = INVALID_PARAMETERS_ERROR;
        }
    }
    else
    {
        /* Not Initialized, flag an error.                                */
        BOT_NOTIFY_ERROR("Platform Manager has not been Initialized.\r\n");
        ret_val = NOT_INITIALIZED_ERROR;
    }

    return ret_val;
}

/* The following function is responsible for Querying the current    */
/* Debug Zone Mask (either Local or Remote).  This function returns  */
/* zero if successful and a negative value if an error occurred.     */
int GattSrv::QueryLocalRemoteDebugZoneMask(ParameterList_t *aParams __attribute__ ((unused)))
{
    int           Result;
    int           ret_val = UNDEFINED_ERROR;
    unsigned long DebugZoneMask;

    /* First, check to make sure that we are not Already Initialized.    */
    if (mInitialized)
    {
        /* Determine if the user would like to query the Local Library    */
        /* Debug Mask or the Remote Services Debug Mask.                  */
        if ((aParams) && (aParams->NumberofParameters >= 1))
        {
            /* Now actually Perform the command.                           */
            Result = BTPM_QueryDebugZoneMask((Boolean_t)((aParams->Params[0].intParam)?TRUE:FALSE), (aParams->NumberofParameters > 1)?aParams->Params[1].intParam:0, &DebugZoneMask);

            if (!Result)
            {
                /* Query Debug Zone Mask request was successful, go ahead   */
                /* and inform the User.                                     */
                BOT_NOTIFY_INFO("BTPM_QueryDebugZoneMask(%s) Success: 0x%08lX.\r\n", aParams->Params[0].intParam?"Remote":"Local", DebugZoneMask);

                /* Return success to the caller.                            */
                ret_val = NO_ERROR;
            }
            else
            {
                /* Error Querying Debug Zone Mask, inform the user.         */
                BOT_NOTIFY_ERROR("BTPM_QueryDebugZoneMask(%s, %d) Failure: %d, %s.\r\n", aParams->Params[0].intParam?"Remote":"Local", (aParams->NumberofParameters > 1)?aParams->Params[1].intParam:0, Result, ERR_ConvertErrorCodeToString(Result));
                ret_val = FUNCTION_ERROR;
            }
        }
        else
        {
            BOT_NOTIFY_ERROR("Usage: QueryDebugZoneMask [0/1 - Local/Service] [Page Number - optional, default 0].\r\n");
            ret_val = INVALID_PARAMETERS_ERROR;
        }
    }
    else
    {
        /* Not Initialized, flag an error.                                */
        BOT_NOTIFY_ERROR("Platform Manager has not been Initialized.\r\n");
        ret_val = NOT_INITIALIZED_ERROR;
    }

    return ret_val;
}

/* The following function is responsible for updating the Debug Zone */
/* Mask of a Remote Client (based on Process ID).  This function     */
/* returns zero if successful and a negative value if an error       */
/* occurred.                                                         */
int GattSrv::SetDebugZoneMaskPID(ParameterList_t *aParams __attribute__ ((unused)))
{
    int Result;
    int ret_val = UNDEFINED_ERROR;

    /* First, check to make sure that we are not Already Initialized.    */
    if (mInitialized)
    {
        /* Make sure the input parameters have been specified.            */
        if ((aParams) && (aParams->NumberofParameters >= 2))
        {
            /* Now actually Perform the command.                           */
            Result = BTPM_SetDebugZoneMaskPID((unsigned long)(aParams->Params[0].intParam), (unsigned long)(aParams->Params[1].intParam));

            if (!Result)
            {
                /* Set Debug Zone Mask request was successful, go ahead and */
                /* inform the User.                                         */
                BOT_NOTIFY_INFO("BTPM_SetDebugZoneMaskPID(%lu) Success: 0x%08lX.\r\n", (unsigned long)aParams->Params[0].intParam, (unsigned long)aParams->Params[1].intParam);

                /* Return success to the caller.                            */
                ret_val = NO_ERROR;
            }
            else
            {
                /* Error Setting Debug Zone Mask, inform the user.          */
                BOT_NOTIFY_ERROR("BTPM_SetDebugZoneMaskPID(%lu) Failure: %d, %s.\r\n", (unsigned long)(aParams->Params[0].intParam), Result, ERR_ConvertErrorCodeToString(Result));
                ret_val = FUNCTION_ERROR;
            }
        }
        else
        {
            BOT_NOTIFY_ERROR("Usage: SetDebugZoneMaskPID [Process ID] [Debug Zone Mask].\r\n");
            ret_val = INVALID_PARAMETERS_ERROR;
        }
    }
    else
    {
        /* Not Initialized, flag an error.                                */
        BOT_NOTIFY_ERROR("Platform Manager has not been Initialized.\r\n");
        ret_val = NOT_INITIALIZED_ERROR;
    }

    return ret_val;
}

/* The following function is responsible for shutting down the remote*/
/* server.  This function returns zero if successful and a negative  */
/* value if an error occurred.                                       */
int GattSrv::ShutdownService(ParameterList_t *aParams __attribute__ ((unused)))
{
    int Result;
    int ret_val = UNDEFINED_ERROR;

    /* First, check to make sure that we have already been Initialized.  */
    if (mInitialized)
    {
        /* Initialized, go ahead and attempt to shutdown the server.      */
        if ((Result = BTPM_ShutdownService()) == 0)
        {
            BOT_NOTIFY_INFO("BTPM_ShutdownService() Success: %d.\r\n", Result);

            /* Flag success.                                               */
            ret_val = NO_ERROR;
        }
        else
        {
            /* Error shutting down the service, inform the user and flag an*/
            /* error.                                                      */
            BOT_NOTIFY_ERROR("BTPM_ShutdownService() Failure: %d, %s.\r\n", Result, ERR_ConvertErrorCodeToString(Result));
            ret_val = FUNCTION_ERROR;
        }
    }
    else
    {
        /* Not Initialized, flag an error.                                */
        BOT_NOTIFY_ERROR("Platform Manager has not been Initialized.\r\n");
        ret_val = NOT_INITIALIZED_ERROR;
    }

    return ret_val;
}

/* The following function is responsible for displaying the current  */
/* local device properties of the server.  This function returns zero*/
/* if successful and a negative value if an error occurred.          */
int GattSrv::QueryLocalDeviceProperties(ParameterList_t *aParams __attribute__ ((unused)))
{
    int                            Result;
    int                            ret_val = UNDEFINED_ERROR;
    DEVM_Local_Device_Properties_t LocalDeviceProperties;

    /* First, check to make sure that we have already been Initialized.  */
    if (mInitialized)
    {
        /* Initialized, go ahead and query the Local Device Properties.   */
        if ((Result = DEVM_QueryLocalDeviceProperties(&LocalDeviceProperties)) >= 0)
        {
            BOT_NOTIFY_INFO("DEVM_QueryLocalDeviceProperties() Success: %d.\r\n", Result);

            /* Next, go ahead and display the properties.                  */
            DisplayLocalDeviceProperties(0, &LocalDeviceProperties);

            /* Flag success.                                               */
            ret_val = NO_ERROR;
        }
        else
        {
            /* Error querying the Local Device Properties, inform the user */
            /* and flag an error.                                          */
            BOT_NOTIFY_ERROR("DEVM_QueryLocalDeviceProperties() Failure: %d, %s.\r\n", Result, ERR_ConvertErrorCodeToString(Result));
            ret_val = FUNCTION_ERROR;
        }
    }
    else
    {
        /* Not Initialized, flag an error.                                */
        BOT_NOTIFY_ERROR("Platform Manager has not been Initialized.\r\n");
        ret_val = NOT_INITIALIZED_ERROR;
    }

    return ret_val;
}

/* The following function is responsible for setting the Local Name  */
/* Device Property of the local device.  This function returns zero  */
/* if successful and a negative value if an error occurred.          */
int GattSrv::SetLocalDeviceName(ParameterList_t *aParams __attribute__ ((unused)))
{
    int                            Result;
    int                            ret_val = UNDEFINED_ERROR;
    DEVM_Local_Device_Properties_t LocalDeviceProperties;

    /* First, check to make sure that we have already been Initialized.  */
    if (mInitialized)
    {
        BTPS_MemInitialize(&LocalDeviceProperties, 0, sizeof(LocalDeviceProperties));

        if ((aParams) && (aParams->NumberofParameters))
        {
            LocalDeviceProperties.DeviceNameLength = strlen(aParams->Params[0].strParam);
            strcpy(LocalDeviceProperties.DeviceName, aParams->Params[0].strParam);
        }

        BOT_NOTIFY_DEBUG("Attempting to set Device Name to: \"%s\".\r\n", LocalDeviceProperties.DeviceName);

        if ((Result = DEVM_UpdateLocalDeviceProperties(DEVM_UPDATE_LOCAL_DEVICE_PROPERTIES_DEVICE_NAME, &LocalDeviceProperties)) >= 0)
        {
            BOT_NOTIFY_INFO("DEVM_UpdateLocalDeviceProperties() Success: %d.\r\n", Result);

            /* Flag success.                                               */
            ret_val = NO_ERROR;
        }
        else
        {
            /* Error updating the Local Device Properties, inform the user */
            /* and flag an error.                                          */
            BOT_NOTIFY_ERROR("DEVM_UpdateLocalDeviceProperties() Failure: %d, %s.\r\n", Result, ERR_ConvertErrorCodeToString(Result));
            ret_val = FUNCTION_ERROR;
        }
    }
    else
    {
        /* Not Initialized, flag an error.                                */
        BOT_NOTIFY_ERROR("Platform Manager has not been Initialized.\r\n");
        ret_val = NOT_INITIALIZED_ERROR;
    }

    return ret_val;
}

/* The following function is responsible for setting the Local Name  */
/* Device Property of the local device.  This function returns zero  */
/* if successful and a negative value if an error occurred.          */
int GattSrv::SetLocalDeviceAppearance(ParameterList_t *aParams __attribute__ ((unused)))
{
    int                            Result;
    int                            ret_val = UNDEFINED_ERROR;
    DEVM_Local_Device_Properties_t LocalDeviceProperties;

    /* First, check to make sure that we have already been Initialized.  */
    if (mInitialized)
    {
        BTPS_MemInitialize(&LocalDeviceProperties, 0, sizeof(LocalDeviceProperties));

        if ((aParams) && (aParams->NumberofParameters))
            LocalDeviceProperties.DeviceAppearance = (Word_t)aParams->Params[0].intParam;

        BOT_NOTIFY_DEBUG("Attempting to set Device Appearance to: %u.\r\n", (unsigned int)LocalDeviceProperties.DeviceAppearance);

        if ((Result = DEVM_UpdateLocalDeviceProperties(DEVM_UPDATE_LOCAL_DEVICE_PROPERTIES_DEVICE_APPEARANCE, &LocalDeviceProperties)) >= 0)
        {
            BOT_NOTIFY_INFO("DEVM_UpdateLocalDeviceProperties() Success: %d.\r\n", Result);

            /* Flag success.                                               */
            ret_val = NO_ERROR;
        }
        else
        {
            /* Error updating the Local Device Properties, inform the user */
            /* and flag an error.                                          */
            BOT_NOTIFY_ERROR("DEVM_UpdateLocalDeviceProperties() Failure: %d, %s.\r\n", Result, ERR_ConvertErrorCodeToString(Result));
            ret_val = FUNCTION_ERROR;
        }
    }
    else
    {
        /* Not Initialized, flag an error.                                */
        BOT_NOTIFY_ERROR("Platform Manager has not been Initialized.\r\n");
        ret_val = NOT_INITIALIZED_ERROR;
    }

    return ret_val;
}

/* The following function is responsible for setting the Local Class */
/* of Device Device Property of the local device.  This function     */
/* returns zero if successful and a negative value if an error       */
/* occurred.                                                         */
int GattSrv::SetLocalClassOfDevice(ParameterList_t *aParams __attribute__ ((unused)))
{
    int                            Result;
    int                            ret_val = UNDEFINED_ERROR;
    DEVM_Local_Device_Properties_t LocalDeviceProperties;

    /* First, check to make sure that we have already been Initialized.  */
    if (mInitialized)
    {
        BTPS_MemInitialize(&LocalDeviceProperties, 0, sizeof(LocalDeviceProperties));

        /* Make sure that all of the parameters required for this function*/
        /* appear to be at least semi-valid.                              */
        if ((aParams) && (aParams->NumberofParameters))
        {
            ASSIGN_CLASS_OF_DEVICE(LocalDeviceProperties.ClassOfDevice, (Byte_t)((aParams->Params[0].intParam) & 0xFF), (Byte_t)(((aParams->Params[0].intParam) >> 8) & 0xFF), (Byte_t)(((aParams->Params[0].intParam) >> 16) & 0xFF));

            BOT_NOTIFY_DEBUG("Attempting to set Class Of Device to: 0x%02X%02X%02X.\r\n", LocalDeviceProperties.ClassOfDevice.Class_of_Device0, LocalDeviceProperties.ClassOfDevice.Class_of_Device1, LocalDeviceProperties.ClassOfDevice.Class_of_Device2);

            if ((Result = DEVM_UpdateLocalDeviceProperties(DEVM_UPDATE_LOCAL_DEVICE_PROPERTIES_CLASS_OF_DEVICE, &LocalDeviceProperties)) >= 0)
            {
                BOT_NOTIFY_INFO("DEVM_UpdateLocalDeviceProperties() Success: %d.\r\n", Result);

                /* Flag success.                                            */
                ret_val = NO_ERROR;
            }
            else
            {
                /* Error updating the Local Device Properties, inform the   */
                /* user and flag an error.                                  */
                BOT_NOTIFY_ERROR("DEVM_UpdateLocalDeviceProperties() Failure: %d, %s.\r\n", Result, ERR_ConvertErrorCodeToString(Result));
                ret_val = FUNCTION_ERROR;
            }
        }
        else
        {
            /* One or more of the necessary parameters is/are invalid.     */
            BOT_NOTIFY_ERROR("Usage: SetLocalClassOfDevice [Class of Device].\r\n");
            ret_val = INVALID_PARAMETERS_ERROR;
        }
    }
    else
    {
        /* Not Initialized, flag an error.                                */
        BOT_NOTIFY_ERROR("Platform Manager has not been Initialized.\r\n");
        ret_val = NOT_INITIALIZED_ERROR;
    }

    return ret_val;
}

/* The following function is responsible for setting the Local       */
/* Discoverability Mode Device Property of the local device.  This   */
/* function returns zero if successful and a negative value if an    */
/* error occurred.                                                   */
int GattSrv::SetDiscoverable(ParameterList_t *aParams __attribute__ ((unused)))
{
    int                            Result;
    int                            ret_val = UNDEFINED_ERROR;
    DEVM_Local_Device_Properties_t LocalDeviceProperties;

    /* First, check to make sure that we have already been Initialized.  */
    if (mInitialized)
    {
        BTPS_MemInitialize(&LocalDeviceProperties, 0, sizeof(LocalDeviceProperties));

        /* Make sure that all of the parameters required for this function*/
        /* appear to be at least semi-valid.                              */
        if ((aParams) && (aParams->NumberofParameters > 0))
        {
            LocalDeviceProperties.DiscoverableMode = (Boolean_t)aParams->Params[0].intParam;

            if (aParams->NumberofParameters > 1)
                LocalDeviceProperties.DiscoverableModeTimeout = aParams->Params[1].intParam;

            if (aParams->Params[0].intParam)
            {
                if (LocalDeviceProperties.DiscoverableModeTimeout)
                    BOT_NOTIFY_DEBUG("Attempting to set Discoverability Mode: General.\r\n");
                else
                    BOT_NOTIFY_DEBUG("Attempting to set Discoverability Mode: Limited (%d Seconds).\r\n", LocalDeviceProperties.DiscoverableModeTimeout);
            }
            else
                BOT_NOTIFY_DEBUG("Attempting to set Discoverability Mode: None.\r\n");

            if ((Result = DEVM_UpdateLocalDeviceProperties(DEVM_UPDATE_LOCAL_DEVICE_PROPERTIES_DISCOVERABLE_MODE, &LocalDeviceProperties)) >= 0)
            {
                BOT_NOTIFY_INFO("DEVM_UpdateLocalDeviceProperties() Success: %d.\r\n", Result);

                /* Flag success.                                            */
                ret_val = NO_ERROR;
            }
            else
            {
                /* Error updating the Local Device Properties, inform the   */
                /* user and flag an error.                                  */
                BOT_NOTIFY_ERROR("DEVM_UpdateLocalDeviceProperties() Failure: %d, %s.\r\n", Result, ERR_ConvertErrorCodeToString(Result));
                ret_val = FUNCTION_ERROR;
            }
        }
        else
        {
            /* One or more of the necessary parameters is/are invalid.     */
            BOT_NOTIFY_ERROR("Usage: SetDiscoverable [Enable/Disable] [Timeout (Enable only)].\r\n");
            ret_val = INVALID_PARAMETERS_ERROR;
        }
    }
    else
    {
        /* Not Initialized, flag an error.                                */
        BOT_NOTIFY_ERROR("Platform Manager has not been Initialized.\r\n");
        ret_val = NOT_INITIALIZED_ERROR;
    }

    return ret_val;
}

/* The following function is responsible for setting the Local       */
/* Connectability Mode Device Property of the local device.  This    */
/* function returns zero if successful and a negative value if an    */
/* error occurred.                                                   */
int GattSrv::SetConnectable(ParameterList_t *aParams __attribute__ ((unused)))
{
    int                            Result;
    int                            ret_val = UNDEFINED_ERROR;
    DEVM_Local_Device_Properties_t LocalDeviceProperties;

    /* First, check to make sure that we have already been Initialized.  */
    if (mInitialized)
    {
        BTPS_MemInitialize(&LocalDeviceProperties, 0, sizeof(LocalDeviceProperties));

        /* Make sure that all of the parameters required for this function*/
        /* appear to be at least semi-valid.                              */
        if ((aParams) && (aParams->NumberofParameters > 0))
        {
            LocalDeviceProperties.ConnectableMode = (Boolean_t)aParams->Params[0].intParam;

            if (aParams->NumberofParameters > 1)
                LocalDeviceProperties.ConnectableModeTimeout = aParams->Params[1].intParam;

            if (aParams->Params[0].intParam)
                BOT_NOTIFY_DEBUG("Attempting to set Connectability Mode: Connectable (%d Seconds).\r\n", LocalDeviceProperties.ConnectableModeTimeout);
            else
                BOT_NOTIFY_DEBUG("Attempting to set Connectability Mode: Non-Connectable.\r\n");

            if ((Result = DEVM_UpdateLocalDeviceProperties(DEVM_UPDATE_LOCAL_DEVICE_PROPERTIES_CONNECTABLE_MODE, &LocalDeviceProperties)) >= 0)
            {
                BOT_NOTIFY_INFO("DEVM_UpdateLocalDeviceProperties() Success: %d.\r\n", Result);

                /* Flag success.                                            */
                ret_val = NO_ERROR;
            }
            else
            {
                /* Error updating the Local Device Properties, inform the   */
                /* user and flag an error.                                  */
                BOT_NOTIFY_ERROR("DEVM_UpdateLocalDeviceProperties() Failure: %d, %s.\r\n", Result, ERR_ConvertErrorCodeToString(Result));
                ret_val = FUNCTION_ERROR;
            }
        }
        else
        {
            /* One or more of the necessary parameters is/are invalid.     */
            BOT_NOTIFY_ERROR("Usage: SetConnectable [Enable/Disable] [Timeout (Enable only)].\r\n");
            ret_val = INVALID_PARAMETERS_ERROR;
        }
    }
    else
    {
        /* Not Initialized, flag an error.                                */
        BOT_NOTIFY_ERROR("Platform Manager has not been Initialized.\r\n");
        ret_val = NOT_INITIALIZED_ERROR;
    }

    return ret_val;
}

/* The following function is responsible for setting the Local       */
/* Pairability Mode Device Property of the local device.  This       */
/* function returns zero if successful and a negative value if an    */
/* error occurred.                                                   */
int GattSrv::SetPairable(ParameterList_t *aParams __attribute__ ((unused)))
{
    int                            Result;
    int                            ret_val = UNDEFINED_ERROR;
    DEVM_Local_Device_Properties_t LocalDeviceProperties;

    /* First, check to make sure that we have already been Initialized.  */
    if (mInitialized)
    {
        BTPS_MemInitialize(&LocalDeviceProperties, 0, sizeof(LocalDeviceProperties));

        /* Make sure that all of the parameters required for this function*/
        /* appear to be at least semi-valid.                              */
        if ((aParams) && (aParams->NumberofParameters > 0))
        {
            LocalDeviceProperties.PairableMode = (Boolean_t)aParams->Params[0].intParam;

            if (aParams->NumberofParameters > 1)
                LocalDeviceProperties.PairableModeTimeout = aParams->Params[1].intParam;

            if (aParams->Params[0].intParam)
                BOT_NOTIFY_DEBUG("Attempting to set Pairability Mode: Pairable (%d Seconds).\r\n", LocalDeviceProperties.PairableModeTimeout);
            else
                BOT_NOTIFY_DEBUG("Attempting to set Pairability Mode: Non-Pairable.\r\n");

            if ((Result = DEVM_UpdateLocalDeviceProperties(DEVM_UPDATE_LOCAL_DEVICE_PROPERTIES_PAIRABLE_MODE, &LocalDeviceProperties)) >= 0)
            {
                BOT_NOTIFY_INFO("DEVM_UpdateLocalDeviceProperties() Success: %d.\r\n", Result);

                /* Flag success.                                            */
                ret_val = NO_ERROR;
            }
            else
            {
                /* Error updating the Local Device Properties, inform the   */
                /* user and flag an error.                                  */
                BOT_NOTIFY_ERROR("DEVM_UpdateLocalDeviceProperties() Failure: %d, %s.\r\n", Result, ERR_ConvertErrorCodeToString(Result));
                ret_val = FUNCTION_ERROR;
            }
        }
        else
        {
            /* One or more of the necessary parameters is/are invalid.     */
            BOT_NOTIFY_ERROR("Usage: SetPairable [Enable/Disable] [Timeout (Enable only)].\r\n");
            ret_val = INVALID_PARAMETERS_ERROR;
        }
    }
    else
    {
        /* Not Initialized, flag an error.                                */
        BOT_NOTIFY_ERROR("Platform Manager has not been Initialized.\r\n");
        ret_val = NOT_INITIALIZED_ERROR;
    }

    return ret_val;
}

/* The following function is responsible for starting a Device       */
/* Discovery on the local device.  This function returns zero if     */
/* successful and a negative value if an error occurred.             */
int GattSrv::StartDeviceDiscovery(ParameterList_t *aParams __attribute__ ((unused)))
{
    int Result;
    int ret_val = UNDEFINED_ERROR;

    /* First, check to make sure that we have already been Initialized.  */
    if (mInitialized)
    {
        /* Make sure that all of the parameters required for this function*/
        /* appear to be at least semi-valid.                              */
        if ((aParams) && (aParams->NumberofParameters >= 2))
        {
            if (aParams->Params[1].intParam)
                BOT_NOTIFY_DEBUG("Attempting to Start Discovery (%d Seconds).\r\n", aParams->Params[1].intParam);
            else
                BOT_NOTIFY_DEBUG("Attempting to Start Discovery (INDEFINITE).\r\n");

            /* Check to see if we are doing an LE or BR/EDR Discovery      */
            /* Process.                                                    */
            if ((Boolean_t)aParams->Params[0].intParam)
            {
                if ((Result = DEVM_StartDeviceScan(aParams->Params[1].intParam)) >= 0)
                {
                    BOT_NOTIFY_INFO("DEVM_StartDeviceScan() Success: %d.\r\n", Result);

                    /* Flag success.                                         */
                    ret_val = NO_ERROR;
                }
                else
                {
                    /* Error attempting to start Device Discovery, inform the*/
                    /* user and flag an error.                               */
                    BOT_NOTIFY_ERROR("DEVM_StartDeviceScan() Failure: %d, %s.\r\n", Result, ERR_ConvertErrorCodeToString(Result));
                    ret_val = FUNCTION_ERROR;
                }
            }
            else
            {
                if ((Result = DEVM_StartDeviceDiscovery(aParams->Params[1].intParam)) >= 0)
                {
                    BOT_NOTIFY_INFO("DEVM_StartDeviceDiscovery() Success: %d.\r\n", Result);

                    /* Flag success.                                         */
                    ret_val = NO_ERROR;
                }
                else
                {
                    /* Error attempting to start Device Discovery, inform the*/
                    /* user and flag an error.                               */
                    BOT_NOTIFY_ERROR("DEVM_StartDeviceDiscovery() Failure: %d, %s.\r\n", Result, ERR_ConvertErrorCodeToString(Result));
                    ret_val = FUNCTION_ERROR;
                }
            }
        }
        else
        {
            /* One or more of the necessary parameters is/are invalid.     */
            BOT_NOTIFY_ERROR("Usage: StartDeviceDiscovery [Type (1 = LE, 0 = BR/EDR)] [Duration].\r\n");
            ret_val = INVALID_PARAMETERS_ERROR;
        }
    }
    else
    {
        /* Not Initialized, flag an error.                                */
        BOT_NOTIFY_ERROR("Platform Manager has not been Initialized.\r\n");
        ret_val = NOT_INITIALIZED_ERROR;
    }

    return ret_val;
}

/* The following function is responsible for stopping a Device       */
/* Discovery on the local device.  This function returns zero if     */
/* successful and a negative value if an error occurred.             */
int GattSrv::StopDeviceDiscovery(ParameterList_t *aParams __attribute__ ((unused)))
{
    int Result;
    int ret_val = UNDEFINED_ERROR;

    /* First, check to make sure that we have already been Initialized.  */
    if (mInitialized)
    {
        /* Make sure that all of the parameters required for this function*/
        /* appear to be at least semi-valid.                              */
        if ((aParams) && (aParams->NumberofParameters))
        {
            /* Check to see what type of discovery should be stopped.      */
            if ((Boolean_t)aParams->Params[0].intParam)
            {
                /* Initialized, go ahead and attempt to stop LE Device      */
                /* Discovery.                                               */
                if ((Result = DEVM_StopDeviceScan()) >= 0)
                {
                    BOT_NOTIFY_INFO("DEVM_StopDeviceScan() Success: %d.\r\n", Result);

                    /* Flag success.                                         */
                    ret_val = NO_ERROR;
                }
                else
                {
                    /* Error stopping Device Discovery, inform the user and  */
                    /* flag an error.                                        */
                    BOT_NOTIFY_ERROR("DEVM_StopDeviceScan() Failure: %d, %s.\r\n", Result, ERR_ConvertErrorCodeToString(Result));
                    ret_val = FUNCTION_ERROR;
                }
            }
            else
            {
                /* Initialized, go ahead and attempt to stop BR/EDR Device  */
                /* Discovery.                                               */
                if ((Result = DEVM_StopDeviceDiscovery()) >= 0)
                {
                    BOT_NOTIFY_INFO("DEVM_StopDeviceDiscovery() Success: %d.\r\n", Result);

                    /* Flag success.                                         */
                    ret_val = NO_ERROR;
                }
                else
                {
                    /* Error stopping Device Discovery, inform the user and  */
                    /* flag an error.                                        */
                    BOT_NOTIFY_ERROR("DEVM_StopDeviceDiscovery() Failure: %d, %s.\r\n", Result, ERR_ConvertErrorCodeToString(Result));
                    ret_val = FUNCTION_ERROR;
                }
            }
        }
        else
        {
            /* One or more of the necessary parameters is/are invalid.     */
            BOT_NOTIFY_ERROR("Usage: StopDeviceDiscovery [Type (1 = LE, 0 = BR/EDR)].\r\n");
            ret_val = INVALID_PARAMETERS_ERROR;
        }
    }
    else
    {
        /* Not Initialized, flag an error.                                */
        BOT_NOTIFY_ERROR("Platform Manager has not been Initialized.\r\n");
        ret_val = NOT_INITIALIZED_ERROR;
    }

    return ret_val;
}

/* The following function is responsible for Querying the Remote     */
/* Device Address List.  This function returns zero if successful and*/
/* a negative value if an error occurred.                            */
int GattSrv::QueryRemoteDeviceList(ParameterList_t *aParams __attribute__ ((unused)))
{
    int                Result;
    int                ret_val = UNDEFINED_ERROR;
    char               Buffer[32];
    BD_ADDR_t         *BD_ADDRList;
    int                Index;
    unsigned int       Filter;
    unsigned int       TotalNumberDevices;
    Class_of_Device_t  ClassOfDevice;

    /* First, check to make sure that we have already been Initialized.  */
    if (mInitialized)
    {
        /* Make sure that all of the parameters required for this function*/
        /* appear to be at least semi-valid.                              */
        if ((aParams) && (aParams->NumberofParameters))
        {
            BOT_NOTIFY_DEBUG("Attempting Query %d Devices.\r\n", aParams->Params[0].intParam);

            if (aParams->Params[0].intParam)
                BD_ADDRList = (BD_ADDR_t *)BTPS_AllocateMemory(sizeof(BD_ADDR_t)*aParams->Params[0].intParam);
            else
                BD_ADDRList = NULL;

            if ((!aParams->Params[0].intParam) || ((aParams->Params[0].intParam) && (BD_ADDRList)))
            {
                /* If there were any optional parameters specified, go ahead*/
                /* and note them.                                           */
                if (aParams->NumberofParameters > 1)
                    Filter = aParams->Params[1].intParam;
                else
                    Filter = 0;

                if (aParams->NumberofParameters > 2)
                {
                    ASSIGN_CLASS_OF_DEVICE(ClassOfDevice, (Byte_t)((aParams->Params[2].intParam) & 0xFF), (Byte_t)(((aParams->Params[2].intParam) >> 8) & 0xFF), (Byte_t)(((aParams->Params[2].intParam) >> 16) & 0xFF));
                }
                else
                {
                    ASSIGN_CLASS_OF_DEVICE(ClassOfDevice, 0, 0, 0);
                }

                if ((Result = DEVM_QueryRemoteDeviceList(Filter, ClassOfDevice, aParams->Params[0].intParam, BD_ADDRList, &TotalNumberDevices)) >= 0)
                {
                    BOT_NOTIFY_INFO("DEVM_QueryRemoteDeviceList() Success: %d, Total Number Devices: %d.\r\n", Result, TotalNumberDevices);

                    if ((Result) && (BD_ADDRList))
                    {
                        BOT_NOTIFY_DEBUG("Returned device list (%d Entries):\r\n", Result);

                        for (Index=0;Index<Result;Index++)
                        {
                            BD_ADDRToStr(BD_ADDRList[Index], Buffer);

                            BOT_NOTIFY_DEBUG("%2d. %s\r\n", (Index+1), Buffer);
                        }
                    }

                    /* Flag success.                                         */
                    ret_val = NO_ERROR;
                }
                else
                {
                    /* Error attempting to query the Remote Device List,     */
                    /* inform the user and flag an error.                    */
                    BOT_NOTIFY_ERROR("DEVM_QueryRemoteDeviceList() Failure: %d, %s.\r\n", Result, ERR_ConvertErrorCodeToString(Result));
                    ret_val = FUNCTION_ERROR;
                }

                /* Free any memory that was allocated.                      */
                if (BD_ADDRList)
                    BTPS_FreeMemory(BD_ADDRList);
            }
            else
            {
                /* Unable to allocate memory for List.                      */
                BOT_NOTIFY_ERROR("Unable to allocate memory for %d Devices.\r\n", aParams->Params[0].intParam);
                ret_val = FUNCTION_ERROR;
            }
        }
        else
        {
            /* One or more of the necessary parameters is/are invalid.     */
            BOT_NOTIFY_ERROR("Usage: QueryRemoteDeviceList [Number of Devices] [Filter (Optional)] [COD Filter (Optional)].\r\n");
            ret_val = INVALID_PARAMETERS_ERROR;
        }
    }
    else
    {
        /* Not Initialized, flag an error.                                */
        BOT_NOTIFY_ERROR("Platform Manager has not been Initialized.\r\n");
        ret_val = NOT_INITIALIZED_ERROR;
    }

    return ret_val;
}

/* The following function is responsible for Querying the Remote     */
/* Device Properties of a specific Remote Device.  This function     */
/* returns zero if successful and a negative value if an error       */
/* occurred.                                                         */
int GattSrv::QueryRemoteDeviceProperties(ParameterList_t *aParams __attribute__ ((unused)))
{
    int                             Result;
    int                             ret_val = UNDEFINED_ERROR;
    BD_ADDR_t                       BD_ADDR;
    unsigned long                   QueryFlags;
    DEVM_Remote_Device_Properties_t RemoteDeviceProperties;

    /* First, check to make sure that we have already been Initialized.  */
    if (mInitialized)
    {
        /* Make sure that all of the parameters required for this function*/
        /* appear to be at least semi-valid.                              */
        if ((aParams) && (aParams->NumberofParameters >= 2))
        {
            /* Convert the parameter to a Bluetooth Device Address.        */
            StrToBD_ADDR(aParams->Params[0].strParam, &BD_ADDR);

            /* Check to see what kind of query is being done.              */
            if (aParams->Params[1].intParam)
                QueryFlags = DEVM_QUERY_REMOTE_DEVICE_PROPERTIES_FLAGS_LOW_ENERGY;
            else
                QueryFlags = 0;

            if ((aParams->NumberofParameters >= 3) && (aParams->Params[2].intParam))
                QueryFlags |= DEVM_QUERY_REMOTE_DEVICE_PROPERTIES_FLAGS_FORCE_UPDATE;

            BOT_NOTIFY_DEBUG("Attempting to Query %s Device Properties: %s, ForceUpdate: %s.\r\n", aParams->Params[0].strParam, (QueryFlags & DEVM_QUERY_REMOTE_DEVICE_PROPERTIES_FLAGS_LOW_ENERGY)?"LE":"BR/EDR", (QueryFlags & DEVM_QUERY_REMOTE_DEVICE_PROPERTIES_FLAGS_FORCE_UPDATE)?"TRUE":"FALSE");

            if ((Result = DEVM_QueryRemoteDeviceProperties(BD_ADDR, QueryFlags, &RemoteDeviceProperties)) >= 0)
            {
                BOT_NOTIFY_INFO("DEVM_QueryRemoteDeviceProperties() Success: %d.\r\n", Result);

                /* Display the Remote Device Properties.                    */
                DisplayRemoteDeviceProperties(0, &RemoteDeviceProperties);

                /* Flag success.                                            */
                ret_val = NO_ERROR;
            }
            else
            {
                /* Error Querying Remote Device, inform the user and flag an*/
                /* error.                                                   */
                BOT_NOTIFY_ERROR("DEVM_QueryRemoteDeviceProperties() Failure: %d, %s.\r\n", Result, ERR_ConvertErrorCodeToString(Result));
                ret_val = FUNCTION_ERROR;
            }
        }
        else
        {
            /* One or more of the necessary parameters is/are invalid.     */
            BOT_NOTIFY_ERROR("Usage: QueryRemoteDeviceProperties [BD_ADDR] [Type (1 = LE, 0 = BR/EDR)] [Force Update].\r\n");
            ret_val = INVALID_PARAMETERS_ERROR;
        }
    }
    else
    {
        /* Not Initialized, flag an error.                                */
        BOT_NOTIFY_ERROR("Platform Manager has not been Initialized.\r\n");
        ret_val = NOT_INITIALIZED_ERROR;
    }

    return ret_val;
}

/* The following function is responsible for Adding the specified    */
/* Remote Device.  This function returns zero if successful and a    */
/* negative value if an error occurred.                              */
int GattSrv::AddRemoteDevice(ParameterList_t *aParams __attribute__ ((unused)))
{
    int                                   Result;
    int                                   ret_val = UNDEFINED_ERROR;
    BD_ADDR_t                             BD_ADDR;
    Class_of_Device_t                     ClassOfDevice;
    DEVM_Remote_Device_Application_Data_t ApplicationData;

    /* First, check to make sure that we have already been Initialized.  */
    if (mInitialized)
    {
        /* Make sure that all of the parameters required for this function*/
        /* appear to be at least semi-valid.                              */
        if ((aParams) && (aParams->NumberofParameters))
        {
            /* Convert the parameter to a Bluetooth Device Address.        */
            StrToBD_ADDR(aParams->Params[0].strParam, &BD_ADDR);

            /* Check to see if a Class of Device was specified.            */
            if (aParams->NumberofParameters > 1)
            {
                ASSIGN_CLASS_OF_DEVICE(ClassOfDevice, (Byte_t)((aParams->Params[1].intParam) & 0xFF), (Byte_t)(((aParams->Params[1].intParam) >> 8) & 0xFF), (Byte_t)(((aParams->Params[1].intParam) >> 16) & 0xFF));
            }
            else
            {
                ASSIGN_CLASS_OF_DEVICE(ClassOfDevice, 0, 0, 0);
            }

            BOT_NOTIFY_DEBUG("Attempting to Add Device: %s.\r\n", aParams->Params[0].strParam);

            /* Check to see if Application Information was specified.      */
            if (aParams->NumberofParameters > 2)
            {
                ApplicationData.FriendlyNameLength = strlen(aParams->Params[2].strParam);

                strcpy(ApplicationData.FriendlyName, aParams->Params[2].strParam);

                ApplicationData.ApplicationInfo = 0;

                if (aParams->NumberofParameters > 3)
                    ApplicationData.ApplicationInfo = aParams->Params[3].intParam;
            }

            if ((Result = DEVM_AddRemoteDevice(BD_ADDR, ClassOfDevice, (aParams->NumberofParameters > 2)?&ApplicationData:NULL)) >= 0)
            {
                BOT_NOTIFY_INFO("DEVM_AddRemoteDevice() Success: %d.\r\n", Result);

                /* Flag success.                                            */
                ret_val = NO_ERROR;
            }
            else
            {
                /* Error Adding Remote Device, inform the user and flag an  */
                /* error.                                                   */
                BOT_NOTIFY_ERROR("DEVM_AddRemoteDevice() Failure: %d, %s.\r\n", Result, ERR_ConvertErrorCodeToString(Result));
                ret_val = FUNCTION_ERROR;
            }
        }
        else
        {
            /* One or more of the necessary parameters is/are invalid.     */
            BOT_NOTIFY_ERROR("Usage: AddRemoteDevice [BD_ADDR] [[COD (Optional)] [Friendly Name (Optional)] [Application Info (Optional)]].\r\n");
            ret_val = INVALID_PARAMETERS_ERROR;
        }
    }
    else
    {
        /* Not Initialized, flag an error.                                */
        BOT_NOTIFY_ERROR("Platform Manager has not been Initialized.\r\n");
        ret_val = NOT_INITIALIZED_ERROR;
    }

    return ret_val;
}

/* The following function is responsible for Deleting the specified  */
/* Remote Device.  This function returns zero if successful and a    */
/* negative value if an error occurred.                              */
int GattSrv::DeleteRemoteDevice(ParameterList_t *aParams __attribute__ ((unused)))
{
    int       Result;
    int       ret_val = UNDEFINED_ERROR;
    BD_ADDR_t BD_ADDR;

    /* First, check to make sure that we have already been Initialized.  */
    if (mInitialized)
    {
        /* Make sure that all of the parameters required for this function*/
        /* appear to be at least semi-valid.                              */
        if ((aParams) && (aParams->NumberofParameters))
        {
            /* Convert the parameter to a Bluetooth Device Address.        */
            StrToBD_ADDR(aParams->Params[0].strParam, &BD_ADDR);

            BOT_NOTIFY_DEBUG("Attempting to Delete Device: %s.\r\n", aParams->Params[0].strParam);

            if ((Result = DEVM_DeleteRemoteDevice(BD_ADDR)) >= 0)
            {
                BOT_NOTIFY_INFO("DEVM_DeleteRemoteDevice() Success: %d.\r\n", Result);

                /* Flag success.                                            */
                ret_val = NO_ERROR;
            }
            else
            {
                /* Error Deleting Remote Device, inform the user and flag an*/
                /* error.                                                   */
                BOT_NOTIFY_ERROR("DEVM_DeleteRemoteDevice() Failure: %d, %s.\r\n", Result, ERR_ConvertErrorCodeToString(Result));
                ret_val = FUNCTION_ERROR;
            }
        }
        else
        {
            /* One or more of the necessary parameters is/are invalid.     */
            BOT_NOTIFY_ERROR("Usage: DeleteRemoteDevice [BD_ADDR].\r\n");
            ret_val = INVALID_PARAMETERS_ERROR;
        }
    }
    else
    {
        /* Not Initialized, flag an error.                                */
        BOT_NOTIFY_ERROR("Platform Manager has not been Initialized.\r\n");
        ret_val = NOT_INITIALIZED_ERROR;
    }

    return ret_val;
}

/* The following function is responsible for Updating the specified  */
/* Remote Device's Applicatoin Data.  This function returns zero if  */
/* successful and a negative value if an error occurred.             */
int GattSrv::UpdateRemoteDeviceApplicationData(ParameterList_t *aParams __attribute__ ((unused)))
{
    int                                   Result;
    int                                   ret_val = UNDEFINED_ERROR;
    BD_ADDR_t                             BD_ADDR;
    DEVM_Remote_Device_Application_Data_t ApplicationData;

    /* First, check to make sure that we have already been Initialized.  */
    if (mInitialized)
    {
        /* Make sure that all of the parameters required for this function*/
        /* appear to be at least semi-valid.                              */
        if ((aParams) && (aParams->NumberofParameters > 1))
        {
            /* Convert the parameter to a Bluetooth Device Address.        */
            StrToBD_ADDR(aParams->Params[0].strParam, &BD_ADDR);

            /* Verify that the correct Application Data formats are        */
            /* present.                                                    */
            if ((!aParams->Params[1].intParam) || ((aParams->Params[1].intParam) && (aParams->NumberofParameters > 3)))
            {
                /* Check to see if Application Information was specified.   */
                if ((aParams->Params[1].intParam) && (aParams->NumberofParameters > 3))
                {
                    ApplicationData.FriendlyNameLength = strlen(aParams->Params[2].strParam);

                    strcpy(ApplicationData.FriendlyName, aParams->Params[2].strParam);

                    ApplicationData.ApplicationInfo = aParams->Params[3].intParam;
                }

                BOT_NOTIFY_DEBUG("Attempting to Update Device Application Data: %s.\r\n", aParams->Params[0].strParam);

                if ((Result = DEVM_UpdateRemoteDeviceApplicationData(BD_ADDR, aParams->Params[1].intParam?&ApplicationData:NULL)) >= 0)
                {
                    BOT_NOTIFY_INFO("DEVM_UpdateRemoteDeviceApplicationData() Success: %d.\r\n", Result);

                    /* Flag success.                                         */
                    ret_val = NO_ERROR;
                }
                else
                {
                    /* Error Deleting Remote Device, inform the user and flag*/
                    /* an error.                                             */
                    BOT_NOTIFY_ERROR("DEVM_UpdateRemoteDeviceApplicationData() Failure: %d, %s.\r\n", Result, ERR_ConvertErrorCodeToString(Result));
                    ret_val = FUNCTION_ERROR;
                }
            }
            else
            {
                /* One or more of the necessary parameters is/are invalid.  */
                BOT_NOTIFY_ERROR("Usage: UpdateRemoteDeviceAppData [BD_ADDR] [Data Valid] [Friendly Name] [Application Info].\r\n");
                ret_val = INVALID_PARAMETERS_ERROR;
            }
        }
        else
        {
            /* One or more of the necessary parameters is/are invalid.     */
            BOT_NOTIFY_ERROR("Usage: UpdateRemoteDeviceAppData [BD_ADDR] [Data Valid] [Friendly Name] [Application Info].\r\n");
            ret_val = INVALID_PARAMETERS_ERROR;
        }
    }
    else
    {
        /* Not Initialized, flag an error.                                */
        BOT_NOTIFY_ERROR("Platform Manager has not been Initialized.\r\n");
        ret_val = NOT_INITIALIZED_ERROR;
    }

    return ret_val;
}

/* The following function is responsible for Deleting the specified  */
/* Remote Devices.  This function returns zero if successful and a   */
/* negative value if an error occurred.                              */
int GattSrv::DeleteRemoteDevices(ParameterList_t *aParams __attribute__ ((unused)))
{
    int Result;
    int ret_val = UNDEFINED_ERROR;

    /* First, check to make sure that we have already been Initialized.  */
    if (mInitialized)
    {
        /* Make sure that all of the parameters required for this function*/
        /* appear to be at least semi-valid.                              */
        if ((aParams) && (aParams->NumberofParameters))
        {
            BOT_NOTIFY_DEBUG("Attempting to Delete Remote Devices, Filter %d.\r\n", aParams->Params[0].intParam);

            if ((Result = DEVM_DeleteRemoteDevices(aParams->Params[0].intParam)) >= 0)
            {
                BOT_NOTIFY_INFO("DEVM_DeleteRemoteDevices() Success: %d.\r\n", Result);

                /* Flag success.                                            */
                ret_val = NO_ERROR;
            }
            else
            {
                /* Error Deleting Remote Devices, inform the user and flag  */
                /* an error.                                                */
                BOT_NOTIFY_ERROR("DEVM_DeleteRemoteDevices() Failure: %d, %s.\r\n", Result, ERR_ConvertErrorCodeToString(Result));
                ret_val = FUNCTION_ERROR;
            }
        }
        else
        {
            /* One or more of the necessary parameters is/are invalid.     */
            BOT_NOTIFY_ERROR("Usage: DeleteRemoteDevices [Device Delete Filter].\r\n");
            ret_val = INVALID_PARAMETERS_ERROR;
        }
    }
    else
    {
        /* Not Initialized, flag an error.                                */
        BOT_NOTIFY_ERROR("Platform Manager has not been Initialized.\r\n");
        ret_val = NOT_INITIALIZED_ERROR;
    }

    return ret_val;
}

/* The following function is responsible for Pairing the specified   */
/* Remote Device.  This function returns zero if successful and a    */
/* negative value if an error occurred.                              */
int GattSrv::PairWithRemoteDevice(ParameterList_t *aParams __attribute__ ((unused)))
{
    int           Result;
    int           ret_val = UNDEFINED_ERROR;
    BD_ADDR_t     BD_ADDR;
    unsigned long Flags;

    /* First, check to make sure that we have already been Initialized.  */
    if (mInitialized)
    {
        /* Make sure that all of the parameters required for this function*/
        /* appear to be at least semi-valid.                              */
        if ((aParams) && (aParams->NumberofParameters >= 2))
        {
            /* Convert the parameter to a Bluetooth Device Address.        */
            StrToBD_ADDR(aParams->Params[0].strParam, &BD_ADDR);

            if (aParams->NumberofParameters > 2)
                Flags = aParams->Params[2].intParam;
            else
                Flags = 0;

            if (aParams->Params[1].intParam)
                Flags |= DEVM_PAIR_WITH_REMOTE_DEVICE_FLAGS_LOW_ENERGY;

            BOT_NOTIFY_DEBUG("Attempting to Pair With Remote %s Device: %s.\r\n", (Flags & DEVM_PAIR_WITH_REMOTE_DEVICE_FLAGS_LOW_ENERGY)?"LE":"BR/EDR", aParams->Params[0].strParam);

            if ((Result = DEVM_PairWithRemoteDevice(BD_ADDR, Flags)) >= 0)
            {
                BOT_NOTIFY_INFO("DEVM_PairWithRemoteDevice() Success: %d.\r\n", Result);

                /* Flag success.                                            */
                ret_val = NO_ERROR;
            }
            else
            {
                /* Error Pairing with Remote Device, inform the user and    */
                /* flag an error.                                           */
                BOT_NOTIFY_ERROR("DEVM_PairWithRemoteDevice() Failure: %d, %s.\r\n", Result, ERR_ConvertErrorCodeToString(Result));
                ret_val = FUNCTION_ERROR;
            }
        }
        else
        {
            /* One or more of the necessary parameters is/are invalid.     */
            BOT_NOTIFY_ERROR("Usage: PairWithRemoteDevice [BD_ADDR] [Type (1 = LE, 0 = BR/EDR)] [Pair Flags (optional)].\r\n");
            ret_val = INVALID_PARAMETERS_ERROR;
        }
    }
    else
    {
        /* Not Initialized, flag an error.                                */
        BOT_NOTIFY_ERROR("Platform Manager has not been Initialized.\r\n");
        ret_val = NOT_INITIALIZED_ERROR;
    }

    return ret_val;
}

/* The following function is responsible for Cancelling an on-going  */
/* Pairing operation with the specified Remote Device.  This function*/
/* returns zero if successful and a negative value if an error       */
/* occurred.                                                         */
int GattSrv::CancelPairWithRemoteDevice(ParameterList_t *aParams __attribute__ ((unused)))
{
    int       Result;
    int       ret_val = UNDEFINED_ERROR;
    BD_ADDR_t BD_ADDR;

    /* First, check to make sure that we have already been Initialized.  */
    if (mInitialized)
    {
        /* Make sure that all of the parameters required for this function*/
        /* appear to be at least semi-valid.                              */
        if ((aParams) && (aParams->NumberofParameters))
        {
            /* Convert the parameter to a Bluetooth Device Address.        */
            StrToBD_ADDR(aParams->Params[0].strParam, &BD_ADDR);

            BOT_NOTIFY_DEBUG("Attempting to Cancel Pair With Remote Device: %s.\r\n", aParams->Params[0].strParam);

            if ((Result = DEVM_CancelPairWithRemoteDevice(BD_ADDR)) >= 0)
            {
                BOT_NOTIFY_INFO("DEVM_CancelPairWithRemoteDevice() Success: %d.\r\n", Result);

                /* Flag success.                                            */
                ret_val = NO_ERROR;
            }
            else
            {
                /* Error Cancelling Pairing with Remote Device, inform the  */
                /* user and flag an error.                                  */
                BOT_NOTIFY_ERROR("DEVM_CancelPairWithRemoteDevice() Failure: %d, %s.\r\n", Result, ERR_ConvertErrorCodeToString(Result));
                ret_val = FUNCTION_ERROR;
            }
        }
        else
        {
            /* One or more of the necessary parameters is/are invalid.     */
            BOT_NOTIFY_ERROR("Usage: CancelPairWithRemoteDevice [BD_ADDR].\r\n");
            ret_val = INVALID_PARAMETERS_ERROR;
        }
    }
    else
    {
        /* Not Initialized, flag an error.                                */
        BOT_NOTIFY_ERROR("Platform Manager has not been Initialized.\r\n");
        ret_val = NOT_INITIALIZED_ERROR;
    }

    return ret_val;
}

/* The following function is responsible for Un-Pairing the specified*/
/* Remote Device.  This function returns zero if successful and a    */
/* negative value if an error occurred.                              */
int GattSrv::UnPairRemoteDevice(ParameterList_t *aParams __attribute__ ((unused)))
{
    int           Result;
    int           ret_val = UNDEFINED_ERROR;
    BD_ADDR_t     BD_ADDR;
    unsigned long UnPairFlags;

    /* First, check to make sure that we have already been Initialized.  */
    if (mInitialized)
    {
        /* Make sure that all of the parameters required for this function*/
        /* appear to be at least semi-valid.                              */
        if ((aParams) && (aParams->NumberofParameters >= 2))
        {
            /* Convert the parameter to a Bluetooth Device Address.        */
            StrToBD_ADDR(aParams->Params[0].strParam, &BD_ADDR);

            BOT_NOTIFY_DEBUG("Attempting to Un-Pair Remote Device: %s.\r\n", aParams->Params[0].strParam);

            if (aParams->Params[1].intParam)
                UnPairFlags = DEVM_UNPAIR_REMOTE_DEVICE_FLAGS_LOW_ENERGY;
            else
                UnPairFlags = 0;

            if ((Result = DEVM_UnPairRemoteDevice(BD_ADDR, UnPairFlags)) >= 0)
            {
                BOT_NOTIFY_INFO("DEVM_UnPairRemoteDevice() Success: %d.\r\n", Result);

                /* Flag success.                                            */
                ret_val = NO_ERROR;
            }
            else
            {
                /* Error Un-Pairing with Remote Device, inform the user and */
                /* flag an error.                                           */
                BOT_NOTIFY_ERROR("DEVM_UnPairRemoteDevice() Failure: %d, %s.\r\n", Result, ERR_ConvertErrorCodeToString(Result));
                ret_val = FUNCTION_ERROR;
            }
        }
        else
        {
            /* One or more of the necessary parameters is/are invalid.     */
            BOT_NOTIFY_ERROR("Usage: UnPairRemoteDevice [BD_ADDR] [Type (1 = LE, 0 = BR/EDR)] .\r\n");
            ret_val = INVALID_PARAMETERS_ERROR;
        }
    }
    else
    {
        /* Not Initialized, flag an error.                                */
        BOT_NOTIFY_ERROR("Platform Manager has not been Initialized.\r\n");
        ret_val = NOT_INITIALIZED_ERROR;
    }

    return ret_val;
}

/* The following function enables LE mSC only mode.                   */
/* In case this mode is enabled, pairing request from peers          */
/* that support legacy pairing only will be rejected.                */
/* Please note that in case this mode is enabled, the mSC flag        */
/* must be set to TRUE                                               */
/* This function returns zero on successful execution and a negative */
/* value on all errors.                                              */
int GattSrv::EnableSCOnly(ParameterList_t *aParams __attribute__ ((unused)))
{
    int       ret_val = UNDEFINED_ERROR;
    Boolean_t EnableSCOnly;
    char     *mode;

    /* First, check to make sure that we have already been Initialized.  */
    if (mInitialized)
    {
        /* Make sure that all of the parameters required for this function*/
        /* appear to be at least semi-valid.                              */
        if ((aParams) && (aParams->NumberofParameters > 0) && (aParams->Params[0].intParam > 0) && (aParams->Params[0].intParam <= 1))
        {
            /* Parameters appear to be valid, map the specified parameters */
            /* into the API specific parameters.                           */
            if (aParams->Params[0].intParam == 0)
            {
                EnableSCOnly = FALSE;
                mode         = (char*) "mSC Only mode is off";
            }
            else
            {
                EnableSCOnly = TRUE;
                mode         = (char*) "mSC Only mode is on - LE Legacy pairing will be rejected";
            }

            ret_val = DEVM_EnableSCOnly(EnableSCOnly);

            if (ret_val >= 0)
            {
                BOT_NOTIFY_INFO("DEVM_EnableSCOnly() Success: %d.\r\n", ret_val);
                BOT_NOTIFY_INFO("%s.\r\n", mode);

                /* If mSC Only mode has been enabled, 	     				*/
                /* even if the mSC flag is FALSE, set it to TRUE,			*/
                /* to avoid a case, the mSC only is set to TRUE,				*/
                /* and the mSC flag is not. inform the user of the 			*/
                /* updated mSC Pairing parameters.    						*/
                if (EnableSCOnly == TRUE)
                {
                    mSC = TRUE;
                }

                /* Flag success to the caller.                              */
                ret_val = NO_ERROR;
            }
            else
            {
                /* Error Pairing with Remote Device, inform the user and    */
                /* flag an error.                                           */
                BOT_NOTIFY_ERROR("DEVM_EnableSCOnly() Failure: %d, %s.\r\n", ret_val, ERR_ConvertErrorCodeToString(ret_val));
                ret_val = FUNCTION_ERROR;
            }
        }
        else
        {
            /* One or more of the necessary parameters is/are invalid.     */
            BOT_NOTIFY_ERROR("Usage: EnableSCOnly [mode (0 = mSC Only mode is off, 1 = mSC Only mode is on].\r\n");
            ret_val = INVALID_PARAMETERS_ERROR;
        }
    }
    else
    {
        /* Not Initialized, flag an error.                                */
        BOT_NOTIFY_ERROR("Platform Manager has not been Initialized.\r\n");
        ret_val = NOT_INITIALIZED_ERROR;
    }

    return ret_val;
}

/* The following function allows the user to generate new P256  		*/
/* private and local keys. This function shall NOT be used 	        */
/* in the middle of a pairing process. 					       		*/
/* It is relevant for LE Secure Conenctions pairing only!			*/
/* This function returns zero on successful execution and a negative */
/* value on all errors.                                              */
int GattSrv::RegenerateP256LocalKeys(ParameterList_t *aParams __attribute__ ((unused)))
{
    int ret_val = UNDEFINED_ERROR;

    /* First, check to make sure that we have already been Initialized.  */
    if (mInitialized)
    {
        ret_val = DEVM_RegenerateP256LocalKeys();

        if ( ret_val >= 0)
        {
            BOT_NOTIFY_INFO("DEVM_RegenerateP256LocalKeys() Success: %d.\r\n", ret_val);

            /* Flag success to the caller.                              */
            ret_val = NO_ERROR;
        }
        else
        {
            /* Error Pairing with Remote Device, inform the user and    */
            /* flag an error.                                           */
            BOT_NOTIFY_ERROR("DEVM_RegenerateP256LocalKeys() Failure: %d, %s.\r\n", ret_val, ERR_ConvertErrorCodeToString(ret_val));
            ret_val = FUNCTION_ERROR;
        }
    }
    else
    {
        /* Not Initialized, flag an error.                                */
        BOT_NOTIFY_ERROR("Platform Manager has not been Initialized.\r\n");
        ret_val = NOT_INITIALIZED_ERROR;
    }

    return ret_val;
}

/* The following function allows the user to generate mSC OOB  		*/
/* local parameters. This function shall NOT be used 	        	*/
/* in the middle of a pairing process. 					       		*/
/* It is relevant for LE Secure Conenctions pairing only!			*/
/* This function returns zero on successful execution and a negative */
/* value on all errors.                                              */
int GattSrv::OOBGenerateParameters(ParameterList_t *aParams __attribute__ ((unused)))
{
    int ret_val = NO_ERROR;

    /* First, check to make sure that we have already been Initialized.  */
    if (mInitialized)
    {
        /* In order to be able to perform LE mSC pairing in OOB method       */
        /* We need to generate local random and confirmation value	 		*/
        /*before the pairing process starts.                                */
        if (mOOBSupport)
        {
            ret_val = DEVM_SC_OOB_Generate_Parameters(&mOOBLocalRandom, &mOOBLocalConfirmation);
            if (ret_val >= 0)
            {
                BOT_NOTIFY_INFO("generated local OOB parameters\n");
                ret_val = NO_ERROR;
            }
            else
            {
                /* Error Pairing with Remote Device, inform the user        */
                /* and flag an error.                                       */
                BOT_NOTIFY_ERROR("DEVM_SC_OOB_Generate_Parameters() Failure: %d, %s.\r\n", ret_val, ERR_ConvertErrorCodeToString(ret_val));
                ret_val = FUNCTION_ERROR;
            }
        }
        else
        {
            BOT_NOTIFY_INFO("This function is relevant only in case we wish to perform\r\n LE mSC pairing using OOB method.\r\n Please, first change the pairing capabilties to support OOB\r\n");
        }
    }
    else
    {
        /* Not Initialized, flag an error.                                */
        BOT_NOTIFY_ERROR("Platform Manager has not been Initialized.\r\n");
        ret_val = NOT_INITIALIZED_ERROR;
    }

    return ret_val;
}

/* The following function is responsible for Querying the Remote     */
/* Device Services of the specified Remote Device.  This function    */
/* returns zero if successful and a negative value if an error       */
/* occurred.                                                         */
int GattSrv::QueryRemoteDeviceServices(ParameterList_t *aParams __attribute__ ((unused)))
{
    int                          Result;
    int                          ret_val = UNDEFINED_ERROR;
    BD_ADDR_t                    BD_ADDR;
    int                          Index;
    unsigned int                 TotalServiceSize;
    unsigned char               *ServiceData;
    unsigned long                QueryFlags;
    DEVM_Parsed_SDP_Data_t       ParsedSDPData;
    DEVM_Parsed_Services_Data_t  ParsedGATTData;

    /* First, check to make sure that we have already been Initialized.  */
    if (mInitialized)
    {
        /* Make sure that all of the parameters required for this function*/
        /* appear to be at least semi-valid.                              */
        if ((aParams) && (aParams->NumberofParameters > 2))
        {
            /* Initialize success.                                         */
            ret_val = NO_ERROR;

            /* Check to see what kind of Services are being requested.     */
            if (aParams->Params[1].intParam)
                QueryFlags = DEVM_QUERY_REMOTE_DEVICE_SERVICES_FLAGS_LOW_ENERGY;
            else
                QueryFlags = 0;

            /* Check to see if a forced update is requested.               */
            if (aParams->Params[2].intParam)
                QueryFlags |= DEVM_QUERY_REMOTE_DEVICE_SERVICES_FLAGS_FORCE_UPDATE;

            /* Convert the parameter to a Bluetooth Device Address.        */
            StrToBD_ADDR(aParams->Params[0].strParam, &BD_ADDR);

            BOT_NOTIFY_DEBUG("Attempting Query Remote Device %s For %s Services.\r\n", aParams->Params[0].strParam, (QueryFlags & DEVM_QUERY_REMOTE_DEVICE_SERVICES_FLAGS_LOW_ENERGY)?"GATT":"SDP");

            if (!(QueryFlags & DEVM_QUERY_REMOTE_DEVICE_SERVICES_FLAGS_FORCE_UPDATE))
            {
                /* Caller has requested to actually retrieve it locally,    */
                /* determine how many bytes were requested.                 */
                if (aParams->NumberofParameters > 3)
                    ServiceData = (unsigned char *)BTPS_AllocateMemory(aParams->Params[3].intParam);
                else
                    ret_val = INVALID_PARAMETERS_ERROR;
            }
            else
                ServiceData = NULL;

            if (!ret_val)
            {
                if ((QueryFlags & DEVM_QUERY_REMOTE_DEVICE_SERVICES_FLAGS_FORCE_UPDATE) || ((!(QueryFlags & DEVM_QUERY_REMOTE_DEVICE_SERVICES_FLAGS_FORCE_UPDATE)) && (ServiceData)))
                {
                    if ((Result = DEVM_QueryRemoteDeviceServices(BD_ADDR, QueryFlags, (QueryFlags & DEVM_QUERY_REMOTE_DEVICE_SERVICES_FLAGS_FORCE_UPDATE)?0:aParams->Params[3].intParam, ServiceData, &TotalServiceSize)) >= 0)
                    {
                        BOT_NOTIFY_INFO("DEVM_QueryRemoteDeviceServices() Success: %d, Total Number Service Bytes: %d.\r\n", Result, (QueryFlags & DEVM_QUERY_REMOTE_DEVICE_SERVICES_FLAGS_FORCE_UPDATE)?0:TotalServiceSize);

                        /* Now convert the Raw Data to parsed data.           */
                        if ((Result) && (ServiceData))
                        {
                            BOT_NOTIFY_DEBUG("Returned Service Data (%d Bytes):\r\n", Result);

                            for (Index=0;Index<Result;Index++)
                                BOT_NOTIFY_DEBUG("%02X", ServiceData[Index]);

                            BOT_NOTIFY_DEBUG("\r\n");
                            BOT_NOTIFY_DEBUG("\r\n");

                            /* Check to see what kind of stream was requested. */
                            if (QueryFlags & DEVM_QUERY_REMOTE_DEVICE_SERVICES_FLAGS_LOW_ENERGY)
                            {
                                /* Convert the Raw GATT Stream to a Parsed GATT */
                                /* Stream.                                      */
                                Result = DEVM_ConvertRawServicesStreamToParsedServicesData((unsigned int)Result, ServiceData, &ParsedGATTData);
                                if (!Result)
                                {
                                    /* Display the Parsed GATT Service Data.     */
                                    DisplayParsedGATTServiceData(&ParsedGATTData);

                                    /* All finished with the parsed data, so free*/
                                    /* it.                                       */
                                    DEVM_FreeParsedServicesData(&ParsedGATTData);
                                }
                                else
                                {
                                    BOT_NOTIFY_WARNING("DEVM_ConvertRawServicesStreamToParsedServicesData() Failure: %d, %s.\r\n", Result, ERR_ConvertErrorCodeToString(Result));
                                }
                            }
                            else
                            {
                                /* Convert the Raw SDP Stream to a Parsed       */
                                /* Stream.                                      */
                                Result = DEVM_ConvertRawSDPStreamToParsedSDPData((unsigned int)Result, ServiceData, &ParsedSDPData);

                                if (!Result)
                                {
                                    /* Success, Display the Parsed Data.         */
                                    DisplayParsedSDPServiceData(&ParsedSDPData);

                                    /* All finished with the parsed data, so free*/
                                    /* it.                                       */
                                    DEVM_FreeParsedSDPData(&ParsedSDPData);
                                }
                                else
                                {
                                    BOT_NOTIFY_WARNING("DEVM_ConvertRawSDPStreamToParsedSDPData() Failure: %d, %s.\r\n", Result, ERR_ConvertErrorCodeToString(Result));
                                }
                            }
                        }

                        /* Flag success.                                      */
                        ret_val = NO_ERROR;
                    }
                    else
                    {
                        /* Error attempting to query Services, inform the user*/
                        /* and flag an error.                                 */
                        BOT_NOTIFY_ERROR("DEVM_QueryRemoteDeviceServices() Failure: %d, %s.\r\n", Result, ERR_ConvertErrorCodeToString(Result));
                        ret_val = FUNCTION_ERROR;
                    }

                    /* Free any memory that was allocated.                   */
                    if (ServiceData)
                        BTPS_FreeMemory(ServiceData);
                }
                else
                {
                    /* Unable to allocate memory for List.                   */
                    BOT_NOTIFY_ERROR("Unable to allocate memory for %d Service Bytes.\r\n", aParams->Params[2].intParam);
                    ret_val = FUNCTION_ERROR;
                }
            }
            else
            {
                /* One or more of the necessary parameters is/are invalid.  */
                BOT_NOTIFY_ERROR("Usage: QueryRemoteDeviceServices [BD_ADDR] [Type (1 = LE, 0 = BR/EDR)] [Force Update] [Bytes to Query (specified if Force is 0)].\r\n");
                ret_val = INVALID_PARAMETERS_ERROR;
            }
        }
        else
        {
            /* One or more of the necessary parameters is/are invalid.     */
            BOT_NOTIFY_ERROR("Usage: QueryRemoteDeviceServices [BD_ADDR] [Type (1 = LE, 0 = BR/EDR)] [Force Update] [Bytes to Query (specified if Force is 0)].\r\n");
            ret_val = INVALID_PARAMETERS_ERROR;
        }
    }
    else
    {
        /* Not Initialized, flag an error.                                */
        BOT_NOTIFY_ERROR("Platform Manager has not been Initialized.\r\n");
        ret_val = NOT_INITIALIZED_ERROR;
    }

    return ret_val;
}

/* The following function is responsible for Querying whether a      */
/* Remote Device claims support for the specified Service. This      */
/* function returns zero if successful and a negative value if an    */
/* error occurred.                                                   */
int GattSrv::QueryRemoteDeviceServiceSupported(ParameterList_t *aParams __attribute__ ((unused)))
{
    int              Result;
    int              ret_val = UNDEFINED_ERROR;
    BD_ADDR_t        BD_ADDR;
    SDP_UUID_Entry_t ServiceUUID;

    /* First, check to make sure that we have already been Initialized.  */
    if (mInitialized)
    {
        /* Make sure that all of the parameters required for this function*/
        /* appear to be at least semi-valid.                              */
        if ((aParams) && (aParams->NumberofParameters >= 2))
        {
            /* Initialize success.                                         */
            ret_val = NO_ERROR;

            /* Convert the first parameter to a Bluetooth Device Address.  */
            StrToBD_ADDR(aParams->Params[0].strParam, &BD_ADDR);

            /* Convert the parameter to a SDP Service Class UUID.          */
            StrToUUIDEntry(aParams->Params[1].strParam, &ServiceUUID);

            if (ServiceUUID.SDP_Data_Element_Type != deNULL)
            {
                BOT_NOTIFY_DEBUG("Attempting Query Remote Device %s Support for Service %s.\r\n", aParams->Params[0].strParam, aParams->Params[1].strParam);

                if ((Result = DEVM_QueryRemoteDeviceServiceSupported(BD_ADDR, ServiceUUID)) >= 0)
                {
                    BOT_NOTIFY_INFO("DEVM_QueryRemoteDeviceServiceSupported() Success: %d.\r\n", Result);

                    /* Flag success.                                         */
                    ret_val = NO_ERROR;
                }
                else
                {
                    /* Error attempting to query Services, inform the user   */
                    /* and flag an error.                                    */
                    BOT_NOTIFY_ERROR("DEVM_QueryRemoteDeviceServiceSupported() Failure: %d, %s.\r\n", Result, ERR_ConvertErrorCodeToString(Result));
                    ret_val = FUNCTION_ERROR;
                }
            }
            else
            {
                /* One or more of the necessary parameters is/are invalid.  */
                BOT_NOTIFY_ERROR("Usage: QueryRemoteDeviceServiceSupported [BD_ADDR] [Service UUID].\r\n");
                ret_val = INVALID_PARAMETERS_ERROR;
            }
        }
        else
        {
            /* One or more of the necessary parameters is/are invalid.     */
            BOT_NOTIFY_ERROR("Usage: QueryRemoteDeviceServiceSupported [BD_ADDR] [Service UUID].\r\n");
            ret_val = INVALID_PARAMETERS_ERROR;
        }
    }
    else
    {
        /* Not Initialized, flag an error.                                */
        BOT_NOTIFY_ERROR("Platform Manager has not been Initialized.\r\n");
        ret_val = NOT_INITIALIZED_ERROR;
    }

    return ret_val;
}

/* The following function is responsible for Querying which Remote   */
/* Devices claim support for the specified Service. This function    */
/* returns zero if successful and a negative value if an error       */
/* occurred.                                                         */
int GattSrv::QueryRemoteDevicesForService(ParameterList_t *aParams __attribute__ ((unused)))
{
    int               Result;
    int               ret_val = UNDEFINED_ERROR;
    char              Buffer[32];
    BD_ADDR_t        *BD_ADDRList;
    int               Index;
    unsigned int      TotalNumberDevices;
    SDP_UUID_Entry_t  ServiceUUID;

    /* First, check to make sure that we have already been Initialized.  */
    if (mInitialized)
    {
        /* Make sure that all of the parameters required for this function*/
        /* appear to be at least semi-valid.                              */
        if ((aParams) && (aParams->NumberofParameters >= 2))
        {
            /* Initialize success.                                         */
            ret_val = NO_ERROR;

            /* Convert the parameter to a SDP Service Class UUID.          */
            StrToUUIDEntry(aParams->Params[0].strParam, &ServiceUUID);

            if (ServiceUUID.SDP_Data_Element_Type != deNULL)
            {
                BOT_NOTIFY_DEBUG("Attempting Query Up To %d Devices For Service %s.\r\n", aParams->Params[1].intParam, aParams->Params[0].strParam);

                if (aParams->Params[1].intParam)
                    BD_ADDRList = (BD_ADDR_t *)BTPS_AllocateMemory(sizeof(BD_ADDR_t)*aParams->Params[1].intParam);
                else
                    BD_ADDRList = NULL;

                if ((!aParams->Params[1].intParam) || ((aParams->Params[1].intParam) && (BD_ADDRList)))
                {
                    if ((Result = DEVM_QueryRemoteDevicesForService(ServiceUUID, aParams->Params[1].intParam, BD_ADDRList, &TotalNumberDevices)) >= 0)
                    {
                        BOT_NOTIFY_INFO("DEVM_QueryRemoteDevicesForService() Success: %d, Total Number Devices: %d.\r\n", Result, TotalNumberDevices);

                        if ((Result) && (BD_ADDRList))
                        {
                            BOT_NOTIFY_DEBUG("Returned device list (%d Entries):\r\n", Result);

                            for (Index=0;Index<Result;Index++)
                            {
                                BD_ADDRToStr(BD_ADDRList[Index], Buffer);

                                BOT_NOTIFY_DEBUG("%2d. %s\r\n", (Index+1), Buffer);
                            }
                        }

                        /* Flag success.                                      */
                        ret_val = NO_ERROR;
                    }
                    else
                    {
                        /* Error attempting to query the Remote Devices,      */
                        /* inform the user and flag an error.                 */
                        BOT_NOTIFY_ERROR("DEVM_QueryRemoteDevicesForService() Failure: %d, %s.\r\n", Result, ERR_ConvertErrorCodeToString(Result));
                        ret_val = FUNCTION_ERROR;
                    }

                    /* Free any memory that was allocated.                   */
                    if (BD_ADDRList)
                        BTPS_FreeMemory(BD_ADDRList);
                }
                else
                {
                    /* Unable to allocate memory for List.                   */
                    BOT_NOTIFY_ERROR("Unable to allocate memory for %d Devices.\r\n", aParams->Params[0].intParam);
                    ret_val = FUNCTION_ERROR;
                }
            }
            else
            {
                /* One or more of the necessary parameters is/are invalid.  */
                BOT_NOTIFY_ERROR("Usage: QueryRemoteDevicesForService [Service UUID] [Number of Devices].\r\n");
                ret_val = INVALID_PARAMETERS_ERROR;
            }
        }
        else
        {
            /* One or more of the necessary parameters is/are invalid.     */
            BOT_NOTIFY_ERROR("Usage: QueryRemoteDevicesForService [Service UUID] [Number of Devices].\r\n");
            ret_val = INVALID_PARAMETERS_ERROR;
        }
    }
    else
    {
        /* Not Initialized, flag an error.                                */
        BOT_NOTIFY_ERROR("Platform Manager has not been Initialized.\r\n");
        ret_val = NOT_INITIALIZED_ERROR;
    }

    return ret_val;
}

/* The following function is responsible for Querying the supported  */
/* Service Classes of the specified Remote Device. This function     */
/* returns zero if successful and a negative value if an error       */
/* occurred.                                                         */
int GattSrv::QueryRemoteDeviceServiceClasses(ParameterList_t *aParams __attribute__ ((unused)))
{
    int               Result;
    int               ret_val = UNDEFINED_ERROR;
    BD_ADDR_t         BD_ADDR;
    int               Index;
    unsigned int      TotalNumberServiceClasses;
    SDP_UUID_Entry_t *ServiceClassList;

    /* First, check to make sure that we have already been Initialized.  */
    if (mInitialized)
    {
        /* Make sure that all of the parameters required for this function*/
        /* appear to be at least semi-valid.                              */
        if ((aParams) && (aParams->NumberofParameters >= 2))
        {
            /* Initialize success.                                         */
            ret_val = NO_ERROR;

            /* Convert the parameter to a Bluetooth Device Address.        */
            StrToBD_ADDR(aParams->Params[0].strParam, &BD_ADDR);

            BOT_NOTIFY_DEBUG("Attempting Query Device %s For Up To %d Service Classes.\r\n", aParams->Params[0].strParam, aParams->Params[1].intParam);

            if (aParams->Params[1].intParam)
                ServiceClassList = (SDP_UUID_Entry_t *)BTPS_AllocateMemory(sizeof(SDP_UUID_Entry_t)*aParams->Params[1].intParam);
            else
                ServiceClassList = NULL;

            if ((!aParams->Params[1].intParam) || ((aParams->Params[1].intParam) && (ServiceClassList)))
            {
                if ((Result = DEVM_QueryRemoteDeviceServiceClasses(BD_ADDR, aParams->Params[1].intParam, ServiceClassList, &TotalNumberServiceClasses)) >= 0)
                {
                    BOT_NOTIFY_INFO("DEVM_QueryRemoteDeviceServiceClasses() Success: %d, Total Number Service Classes: %d.\r\n", Result, TotalNumberServiceClasses);

                    if ((Result) && (ServiceClassList))
                    {
                        BOT_NOTIFY_DEBUG("Returned service classes (%d Entries):\r\n", Result);

                        for (Index=0;Index<Result;Index++)
                        {
                            switch(ServiceClassList[Index].SDP_Data_Element_Type)
                            {
                            case deUUID_16:
                                BOT_NOTIFY_DEBUG("%2d. 0x%02X%02X\r\n", (Index+1), ServiceClassList[Index].UUID_Value.UUID_16.UUID_Byte0, ServiceClassList[Index].UUID_Value.UUID_16.UUID_Byte1);
                                break;
                            case deUUID_32:
                                BOT_NOTIFY_DEBUG("%2d. 0x%02X%02X%02X%02X\r\n", (Index+1), ServiceClassList[Index].UUID_Value.UUID_32.UUID_Byte0, ServiceClassList[Index].UUID_Value.UUID_32.UUID_Byte1, ServiceClassList[Index].UUID_Value.UUID_32.UUID_Byte2, ServiceClassList[Index].UUID_Value.UUID_32.UUID_Byte3);
                                break;
                            case deUUID_128:
                                BOT_NOTIFY_DEBUG("%2d. %02X%02X%02X%02X-%02X%02X-%02X%02X-%02X%02X-%02X%02X%02X%02X%02X%02X\r\n", (Index+1), ServiceClassList[Index].UUID_Value.UUID_128.UUID_Byte0, ServiceClassList[Index].UUID_Value.UUID_128.UUID_Byte1, ServiceClassList[Index].UUID_Value.UUID_128.UUID_Byte2, ServiceClassList[Index].UUID_Value.UUID_128.UUID_Byte3, ServiceClassList[Index].UUID_Value.UUID_128.UUID_Byte4, ServiceClassList[Index].UUID_Value.UUID_128.UUID_Byte5, ServiceClassList[Index].UUID_Value.UUID_128.UUID_Byte6, ServiceClassList[Index].UUID_Value.UUID_128.UUID_Byte7, ServiceClassList[Index].UUID_Value.UUID_128.UUID_Byte8, ServiceClassList[Index].UUID_Value.UUID_128.UUID_Byte9, ServiceClassList[Index].UUID_Value.UUID_128.UUID_Byte10, ServiceClassList[Index].UUID_Value.UUID_128.UUID_Byte11, ServiceClassList[Index].UUID_Value.UUID_128.UUID_Byte12, ServiceClassList[Index].UUID_Value.UUID_128.UUID_Byte13, ServiceClassList[Index].UUID_Value.UUID_128.UUID_Byte14, ServiceClassList[Index].UUID_Value.UUID_128.UUID_Byte15);
                                break;
                            default:
                                BOT_NOTIFY_DEBUG("%2d. (invalid: %u)\r\n", (Index+1), ServiceClassList[Index].SDP_Data_Element_Type);
                                break;
                            }
                        }
                    }

                    /* Flag success.                                         */
                    ret_val = NO_ERROR;
                }
                else
                {
                    /* Error attempting to query the Remote Devices, inform  */
                    /* the user and flag an error.                           */
                    BOT_NOTIFY_ERROR("DEVM_QueryRemoteDeviceServiceClasses() Failure: %d, %s.\r\n", Result, ERR_ConvertErrorCodeToString(Result));
                    ret_val = FUNCTION_ERROR;
                }
            }
            else
            {
                /* Unable to allocate memory for List.                      */
                BOT_NOTIFY_ERROR("Unable to allocate memory for %d Devices.\r\n", aParams->Params[0].intParam);
                ret_val = FUNCTION_ERROR;
            }

            /* Free any memory that was allocated.                         */
            if (ServiceClassList)
                BTPS_FreeMemory(ServiceClassList);
        }
        else
        {
            /* One or more of the necessary parameters is/are invalid.     */
            BOT_NOTIFY_ERROR("Usage: QueryRemoteDeviceServiceClasses [BD_ADDR] [Number of Service Classes].\r\n");
            ret_val = INVALID_PARAMETERS_ERROR;
        }
    }
    else
    {
        /* Not Initialized, flag an error.                                */
        BOT_NOTIFY_ERROR("Platform Manager has not been Initialized.\r\n");
        ret_val = NOT_INITIALIZED_ERROR;
    }

    return ret_val;
}

/* The following function is responsible for Authenticating the      */
/* specified Remote Device.  This function returns zero if successful*/
/* and a negative value if an error occurred.                        */
int GattSrv::AuthenticateRemoteDevice(ParameterList_t *aParams __attribute__ ((unused)))
{
    int           Result;
    int           ret_val = UNDEFINED_ERROR;
    BD_ADDR_t     BD_ADDR;
    unsigned long AuthenticateFlags;

    /* First, check to make sure that we have already been Initialized.  */
    if (mInitialized)
    {
        /* Make sure that all of the parameters required for this function*/
        /* appear to be at least semi-valid.                              */
        if ((aParams) && (aParams->NumberofParameters >= 2))
        {
            /* Convert the parameter to a Bluetooth Device Address.        */
            StrToBD_ADDR(aParams->Params[0].strParam, &BD_ADDR);

            BOT_NOTIFY_DEBUG("Attempting to Authenticate Remote Device: %s over %s.\r\n", aParams->Params[0].strParam, aParams->Params[1].intParam?"LE":"BR/EDR");

            /* Check to see what type of encryption operation to perform.  */
            if (aParams->Params[1].intParam)
                AuthenticateFlags = DEVM_AUTHENTICATE_REMOTE_DEVICE_FLAGS_LOW_ENERGY;
            else
                AuthenticateFlags = 0;

            if ((Result = DEVM_AuthenticateRemoteDevice(BD_ADDR, AuthenticateFlags)) >= 0)
            {
                BOT_NOTIFY_INFO("DEVM_AuthenticateRemoteDevice() Success: %d.\r\n", Result);

                /* Flag success.                                            */
                ret_val = NO_ERROR;
            }
            else
            {
                /* Error Authenticating Remote Device, inform the user and  */
                /* flag an error.                                           */
                BOT_NOTIFY_ERROR("DEVM_AuthenticateRemoteDevice() Failure: %d, %s.\r\n", Result, ERR_ConvertErrorCodeToString(Result));
                ret_val = FUNCTION_ERROR;
            }
        }
        else
        {
            /* One or more of the necessary parameters is/are invalid.     */
            BOT_NOTIFY_ERROR("Usage: AuthenticateRemoteDevice [BD_ADDR] [Type (LE = 1, BR/EDR = 0)].\r\n");
            ret_val = INVALID_PARAMETERS_ERROR;
        }
    }
    else
    {
        /* Not Initialized, flag an error.                                */
        BOT_NOTIFY_ERROR("Platform Manager has not been Initialized.\r\n");
        ret_val = NOT_INITIALIZED_ERROR;
    }

    return ret_val;
}

/* The following function is responsible for Encrypting the specified*/
/* Remote Device.  This function returns zero if successful and a    */
/* negative value if an error occurred.                              */
int GattSrv::EncryptRemoteDevice(ParameterList_t *aParams __attribute__ ((unused)))
{
    int           Result;
    int           ret_val = UNDEFINED_ERROR;
    BD_ADDR_t     BD_ADDR;
    unsigned long EncryptFlags;

    /* First, check to make sure that we have already been Initialized.  */
    if (mInitialized)
    {
        /* Make sure that all of the parameters required for this function*/
        /* appear to be at least semi-valid.                              */
        if ((aParams) && (aParams->NumberofParameters >= 2))
        {
            /* Convert the parameter to a Bluetooth Device Address.        */
            StrToBD_ADDR(aParams->Params[0].strParam, &BD_ADDR);

            BOT_NOTIFY_DEBUG("Attempting to Encrypt Remote Device: %s over %s.\r\n", aParams->Params[0].strParam, aParams->Params[1].intParam?"LE":"BR/EDR");

            /* Check to see what type of encryption operation to perform.  */
            if (aParams->Params[1].intParam)
                EncryptFlags = DEVM_ENCRYPT_REMOTE_DEVICE_FLAGS_LOW_ENERGY;
            else
                EncryptFlags = 0;

            if ((Result = DEVM_EncryptRemoteDevice(BD_ADDR, EncryptFlags)) >= 0)
            {
                BOT_NOTIFY_INFO("DEVM_EncryptRemoteDevice() Success: %d.\r\n", Result);

                /* Flag success.                                            */
                ret_val = NO_ERROR;
            }
            else
            {
                /* Error Encrypting Remote Device, inform the user and flag */
                /* an error.                                                */
                BOT_NOTIFY_ERROR("DEVM_EncryptRemoteDevice() Failure: %d, %s.\r\n", Result, ERR_ConvertErrorCodeToString(Result));
                ret_val = FUNCTION_ERROR;
            }
        }
        else
        {
            /* One or more of the necessary parameters is/are invalid.     */
            BOT_NOTIFY_ERROR("Usage: EncryptRemoteDevice [BD_ADDR] [Type (LE = 1, BR/EDR = 0)].\r\n");
            ret_val = INVALID_PARAMETERS_ERROR;
        }
    }
    else
    {
        /* Not Initialized, flag an error.                                */
        BOT_NOTIFY_ERROR("Platform Manager has not been Initialized.\r\n");
        ret_val = NOT_INITIALIZED_ERROR;
    }

    return ret_val;
}

/* The following function is responsible for Connecting with the     */
/* specified Remote Device.  This function returns zero if successful*/
/* and a negative value if an error occurred.                        */
int GattSrv::ConnectWithRemoteDevice(ParameterList_t *aParams __attribute__ ((unused)))
{
    int           Result;
    int           ret_val = UNDEFINED_ERROR;
    BD_ADDR_t     BD_ADDR;
    unsigned long ConnectFlags;

    /* First, check to make sure that we have already been Initialized.  */
    if (mInitialized)
    {
        /* Make sure that all of the parameters required for this function*/
        /* appear to be at least semi-valid.                              */
        if ((aParams) && (aParams->NumberofParameters>=2))
        {
            /* Convert the parameter to a Bluetooth Device Address.        */
            StrToBD_ADDR(aParams->Params[0].strParam, &BD_ADDR);

            if (aParams->NumberofParameters > 2)
                ConnectFlags = (unsigned long)(aParams->Params[2].intParam);
            else
                ConnectFlags = 0;

            /* Check to see if we should connect LE.                       */
            if (aParams->Params[1].intParam)
                ConnectFlags |= DEVM_CONNECT_WITH_REMOTE_DEVICE_FORCE_LOW_ENERGY;

            BOT_NOTIFY_DEBUG("Attempting to Connect With (%s) Remote Device: %s (Flags = 0x%08lX).\r\n", (ConnectFlags & DEVM_CONNECT_WITH_REMOTE_DEVICE_FORCE_LOW_ENERGY)?"LE":"BR/EDR", aParams->Params[0].strParam, ConnectFlags);

            if ((Result = DEVM_ConnectWithRemoteDevice(BD_ADDR, ConnectFlags)) >= 0)
            {
                BOT_NOTIFY_INFO("DEVM_ConnectWithRemoteDevice() Success: %d.\r\n", Result);

                /* Flag success.                                            */
                ret_val = NO_ERROR;
            }
            else
            {
                /* Error Connecting With Remote Device, inform the user and */
                /* flag an error.                                           */
                BOT_NOTIFY_ERROR("DEVM_ConnectWithRemoteDevice() Failure: %d, %s.\r\n", Result, ERR_ConvertErrorCodeToString(Result));
                ret_val = FUNCTION_ERROR;
            }
        }
        else
        {
            /* One or more of the necessary parameters is/are invalid.     */
            BOT_NOTIFY_ERROR("Usage: ConnectWithRemoteDevice [BD_ADDR] [Connect LE (1 = LE, 0 = BR/EDR)] [ConnectFlags (Optional)].\r\n");
            BOT_NOTIFY_ERROR("where ConnectFlags is a bitmask of\r\n");
            BOT_NOTIFY_ERROR("    0x00000001 = Authenticate\r\n");
            BOT_NOTIFY_ERROR("    0x00000002 = Encrypt\r\n");
            BOT_NOTIFY_ERROR("    0x00000004 = Use Local Random Address\r\n");
            BOT_NOTIFY_ERROR("    0x00000008 = Use Peer Random Address\r\n");
            ret_val = INVALID_PARAMETERS_ERROR;
        }
    }
    else
    {
        /* Not Initialized, flag an error.                                */
        BOT_NOTIFY_ERROR("Platform Manager has not been Initialized.\r\n");
        ret_val = NOT_INITIALIZED_ERROR;
    }

    return ret_val;
}

/* The following function is responsible for Disconnecting from the  */
/* specified Remote Device.  This function returns zero if successful*/
/* and a negative value if an error occurred.                        */
int GattSrv::DisconnectRemoteDevice(ParameterList_t *aParams __attribute__ ((unused)))
{
    int           Result;
    int           ret_val = UNDEFINED_ERROR;
    BD_ADDR_t     BD_ADDR;
    unsigned long DisconnectFlags;

    /* First, check to make sure that we have already been Initialized.  */
    if (mInitialized)
    {
        /* Make sure that all of the parameters required for this function*/
        /* appear to be at least semi-valid.                              */
        if ((aParams) && (aParams->NumberofParameters >= 2))
        {
            /* Convert the parameter to a Bluetooth Device Address.        */
            StrToBD_ADDR(aParams->Params[0].strParam, &BD_ADDR);

            /* Determine if the Force Flag was specified.                  */
            if (aParams->NumberofParameters > 2)
                DisconnectFlags = (aParams->Params[2].intParam?DEVM_DISCONNECT_FROM_REMOTE_DEVICE_FLAGS_FORCE:0);
            else
                DisconnectFlags = 0;

            /* Disconnect from an LE Device.                               */
            if (aParams->Params[1].intParam)
                DisconnectFlags |= DEVM_DISCONNECT_FROM_REMOTE_DEVICE_FLAGS_LOW_ENERGY;

            BOT_NOTIFY_DEBUG("Attempting to Disconnect Remote Device: %s.\r\n", aParams->Params[0].strParam);

            if ((Result = DEVM_DisconnectRemoteDevice(BD_ADDR, DisconnectFlags)) >= 0)
            {
                BOT_NOTIFY_INFO("DEVM_DisconnectRemoteDevice() Success: %d.\r\n", Result);

                /* Flag success.                                            */
                ret_val = NO_ERROR;
            }
            else
            {
                /* Error Disconnecting Remote Device, inform the user and   */
                /* flag an error.                                           */
                BOT_NOTIFY_ERROR("DEVM_DisconnectRemoteDevice() Failure: %d, %s.\r\n", Result, ERR_ConvertErrorCodeToString(Result));
                ret_val = FUNCTION_ERROR;
            }
        }
        else
        {
            /* One or more of the necessary parameters is/are invalid.     */
            BOT_NOTIFY_ERROR("Usage: DisconnectRemoteDevice [BD_ADDR] [LE Device (1= LE, 0 = BR/EDR)] [Force Flag (Optional)].\r\n");
            ret_val = INVALID_PARAMETERS_ERROR;
        }
    }
    else
    {
        /* Not Initialized, flag an error.                                */
        BOT_NOTIFY_ERROR("Platform Manager has not been Initialized.\r\n");
        ret_val = NOT_INITIALIZED_ERROR;
    }

    return ret_val;
}

/* The following function is responsible for setting the link for a  */
/* specified Remote Device to active.  This function returns zero if */
/* successful and a negative value if an error occurred.             */
int GattSrv::SetRemoteDeviceLinkActive(ParameterList_t *aParams __attribute__ ((unused)))
{
    int       Result;
    int       ret_val = UNDEFINED_ERROR;
    BD_ADDR_t BD_ADDR;

    /* First, check to make sure that we have already been Initialized.  */
    if (mInitialized)
    {
        /* Make sure that all of the parameters required for this function*/
        /* appear to be at least semi-valid.                              */
        if ((aParams) && (aParams->NumberofParameters))
        {
            /* Convert the parameter to a Bluetooth Device Address.        */
            StrToBD_ADDR(aParams->Params[0].strParam, &BD_ADDR);

            BOT_NOTIFY_DEBUG("Attempting to set link active for Remote Device: %s.\r\n", aParams->Params[0].strParam);

            if ((Result = DEVM_SetRemoteDeviceLinkActive(BD_ADDR)) >= 0)
            {
                BOT_NOTIFY_INFO("DEVM_SetRemoteDeviceLinkActive() Success: %d.\r\n", Result);

                /* Flag success.                                            */
                ret_val = NO_ERROR;
            }
            else
            {
                /* Error setting link for Remote Device, inform the user and*/
                /* flag an error.                                           */
                BOT_NOTIFY_ERROR("DEVM_SetRemoteDeviceLinkActive() Failure: %d, %s.\r\n", Result, ERR_ConvertErrorCodeToString(Result));
                ret_val = FUNCTION_ERROR;
            }
        }
        else
        {
            /* One or more of the necessary parameters is/are invalid.     */
            BOT_NOTIFY_ERROR("Usage: SetRemoteDeviceLinkActive [BD_ADDR].\r\n");
            ret_val = INVALID_PARAMETERS_ERROR;
        }
    }
    else
    {
        /* Not Initialized, flag an error.                                */
        BOT_NOTIFY_ERROR("Platform Manager has not been Initialized.\r\n");
        ret_val = NOT_INITIALIZED_ERROR;
    }

    return ret_val;
}

/* The following function is responsible for Registering a sample SDP*/
/* Record in the SDP Database of the Local Device.  This function    */
/* returns zero if successful and a negative value if an error       */
/* occurred.                                                         */
int GattSrv::CreateSDPRecord(ParameterList_t *aParams __attribute__ ((unused)))
{
    int                ret_val = UNDEFINED_ERROR;
    int                Result;
    long               RecordHandle;
    char               ServiceName[32];
    Boolean_t          Persistent;
    unsigned int       Port;
    SDP_UUID_Entry_t   SDPUUIDEntries;
    SDP_Data_Element_t SDP_Data_Element[3];
    SDP_Data_Element_t SDP_Data_Element_L2CAP;
    SDP_Data_Element_t SDP_Data_Element_RFCOMM[2];
    SDP_Data_Element_t SDP_Data_Element_Language[4];

    /* First, check to make sure that we have already been Initialized.  */
    if (mInitialized)
    {
        /* Make sure that all of the parameters required for this function*/
        /* appear to be at least semi-valid.                              */
        if ((aParams) && (aParams->NumberofParameters > 0))
            Port = aParams->Params[0].intParam;
        else
            Port = 1;

        if ((aParams) && (aParams->NumberofParameters > 1))
            Persistent = (Boolean_t)((aParams->Params[1].intParam)?TRUE:FALSE);
        else
            Persistent = (Boolean_t)FALSE;

        BOT_NOTIFY_DEBUG("Attempting to Add sample SPP SDP Record (Port %d, Persistent: %s).\r\n", Port, Persistent?"TRUE":"FALSE");

        /* Initialize the Serial Port Profile.                            */
        SDPUUIDEntries.SDP_Data_Element_Type = deUUID_16;
        SDP_ASSIGN_SERIAL_PORT_PROFILE_UUID_16(SDPUUIDEntries.UUID_Value.UUID_16);

        if ((RecordHandle = DEVM_CreateServiceRecord(Persistent, 1, &SDPUUIDEntries)) >= 0)
        {
            BOT_NOTIFY_INFO("DEVM_CreateServiceRecord() Success: %ld (0x%08lX).\r\n", (unsigned long)RecordHandle, (unsigned long)RecordHandle);

            /* Next, add the Protocol Descriptor List.                     */
            SDP_Data_Element[0].SDP_Data_Element_Type                      = deSequence;
            SDP_Data_Element[0].SDP_Data_Element_Length                    = 2;
            SDP_Data_Element[0].SDP_Data_Element.SDP_Data_Element_Sequence = &SDP_Data_Element[1];

            SDP_Data_Element[1].SDP_Data_Element_Type                      = deSequence;
            SDP_Data_Element[1].SDP_Data_Element_Length                    = 1;
            SDP_Data_Element[1].SDP_Data_Element.SDP_Data_Element_Sequence = &SDP_Data_Element_L2CAP;

            SDP_Data_Element_L2CAP.SDP_Data_Element_Type                   = deUUID_16;
            SDP_Data_Element_L2CAP.SDP_Data_Element_Length                 = sizeof(SDP_Data_Element_L2CAP.SDP_Data_Element.UUID_16);
            SDP_ASSIGN_L2CAP_UUID_16(SDP_Data_Element_L2CAP.SDP_Data_Element.UUID_16);

            SDP_Data_Element[2].SDP_Data_Element_Type                      = deSequence;
            SDP_Data_Element[2].SDP_Data_Element_Length                    = 2;
            SDP_Data_Element[2].SDP_Data_Element.SDP_Data_Element_Sequence = SDP_Data_Element_RFCOMM;

            SDP_Data_Element_RFCOMM[0].SDP_Data_Element_Type                 = deUUID_16;
            SDP_Data_Element_RFCOMM[0].SDP_Data_Element_Length               = UUID_16_SIZE;
            SDP_ASSIGN_RFCOMM_UUID_16(SDP_Data_Element_RFCOMM[0].SDP_Data_Element.UUID_16);

            SDP_Data_Element_RFCOMM[1].SDP_Data_Element_Type                 = deUnsignedInteger1Byte;
            SDP_Data_Element_RFCOMM[1].SDP_Data_Element_Length               = sizeof(SDP_Data_Element_RFCOMM[1].SDP_Data_Element.UnsignedInteger1Byte);
            SDP_Data_Element_RFCOMM[1].SDP_Data_Element.UnsignedInteger1Byte = (Byte_t)Port;

            Result = DEVM_AddServiceRecordAttribute((unsigned long)RecordHandle, SDP_ATTRIBUTE_ID_PROTOCOL_DESCRIPTOR_LIST, SDP_Data_Element);

            if (!Result)
            {
                /* Add a default Language Attribute ID List Entry for UTF-8 */
                /* English.                                                 */
                SDP_Data_Element_Language[0].SDP_Data_Element_Type                      = deSequence;
                SDP_Data_Element_Language[0].SDP_Data_Element_Length                    = 3;
                SDP_Data_Element_Language[0].SDP_Data_Element.SDP_Data_Element_Sequence = &SDP_Data_Element_Language[1];
                SDP_Data_Element_Language[1].SDP_Data_Element_Type                      = deUnsignedInteger2Bytes;
                SDP_Data_Element_Language[1].SDP_Data_Element_Length                    = SDP_UNSIGNED_INTEGER_2_BYTES_SIZE;
                SDP_Data_Element_Language[1].SDP_Data_Element.UnsignedInteger2Bytes     = SDP_NATURAL_LANGUAGE_ENGLISH_UTF_8;
                SDP_Data_Element_Language[2].SDP_Data_Element_Type                      = deUnsignedInteger2Bytes;
                SDP_Data_Element_Language[2].SDP_Data_Element_Length                    = SDP_UNSIGNED_INTEGER_2_BYTES_SIZE;
                SDP_Data_Element_Language[2].SDP_Data_Element.UnsignedInteger2Bytes     = SDP_UTF_8_CHARACTER_ENCODING;
                SDP_Data_Element_Language[3].SDP_Data_Element_Type                      = deUnsignedInteger2Bytes;
                SDP_Data_Element_Language[3].SDP_Data_Element_Length                    = SDP_UNSIGNED_INTEGER_2_BYTES_SIZE;
                SDP_Data_Element_Language[3].SDP_Data_Element.UnsignedInteger2Bytes     = SDP_DEFAULT_LANGUAGE_BASE_ATTRIBUTE_ID;

                Result = DEVM_AddServiceRecordAttribute((unsigned long)RecordHandle, SDP_ATTRIBUTE_ID_LANGUAGE_BASE_ATTRIBUTE_ID_LIST, SDP_Data_Element_Language);

                /* Finally Add the Service Name to the SDP Database.        */
                if (!Result)
                {
                    /* Build the Service Name.                               */
                    sprintf(ServiceName, "Sample Serial Port: %d.", Port);

                    SDP_Data_Element[0].SDP_Data_Element_Type       = deTextString;
                    SDP_Data_Element[0].SDP_Data_Element_Length     = BTPS_StringLength(ServiceName);
                    SDP_Data_Element[0].SDP_Data_Element.TextString = (Byte_t *)ServiceName;

                    if (!(Result = DEVM_AddServiceRecordAttribute((unsigned long)RecordHandle, (SDP_DEFAULT_LANGUAGE_BASE_ATTRIBUTE_ID + SDP_ATTRIBUTE_OFFSET_ID_SERVICE_NAME), SDP_Data_Element)))
                    {
                        BOT_NOTIFY_DEBUG("Sample SPP SDP Record successfully created: %ld (%08lX).\r\n", (unsigned long)RecordHandle, (unsigned long)RecordHandle);

                        /* Flag success.                                      */
                        ret_val = NO_ERROR;
                    }
                    else
                    {
                        BOT_NOTIFY_ERROR("DEVM_AddServiceRecordAttribute() Failure - Service Name: %d, %s.\r\n", Result, ERR_ConvertErrorCodeToString(Result));
                        ret_val = FUNCTION_ERROR;
                    }
                }
                else
                {
                    BOT_NOTIFY_ERROR("DEVM_AddServiceRecordAttribute() Failure - Language Attribute ID: %d, %s.\r\n", Result, ERR_ConvertErrorCodeToString(Result));
                    ret_val = FUNCTION_ERROR;
                }
            }
            else
            {
                BOT_NOTIFY_ERROR("DEVM_AddServiceRecordAttribute() Failure - Protocol Desriptor List: %d, %s.\r\n", Result, ERR_ConvertErrorCodeToString(Result));
                ret_val = FUNCTION_ERROR;
            }

            /* If an error occurred, to ahead and delete the record.       */
            if ((Result) && (((unsigned long)RecordHandle) > 0))
                DEVM_DeleteServiceRecord((unsigned long)RecordHandle);
        }
        else
        {
            /* Error attempting to create Record, inform the user and flag */
            /* an error.                                                   */
            BOT_NOTIFY_ERROR("DEVM_CreateServiceRecord() Failure: %ld, %s.\r\n", RecordHandle, ERR_ConvertErrorCodeToString(RecordHandle));
            ret_val = FUNCTION_ERROR;
        }
    }
    else
    {
        /* Not Initialized, flag an error.                                */
        BOT_NOTIFY_ERROR("Platform Manager has not been Initialized.\r\n");
        ret_val = NOT_INITIALIZED_ERROR;
    }

    return ret_val;
}

/* The following function is responsible for Deleting an SDP Record  */
/* in the SDP Database of the Local Device.  This function returns   */
/* zero if successful and a negative value if an error occurred.     */
int GattSrv::DeleteSDPRecord(ParameterList_t *aParams __attribute__ ((unused)))
{
    int           ret_val = UNDEFINED_ERROR;
    int           Result;
    unsigned long RecordHandle;

    /* First, check to make sure that we have already been Initialized.  */
    if (mInitialized)
    {
        /* Make sure that all of the parameters required for this function*/
        /* appear to be at least semi-valid.                              */
        if ((aParams) && (aParams->NumberofParameters > 0))
        {
            RecordHandle = (unsigned long)aParams->Params[0].intParam;

            BOT_NOTIFY_DEBUG("Attempting to Delete SDP Record %ld (0x%08lX).\r\n", RecordHandle, RecordHandle);

            if (!(Result = DEVM_DeleteServiceRecord(RecordHandle)))
            {
                BOT_NOTIFY_INFO("DEVM_DeleteServiceRecord() Success: %d.\r\n", Result);

                /* Flag success.                                            */
                ret_val = NO_ERROR;
            }
            else
            {
                /* Error attempting to delete Record, inform the user and   */
                /* flag an error.                                           */
                BOT_NOTIFY_ERROR("DEVM_DeleteServiceRecord() Failure: %d, %s.\r\n", Result, ERR_ConvertErrorCodeToString(Result));
                ret_val = FUNCTION_ERROR;
            }
        }
        else
        {
            /* One or more of the necessary parameters is/are invalid.     */
            BOT_NOTIFY_ERROR("Usage: DeleteSDPRecord [Service Record Handle].\r\n");
            ret_val = INVALID_PARAMETERS_ERROR;
        }
    }
    else
    {
        /* Not Initialized, flag an error.                                */
        BOT_NOTIFY_ERROR("Platform Manager has not been Initialized.\r\n");
        ret_val = NOT_INITIALIZED_ERROR;
    }

    return ret_val;
}

/* The following function is responsible for adding an SDP Attribute */
/* to an SDP Record in the SDP Database of the Local Device.  This   */
/* function returns zero if successful and a negative value if an    */
/* error occurred.                                                   */
int GattSrv::AddSDPAttribute(ParameterList_t *aParams __attribute__ ((unused)))
{
    int                ret_val = UNDEFINED_ERROR;
    int                Result;
    unsigned int       Value;
    unsigned long      RecordHandle;
    SDP_Data_Element_t SDP_Data_Element;

    /* First, check to make sure that we have already been Initialized.  */
    if (mInitialized)
    {
        /* Make sure that all of the parameters required for this function*/
        /* appear to be at least semi-valid.                              */
        if ((aParams) && (aParams->NumberofParameters > 1))
        {
            RecordHandle = (unsigned long)aParams->Params[0].intParam;

            BOT_NOTIFY_DEBUG("Attempting to Create SDP Attribute %d (0x%08X) for Record %ld (0x%08lX).\r\n", aParams->Params[1].intParam, aParams->Params[1].intParam, RecordHandle, RecordHandle);

            if (aParams->NumberofParameters > 2)
                Value = aParams->Params[2].intParam;
            else
                Value = 1;

            SDP_Data_Element.SDP_Data_Element_Type                  = deUnsignedInteger4Bytes;
            SDP_Data_Element.SDP_Data_Element_Length                = sizeof(SDP_Data_Element.SDP_Data_Element.UnsignedInteger4Bytes);
            SDP_Data_Element.SDP_Data_Element.UnsignedInteger4Bytes = (DWord_t)Value;

            if (!(Result = DEVM_AddServiceRecordAttribute(RecordHandle, aParams->Params[1].intParam, &SDP_Data_Element)))
            {
                BOT_NOTIFY_INFO("DEVM_AddServiceRecordAttribute() Success.\r\n");

                /* Flag success.                                            */
                ret_val = NO_ERROR;
            }
            else
            {
                /* Error attempting to delete Record, inform the user and   */
                /* flag an error.                                           */
                BOT_NOTIFY_ERROR("DEVM_AddServiceRecordAttribute() Failure: %d, %s.\r\n", Result, ERR_ConvertErrorCodeToString(Result));
                ret_val = FUNCTION_ERROR;
            }
        }
        else
        {
            /* One or more of the necessary parameters is/are invalid.     */
            BOT_NOTIFY_ERROR("Usage: AddSDPAttribute [Service Record Handle] [Attribute ID] [Attribute Value (optional)].\r\n");
            ret_val = INVALID_PARAMETERS_ERROR;
        }
    }
    else
    {
        /* Not Initialized, flag an error.                                */
        BOT_NOTIFY_ERROR("Platform Manager has not been Initialized.\r\n");
        ret_val = NOT_INITIALIZED_ERROR;
    }

    return ret_val;
}

/* The following function is responsible for deleting an SDP         */
/* Attribute from an SDP Record in the SDP Database of the Local     */
/* Device.  This function returns zero if successful and a negative  */
/* value if an error occurred.                                       */
int GattSrv::DeleteSDPAttribute(ParameterList_t *aParams __attribute__ ((unused)))
{
    int           ret_val = UNDEFINED_ERROR;
    int           Result;
    unsigned long RecordHandle;

    /* First, check to make sure that we have already been Initialized.  */
    if (mInitialized)
    {
        /* Make sure that all of the parameters required for this function*/
        /* appear to be at least semi-valid.                              */
        if ((aParams) && (aParams->NumberofParameters > 1))
        {
            RecordHandle = (unsigned long)aParams->Params[0].intParam;

            BOT_NOTIFY_DEBUG("Attempting to Delete SDP Attribute %d (0x%08X) from Record %ld (0x%08lX).\r\n", aParams->Params[1].intParam, aParams->Params[1].intParam, RecordHandle, RecordHandle);

            if (!(Result = DEVM_DeleteServiceRecordAttribute(RecordHandle, aParams->Params[1].intParam)))
            {
                BOT_NOTIFY_INFO("DEVM_DeleteServiceRecordAttribute() Success.\r\n");

                /* Flag success.                                            */
                ret_val = NO_ERROR;
            }
            else
            {
                /* Error attempting to delete Record, inform the user and   */
                /* flag an error.                                           */
                BOT_NOTIFY_ERROR("DEVM_DeleteServiceRecordAttribute() Failure: %d, %s.\r\n", Result, ERR_ConvertErrorCodeToString(Result));
                ret_val = FUNCTION_ERROR;
            }
        }
        else
        {
            /* One or more of the necessary parameters is/are invalid.     */
            BOT_NOTIFY_ERROR("Usage: DeleteSDPRecord [Service Record Handle] [Attribute ID].\r\n");
            ret_val = INVALID_PARAMETERS_ERROR;
        }
    }
    else
    {
        /* Not Initialized, flag an error.                                */
        BOT_NOTIFY_ERROR("Platform Manager has not been Initialized.\r\n");
        ret_val = NOT_INITIALIZED_ERROR;
    }

    return ret_val;
}

/* The following function is responsible for enabling/disabling      */
/* Bluetooth Debugging.  This function returns zero if successful and*/
/* a negative value if an error occurred.                            */
int GattSrv::EnableBluetoothDebug(ParameterList_t *aParams __attribute__ ((unused)))
{
    int            Result;
    int            ret_val = UNDEFINED_ERROR;
    unsigned long  Flags;
    unsigned int   Type;
    unsigned int   ParameterDataLength;
    unsigned char *ParameterData;

    /* First, check to make sure that we are not Already Initialized.    */
    if (mInitialized)
    {
        /* Make sure the input parameters have been specified.            */
        if ((aParams) && (aParams->NumberofParameters >= 1))
        {
            /* Initialize no parameters.                                   */
            ret_val             = 0;

            Flags               = 0;
            ParameterDataLength = 0;
            ParameterData       = NULL;

            /* Check to see if a Disable operation is being requested.     */
            if (!aParams->Params[0].intParam)
            {
                /* Disable - No other parameters are required.              */
                Flags = 0;
                Type  = 0;
            }
            else
            {
                /* Enable - Make sure we have the Type specified.           */
                if (aParams->NumberofParameters >= 2)
                {
                    /* Note the Debug Type.                                  */
                    Type = (unsigned int)aParams->Params[1].intParam;

                    /* Check for optional flags.                             */
                    if (aParams->NumberofParameters >= 3)
                        Flags = (unsigned int)aParams->Params[2].intParam;
                    else
                        Flags = 0;

                    /* Determine if there are any Parameters specified.      */
                    if ((aParams->NumberofParameters >= 4) && (aParams->Params[3].strParam))
                    {
                        ParameterDataLength = BTPS_StringLength(aParams->Params[3].strParam) + 1;
                        ParameterData       = (unsigned char *)aParams->Params[3].strParam;
                    }
                }
                else
                {
                    BOT_NOTIFY_ERROR("Usage: EnableBluetoothDebug [Enable (0/1)] [Type (1 - ASCII File, 2 - Terminal, 3 - FTS File)] [Debug Flags] [Debug Parameter String (no spaces)].\r\n");

                    /* One or more of the necessary parameters is/are        */
                    /* invalid.                                              */
                    ret_val = INVALID_PARAMETERS_ERROR;
                }
            }

            if (!ret_val)
            {
                /* Now actually Perform the command.                        */
                Result = DEVM_EnableBluetoothDebug((Boolean_t)(aParams->Params[0].intParam), Type, Flags, ParameterDataLength, ParameterData);

                if (!Result)
                {
                    /* Enable Bluetooth Debugging request was successful, go */
                    /* ahead and inform the User.                            */
                    BOT_NOTIFY_INFO("DEVM_EnableBluetoothDebug(%s) Success.\r\n", aParams->Params[0].intParam?"TRUE":"FALSE");

                    /* Return success to the caller.                         */
                    ret_val = NO_ERROR;
                }
                else
                {
                    /* Error Enabling Bluetooth Debugging, inform the user.  */
                    BOT_NOTIFY_ERROR("DEVM_EnableBluetoothDebug(%s) Failure: %d, %s.\r\n", aParams->Params[0].intParam?"TRUE":"FALSE", Result, ERR_ConvertErrorCodeToString(Result));
                    ret_val = FUNCTION_ERROR;
                }
            }
        }
        else
        {
            /* One or more of the necessary parameters is/are invalid.     */
            BOT_NOTIFY_ERROR("Usage: EnableBluetoothDebug [Enable (0/1)] [Type (1 - ASCII File, 2 - Terminal, 3 - FTS File)] [Debug Flags] [Debug Parameter String (no spaces)].\r\n");
            ret_val = INVALID_PARAMETERS_ERROR;
        }
    }
    else
    {
        /* Not Initialized, flag an error.                                */
        BOT_NOTIFY_ERROR("Platform Manager has not been Initialized.\r\n");
        ret_val = NOT_INITIALIZED_ERROR;
    }

    return ret_val;
}

/* The following function is responsible for Registering the Local   */
/* Client to receive (and process) Authentication Events.  This      */
/* function returns zero if successful and a negative value if an    */
/* error occurred.                                                   */
int GattSrv::RegisterAuthentication(ParameterList_t *aParams __attribute__ ((unused)))
{
    int Result;
    int ret_val = UNDEFINED_ERROR;

    /* First, check to make sure that we have already been Initialized.  */
    if (mInitialized)
    {
        /* Initialized, go ahead and attempt to Register for              */
        /* Authentication.                                                */
        if ((Result = DEVM_RegisterAuthentication(DEVM_Authentication_Callback, NULL)) >= 0)
        {
            BOT_NOTIFY_INFO("DEVM_RegisterAuthentication() Success: %d.\r\n", Result);

            /* Note the Authentication Callback ID.                        */
            mAuthenticationCallbackID = (unsigned int)Result;

            /* Flag success.                                               */
            ret_val                  = 0;
        }
        else
        {
            /* Error Registering for Authentication, inform the user and   */
            /* flag an error.                                              */
            BOT_NOTIFY_ERROR("DEVM_RegisterAuthentication() Failure: %d, %s.\r\n", Result, ERR_ConvertErrorCodeToString(Result));
            ret_val = FUNCTION_ERROR;
        }
    }
    else
    {
        /* Not Initialized, flag an error.                                */
        BOT_NOTIFY_ERROR("Platform Manager has not been Initialized.\r\n");
        ret_val = NOT_INITIALIZED_ERROR;
    }

    return ret_val;
}

/* The following function is responsible for Un-Registering the Local*/
/* Client to receive (and process) Authentication Events.  This      */
/* function returns zero if successful and a negative value if an    */
/* error occurred.                                                   */
int GattSrv::UnRegisterAuthentication(ParameterList_t *aParams __attribute__ ((unused)))
{
    int ret_val = UNDEFINED_ERROR;

    /* First, check to make sure that we have already been Initialized.  */
    if (mInitialized)
    {
        /* Initialized, go ahead and attempt to Register for              */
        /* Authentication.                                                */
        DEVM_UnRegisterAuthentication(mAuthenticationCallbackID);

        BOT_NOTIFY_INFO("DEVM_UnRegisterAuthentication() Success.\r\n");

        /* Clear the Authentication Callback ID.                          */
        mAuthenticationCallbackID = 0;

        /* Flag success.                                                  */
        ret_val                  = 0;
    }
    else
    {
        /* Not Initialized, flag an error.                                */
        BOT_NOTIFY_ERROR("Platform Manager has not been Initialized.\r\n");
        ret_val = NOT_INITIALIZED_ERROR;
    }

    return ret_val;
}

/* The following function is responsible for issuing a GAP           */
/* Authentication Response with a PIN Code value specified via the   */
/* input parameter.  This function returns zero on successful        */
/* execution and a negative value on all errors.                     */
int GattSrv::PINCodeResponse(ParameterList_t *aParams __attribute__ ((unused)))
{
    int                               Result;
    int                               ret_val = UNDEFINED_ERROR;
    BD_ADDR_t                         NullADDR;
    PIN_Code_t                        PINCode;
    DEVM_Authentication_Information_t AuthenticationResponseInformation;

    ASSIGN_BD_ADDR(NullADDR, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00);

    /* First, check to make sure that we have already been Initialized.  */
    if (mInitialized)
    {
        /* First, check to see if there is an on-going Pairing operation  */
        /* active.                                                        */
        if (!COMPARE_BD_ADDR(mCurrentRemoteBD_ADDR, NullADDR))
        {
            /* Make sure that all of the parameters required for this      */
            /* function appear to be at least semi-valid.                  */
            if ((aParams) && (aParams->NumberofParameters > 0) && (aParams->Params[0].strParam) && (strlen(aParams->Params[0].strParam) > 0) && (strlen(aParams->Params[0].strParam) <= sizeof(PIN_Code_t)))
            {
                /* Parameters appear to be valid, go ahead and convert the  */
                /* input parameter into a PIN Code.                         */

                /* Initialize the PIN code.                                 */
                ASSIGN_PIN_CODE(PINCode, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0);

                memcpy(&PINCode, aParams->Params[0].strParam, strlen(aParams->Params[0].strParam));

                /* Populate the response structure.                         */
                BTPS_MemInitialize(&AuthenticationResponseInformation, 0, sizeof(AuthenticationResponseInformation));

                AuthenticationResponseInformation.BD_ADDR                    = mCurrentRemoteBD_ADDR;
                AuthenticationResponseInformation.AuthenticationAction       = DEVM_AUTHENTICATION_ACTION_PIN_CODE_RESPONSE;
                AuthenticationResponseInformation.AuthenticationDataLength   = (Byte_t)(strlen(aParams->Params[0].strParam));

                AuthenticationResponseInformation.AuthenticationData.PINCode = PINCode;

                /* Submit the Authentication Response.                      */
                Result = DEVM_AuthenticationResponse(mAuthenticationCallbackID, &AuthenticationResponseInformation);

                /* Check the return value for the submitted command for     */
                /* success.                                                 */
                if (!Result)
                {
                    /* Operation was successful, inform the user.            */
                    BOT_NOTIFY_INFO("DEVM_AuthenticationResponse(), Pin Code Response Success.\r\n");

                    /* Flag success to the caller.                           */
                    ret_val = NO_ERROR;
                }
                else
                {
                    /* Inform the user that the Authentication Response was  */
                    /* not successful.                                       */
                    BOT_NOTIFY_ERROR("DEVM_AuthenticationResponse() Failure: %d, %s.\r\n", Result, ERR_ConvertErrorCodeToString(Result));
                    ret_val = FUNCTION_ERROR;
                }

                /* Flag that there is no longer a current Authentication    */
                /* procedure in progress.                                   */
                ASSIGN_BD_ADDR(mCurrentRemoteBD_ADDR, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00);
            }
            else
            {
                /* One or more of the necessary parameters is/are invalid.  */
                BOT_NOTIFY_ERROR("Usage: PINCodeResponse [PIN Code].\r\n");
                ret_val = INVALID_PARAMETERS_ERROR;
            }
        }
        else
        {
            /* There is not currently an on-going authentication operation,*/
            /* inform the user of this error condition.                    */
            BOT_NOTIFY_ERROR("Unable to issue PIN Code Authentication Response: Authentication is not currently in progress.\r\n");
            ret_val = FUNCTION_ERROR;
        }
    }
    else
    {
        /* Not Initialized, flag an error.                                */
        BOT_NOTIFY_ERROR("Platform Manager has not been Initialized.\r\n");
        ret_val = NOT_INITIALIZED_ERROR;
    }

    return ret_val;
}

/* The following function is responsible for issuing a GAP           */
/* Authentication Response with a Pass Key value specified via the   */
/* input parameter.  This function returns zero on successful        */
/* execution and a negative value on all errors.                     */
int GattSrv::PassKeyResponse(ParameterList_t *aParams __attribute__ ((unused)))
{
    int                               Result;
    int                               ret_val = UNDEFINED_ERROR;
    BD_ADDR_t                         NullADDR;
    DEVM_Authentication_Information_t AuthenticationResponseInformation;

    ASSIGN_BD_ADDR(NullADDR, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00);

    /* First, check to make sure that we have already been Initialized.  */
    if (mInitialized)
    {
        /* First, check to see if there is an on-going Pairing operation  */
        /* active.                                                        */
        if (!COMPARE_BD_ADDR(mCurrentRemoteBD_ADDR, NullADDR))
        {
            /* Make sure that all of the parameters required for this      */
            /* function appear to be at least semi-valid.                  */
            if ((aParams) && (aParams->NumberofParameters > 0) && (strlen(aParams->Params[0].strParam) <= GAP_PASSKEY_MAXIMUM_NUMBER_OF_DIGITS))
            {
                /* Parameters appear to be valid, go ahead and populate the */
                /* response structure.                                      */
                BTPS_MemInitialize(&AuthenticationResponseInformation, 0, sizeof(AuthenticationResponseInformation));

                AuthenticationResponseInformation.BD_ADDR                    = mCurrentRemoteBD_ADDR;
                AuthenticationResponseInformation.AuthenticationAction       = DEVM_AUTHENTICATION_ACTION_PASSKEY_RESPONSE;
                AuthenticationResponseInformation.AuthenticationDataLength   = sizeof(AuthenticationResponseInformation.AuthenticationData.Passkey);

                AuthenticationResponseInformation.AuthenticationData.Passkey = (DWord_t)(aParams->Params[0].intParam);

                if (mCurrentLowEnergy)
                    AuthenticationResponseInformation.AuthenticationAction |= DEVM_AUTHENTICATION_ACTION_LOW_ENERGY_OPERATION_MASK;

                /* Submit the Authentication Response.                      */
                Result = DEVM_AuthenticationResponse(mAuthenticationCallbackID, &AuthenticationResponseInformation);

                /* Check the return value for the submitted command for     */
                /* success.                                                 */
                if (!Result)
                {
                    /* Operation was successful, inform the user.            */
                    BOT_NOTIFY_INFO("DEVM_AuthenticationResponse(), Passkey Response Success.\r\n");

                    /* Flag success to the caller.                           */
                    ret_val = NO_ERROR;
                }
                else
                {
                    /* Inform the user that the Authentication Response was  */
                    /* not successful.                                       */
                    BOT_NOTIFY_ERROR("DEVM_AuthenticationResponse() Failure: %d, %s.\r\n", Result, ERR_ConvertErrorCodeToString(Result));
                    ret_val = FUNCTION_ERROR;
                }

                /* Flag that there is no longer a current Authentication    */
                /* procedure in progress.                                   */
                ASSIGN_BD_ADDR(mCurrentRemoteBD_ADDR, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00);
            }
            else
            {
                /* One or more of the necessary parameters is/are invalid.  */
                BOT_NOTIFY_ERROR("Usage: PassKeyResponse [Numeric Passkey (0 - 999999)].\r\n");
                ret_val = INVALID_PARAMETERS_ERROR;
            }
        }
        else
        {
            /* There is not currently an on-going authentication operation,*/
            /* inform the user of this error condition.                    */
            BOT_NOTIFY_ERROR("Unable to issue Pass Key Authentication Response: Authentication is not currently in progress.\r\n");
            ret_val = FUNCTION_ERROR;
        }
    }
    else
    {
        /* Not Initialized, flag an error.                                */
        BOT_NOTIFY_ERROR("Platform Manager has not been Initialized.\r\n");
        ret_val = NOT_INITIALIZED_ERROR;
    }

    return ret_val;
}

/* The following function is responsible for issuing a GAP           */
/* Authentication Response with a User Confirmation value specified  */
/* via the input parameter.  This function returns zero on successful*/
/* execution and a negative value on all errors.                     */
int GattSrv::UserConfirmationResponse(ParameterList_t *aParams __attribute__ ((unused)))
{
    int                               Result;
    int                               ret_val = UNDEFINED_ERROR;
    DEVM_Authentication_Information_t AuthenticationResponseInformation;

    /* First, check to make sure that we have already been Initialized.  */
    if (mInitialized)
    {
        /* First, check to see if there is an on-going Pairing operation  */
        /* active.                                                        */
        if (!COMPARE_NULL_BD_ADDR(mCurrentRemoteBD_ADDR))
        {
            /* Make sure that all of the parameters required for this      */
            /* function appear to be at least semi-valid.                  */
            if ((aParams) && (aParams->NumberofParameters > 0))
            {
                /* Parameters appear to be valid, go ahead and populate the */
                /* response structure.                                      */
                BTPS_MemInitialize(&AuthenticationResponseInformation, 0, sizeof(AuthenticationResponseInformation));

                AuthenticationResponseInformation.BD_ADDR                         = mCurrentRemoteBD_ADDR;
                AuthenticationResponseInformation.AuthenticationAction            = DEVM_AUTHENTICATION_ACTION_USER_CONFIRMATION_RESPONSE;
                AuthenticationResponseInformation.AuthenticationDataLength        = sizeof(AuthenticationResponseInformation.AuthenticationData.Confirmation);

                AuthenticationResponseInformation.AuthenticationData.Confirmation = (Boolean_t)(aParams->Params[0].intParam?TRUE:FALSE);

                if (mCurrentLowEnergy)
                {
                    AuthenticationResponseInformation.AuthenticationAction                    = DEVM_AUTHENTICATION_ACTION_NUMERIC_COMPARISON_RESPONSE;
                    AuthenticationResponseInformation.AuthenticationAction                   |= DEVM_AUTHENTICATION_ACTION_LOW_ENERGY_OPERATION_MASK;
                    AuthenticationResponseInformation.AuthenticationDataLength                = sizeof(AuthenticationResponseInformation.AuthenticationData.AcceptedNumericValue);

                    AuthenticationResponseInformation.AuthenticationData.AcceptedNumericValue = (Boolean_t)(aParams->Params[0].intParam?TRUE:FALSE);
                }

                /* Submit the Authentication Response.                      */
                Result = DEVM_AuthenticationResponse(mAuthenticationCallbackID, &AuthenticationResponseInformation);

                /* Check the return value for the submitted command for     */
                /* success.                                                 */
                if (!Result)
                {
                    /* Operation was successful, inform the user.            */
                    BOT_NOTIFY_INFO("DEVM_AuthenticationResponse(), User Confirmation Response Success.\r\n");

                    /* Flag success to the caller.                           */
                    ret_val = NO_ERROR;
                }
                else
                {
                    /* Inform the user that the Authentication Response was  */
                    /* not successful.                                       */
                    BOT_NOTIFY_ERROR("DEVM_AuthenticationResponse() Failure: %d, %s.\r\n", Result, ERR_ConvertErrorCodeToString(Result));
                    ret_val = FUNCTION_ERROR;
                }

                /* Flag that there is no longer a current Authentication    */
                /* procedure in progress.                                   */
                ASSIGN_BD_ADDR(mCurrentRemoteBD_ADDR, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00);
            }
            else
            {
                /* One or more of the necessary parameters is/are invalid.  */
                BOT_NOTIFY_ERROR("Usage: UserConfirmationResponse [Confirmation (0 = No, 1 = Yes)].\r\n");
                ret_val = INVALID_PARAMETERS_ERROR;
            }
        }
        else
        {
            /* There is not currently an on-going authentication operation,*/
            /* inform the user of this error condition.                    */
            BOT_NOTIFY_ERROR("Unable to issue User Confirmation Authentication Response: Authentication is not currently in progress.\r\n");
            ret_val = FUNCTION_ERROR;
        }
    }
    else
    {
        /* Not Initialized, flag an error.                                */
        BOT_NOTIFY_ERROR("Platform Manager has not been Initialized.\r\n");
        ret_val = NOT_INITIALIZED_ERROR;
    }

    return ret_val;
}

/* The following function is responsible for changing the Secure     */
/* Simple Pairing Parameters that are exchanged during the Pairing   */
/* procedure when Secure Simple Pairing (Security Level 4) is used.  */
/* This function returns zero on successful execution and a negative */
/* value on all errors.                                              */
int GattSrv::ChangeSimplePairingParameters(ParameterList_t *aParams __attribute__ ((unused)))
{
    int ret_val = UNDEFINED_ERROR;

    /* First, check to make sure that we have already been Initialized.  */
    if (mInitialized)
    {
        /* Make sure that all of the parameters required for this function*/
        /* appear to be at least semi-valid.                              */
        if ((aParams) && (aParams->NumberofParameters >= 2) && (aParams->Params[0].intParam <= 3))
        {
            /* Parameters appear to be valid, map the specified parameters */
            /* into the API specific parameters.                           */
            if (aParams->Params[0].intParam == 0)
                mIOCapability = icDisplayOnly;
            else
            {
                if (aParams->Params[0].intParam == 1)
                    mIOCapability = icDisplayYesNo;
                else
                {
                    if (aParams->Params[0].intParam == 2)
                        mIOCapability = icKeyboardOnly;
                    else
                        mIOCapability = icNoInputNoOutput;
                }
            }

            /* Finally map the Man in the Middle (MITM) Protection valud.  */
            mMITMProtection = (Boolean_t)(aParams->Params[1].intParam?TRUE:FALSE);

            /* Inform the user of the New I/O Capablities.                 */
            BOT_NOTIFY_INFO("Current I/O Capabilities: %s, MITM Protection: %s.\r\n", IOCapabilitiesStrings[(unsigned int)mIOCapability], mMITMProtection?"TRUE":"FALSE");

            /* Flag success to the caller.                                 */
            ret_val = NO_ERROR;
        }
        else
        {
            /* One or more of the necessary parameters is/are invalid.     */
            BOT_NOTIFY_ERROR("Usage: ChangeSimplePairingParameters [I/O Capability (0 = Display Only, 1 = Display Yes/No, 2 = Keyboard Only, 3 = No Input/Output)] [MITM Requirement (0 = No, 1 = Yes)].\r\n");
            ret_val = INVALID_PARAMETERS_ERROR;
        }
    }
    else
    {
        /* Not Initialized, flag an error.                                */
        BOT_NOTIFY_ERROR("Platform Manager has not been Initialized.\r\n");
        ret_val = NOT_INITIALIZED_ERROR;
    }

    return ret_val;
}

/* The following function is responsible for changing the Secure     */
/* Simple Pairing Parameters that are exchanged during the Pairing   */
/* procedure when Secure Simple Pairing (Security Level 4) is used.  */
/* This function returns zero on successful execution and a negative */
/* value on all errors.                                              */
int GattSrv::ChangeLEPairingParameters(ParameterList_t *aParams __attribute__ ((unused)))
{
    int ret_val = NO_ERROR;

    /* First, check to make sure that we have already been Initialized.  */
    if (mInitialized)
    {
        /* Make sure that at least 5 of the parameters required for this  */
        /* function appear to be at least semi-valid.                     */
        /* Note: Minimum 5 parameters are mandatory, when more parameters */
        /* are in use, they will change the default settings.             */
        if ((aParams) && (aParams->NumberofParameters >= 5)
                /*licDisplayOnly = 0 - unsigned int always >= licDisplayOnly*/
                && (/*(aParams->Params[0].intParam >= licDisplayOnly) && */ (aParams->Params[0].intParam <= licKeyboardDisplay))
                /*lbtNoBonding = 0 - unsigned int always >= licDisplayOnly*/
                && (/*(aParams->Params[1].intParam >= lbtNoBonding) && */ (aParams->Params[1].intParam <= lbtBonding))
                && ((aParams->Params[2].intParam == 0) || (aParams->Params[2].intParam == 1))
                && ((aParams->Params[3].intParam == 0) || (aParams->Params[3].intParam == 1))
                && ((aParams->Params[4].intParam == 0) || (aParams->Params[4].intParam == 1)))
        {
            /* When using more than 5 parameters, the 6th parameter will be*/
            /* set to the user input.                                      */
            if (aParams->NumberofParameters >= 6)
            {
                if ((aParams->Params[5].intParam == 0) || (aParams->Params[5].intParam == 1))
                {
                    /* Set the OOB support value in case it was asked      */
                    mOOBSupport = (Boolean_t)(aParams->Params[5].intParam?TRUE:FALSE);
                }
                else
                {
                    ret_val = INVALID_PARAMETERS_ERROR;
                }
            }
            else
            {
                mOOBSupport = (Boolean_t)FALSE;
            }

            /* When using 7 parameters, the 7th parameter will be set to   */
            /* the user input.                                             */
            if (aParams->NumberofParameters == 7)
            {
                if ((aParams->Params[6].intParam == 0) || (aParams->Params[6].intParam == 1))
                {
                    /* Set the mKeypress value in case it was asked         */
                    mKeypress = (Boolean_t)(aParams->Params[6].intParam?TRUE:FALSE);
                }
                else
                {
                    ret_val = INVALID_PARAMETERS_ERROR;
                }
            }
            else
            {
                mKeypress = (Boolean_t)FALSE;
            }

            if (!ret_val)
            {
                /* Parameters appear to be valid, map the specified parameters */
                /* into the API specific parameters.                           */
                mLEIOCapability = (GAP_LE_IO_Capability_t)aParams->Params[0].intParam;

                /* Parameters appear to be valid, map the specified parameters */
                /* into the API specific parameters.                           */
                mBondingType= (GAP_LE_Bonding_Type_t)aParams->Params[1].intParam;

                /* Finally map the Man in the Middle (MITM) Protection value.  */
                mLEMITMProtection = (Boolean_t)(aParams->Params[2].intParam?TRUE:FALSE);
                /* Set the mSC value in case it was asked                       */
                mSC = (Boolean_t)(aParams->Params[3].intParam?TRUE:FALSE);
                /* Set the P256 Debug mode in case it was asked                */
                mP256DebugMode = (Boolean_t)(aParams->Params[4].intParam?TRUE:FALSE);

                /* In case the requested configuration is P256 debug set to    */
                /* TRUE,and Secure Connections (SC) set to FALSE. return error.*/
                if ( (mP256DebugMode == TRUE) && (mSC == FALSE) )
                {
                    BOT_NOTIFY_ERROR("P256 debug mode is relevant to only in case of SC.\r\n");
                    return INVALID_PARAMETERS_ERROR;
                }

                /* The Mode was changed successfully.                          */
                BOT_NOTIFY_INFO("Current I/O Capabilities %d: %s\r\n", mLEIOCapability,
                                    ((mLEIOCapability >= 0) && (mLEIOCapability < IOCAPABILITIESSTRINGS_SIZE)) ? IOCapabilitiesStrings[mLEIOCapability] : "Wrong Value!");

                /* The Mode was changed successfully.                          */
                BOT_NOTIFY_INFO("Bonding Mode %d: %s\r\n", mBondingType,
                       (lbtNoBonding == mBondingType) ? "No Bonding" : (lbtBonding == mBondingType) ? "Bonding" : "Wrong Value!");

                /* Inform the user of the New I/O Capablities.                  */
                BOT_NOTIFY_INFO("MITM Protection: %s\r\nSC: %s\r\nmP256DebugMode %s\r\nmOOBSupport %s\r\nmKeypress %s.\r\n",\
                       mLEMITMProtection?"TRUE":"FALSE", mSC?"TRUE":"FALSE",\
                       mP256DebugMode?"TRUE":"FALSE",mOOBSupport?"TRUE":"FALSE", mKeypress?"TRUE":"FALSE");

                if (mP256DebugMode)
                {
                    /* In case the user asked to use P256 debug mode,           */
                    /* let him know that this is a security breach              */
                    BOT_NOTIFY_WARNING("\nYou asked for P256 Debug Mode, please be aware that the link is not secured!\r\n");
                }
            }
        }
        else
        {
            ret_val = INVALID_PARAMETERS_ERROR;
        }

        if (ret_val)
        {
            /* One or more of the necessary parameters is/are invalid.     */
            BOT_NOTIFY_ERROR("Usage: ChangeLEPairingParameters:\r\n");
            BOT_NOTIFY_ERROR("       [I/O Capability (0 = Display Only, 1 = Display Yes/No, 2 = Keyboard Only, \r\n");
            BOT_NOTIFY_ERROR("	                     3 = No Input/Output, 4 = Keyboard Display )] \r\n");
            BOT_NOTIFY_ERROR("       [Bonding Type (0 = No Bonding, 1 = Bonding)]\r\n");
            BOT_NOTIFY_ERROR("       [MITM Requirement (0 = No, 1 = Yes)]\r\n");
            BOT_NOTIFY_ERROR("       [mSC Enable (0 = No, 1 = Yes)] \r\n");
            BOT_NOTIFY_ERROR("       [mP256DebugMode (0 = No, 1 = Yes)]\r\n");
            BOT_NOTIFY_ERROR("       [mOOBSupport (0 = No - Default, 1 = Yes)]\r\n");
            BOT_NOTIFY_ERROR("       [mKeypress (0 = No - Default, 1 = Yes)].\r\n");
        }
    }
    else
    {
        /* Not Initialized, flag an error.                                */
        BOT_NOTIFY_ERROR("Platform Manager has not been Initialized.\r\n");
        ret_val = NOT_INITIALIZED_ERROR;
    }

    return ret_val;
}


/* The following function is responsible for Registering a Local     */
/* Generic Attribute Profile Manager Callback with the Bluetopia     */
/* Platform Manager Framework.  This function returns zero if        */
/* successful and a negative value if an error occurred.             */
int GattSrv::RegisterGATMEventCallback(ParameterList_t *aParams __attribute__ ((unused)))
{
    int Result;
    int ret_val = UNDEFINED_ERROR;

    /* First, check to make sure that we have already been Initialized.  */
    if (mInitialized)
    {
        /* If there is an Event Callback Registered, then we need to flag */
        /* an error.                                                      */
        if (!mGATMCallbackID)
        {
            /* Callback has not been registered, go ahead and attempt to   */
            /* register it.                                                */
            if ((Result = GATM_RegisterEventCallback(GATM_Event_Callback, NULL)) > 0)
            {
                BOT_NOTIFY_INFO("GATM_RegisterEventCallback() Success: %d.\r\n", Result);

                /* Note the Callback ID and flag success.                   */
                mGATMCallbackID = (unsigned int)Result;

                ret_val = NO_ERROR;
            }
            else
            {
                /* Error registering the Callback, inform user and flag an  */
                /* error.                                                   */
                BOT_NOTIFY_ERROR("GATM_RegisterEventCallback() Failure: %d, %s.\r\n", Result, ERR_ConvertErrorCodeToString(Result));
                ret_val = FUNCTION_ERROR;
            }
        }
        else
        {
            /* Callback already registered, go ahead and notify the user.  */
            BOT_NOTIFY_ERROR("Generic Attribute Profile Manager Event Callback already registered.\r\n");
            ret_val = FUNCTION_ERROR;
        }
    }
    else
    {
        /* Not Initialized, flag an error.                                */
        BOT_NOTIFY_ERROR("Platform Manager has not been Initialized.\r\n");
        ret_val = NOT_INITIALIZED_ERROR;
    }

    return ret_val;
}

/* The following function is responsible for Un-Registering a Local  */
/* Generic Attribute Profile Manager Callback that has previously    */
/* been registered with the Bluetopia Platform Manager Framework.    */
/* This function returns zero if successful and a negative value if  */
/* an error occurred.                                                */
int GattSrv::UnRegisterGATMEventCallback(ParameterList_t *aParams __attribute__ ((unused)))
{
    int ret_val = UNDEFINED_ERROR;

    /* First, check to make sure that we have already been Initialized.  */
    if (mInitialized)
    {
        /* Next, check to make sure that there is an Event Callback       */
        /* already registered.                                            */
        if (mGATMCallbackID)
        {
            /* Callback has been registered, go ahead and attempt to       */
            /* un-register it.                                             */
            if (!(ret_val = GATM_UnRegisterEventCallback(mGATMCallbackID)))
            {
                BOT_NOTIFY_INFO("GATM_UnRegisterEventCallback() Success.\r\n");

                /* Flag that there is no longer a Callback registered.      */
                mGATMCallbackID = 0;
            }
            else
            {
                /* Error un-registering the Callback, inform user and flag  */
                /* an error.                                                */
                BOT_NOTIFY_ERROR("GATM_UnRegisterEventCallback() Failure: %d, %s.\r\n", ret_val, ERR_ConvertErrorCodeToString(ret_val));
                ret_val = FUNCTION_ERROR;
            }
        }
        else
        {
            /* Callback already registered, go ahead and notify the user.  */
            BOT_NOTIFY_ERROR("Generic Attribute Profile Manager Event Callback is not registered.\r\n");
            ret_val = FUNCTION_ERROR;
        }
    }
    else
    {
        /* Not Initialized, flag an error.                                */
        BOT_NOTIFY_ERROR("Platform Manager has not been Initialized.\r\n");
        ret_val = NOT_INITIALIZED_ERROR;
    }

    return ret_val;
}

/* The following function is responsible for querying the Current    */
/* active GATT connections.  This function returns zero if successful*/
/* and a negative value if an error occurred.                        */
int GattSrv::GATTQueryConnectedDevices(ParameterList_t *aParams __attribute__ ((unused)))
{
    int                            Result;
    int                            ret_val = UNDEFINED_ERROR;
    char                           Buffer[32];
    int                            Index;
    unsigned int                   TotalConnected;
    GATM_Connection_Information_t *ConnectionList;

    /* First, check to make sure that we have already been Initialized.  */
    if (mInitialized)
    {
        /* First determine the total number of connected devices.         */
        if (!(Result = GATM_QueryConnectedDevices(0, NULL, &TotalConnected)))
        {
            BOT_NOTIFY_DEBUG("Number of GATT Connections: %u.\r\n\r\n", TotalConnected);
            ret_val = NO_ERROR;

            if (TotalConnected)
            {
                if ((ConnectionList = (GATM_Connection_Information_t *) BTPS_AllocateMemory(TotalConnected*sizeof(GATM_Connection_Information_t))) != NULL)
                {
                    /* Finally attempt to query the number of gatt           */
                    /* connections.                                          */
                    if ((Result = GATM_QueryConnectedDevices(TotalConnected, ConnectionList, NULL)) >= 0)
                    {
                        for (Index=0;Index<Result;Index++)
                        {
                            BOT_NOTIFY_DEBUG("%u: Connection Type: %s.\r\n", Index+1, ((ConnectionList[Index].ConnectionType == gctLE)?"LE":"BR/EDR"));
                            BD_ADDRToStr(ConnectionList[Index].RemoteDeviceAddress, Buffer);
                            BOT_NOTIFY_DEBUG("%u: BD_ADDR:         %s.\r\n", Index+1, Buffer);
                        }

                        BOT_NOTIFY_DEBUG("\r\n");

                        /* Finally return success to the caller.              */
                        ret_val = NO_ERROR;
                    }
                    else
                    {
                        /* Error querying number of connected devices, inform */
                        /* the user and flag an error.                        */
                        BOT_NOTIFY_ERROR("GATM_QueryConnectedDevices() Failure: %d, %s.\r\n", Result, ERR_ConvertErrorCodeToString(Result));
                        ret_val = FUNCTION_ERROR;
                    }

                    /* Free the memory that was previously allocated.        */
                    BTPS_FreeMemory(ConnectionList);
                }
                else
                {
                    BOT_NOTIFY_ERROR("Error allocating memory for connection list.\r\n");
                    ret_val = FUNCTION_ERROR;
                }
            }
        }
        else
        {
            /* Error querying number of connected devices, inform the user */
            /* and flag an error.                                          */
            BOT_NOTIFY_ERROR("GATM_QueryConnectedDevices() Failure: %d, %s.\r\n", Result, ERR_ConvertErrorCodeToString(Result));
            ret_val = FUNCTION_ERROR;
        }
    }
    else
    {
        /* Not Initialized, flag an error.                                */
        BOT_NOTIFY_ERROR("Platform Manager has not been Initialized.\r\n");
        ret_val = NOT_INITIALIZED_ERROR;
    }

    return ret_val;
}

/* The following function is responsible for starting an advertising */
/* process.  This function returns zero if successful and a negative */
/* value if an error occurred.                                       */
int GattSrv::StartAdvertising(ParameterList_t *aParams __attribute__ ((unused)))
{
    int                              ret_val = NO_ERROR;
    DEVM_Advertising_Information_t   AdvertisingInfo;

    /* First, check to make sure that we have already been Initialized.  */
    if (mInitialized)
    {
        /* Make sure that all of the parameters required for this function*/
        /* appear to be at least semi-valid.                              */
        if ((aParams) && (aParams->NumberofParameters >= 2) && (aParams->Params[1].intParam))
        {
            /* Format the Advertising Information.                         */
            BTPS_MemInitialize(&AdvertisingInfo, 0, sizeof(DEVM_Advertising_Information_t));

            AdvertisingInfo.AdvertisingFlags    = aParams->Params[0].intParam;
            AdvertisingInfo.AdvertisingDuration = aParams->Params[1].intParam;

            /* Make sure that BD_ADDR input exists when Directed Connection*/
            /* is being used.                                              */
            if (aParams->Params[0].intParam >= DIRECT_CONNECTABLE_MODE)
            {
                if ((aParams->NumberofParameters == 3) && (strlen(aParams->Params[2].strParam) == sizeof(BD_ADDR_t)*2))
                {
                    /* Convert the parameter to a Bluetooth Device Address.*/
                    StrToBD_ADDR(aParams->Params[2].strParam, &(AdvertisingInfo.Peer_BD_ADDR));
                }
                else
                {
                    ret_val = INVALID_PARAMETERS_ERROR;
                }
            }
        }
        else
        {
            ret_val = INVALID_PARAMETERS_ERROR;
        }

        if (!ret_val)
        {
            /* Submit the Start Advertising Command.                       */
            if ((ret_val = DEVM_StartAdvertising(&AdvertisingInfo)) == 0)
            {
                /* When using High Duty cycle mode, the advertise will work */
                /* Only for 1.28 Seconds.									*/
                if ((aParams->Params[0].intParam & DIRECT_CONNECTABLE_MODE) &&
                        (!(aParams->Params[0].intParam & LOW_DUTY_CYCLE_DIRECT_CONNECTABLE)))
                {
                    BOT_NOTIFY_INFO("DEVM_StartAdvertising() Success: Duration 1.28 seconds.\r\n");
                }
                else
                {
                    BOT_NOTIFY_INFO("DEVM_StartAdvertising() Success: Duration %lu seconds.\r\n", AdvertisingInfo.AdvertisingDuration);
                }
            }
            else
            {
                /* Error Connecting With Remote Device, inform the user and */
                /* flag an error.                                           */
                BOT_NOTIFY_ERROR("DEVM_StartAdvertising() Failure: %d, %s.\r\n", ret_val, ERR_ConvertErrorCodeToString(ret_val));
                ret_val = FUNCTION_ERROR;
            }
        }
        else
        {
            /* One or more of the necessary parameters is/are invalid.     */
            BOT_NOTIFY_ERROR("Usage: StartAdvertising [Flags] [Duration] [BD ADDR].\r\n");
            BOT_NOTIFY_ERROR("where Flags is a bitmask in the following table\r\n");
            BOT_NOTIFY_ERROR("Bitmask Number - [Bit is set]           [Bit is off]\r\n");
            BOT_NOTIFY_ERROR("    0x00000001 - [Local Public Address] [Local Random Address]\r\n");
            BOT_NOTIFY_ERROR("    0x00000002 - [Discoverable]         [Non Discoverable]\r\n");
            BOT_NOTIFY_ERROR("    0x00000004 - [Connectable]          [Non Connectable]\r\n");
            BOT_NOTIFY_ERROR("    0x00000010 - [Advertise Name]       [Advertise Name off]\r\n");
            BOT_NOTIFY_ERROR("    0x00000020 - [Advertise Tx Power]   [Advertise Tx Power off]\r\n");
            BOT_NOTIFY_ERROR("    0x00000040 - [Advertise Appearance] [Advertise Appearance off]\r\n");
            BOT_NOTIFY_ERROR("    0x00000100 - [Peer Public Address]  [Peer Random Address]\r\n");
            BOT_NOTIFY_ERROR("\r\nWhen Connectable bit (0x0004) is set:\r\n");
            BOT_NOTIFY_ERROR("    0x00000200 - [Direct Connectable]   [Undirect Connectable]\r\n");
            BOT_NOTIFY_ERROR("\r\nWhen Direct Connectable bit (0x0200) is set:\r\n");
            BOT_NOTIFY_ERROR("    0x00000400 - [Low Duty Cycle]       [High Duty Cycle]\r\n");
            BOT_NOTIFY_ERROR("\r\nNote: BD ADDR is needed only when Direct Connectable flags is in use.\r\n\r\n");

            ret_val = INVALID_PARAMETERS_ERROR;
        }
    }
    else
    {
        /* Not Initialized, flag an error.                                */
        BOT_NOTIFY_ERROR("Platform Manager has not been Initialized.\r\n");
        ret_val = NOT_INITIALIZED_ERROR;
    }

    return ret_val;
}

/* The following function is responsible for stopping an advertising */
/* process.  This function returns zero if successful and a negative */
/* value if an error occurred.                                       */
int GattSrv::StopAdvertising(ParameterList_t *aParams __attribute__ ((unused)))
{
    int ret_val = UNDEFINED_ERROR;

    /* First, check to make sure that we have already been Initialized.  */
    if (mInitialized)
    {
        /* Make sure that all of the parameters required for this function*/
        /* appear to be at least semi-valid.                              */
        if ((aParams) && (aParams->NumberofParameters >= 1))
        {
            /* Submit the Stop Advertising Command.                        */
            if ((ret_val = DEVM_StopAdvertising(aParams->Params[0].intParam)) == 0)
            {
                BOT_NOTIFY_INFO("DEVM_StopAdvertising() Success.\r\n");
            }
            else
            {
                /* Error Connecting With Remote Device, inform the user and */
                /* flag an error.                                           */
                BOT_NOTIFY_ERROR("DEVM_StopAdvertising() Failure: %d, %s.\r\n", ret_val, ERR_ConvertErrorCodeToString(ret_val));
                ret_val = FUNCTION_ERROR;
            }
        }
        else
        {
            /* One or more of the necessary parameters is/are invalid.     */
            BOT_NOTIFY_ERROR("Usage: StopAdvertising [Flags].\r\n");
            BOT_NOTIFY_ERROR("where Flags bitmask of\r\n");
            BOT_NOTIFY_ERROR("    0x00000001 = Force advertising stop\r\n");
            ret_val = INVALID_PARAMETERS_ERROR;
        }
    }
    else
    {
        /* Not Initialized, flag an error.                                */
        BOT_NOTIFY_ERROR("Platform Manager has not been Initialized.\r\n");
        ret_val = NOT_INITIALIZED_ERROR;
    }

    return ret_val;
}

/* The following function is provided to allow a mechanism for local */
/* modules to send Set Authenticated Payload Timeout. Used in order 	*/
/* to change the Authenticated payload timeout of the ping. The ping */
/* starts working automatically after finishing the pairing          */
/* procedure. This function returns zero if successful, or a negative*/
/* return error code if there was an error.                          */
int GattSrv::SetAuthenticatedPayloadTimeout(ParameterList_t *aParams __attribute__ ((unused)))
{
    int           ret_val = UNDEFINED_ERROR;
    BD_ADDR_t     BD_ADDR;
    Word_t        PayloadTimeout;

    /* First, check to make sure that we have already been Initialized.  */
    if (mInitialized)
    {
        /* Make sure that all of the parameters required for this function*/
        /* appear to be at least semi-valid.                              */
        if ((aParams) && (aParams->NumberofParameters>=2))
        {
            /* Convert the parameter to a Bluetooth Device Address.        */
            StrToBD_ADDR(aParams->Params[0].strParam, &BD_ADDR);

            /* Copy the timeout from user input to the local parameter.    */
            PayloadTimeout = (Word_t)aParams->Params[1].intParam;

            ret_val = DEVM_Set_Authenticated_Payload_Timeout(BD_ADDR, PayloadTimeout);

            if (ret_val >= 0)
            {
                BOT_NOTIFY_INFO("DEVM_SetAuthenticatedPayloadTimeout() Success.\r\n");
                ret_val = NO_ERROR;
            }
            else
            {
                BOT_NOTIFY_ERROR("DEVM_SetAuthenticatedPayloadTimeout() Error: %d.\r\n",ret_val);
            }
        }
        else
        {
            /* One or more of the necessary parameters is/are invalid.     */
            BOT_NOTIFY_ERROR("Usage: SetAuthenticatedPayloadTimeout [BD_ADDR] [Authenticated Payload Timout (In ms)].\r\n");
            ret_val = INVALID_PARAMETERS_ERROR;
        }
    }
    else
    {
        /* Not Initialized, flag an error.                                */
        BOT_NOTIFY_ERROR("Platform Manager has not been Initialized.\r\n");
        ret_val = NOT_INITIALIZED_ERROR;
    }

    return ret_val;
}

/* The following function is provided to allow a mechanism for local */
/* modules to send Query Authenticated Payload Timeout. Used in order*/
/* to query the Authenticated payload timeout of the ping.           */
/* This function returns zero if successful, or a negative return    */
/* error code if there was an error.                                 */
int GattSrv::QueryAuthenticatedPayloadTimeout(ParameterList_t *aParams __attribute__ ((unused)))
{
    int           ret_val = UNDEFINED_ERROR;
    BD_ADDR_t     BD_ADDR;
    Word_t        PayloadTimeout;

    /* First, check to make sure that we have already been Initialized.  */
    if (mInitialized)
    {
        /* Make sure that all of the parameters required for this function*/
        /* appear to be at least semi-valid.                              */
        if ((aParams) && (aParams->NumberofParameters>=1))
        {
            /* Convert the parameter to a Bluetooth Device Address.        */
            StrToBD_ADDR(aParams->Params[0].strParam, &BD_ADDR);

            ret_val = DEVM_Query_Authenticated_Payload_Timeout(BD_ADDR, &PayloadTimeout);

            if (ret_val >= 0)
            {
                BOT_NOTIFY_DEBUG("Authenticated Payload Timeout: %d ms.\r\n", PayloadTimeout);
                ret_val = NO_ERROR;
            }
            else
            {
                BOT_NOTIFY_ERROR("DEVM_Query_Authenticated_Payload_Timeout() Error: %d.\r\n", ret_val);
            }
        }
        else
        {
            /* One or more of the necessary parameters is/are invalid.     */
            BOT_NOTIFY_ERROR("Usage: QueryAuthenticatedPayloadTimeout [BD_ADDR].\r\n");
            ret_val = INVALID_PARAMETERS_ERROR;
        }
    }
    else
    {
        /* Not Initialized, flag an error.                                */
        BOT_NOTIFY_ERROR("Platform Manager has not been Initialized.\r\n");
        ret_val = NOT_INITIALIZED_ERROR;
    }

    return ret_val;
}

/* The following function is responsible for setting the advertising */
/* intervals for the local device.                      				*/
/* This function returns zero if successful, or a negative return    */
/* error code if there was an error.                                 */
int GattSrv::SetAdvertisingInterval(ParameterList_t *aParams __attribute__ ((unused)))
{
    int           ret_val = UNDEFINED_ERROR;
    Word_t        Advertising_Interval_Min;
    Word_t        Advertising_Interval_Max;

    /* First, check to make sure that we have already been Initialized.  */
    if (mInitialized)
    {
        /* Make sure that all of the parameters required for this function*/
        /* appear to be at least semi-valid.                              */
        if ((aParams) && (aParams->NumberofParameters > 1))
        {
            Advertising_Interval_Min = aParams->Params[0].intParam;
            Advertising_Interval_Max = aParams->Params[1].intParam;
            ret_val = DEVM_Set_Advertising_Intervals(Advertising_Interval_Min, Advertising_Interval_Max);

            if (ret_val >= 0)
            {
                BOT_NOTIFY_INFO("Set advertising intervals success, Parameters stored.\r\n");
                ret_val = NO_ERROR;
            }
            else
            {
                BOT_NOTIFY_ERROR("DEVM_Set_Advertising_Intervals() Error: %d.\r\n",ret_val);
            }
        }
        else
        {
            /* One or more of the necessary parameters is/are invalid.     */
            BOT_NOTIFY_ERROR("Usage: SetAdvertisingInterval [Advertising Interval Min] [Advertising Interval Max] (Range: 20..10240 in ms).\r\n");
            ret_val = INVALID_PARAMETERS_ERROR;
        }
    }
    else
    {
        /* Not Initialized, flag an error.                                */
        BOT_NOTIFY_ERROR("Platform Manager has not been Initialized.\r\n");
        ret_val = NOT_INITIALIZED_ERROR;
    }

    return ret_val;
}

/* The following function is responsible for setting the connection  */
/* and scan parameters for the local device and change the default   */
/* parameters. This function returns zero if successful, or a        */
/* negative return error code if there was an error.                 */
int GattSrv::SetAndUpdateConnectionAndScanBLEParameters(ParameterList_t *aParams __attribute__ ((unused)))
{
    int                            ret_val = UNDEFINED_ERROR;
    BD_ADDR_t                      BD_ADDR;
    AddressType_t                  AddressType;
    GAP_LE_Connection_Parameters_t ConnectionParameters;
    Word_t                         ConnectionScanInterval;
    Word_t                         ConnectionScanWindow;
    Word_t                         ScanInterval;
    Word_t                         ScanWindow;

    /* First, check to make sure that we have already been Initialized. */
    if (mInitialized)
    {
        /* Make sure that all of the parameters required for this       */
        /* function appear to be at least semi-valid.                   */
        if ((aParams) && (aParams->NumberofParameters == 10))
        {

            /* Convert the parameter to a Bluetooth Device Address.     */
            StrToBD_ADDR(aParams->Params[0].strParam, &BD_ADDR);

            AddressType = (AddressType_t)aParams->Params[1].intParam;

            /* Set the LE Intervals and test them.                      */
            ConnectionParameters.Connection_Interval_Min    = aParams->Params[2].intParam;
            ConnectionParameters.Connection_Interval_Max    = aParams->Params[3].intParam;
            ConnectionParameters.Slave_Latency              = aParams->Params[4].intParam;
            ConnectionParameters.Supervision_Timeout        = aParams->Params[5].intParam;
            ConnectionScanInterval                          = aParams->Params[6].intParam;
            ConnectionScanWindow                            = aParams->Params[7].intParam;
            ScanInterval                                    = aParams->Params[8].intParam;
            ScanWindow                                      = aParams->Params[9].intParam;

            ret_val = DEVM_Set_And_Update_Connection_And_Scan_Parameters(&BD_ADDR, &AddressType, &ConnectionParameters,
                                                                         &ConnectionScanInterval, &ConnectionScanWindow,
                                                                         &ScanInterval, &ScanWindow);

            if (ret_val == 0)
            {
                BOT_NOTIFY_INFO("Set and update connection parameters success, Default parameters changed and updated.\r\n");
                ret_val = NO_ERROR;
            }
            else if ((ret_val == BTPM_ERROR_CODE_UNKNOWN_BLUETOOTH_DEVICE) || (ret_val == BTPM_ERROR_CODE_LOW_ENERGY_NOT_CONNECTED))
            {
                BOT_NOTIFY_ERROR("Only Default parameters stored, Bluetooth Device doesn't exists Error: %d.\r\n",ret_val);
                ret_val = FUNCTION_ERROR;
            }
            else
            {
                BOT_NOTIFY_ERROR("DEVM_Set_And_Update_Connection_Parameters() Error: %d.\r\n",ret_val);
            }
        }
        else
        {
            /* One or more of the necessary parameters is/are invalid.  */
            BOT_NOTIFY_ERROR("Usage: SetAndUpdateConnectionAndScanBLEParameters\r\n");
            BOT_NOTIFY_ERROR("       [Remote BD ADDR] [Address Type] (0 - Public, 1 - Static, 2 - Private Resolvable,\r\n");
            BOT_NOTIFY_ERROR("                                        3 - Private NonResolvable)\r\n");
            BOT_NOTIFY_ERROR("       [Connection_Interval_Min] (Range: 10..4000 in ms) [Connection_Interval_Max] (Range: 20..4000 in ms)\r\n");
            BOT_NOTIFY_ERROR("       [Slave_Latency] (Range: 0..500 in ms)[Supervision_Timeout] (Range: 100..32000 in ms)\r\n");
            BOT_NOTIFY_ERROR("       [ConnectionScanInterval] [ConnectionScanWindow] (Range: 3..10240 in ms)\r\n");
            BOT_NOTIFY_ERROR("       [ScanInterval] [ScanWindow] (Range: 3..10240 in ms)\r\n");
            BOT_NOTIFY_ERROR("SetAndUpdateConnectionAndScnBLEParameters default parameters: 20 40 0 5000 100 100 20 60\r\n");
            ret_val = INVALID_PARAMETERS_ERROR;
        }
    }
    else
    {
        /* Not Initialized, flag an error.                              */
        BOT_NOTIFY_ERROR("Platform Manager has not been Initialized.\r\n");
        ret_val = NOT_INITIALIZED_ERROR;
    }

    return ret_val;
}

/* The following function is a utility function which is used to     */
/* register the specified service.                                   */
int GattSrv::RegisterService(unsigned int ServiceIndex)
{
    int                            ret_val = UNDEFINED_ERROR;
    int                            Result;
    Boolean_t                      PrimaryService;
    GATT_UUID_t                    UUID;
    unsigned int                   Index;
    unsigned int                   NumberOfAttributes;
    DescriptorInfo_t              *DescriptorInfo;
    CharacteristicInfo_t          *CharacteristicInfo;
    GATT_Attribute_Handle_Group_t  ServiceHandleRangeResult;
    GATT_Attribute_Handle_Group_t  ServiceHandleRange;

    /* Verify that the input parameter is semi-valid.                    */
    if (!mServiceTable)
    {
        BOT_NOTIFY_ERROR("Error: Service Table was not initialized.\r\n");
        ret_val = NOT_INITIALIZED_ERROR;

    }
    else if (ServiceIndex < mServiceCount)
    {
        /* Verify that a GATM Event Callback is registered.               */
        if (mGATMCallbackID)
        {
            /* Verify that the service is not already registered.          */
            if (mServiceTable[ServiceIndex].ServiceID == 0)
            {
                /* Calculate the Number of Attributes needed for this       */
                /* service.                                                 */
                NumberOfAttributes = CalculateNumberAttributes(&(mServiceTable[ServiceIndex]));

                /* Check to see if we need to register a persistent UID.    */
                if ((mServiceTable[ServiceIndex].Flags & SERVICE_TABLE_FLAGS_USE_PERSISTENT_UID) && (mServiceTable[ServiceIndex].PersistentUID == 0))
                {
                    /* Attempt to register a persistent UID.                 */
                    ret_val = GATM_RegisterPersistentUID(NumberOfAttributes, &(mServiceTable[ServiceIndex].PersistentUID), &ServiceHandleRangeResult);
                    if (!ret_val)
                    {
                        BOT_NOTIFY_DEBUG("Registered Persistent UID: 0x%08X.\r\n", (unsigned int)mServiceTable[ServiceIndex].PersistentUID);
                        BOT_NOTIFY_DEBUG("Start Handle:              0x%04X.\r\n", ServiceHandleRangeResult.Starting_Handle);
                        BOT_NOTIFY_DEBUG("End Handle:                0x%04X.\r\n", ServiceHandleRangeResult.Ending_Handle);
                    }
                    else
                    {
                        BOT_NOTIFY_ERROR("Error - GATM_RegisterPersistentUID() %d, %s\n", ret_val, ERR_ConvertErrorCodeToString(ret_val));
                        ret_val = FUNCTION_ERROR;
                    }
                }
                else
                {
                    ret_val = NO_ERROR;
                }

                /* Continue only if no error has occurred.                  */
                if (!ret_val)
                {
                    /* Determine if this is a primary service.               */
                    PrimaryService     = (Boolean_t)(!(mServiceTable[ServiceIndex].Flags & SERVICE_TABLE_FLAGS_SECONDARY_SERVICE));

                    /* Configure the Service UUID.  Not this sample          */
                    /* application only supports registering 128 bit UUIDs,  */
                    /* however GATM allows other types as well.              */
                    UUID.UUID_Type     = guUUID_128;
                    UUID.UUID.UUID_128 = mServiceTable[ServiceIndex].ServiceUUID;

                    DisplayGATTUUID(&UUID, "Registering Service, UUID", 0);

                    /* Go ahead and attempt to register the service.         */
                    ret_val = GATM_RegisterService(PrimaryService, NumberOfAttributes, &UUID, (mServiceTable[ServiceIndex].PersistentUID?&(mServiceTable[ServiceIndex].PersistentUID):NULL));
                    if (ret_val > 0)
                    {
                        BOT_NOTIFY_INFO(" Registered Service, Service ID: %u.\r\n", (unsigned int)ret_val);
                        /* Save the Service ID.                               */
                        mServiceTable[ServiceIndex].ServiceID = (unsigned int) ret_val;
                        ret_val = NO_ERROR;

                        /* Loop through all of the attributes and register    */
                        /* them.                                              */
                        for (Index=0,ret_val=0;(!ret_val) && (Index<mServiceTable[ServiceIndex].NumberAttributes);Index++)
                        {
                            /* Handle the registration based on the type of    */
                            /* attribute.                                      */
                            switch(mServiceTable[ServiceIndex].AttributeList[Index].AttributeType)
                            {
                            case atInclude:
                                /* We can only register an include in this   */
                                /* application if the previous entry in the  */
                                /* mServiceTable is already registered.  This */
                                /* is a functioning of the application and   */
                                /* not the GATM APIs.                        */
                                if ((ServiceIndex) && (mServiceTable[ServiceIndex-1].ServiceID))
                                {
                                    /* Attempt to an include reference to the */
                                    /* previous service in mServiceTable.      */
                                    ret_val = GATM_AddServiceInclude(0, mServiceTable[ServiceIndex].ServiceID, mServiceTable[ServiceIndex].AttributeList[Index].AttributeOffset, mServiceTable[ServiceIndex-1].ServiceID);
                                    if (!ret_val)
                                    {
                                        BOT_NOTIFY_INFO(" Registered Include to Service ID %u.\r\n", mServiceTable[ServiceIndex-1].ServiceID);
                                        ret_val = NO_ERROR;
                                    }
                                    else
                                    {
                                        BOT_NOTIFY_ERROR("Error - GATM_AddServiceInclude() %d, %s\n", ret_val, ERR_ConvertErrorCodeToString(ret_val));
                                        BOT_NOTIFY_ERROR("Error - ServiceIndex %d, attr idx %d\n", ServiceIndex, Index);
                                        ret_val = FUNCTION_ERROR;
                                    }
                                }
                                break;
                            case atCharacteristic:
                                /* Get a pointer to the characteristic       */
                                /* information.                              */
                                if ((CharacteristicInfo = (CharacteristicInfo_t *)mServiceTable[ServiceIndex].AttributeList[Index].Attribute) != NULL)
                                {
                                    /* Configure the Characteristic UUID.  Not*/
                                    /* this sample application only supports  */
                                    /* registering 128 bit UUIDs, however GATM*/
                                    /* allows other types as well.            */
                                    UUID.UUID_Type     = guUUID_128;
                                    UUID.UUID.UUID_128 = CharacteristicInfo->CharacteristicUUID;

                                    /* Attempt to add this characteristic to  */
                                    /* the table.                             */
                                    ret_val = GATM_AddServiceCharacteristic(mServiceTable[ServiceIndex].ServiceID, mServiceTable[ServiceIndex].AttributeList[Index].AttributeOffset, CharacteristicInfo->CharacteristicPropertiesMask, CharacteristicInfo->SecurityPropertiesMask, &UUID);
                                    if (ret_val == NO_ERROR)
                                    {
                                        DisplayGATTUUID(&UUID, "Registered Characteristic, UUID", 0);
                                    }
                                    else
                                    {
                                        BOT_NOTIFY_ERROR("Error - GATM_AddServiceCharacteristic() %d, %s\n", ret_val, ERR_ConvertErrorCodeToString(ret_val));
                                        BOT_NOTIFY_ERROR("Error - ServiceIndex %d, attr idx %d\n", ServiceIndex, Index);
                                        DisplayGATTUUID(&UUID, "Characteristic UUID", 0);

                                        BOT_NOTIFY_TRACE("mServiceTable[ServiceIndex].ServiceID = %d\n", mServiceTable[ServiceIndex].ServiceID);
                                        BOT_NOTIFY_TRACE("mServiceTable[ServiceIndex].AttributeList[Index].AttributeOffset = 0x%08X \n", mServiceTable[ServiceIndex].AttributeList[Index].AttributeOffset);
                                        BOT_NOTIFY_TRACE("CharacteristicInfo->CharacteristicPropertiesMask = 0x%08X \n", CharacteristicInfo->CharacteristicPropertiesMask);
                                        BOT_NOTIFY_TRACE("CharacteristicInfo->SecurityPropertiesMask = 0x%08X \n", CharacteristicInfo->SecurityPropertiesMask);
                                        ret_val = FUNCTION_ERROR;
                                    }
                                }
                                break;
                            case atDescriptor:
                                /* Get a pointer to the descriptor           */
                                /* information.                              */
                                if ((DescriptorInfo = (DescriptorInfo_t *)mServiceTable[ServiceIndex].AttributeList[Index].Attribute) != NULL)
                                {
                                    /* Configure the Descriptor UUID.  Not    */
                                    /* this sample application only supports  */
                                    /* registering 128 bit UUIDs, however GATM*/
                                    /* allows other types as well.            */
                                    UUID.UUID_Type     = guUUID_128;
                                    UUID.UUID.UUID_128 = DescriptorInfo->CharacteristicUUID;

                                    /* Attempt to add this descriptor to the  */
                                    /* table.                                 */
                                    ret_val = GATM_AddServiceDescriptor(mServiceTable[ServiceIndex].ServiceID, mServiceTable[ServiceIndex].AttributeList[Index].AttributeOffset, DescriptorInfo->DescriptorPropertiesMask, DescriptorInfo->SecurityPropertiesMask, &UUID);
                                    if (!ret_val)
                                        DisplayGATTUUID(&UUID, "Registered Descriptor, UUID", 0);
                                    else
                                    {
                                        BOT_NOTIFY_ERROR("Error - GATM_AddServiceDescriptor() %d, %s\n", ret_val, ERR_ConvertErrorCodeToString(ret_val));
                                        BOT_NOTIFY_ERROR("Error - ServiceIndex %d, attr idx %d\n", ServiceIndex, Index);
                                        ret_val = FUNCTION_ERROR;
                                    }
                                }
                                break;
                            }
                        }

                        /* If no error occurred go ahead and publish the      */
                        /* service.                                           */
                        if (!ret_val)
                        {
                            /* Attempt to register the Service into the GATT   */
                            /* database.                                       */
                            if ((ret_val = GATM_PublishService(mServiceTable[ServiceIndex].ServiceID, mGATMCallbackID, (GATM_SERVICE_FLAGS_SUPPORT_LOW_ENERGY | GATM_SERVICE_FLAGS_SUPPORT_CLASSIC_BLUETOOTH), &ServiceHandleRange)) == 0)
                            {
                                /* Store the Published Handle Range for this    */
                                /* service.                                     */
                                mServiceTable[ServiceIndex].ServiceHandleRange = ServiceHandleRange;

                                BOT_NOTIFY_INFO(" Service Registered.\r\n");
                                BOT_NOTIFY_DEBUG("******************************************************************\r\n");
                                BOT_NOTIFY_DEBUG("   Service Start Handle: 0x%04X\r\n", ServiceHandleRange.Starting_Handle);
                                BOT_NOTIFY_DEBUG("   Service End Handle:   0x%04X\r\n\r\n", ServiceHandleRange.Ending_Handle);

                                /* Print the Handles of all of the attributes in*/
                                /* the table.                                   */
                                for (Index=0;Index<mServiceTable[ServiceIndex].NumberAttributes;Index++)
                                {
                                    switch(mServiceTable[ServiceIndex].AttributeList[Index].AttributeType)
                                    {
                                    case atInclude:
                                        BOT_NOTIFY_DEBUG("Include Attribute @ Handle: 0x%04X\r\n", (ServiceHandleRange.Starting_Handle + mServiceTable[ServiceIndex].AttributeList[Index].AttributeOffset));
                                        break;
                                    case atCharacteristic:
                                        /* Get a pointer to the characteristic */
                                        /* information.                        */
                                        if ((CharacteristicInfo = (CharacteristicInfo_t *)mServiceTable[ServiceIndex].AttributeList[Index].Attribute) != NULL)
                                        {
                                            /* Configure the Characteristic     */
                                            /* UUID.  Not this sample           */
                                            /* application only supports        */
                                            /* registering 128 bit UUIDs,       */
                                            /* however GATM allows other types  */
                                            /* as well.                         */
                                            UUID.UUID_Type     = guUUID_128;
                                            UUID.UUID.UUID_128 = CharacteristicInfo->CharacteristicUUID;
                                            DisplayGATTUUID(&UUID, "Characteristic: ", 0);

                                            BOT_NOTIFY_DEBUG("Characteristic Declaration @ Handle: 0x%04X\r\n", (ServiceHandleRange.Starting_Handle + mServiceTable[ServiceIndex].AttributeList[Index].AttributeOffset));
                                            BOT_NOTIFY_DEBUG("Characteristic Value       @ Handle: 0x%04X\r\n", (ServiceHandleRange.Starting_Handle + mServiceTable[ServiceIndex].AttributeList[Index].AttributeOffset + 1));
                                        }
                                        break;
                                    case atDescriptor:
                                        /* Get a pointer to the descriptor     */
                                        /* information.                        */
                                        if ((DescriptorInfo = (DescriptorInfo_t *)mServiceTable[ServiceIndex].AttributeList[Index].Attribute) != NULL)
                                        {
                                            /* Configure the Descriptor UUID.   */
                                            /* Not this sample application only */
                                            /* supports registering 128 bit     */
                                            /* UUIDs, however GATM allows other */
                                            /* types as well.                   */
                                            UUID.UUID_Type     = guUUID_128;
                                            UUID.UUID.UUID_128 = DescriptorInfo->CharacteristicUUID;
                                            DisplayGATTUUID(&UUID, "Characteristic Descriptor: ", 0);

                                            BOT_NOTIFY_DEBUG("Characteristic Descriptor @ Handle: 0x%04X\r\n", (ServiceHandleRange.Starting_Handle + mServiceTable[ServiceIndex].AttributeList[Index].AttributeOffset));
                                        }
                                        break;
                                    }
                                }

                                BOT_NOTIFY_DEBUG("******************************************************************\r\n");
                            }
                            else
                            {
                                BOT_NOTIFY_ERROR("Error - GATM_PublishService() %d, %s\n", ret_val, ERR_ConvertErrorCodeToString(ret_val));
                                ret_val = FUNCTION_ERROR;
                            }
                        }

                        /* Check to see if we need to delete the registered   */
                        /* service if an error occcurred.                     */
                        if (ret_val)
                        {
                            /* If an error occurred go ahead and delete the    */
                            /* service.                                        */
                            Result = GATM_DeleteService(mServiceTable[ServiceIndex].ServiceID);
                            if (Result)
                                BOT_NOTIFY_WARNING("Error - GATM_DeleteService() %d, %s\n", Result, ERR_ConvertErrorCodeToString(Result));

                            /* Flag that this service is not registered.       */
                            mServiceTable[ServiceIndex].ServiceID = 0;
                        }
                    }
                    else
                    {
                        BOT_NOTIFY_ERROR("Error - GATM_RegisterService() %d, %s\n", ret_val, ERR_ConvertErrorCodeToString(ret_val));
                        ret_val = FUNCTION_ERROR;
                    }
                }
            }
            else
            {
                BOT_NOTIFY_ERROR("Service already registered.\r\n");
                ret_val = FUNCTION_ERROR;
            }
        }
        else
        {
            /* Callback not registered, go ahead and notify the user.      */
            BOT_NOTIFY_ERROR("Generic Attribute Profile Manager Event Callback is not registered.\r\n");
            ret_val = FUNCTION_ERROR;
        }
    }
    else
    {
        BOT_NOTIFY_ERROR("Invalid Service Index %u, Maximum %u.\r\n", ServiceIndex, (mServiceCount-1));
        ret_val = INVALID_PARAMETERS_ERROR;
    }

    return ret_val;
}

/* The following function is a utility function which is used to     */
/* un-register the specified service.                                */
int GattSrv::UnRegisterService(unsigned int ServiceIndex)
{
    int ret_val = UNDEFINED_ERROR;

    /* Verify that the input parameter is semi-valid.                    */
    if (mServiceTable && (ServiceIndex < mServiceCount))
    {
        /* Verify that a GATM Event Callback is registered.               */
        if (mGATMCallbackID)
        {
            /* Verify that the service is already registered.              */
            if (mServiceTable[ServiceIndex].ServiceID)
            {
                /* Go ahead and attempt to delete the service.              */
                if ((ret_val = GATM_DeleteService(mServiceTable[ServiceIndex].ServiceID)) == 0)
                {
                    BOT_NOTIFY_DEBUG("Service ID %u, successfully un-registered.\r\n", mServiceTable[ServiceIndex].ServiceID);

                    mServiceTable[ServiceIndex].ServiceID = 0;
                }
                else
                {
                    BOT_NOTIFY_ERROR("Error - GATM_DeleteService() %d, %s\n", ret_val, ERR_ConvertErrorCodeToString(ret_val));
                    ret_val = FUNCTION_ERROR;
                }
            }
            else
            {
                BOT_NOTIFY_ERROR("Service not registered.\r\n");
                ret_val = FUNCTION_ERROR;
            }
        }
        else
        {
            /* Callback not registered, go ahead and notify the user.      */
            BOT_NOTIFY_ERROR("Generic Attribute Profile Manager Event Callback is not registered.\r\n");
            ret_val = FUNCTION_ERROR;
        }
    }
    else
    {
        BOT_NOTIFY_ERROR("Invalid Service Index %u, Maximum %u.\r\n", ServiceIndex, (mServiceCount-1));
        ret_val = INVALID_PARAMETERS_ERROR;
    }

    return ret_val;
}

/* The following function is a utility function which is used to     */
/* process a GATM Read Request Event.                                */
void GattSrv::ProcessReadRequestEvent(GATM_Read_Request_Event_Data_t *ReadRequestData)
{
    int              Result;
    Byte_t          *Value;
    Byte_t           ErrorCode;
    unsigned int     ValueLength;
    AttributeInfo_t *AttributeInfo;

    /* Verify that the input parameter is semi-valid.                    */
    if (ReadRequestData)
    {
        /* Initialize the error code.                                     */
        ErrorCode = 0;

        /* Wait on access to the Service Info List Mutex.                 */
        if (BTPS_WaitMutex(mServiceMutex, BTPS_INFINITE_WAIT))
        {
            /* Search for the Attribute for this read request (which must  */
            /* be a characteristic or descriptor).                         */
            AttributeInfo = SearchServiceListByOffset(ReadRequestData->ServiceID, ReadRequestData->AttributeOffset);
            if ((AttributeInfo) && ((AttributeInfo->AttributeType == atCharacteristic) || (AttributeInfo->AttributeType == atDescriptor)) && (AttributeInfo->Attribute))
            {
                /* Handle the read based on the type of attribute.          */
                switch(AttributeInfo->AttributeType)
                {
                case atCharacteristic:
                    ValueLength = ((CharacteristicInfo_t *)AttributeInfo->Attribute)->ValueLength;
                    Value       = ((CharacteristicInfo_t *)AttributeInfo->Attribute)->Value;
                    break;
                case atDescriptor:
                    ValueLength = ((DescriptorInfo_t *)AttributeInfo->Attribute)->ValueLength;
                    Value       = ((DescriptorInfo_t *)AttributeInfo->Attribute)->Value;
                    break;
                default:
                    /* Do nothing.                                        */
                    break;
                }

                /* Next check the requested offset of the read versus the   */
                /* length.                                                  */
                if (ReadRequestData->AttributeValueOffset <= ValueLength)
                {
                    /* Verify that the length and pointer is valid.          */
                    if ((ValueLength) && (Value))
                    {
                        /* Calculate the length of the value from the         */
                        /* specified offset.                                  */
                        ValueLength -= ReadRequestData->AttributeValueOffset;

                        /* Get a pointer to the value at the specified offset.*/
                        if (ValueLength)
                            Value     = &(Value[ReadRequestData->AttributeValueOffset]);
                        else
                            Value     = NULL;
                    }
                    else
                        ValueLength = 0;

                    /* Go ahead and respond to the read request.             */
                    if ((Result = GATM_ReadResponse(ReadRequestData->RequestID, ValueLength, Value)) == 0)
                        BOT_NOTIFY_INFO("GATM_ReadResponse() success.\n");
                    else
                        BOT_NOTIFY_WARNING("Error - GATM_ReadResponse() %d, %s\n", Result, ERR_ConvertErrorCodeToString(Result));
                }
                else
                    ErrorCode = ATT_PROTOCOL_ERROR_CODE_INVALID_OFFSET;
            }
            else
                ErrorCode = ATT_PROTOCOL_ERROR_CODE_INVALID_HANDLE;

            /* Release the mutex that was previously acquired.             */
            BTPS_ReleaseMutex(mServiceMutex);
        }
        else
        {
            /* Simply send an error response.                              */
            ErrorCode = ATT_PROTOCOL_ERROR_CODE_UNLIKELY_ERROR;
        }

        /* If requested go ahead and issue an error response to the       */
        /* request.                                                       */
        if (ErrorCode)
        {
            if ((Result = GATM_ErrorResponse(ReadRequestData->RequestID, ErrorCode)) == 0)
                BOT_NOTIFY_INFO("GATM_ErrorResponse() success.\n");
            else
                BOT_NOTIFY_WARNING("Error - GATM_ErrorResponse() %d, %s\n", Result, ERR_ConvertErrorCodeToString(Result));
        }
    }
}

/* The following function is responsible for registering a specified */
/* service.  This function returns zero if successful and a negative */
/* value if an error occurred.                                       */
int GattSrv::GATTRegisterService(ParameterList_t *aParams __attribute__ ((unused)))
{
    int ret_val = UNDEFINED_ERROR;

    /* First, check to make sure that we have already been Initialized.  */
    if (mInitialized)
    {
        /* Make sure that all of the parameters required for this function*/
        /* appear to be at least semi-valid.                              */
        if ((aParams) && (aParams->NumberofParameters >= 1))
        {
            /* Simply call the internal function to attempt to register the*/
            /* service.                                                    */
            ret_val = RegisterService(aParams->Params[0].intParam);
        }
        else
        {
            /* One or more of the necessary parameters is/are invalid.     */
            BOT_NOTIFY_ERROR("Usage: RegisterService [Service Index (0 - %u)].\r\n", (mServiceCount-1));
            ret_val = INVALID_PARAMETERS_ERROR;
        }
    }
    else
    {
        /* Not Initialized, flag an error.                                */
        BOT_NOTIFY_ERROR("Platform Manager has not been Initialized.\r\n");
        ret_val = NOT_INITIALIZED_ERROR;
    }

    return ret_val;
}

/* The following function is responsible for un-registering a        */
/* specified service.  This function returns zero if successful and a*/
/* negative value if an error occurred.                              */
int GattSrv::GATTUnRegisterService(ParameterList_t *aParams __attribute__ ((unused)))
{
    int ret_val = UNDEFINED_ERROR;

    /* First, check to make sure that we have already been Initialized.  */
    if (mInitialized)
    {
        /* Make sure that all of the parameters required for this function*/
        /* appear to be at least semi-valid.                              */
        if ((aParams) && (aParams->NumberofParameters >= 1))
        {
            /* Simply call the internal function to attempt to un-register */
            /* the service.                                                */
            ret_val = UnRegisterService(aParams->Params[0].intParam);
        }
        else
        {
            /* One or more of the necessary parameters is/are invalid.     */
            BOT_NOTIFY_ERROR("Usage: UnRegisterService [Service Index (0 - %u)].\r\n", (mServiceCount-1));
            ret_val = INVALID_PARAMETERS_ERROR;
        }
    }
    else
    {
        /* Not Initialized, flag an error.                                */
        BOT_NOTIFY_ERROR("Platform Manager has not been Initialized.\r\n");
        ret_val = NOT_INITIALIZED_ERROR;
    }

    return ret_val;
}

/* The following function is responsible for indicating a specified  */
/* characteristic to a specified device.  This function returns zero */
/* if successful and a negative value if an error occurred.          */
int GattSrv::GATTIndicateCharacteristic(ParameterList_t *aParams __attribute__ ((unused)))
{
    int                   ret_val = UNDEFINED_ERROR;
    Boolean_t             DisplayUsage = FALSE;
    BD_ADDR_t             BD_ADDR;
    unsigned int          Index1;
    unsigned int          AttributeHandle;
    unsigned int          Index2;
    AttributeInfo_t      *AttributeInfo;
    CharacteristicInfo_t *CharacteristicInfo;
    char                 *DataString = aParams->Params[3].strParam;

    /* First, check to make sure that we have already been Initialized.  */
    if (mInitialized && mServiceTable)
    {
        /* Verify that a GATM Event Callback is registered.               */
        if (mGATMCallbackID)
        {
            /* Make sure that all of the parameters required for this      */
            /* function appear to be at least semi-valid.                  */
            if ((aParams) && (aParams->NumberofParameters >= 4) &&
                    (aParams->Params[0].intParam < mServiceCount) && (aParams->Params[1].intParam) && (aParams->Params[2].strParam) && aParams->Params[3].strParam)
            {
                /* Find the attribute info for this indication.             */
                if ((AttributeInfo = SearchServiceListByOffset(mServiceTable[aParams->Params[0].intParam].ServiceID, aParams->Params[1].intParam)) != NULL)
                {
                    /* Convert the parameter to a Bluetooth Device Address.  */
                    StrToBD_ADDR(aParams->Params[2].strParam, &BD_ADDR);

                    /* Verify that this is a characteristic that is          */
                    /* indicatable.                                          */
                    CharacteristicInfo = ((CharacteristicInfo_t *)AttributeInfo->Attribute);
                    if ((AttributeInfo->AttributeType == atCharacteristic) && (CharacteristicInfo) && (CharacteristicInfo->CharacteristicPropertiesMask & GATM_CHARACTERISTIC_PROPERTIES_INDICATE))
                    {
                        /* Either indicate the current value of the           */
                        /* characteristic or the test string.                 */
                        if ((CharacteristicInfo->ValueLength) && (CharacteristicInfo->Value))
                            ret_val = GATM_SendHandleValueIndication(mServiceTable[aParams->Params[0].intParam].ServiceID, BD_ADDR, AttributeInfo->AttributeOffset, CharacteristicInfo->ValueLength, CharacteristicInfo->Value);
                        else
                            ret_val = GATM_SendHandleValueIndication(mServiceTable[aParams->Params[0].intParam].ServiceID, BD_ADDR, AttributeInfo->AttributeOffset, BTPS_StringLength(DataString), (Byte_t *)DataString);

                        if (ret_val > 0)
                        {
                            BOT_NOTIFY_INFO("Sent Indication, TransactionID %u.\r\n", (unsigned int)ret_val);

                            ret_val = NO_ERROR;
                        }
                        else
                        {
                            BOT_NOTIFY_ERROR("Error - GATM_SendHandleValueIndication() %d, %s\n", ret_val, ERR_ConvertErrorCodeToString(ret_val));
                            ret_val = FUNCTION_ERROR;
                        }
                    }
                    else
                    {
                        BOT_NOTIFY_ERROR("Invalid Attribute Offset.\r\n)");
                        DisplayUsage = TRUE;
                    }
                }
                else
                {
                    BOT_NOTIFY_ERROR("Invalid Service Index or Attribute Offset.\r\n)");
                    DisplayUsage = TRUE;
                }
            }
            else
            {
                BOT_NOTIFY_ERROR("Invalid parameter or number of parameters.\r\n)");

                DisplayUsage = TRUE;
            }

            /* If requested show the possible values to this function.     */
            if (DisplayUsage)
            {
                /* One or more of the necessary parameters is/are invalid.  */
                BOT_NOTIFY_ERROR("Usage: IndicateCharacteristic [Service Index (0 - %u)] [Attribute Offset] [BD_ADDR].\r\n", (mServiceCount-1));
                BOT_NOTIFY_ERROR("Valid usages:\r\n\r\n");
                ret_val = INVALID_PARAMETERS_ERROR;

                for (Index1=0;Index1<mServiceCount;Index1++)
                {
                    if (mServiceTable[Index1].ServiceID)
                    {
                        for (Index2=0;Index2<mServiceTable[Index1].NumberAttributes;Index2++)
                        {
                            if ((mServiceTable[Index1].AttributeList[Index2].AttributeType == atCharacteristic) && (mServiceTable[Index1].AttributeList[Index2].Attribute))
                            {
                                if (((CharacteristicInfo_t *)(mServiceTable[Index1].AttributeList[Index2].Attribute))->CharacteristicPropertiesMask & GATM_CHARACTERISTIC_PROPERTIES_INDICATE)
                                {
                                    AttributeHandle = mServiceTable[Index1].ServiceHandleRange.Starting_Handle + mServiceTable[Index1].AttributeList[Index2].AttributeOffset + 1;

                                    BOT_NOTIFY_DEBUG("   IndicateCharacteristic %u %u [BD_ADDR] (Attribute Handle is 0x%04X (%u)\r\n", Index1, mServiceTable[Index1].AttributeList[Index2].AttributeOffset, AttributeHandle, AttributeHandle);
                                }
                            }
                        }
                    }
                }

                BOT_NOTIFY_DEBUG("\r\n");
            }
        }
        else
        {
            /* Callback not registered, go ahead and notify the user.      */
            BOT_NOTIFY_ERROR("Generic Attribute Profile Manager Event Callback is not registered.\r\n");
            ret_val = FUNCTION_ERROR;
        }
    }
    else
    {
        /* Not Initialized, flag an error.                                */
        BOT_NOTIFY_ERROR("Platform Manager has not been Initialized.\r\n");
        ret_val = NOT_INITIALIZED_ERROR;
    }

    return ret_val;
}

/* The following function is responsible for notifying a specified   */
/* characteristic to a specified device.  This function returns zero */
/* if successful and a negative value if an error occurred.          */
int GattSrv::GATTNotifyCharacteristic(ParameterList_t *aParams __attribute__ ((unused)))
{
    int                   ret_val = UNDEFINED_ERROR;
    Boolean_t             DisplayUsage = FALSE;
    BD_ADDR_t             BD_ADDR;
    unsigned int          Index1;
    unsigned int          AttributeHandle;
    unsigned int          Index2;
    AttributeInfo_t      *AttributeInfo;
    CharacteristicInfo_t *CharacteristicInfo;
    char                 *DataString = aParams->Params[3].strParam;

    /* First, check to make sure that we have already been Initialized.  */
    if (mInitialized && mServiceTable)
    {
        /* Verify that a GATM Event Callback is registered.               */
        if (mGATMCallbackID)
        {
            /* Make sure that all of the parameters required for this      */
            /* function appear to be at least semi-valid.                  */
            if ((aParams) && (aParams->NumberofParameters >= 4) &&
               (aParams->Params[0].intParam < mServiceCount) && (aParams->Params[1].intParam) && (aParams->Params[2].strParam) && (aParams->Params[3].strParam))
            {
                /* Find the attribute info for this indication.             */
                if ((AttributeInfo = SearchServiceListByOffset(mServiceTable[aParams->Params[0].intParam].ServiceID, aParams->Params[1].intParam)) != NULL)
                {
                    /* Convert the parameter to a Bluetooth Device Address.  */
                    StrToBD_ADDR(aParams->Params[2].strParam, &BD_ADDR);

                    /* Verify that this is a characteristic that is          */
                    /* notifiable.                                           */
                    CharacteristicInfo = ((CharacteristicInfo_t *)AttributeInfo->Attribute);
                    if ((AttributeInfo->AttributeType == atCharacteristic) && (CharacteristicInfo) && (CharacteristicInfo->CharacteristicPropertiesMask & GATM_CHARACTERISTIC_PROPERTIES_NOTIFY))
                    {
                        /* Either notify the current value of the             */
                        /* characteristic or the test string.                 */
                        if ((CharacteristicInfo->ValueLength) && (CharacteristicInfo->Value))
                            ret_val = GATM_SendHandleValueNotification(mServiceTable[aParams->Params[0].intParam].ServiceID, BD_ADDR, AttributeInfo->AttributeOffset, CharacteristicInfo->ValueLength, CharacteristicInfo->Value);
                        else
                            ret_val = GATM_SendHandleValueNotification(mServiceTable[aParams->Params[0].intParam].ServiceID, BD_ADDR, AttributeInfo->AttributeOffset, BTPS_StringLength(DataString), (Byte_t *)DataString);

                        if (!ret_val)
                            BOT_NOTIFY_INFO("Notification sent successfully.\r\n");
                        else
                        {
                            BOT_NOTIFY_ERROR("Error - GATM_SendHandleValueIndication() %d, %s\n", ret_val, ERR_ConvertErrorCodeToString(ret_val));
                            ret_val = FUNCTION_ERROR;
                        }
                    }
                    else
                    {
                        BOT_NOTIFY_ERROR("Invalid Attribute Offset.\r\n)");

                        DisplayUsage = TRUE;
                    }
                }
                else
                {
                    BOT_NOTIFY_ERROR("Invalid Service Index or Attribute Offset.\r\n)");

                    DisplayUsage = TRUE;
                }
            }
            else
            {
                BOT_NOTIFY_ERROR("Invalid parameter or number of parameters.\r\n)");

                DisplayUsage = TRUE;
            }

            /* If requested show the possible values to this function.     */
            if (DisplayUsage)
            {
                /* One or more of the necessary parameters is/are invalid.  */
                BOT_NOTIFY_ERROR("Usage: NotifyCharacteristic [Service Index (0 - %u)] [Attribute Offset] [BD_ADDR].\r\n", (mServiceCount-1));
                BOT_NOTIFY_ERROR("Valid usages:\r\n\r\n");
                ret_val = INVALID_PARAMETERS_ERROR;

                for (Index1=0;Index1<mServiceCount;Index1++)
                {
                    if (mServiceTable[Index1].ServiceID)
                    {
                        for (Index2=0;Index2<mServiceTable[Index1].NumberAttributes;Index2++)
                        {
                            if ((mServiceTable[Index1].AttributeList[Index2].AttributeType == atCharacteristic) && (mServiceTable[Index1].AttributeList[Index2].Attribute))
                            {
                                if (((CharacteristicInfo_t *)(mServiceTable[Index1].AttributeList[Index2].Attribute))->CharacteristicPropertiesMask & GATM_CHARACTERISTIC_PROPERTIES_NOTIFY)
                                {
                                    AttributeHandle = mServiceTable[Index1].ServiceHandleRange.Starting_Handle + mServiceTable[Index1].AttributeList[Index2].AttributeOffset + 1;

                                    BOT_NOTIFY_DEBUG("   NotifyCharacteristic %u %u [BD_ADDR] (Attribute Handle is 0x%04X (%u)\r\n", Index1, mServiceTable[Index1].AttributeList[Index2].AttributeOffset, AttributeHandle, AttributeHandle);
                                }
                            }
                        }
                    }
                }

                BOT_NOTIFY_DEBUG("\r\n");
            }
        }
        else
        {
            /* Callback not registered, go ahead and notify the user.      */
            BOT_NOTIFY_ERROR("Generic Attribute Profile Manager Event Callback is not registered.\r\n");
            ret_val = FUNCTION_ERROR;
        }
    }
    else
    {
        /* Not Initialized, flag an error.                                */
        BOT_NOTIFY_ERROR("Platform Manager has not been Initialized.\r\n");
        ret_val = NOT_INITIALIZED_ERROR;
    }

    return ret_val;
}

/* The following function is responsible for listing the             */
/* characteristics and their properties for all currently registered */
/* characteristics.  This function returns zero if successful and a  */
/* negative value if an error occurred.                              */
int GattSrv::ListCharacteristics(ParameterList_t *aParams __attribute__ ((unused)))
{
    int                   ret_val = UNDEFINED_ERROR;
    char                  Buffer[512];
    unsigned int          Index1;
    unsigned int          AttributeHandle;
    unsigned int          Index2;
    CharacteristicInfo_t *CharacteristicInfo;

    /* First, check to make sure that we have already been Initialized.  */
    if (mInitialized && mServiceTable)
    {
        /* Verify that a GATM Event Callback is registered.               */
        if (mGATMCallbackID)
        {
            BOT_NOTIFY_DEBUG("Key: \r\n");
            BOT_NOTIFY_DEBUG("x (xx), where X is the operation and XX are any security flags\r\n");
            BOT_NOTIFY_DEBUG("ENC = Encryption required, MITM = Man in the Middle protection required\r\n");
            BOT_NOTIFY_DEBUG("\r\n");
            BOT_NOTIFY_DEBUG("W  = Writable\r\n");
            BOT_NOTIFY_DEBUG("WO = Write without Response\r\n");
            BOT_NOTIFY_DEBUG("R  = Readable\r\n");
            BOT_NOTIFY_DEBUG("N  = Notifiable\r\n");
            BOT_NOTIFY_DEBUG("I  = Indicatable\r\n");
            BOT_NOTIFY_DEBUG("S  = Signed Writes allowed\r\n");
            BOT_NOTIFY_DEBUG("\r\n");

            /* Loop through the service list and list all of the           */
            /* characteristics that are valid.                             */
            for (Index1=0;Index1<mServiceCount;Index1++)
            {
                if (mServiceTable[Index1].ServiceID)
                {
                    for (Index2=0;Index2<mServiceTable[Index1].NumberAttributes;Index2++)
                    {
                        CharacteristicInfo = (CharacteristicInfo_t *)(mServiceTable[Index1].AttributeList[Index2].Attribute);
                        if ((mServiceTable[Index1].AttributeList[Index2].AttributeType == atCharacteristic) && (CharacteristicInfo))
                        {
                            Buffer[0] = '\0';

                            if (CharacteristicInfo->CharacteristicPropertiesMask & GATM_CHARACTERISTIC_PROPERTIES_READ)
                            {
                                BTPS_StringCopy(&Buffer[BTPS_StringLength(Buffer)], "R");

                                if (CharacteristicInfo->SecurityPropertiesMask & GATM_SECURITY_PROPERTIES_AUTHENTICATED_ENCRYPTION_READ)
                                {
                                    BTPS_StringCopy(&Buffer[BTPS_StringLength(Buffer)], " (ENC,MITM), ");
                                }
                                else
                                {
                                    if (CharacteristicInfo->SecurityPropertiesMask & GATM_SECURITY_PROPERTIES_UNAUTHENTICATED_ENCRYPTION_READ)
                                    {
                                        BTPS_StringCopy(&Buffer[BTPS_StringLength(Buffer)], " (ENC), ");
                                    }
                                    else
                                    {
                                        BTPS_StringCopy(&Buffer[BTPS_StringLength(Buffer)], ", ");
                                    }
                                }
                            }

                            if (CharacteristicInfo->CharacteristicPropertiesMask & GATM_CHARACTERISTIC_PROPERTIES_WRITE)
                            {
                                BTPS_StringCopy(&Buffer[BTPS_StringLength(Buffer)], "W");

                                if (CharacteristicInfo->SecurityPropertiesMask & GATM_SECURITY_PROPERTIES_AUTHENTICATED_ENCRYPTION_WRITE)
                                {
                                    BTPS_StringCopy(&Buffer[BTPS_StringLength(Buffer)], " (ENC,MITM), ");
                                }
                                else
                                {
                                    if (CharacteristicInfo->SecurityPropertiesMask & GATM_SECURITY_PROPERTIES_UNAUTHENTICATED_ENCRYPTION_WRITE)
                                    {
                                        BTPS_StringCopy(&Buffer[BTPS_StringLength(Buffer)], " (ENC), ");
                                    }
                                    else
                                    {
                                        BTPS_StringCopy(&Buffer[BTPS_StringLength(Buffer)], ", ");
                                    }
                                }
                            }

                            if (CharacteristicInfo->CharacteristicPropertiesMask & GATM_CHARACTERISTIC_PROPERTIES_WRITE_WO_RESP)
                            {
                                BTPS_StringCopy(&Buffer[BTPS_StringLength(Buffer)], "WO");

                                if (CharacteristicInfo->SecurityPropertiesMask & GATM_SECURITY_PROPERTIES_AUTHENTICATED_ENCRYPTION_WRITE)
                                {
                                    BTPS_StringCopy(&Buffer[BTPS_StringLength(Buffer)], " (ENC,MITM), ");
                                }
                                else
                                {
                                    if (CharacteristicInfo->SecurityPropertiesMask & GATM_SECURITY_PROPERTIES_UNAUTHENTICATED_ENCRYPTION_WRITE)
                                    {
                                        BTPS_StringCopy(&Buffer[BTPS_StringLength(Buffer)], " (ENC), ");
                                    }
                                    else
                                    {
                                        BTPS_StringCopy(&Buffer[BTPS_StringLength(Buffer)], ", ");
                                    }
                                }
                            }

                            if ((CharacteristicInfo->CharacteristicPropertiesMask & GATM_CHARACTERISTIC_PROPERTIES_AUTHENTICATED_SIGNED_WRITES) && (CharacteristicInfo->SecurityPropertiesMask & (GATM_SECURITY_PROPERTIES_UNAUTHENTICATED_SIGNED_WRITES | GATM_SECURITY_PROPERTIES_AUTHENTICATED_SIGNED_WRITES)))
                            {
                                BTPS_StringCopy(&Buffer[BTPS_StringLength(Buffer)], "S");

                                if (CharacteristicInfo->SecurityPropertiesMask & GATM_SECURITY_PROPERTIES_AUTHENTICATED_SIGNED_WRITES)
                                {
                                    BTPS_StringCopy(&Buffer[BTPS_StringLength(Buffer)], " (MITM), ");
                                }
                                else
                                {
                                    BTPS_StringCopy(&Buffer[BTPS_StringLength(Buffer)], ", ");
                                }
                            }

                            if (CharacteristicInfo->CharacteristicPropertiesMask & GATM_CHARACTERISTIC_PROPERTIES_NOTIFY)
                            {
                                BTPS_StringCopy(&Buffer[BTPS_StringLength(Buffer)], "N, ");
                            }

                            if (CharacteristicInfo->CharacteristicPropertiesMask & GATM_CHARACTERISTIC_PROPERTIES_INDICATE)
                            {
                                BTPS_StringCopy(&Buffer[BTPS_StringLength(Buffer)], "I, ");
                            }

                            /* Print the information on the characteristic.    */
                            if (BTPS_StringLength(Buffer) >= 3)
                                Buffer[BTPS_StringLength(Buffer) - 2] = '\0';

                            AttributeHandle = (mServiceTable[Index1].ServiceHandleRange.Starting_Handle + mServiceTable[Index1].AttributeList[Index2].AttributeOffset + 1);

                            BOT_NOTIFY_DEBUG("Characteristic Handle:     0x%04X (%u)\r\n", AttributeHandle, AttributeHandle);
                            BOT_NOTIFY_DEBUG("Characteristic Properties: %s.\r\n", Buffer);
                        }
                    }
                }
            }

            /* Simply return success to the caller.                        */
            ret_val = NO_ERROR;
        }
        else
        {
            /* Callback not registered, go ahead and notify the user.      */
            BOT_NOTIFY_ERROR("Generic Attribute Profile Manager Event Callback is not registered.\r\n");
            ret_val = FUNCTION_ERROR;
        }
    }
    else
    {
        /* Not Initialized, flag an error.                                */
        BOT_NOTIFY_ERROR("Platform Manager has not been Initialized.\r\n");
        ret_val = NOT_INITIALIZED_ERROR;
    }

    return ret_val;
}

/* The following function is responsible for listing the descriptors */
/* and their properties for all currently registered characteristics.*/
/* This function returns zero if successful and a negative value if  */
/* an error occurred.                                                */
int GattSrv::ListDescriptors(ParameterList_t *aParams __attribute__ ((unused)))
{
    int               ret_val = UNDEFINED_ERROR;
    char              Buffer[512];
    unsigned int      Index1;
    unsigned int      AttributeHandle;
    unsigned int      Index2;
    DescriptorInfo_t *DescriptorInfo;

    /* First, check to make sure that we have already been Initialized.  */
    if (mInitialized && mServiceTable)
    {
        /* Verify that a GATM Event Callback is registered.               */
        if (mGATMCallbackID)
        {
            BOT_NOTIFY_DEBUG("Key: \r\n");
            BOT_NOTIFY_DEBUG("x (xx), where X is the operation and XX are any security flags\r\n");
            BOT_NOTIFY_DEBUG("ENC = Encryption required, MITM = Man in the Middle protection required\r\n");
            BOT_NOTIFY_DEBUG("\r\n");
            BOT_NOTIFY_DEBUG("W  = Writable\r\n");
            BOT_NOTIFY_DEBUG("R  = Readable\r\n");
            BOT_NOTIFY_DEBUG("\r\n");

            /* Loop through the service list and list all of the           */
            /* characteristics that are valid.                             */
            for (Index1=0;Index1<mServiceCount;Index1++)
            {
                if (mServiceTable[Index1].ServiceID)
                {
                    for (Index2=0;Index2<mServiceTable[Index1].NumberAttributes;Index2++)
                    {
                        DescriptorInfo = (DescriptorInfo_t *)(mServiceTable[Index1].AttributeList[Index2].Attribute);
                        if ((mServiceTable[Index1].AttributeList[Index2].AttributeType == atDescriptor) && (DescriptorInfo))
                        {
                            Buffer[0] = '\0';

                            if (DescriptorInfo->DescriptorPropertiesMask & GATM_DESCRIPTOR_PROPERTIES_READ)
                            {
                                BTPS_StringCopy(&Buffer[BTPS_StringLength(Buffer)], "R");

                                if (DescriptorInfo->SecurityPropertiesMask & GATM_SECURITY_PROPERTIES_AUTHENTICATED_ENCRYPTION_READ)
                                {
                                    BTPS_StringCopy(&Buffer[BTPS_StringLength(Buffer)], " (ENC,MITM), ");
                                }
                                else
                                {
                                    if (DescriptorInfo->SecurityPropertiesMask & GATM_SECURITY_PROPERTIES_UNAUTHENTICATED_ENCRYPTION_READ)
                                    {
                                        BTPS_StringCopy(&Buffer[BTPS_StringLength(Buffer)], " (ENC), ");
                                    }
                                    else
                                    {
                                        BTPS_StringCopy(&Buffer[BTPS_StringLength(Buffer)], ", ");
                                    }
                                }
                            }

                            if (DescriptorInfo->DescriptorPropertiesMask & GATM_DESCRIPTOR_PROPERTIES_WRITE)
                            {
                                BTPS_StringCopy(&Buffer[BTPS_StringLength(Buffer)], "W");

                                if (DescriptorInfo->SecurityPropertiesMask & GATM_SECURITY_PROPERTIES_AUTHENTICATED_ENCRYPTION_WRITE)
                                {
                                    BTPS_StringCopy(&Buffer[BTPS_StringLength(Buffer)], " (ENC,MITM), ");
                                }
                                else
                                {
                                    if (DescriptorInfo->SecurityPropertiesMask & GATM_SECURITY_PROPERTIES_UNAUTHENTICATED_ENCRYPTION_WRITE)
                                    {
                                        BTPS_StringCopy(&Buffer[BTPS_StringLength(Buffer)], " (ENC), ");
                                    }
                                    else
                                    {
                                        BTPS_StringCopy(&Buffer[BTPS_StringLength(Buffer)], ", ");
                                    }
                                }
                            }

                            /* Print the information on the characteristic.    */
                            if (BTPS_StringLength(Buffer) >= 3)
                                Buffer[BTPS_StringLength(Buffer) - 2] = '\0';

                            AttributeHandle = (mServiceTable[Index1].ServiceHandleRange.Starting_Handle + mServiceTable[Index1].AttributeList[Index2].AttributeOffset);

                            BOT_NOTIFY_DEBUG("Descriptor Handle:     0x%04X (%u)\r\n", AttributeHandle, AttributeHandle);
                            BOT_NOTIFY_DEBUG("Descriptor Properties: %s.\r\n", Buffer);
                        }
                    }
                }
            }

            /* Simply return success to the caller.                        */
            ret_val = NO_ERROR;
        }
        else
        {
            /* Callback not registered, go ahead and notify the user.      */
            BOT_NOTIFY_ERROR("Generic Attribute Profile Manager Event Callback is not registered.\r\n");
            ret_val = FUNCTION_ERROR;
        }
    }
    else
    {
        /* Not Initialized, flag an error.                                */
        BOT_NOTIFY_ERROR("Platform Manager has not been Initialized.\r\n");
        ret_val = NOT_INITIALIZED_ERROR;
    }

    return ret_val;
}

/* The following function is responsible for determining the total    */
/* number of published services and returning the number of published */
/* services that have been placed into the buffer.  This function     */
/* takes a single parameter list, which contains a parameter for the  */
/* maximum number of pubished services that will be placed into the   */
/* buffer and an optional parameter for a service UUID if specified.  */
/* This function returns a non-negative value if successful, which    */
/* represents the number of published services that were copied into  */
/* the specified input buffer and a negative value if an error        */
/* occurred.                                                          */
int GattSrv::GATTQueryPublishedServices(ParameterList_t *aParams __attribute__ ((unused)))
{
    int                         ret_val = UNDEFINED_ERROR;
    GATT_UUID_t                 ServiceUUID;
    GATT_UUID_t                *ServiceUUIDPtr = NULL;
    unsigned int                TotalServices;
    unsigned int                StringLength;
    unsigned int                Index;
    SDP_UUID_Entry_t            SDPUUIDEntry;
    GATM_Service_Information_t *PublishedServiceList = NULL;
    GATM_Service_Information_t *tempPtr = NULL;

    /* First, check to make sure that we have already been Initialized.  */
    if (mInitialized)
    {
        /* Verify that a GATM Event Callback is registered.               */
        if (mGATMCallbackID)
        {
            /* Check if there is an optional UUID parameter.               */
            if ((aParams) && (aParams->NumberofParameters == 1))
            {
                /* Check the size of the UUID.                              */
                StringLength = strlen(aParams->Params[0].strParam);

                /* Check for any "hex format" prefix.                       */
                if ((StringLength >= 2) && (aParams->Params[0].strParam[0] == '0') && ((aParams->Params[0].strParam[1] == 'x') || (aParams->Params[0].strParam[1] == 'X')))
                    StringLength -= 2;

                if ((StringLength == 4) || (StringLength == 32))
                {
                    /* Change the string to 128 bit UUID.                    */
                    StrToUUIDEntry(aParams->Params[0].strParam, &SDPUUIDEntry);

                    /* Configure the Service UUID.  Not this sample          */
                    /* application only supports registering 128 bit UUIDs,  */
                    /* however GATM allows other types as well.              */
                    if (SDPUUIDEntry.SDP_Data_Element_Type == deUUID_16)
                    {
                        ServiceUUID.UUID_Type     = guUUID_16;
                        ServiceUUID.UUID.UUID_16  = SDPUUIDEntry.UUID_Value.UUID_16;
                    }
                    else
                    {
                        ServiceUUID.UUID_Type     = guUUID_128;
                        ServiceUUID.UUID.UUID_128 = SDPUUIDEntry.UUID_Value.UUID_128;
                    }

                    ServiceUUIDPtr = &ServiceUUID;

                    ret_val = NO_ERROR;
                }
                else
                {
                    /* Invalid Size of UUID String.                          */
                    BOT_NOTIFY_ERROR("Invalid size of 16-bit/128-bit UUID.\r\n");
                    BOT_NOTIFY_ERROR("Usage: [16-bit/128-bit UUID (Optional Prefix['0x|0X']) 'AABB | AABBCCDDEEFFAABBCCDDEEFF'].\r\n");
                    ret_val = INVALID_PARAMETERS_ERROR;
                }
            }
            else
                ret_val = NO_ERROR;

            /* Continue only if no error occurred.                         */
            if (!ret_val)
            {
                /* Determine the total number of published services.        */
                if ((ret_val = GATM_QueryPublishedServices(0, NULL, ServiceUUIDPtr, &TotalServices)) == 0)
                {
                    /* Print the total number of published services.         */
                    BOT_NOTIFY_DEBUG("The Total Number of Published Services is: %u\r\n\r\n", TotalServices);

                    /* If there are any services published go ahead and query*/
                    /* information on the published services.                */
                    if (TotalServices)
                    {
                        /* Attempt to allocate memory for the buffer to hold  */
                        /* all of the requested services.                     */
                        if ((PublishedServiceList = (GATM_Service_Information_t *) BTPS_AllocateMemory(TotalServices * sizeof(GATM_Service_Information_t))) != NULL)
                        {
                            /* Attempt to query all of the published services. */
                            if ((ret_val = GATM_QueryPublishedServices(TotalServices, PublishedServiceList, ServiceUUIDPtr, &TotalServices)) >= 0)
                            {
                                BOT_NOTIFY_DEBUG("Printing the Published Services List:\r\n");

                                for (Index=0,tempPtr=PublishedServiceList;Index<((unsigned int)ret_val);Index++,tempPtr++)
                                {
                                    BOT_NOTIFY_DEBUG("ServiceID: %u\r\n", tempPtr->ServiceID);
                                    DisplayGATTUUID(&tempPtr->ServiceUUID, "\bServiceUUID", 0);
                                    BOT_NOTIFY_DEBUG("Start Handle: %u\r\n", tempPtr->StartHandle);
                                    BOT_NOTIFY_DEBUG("End Handle: %u\r\n", tempPtr->EndHandle);
                                }

                                BOT_NOTIFY_DEBUG("\r\n");

                                /* Return success to the caller.                */
                                ret_val = NO_ERROR;
                            }

                            /* Free the allocated memory.                      */
                            BTPS_FreeMemory(PublishedServiceList);
                            PublishedServiceList = NULL;
                        }
                    }
                }

                /* Print Success or Error Message.                          */
                if (!ret_val)
                    BOT_NOTIFY_INFO("GATM_QueryPublishedServices() success\r\n\r\n");
                else
                {
                    BOT_NOTIFY_ERROR("Error - GATM_QueryPublishedServices() %d, %s\r\n\r\n", ret_val, ERR_ConvertErrorCodeToString(ret_val));
                    ret_val = FUNCTION_ERROR;
                }
            }
        }
        else
        {
            /* Callback not registered, go ahead and notify the user.      */
            BOT_NOTIFY_ERROR("Generic Attribute Profile Manager Event Callback is not registered.\r\n");
            ret_val = FUNCTION_ERROR;
        }
    }
    else
    {
        /* Not Initialized, flag an error.                                */
        BOT_NOTIFY_ERROR("Platform Manager has not been Initialized.\r\n");
        ret_val = NOT_INITIALIZED_ERROR;
    }

    return ret_val;
}

void GattSrv::SetCurrentRemoteBD_ADDR(BD_ADDR_t aCurrentRemoteBD_ADDR)
{
    mCurrentRemoteBD_ADDR = aCurrentRemoteBD_ADDR;
}

BD_ADDR_t GattSrv::GetCurrentRemoteBD_ADDR()
{
    return mCurrentRemoteBD_ADDR;
}

void GattSrv::SetCurrentLowEnergy(Boolean_t aCurrentLowEnergy)
{
    mCurrentLowEnergy = aCurrentLowEnergy;
}

Boolean_t GattSrv::GetCurrentLowEnergy()
{
    return mCurrentLowEnergy;
}

void GattSrv::CopyRandomValues()
{
    BTPS_MemCopy(&mOOBRemoteRandom, &mOOBLocalRandom, SM_RANDOM_VALUE_SIZE);
}

void GattSrv::CopyConfirmValues()
{
    BTPS_MemCopy(&mOOBRemoteConfirmation, &mOOBLocalConfirmation, SM_CONFIRM_VALUE_SIZE);
}

void GattSrv::CopyRandomValues(void * dst)
{
    BTPS_MemCopy(dst, &mOOBRemoteRandom , SM_RANDOM_VALUE_SIZE);
}

void GattSrv::CopyConfirmValues(void * dst)
{
    BTPS_MemCopy(dst, &mOOBRemoteConfirmation , SM_CONFIRM_VALUE_SIZE);
}

unsigned int GattSrv::GetDEVMCallbackID()
{
    return mDEVMCallbackID;
}

unsigned int GattSrv::GetGATMCallbackID()
{
    return mGATMCallbackID;
}

unsigned int GattSrv::GetAuthenticationCallbackID()
{
    return mAuthenticationCallbackID;
}

Boolean_t GattSrv::GetOOBSupport()
{
    return mOOBSupport;
}

Boolean_t GattSrv::GetSC()
{
    return mSC;
}

Boolean_t GattSrv::GetKeypress()
{
    return mKeypress;
}

Boolean_t GattSrv::GetMITMProtection()
{
    return mMITMProtection;
}

Boolean_t GattSrv::GetLEMITMProtection()
{
    return mLEMITMProtection;
}

Boolean_t GattSrv::GetP256DebugMode()
{
    return mP256DebugMode;
}

GAP_IO_Capability_t GattSrv::GetIOCapability()
{
    return mIOCapability;
}

GAP_LE_IO_Capability_t GattSrv::GetLEIOCapability()
{
    return mLEIOCapability;
}

GAP_LE_Bonding_Type_t GattSrv::GetBondingType()
{
    return mBondingType;
}

////////////////////////// callback fundtions /////////////////////////////
////////////////////////// callback fundtions /////////////////////////////
////////////////////////// callback fundtions /////////////////////////////

/* The following function is the Device Manager Event Callback       */
/* function that is Registered with the Device Manager.  This        */
/* callback is responsible for processing all Device Manager Events. */
static void BTPSAPI DEVM_Event_Callback(DEVM_Event_Data_t *EventData, void *CallbackParameter __attribute__ ((unused)))
{
    char Buffer[32];
    GattSrv * gatt = (GattSrv *) GattSrv::getInstance();

    if (EventData)
    {
        BOT_NOTIFY_DEBUG("\r\n");

        switch(EventData->EventType)
        {
        case detDevicePoweredOn:
            BOT_NOTIFY_DEBUG("Device Powered On.\r\n");
            break;
        case detDevicePoweringOff:
            BOT_NOTIFY_DEBUG("Device Powering Off Event, Timeout: 0x%08X.\r\n", EventData->EventData.DevicePoweringOffEventData.PoweringOffTimeout);
            break;
        case detDevicePoweredOff:
            BOT_NOTIFY_DEBUG("Device Powered Off.\r\n");
            break;
        case detLocalDevicePropertiesChanged:
            BOT_NOTIFY_DEBUG("Local Device Properties Changed.\r\n");

            gatt->DisplayLocalDeviceProperties(EventData->EventData.LocalDevicePropertiesChangedEventData.ChangedMemberMask, &(EventData->EventData.LocalDevicePropertiesChangedEventData.LocalDeviceProperties));
            break;
        case detDeviceScanStarted:
            BOT_NOTIFY_DEBUG("LE Device Discovery Started.\r\n");
            break;
        case detDeviceScanStopped:
            BOT_NOTIFY_DEBUG("LE Device Discovery Stopped.\r\n");
            break;
        case detDeviceAdvertisingStarted:
            BOT_NOTIFY_DEBUG("LE Advertising Started.\r\n");
            break;
        case detDeviceAdvertisingStopped:
            BOT_NOTIFY_DEBUG("LE Advertising Stopped.\r\n");
            break;
        case detAdvertisingTimeout:
            BOT_NOTIFY_DEBUG("LE Advertising Timeout.\r\n");
            break;
        case detDeviceDiscoveryStarted:
            BOT_NOTIFY_DEBUG("Device Discovery Started.\r\n");
            break;
        case detDeviceDiscoveryStopped:
            BOT_NOTIFY_DEBUG("Device Discovery Stopped.\r\n");
            break;
        case detRemoteDeviceFound:
            BOT_NOTIFY_DEBUG("Remote Device Found.\r\n");

            gatt->DisplayRemoteDeviceProperties(0, &(EventData->EventData.RemoteDeviceFoundEventData.RemoteDeviceProperties));
            break;
        case detRemoteDeviceDeleted:
            gatt->BD_ADDRToStr(EventData->EventData.RemoteDeviceDeletedEventData.RemoteDeviceAddress, Buffer);
            BOT_NOTIFY_DEBUG("Remote Device Deleted: %s.\r\n", Buffer);
            break;
        case detRemoteDevicePropertiesChanged:
            BOT_NOTIFY_DEBUG("Remote Device Properties Changed.\r\n");
            gatt->DisplayRemoteDeviceProperties(EventData->EventData.RemoteDevicePropertiesChangedEventData.ChangedMemberMask, &(EventData->EventData.RemoteDevicePropertiesChangedEventData.RemoteDeviceProperties));
            break;
        case detRemoteDeviceAddressChanged:
            BOT_NOTIFY_DEBUG("Remote Device Address Changed.\r\n");

            gatt->BD_ADDRToStr(EventData->EventData.RemoteDeviceAddressChangeEventData.RemoteDeviceAddress, Buffer);
            BOT_NOTIFY_DEBUG("Remote Device Address:          %s.\r\n", Buffer);
            gatt->BD_ADDRToStr(EventData->EventData.RemoteDeviceAddressChangeEventData.PreviousRemoteDeviceAddress, Buffer);
            BOT_NOTIFY_DEBUG("Previous Remote Device Address: %s.\r\n", Buffer);
            break;
        case detRemoteDevicePropertiesStatus:
            gatt->BD_ADDRToStr(EventData->EventData.RemoteDevicePropertiesStatusEventData.RemoteDeviceProperties.BD_ADDR, Buffer);

            BOT_NOTIFY_DEBUG("Remote Device Properties Status: %s, %s.\r\n", Buffer, EventData->EventData.RemoteDevicePropertiesStatusEventData.Success?"SUCCESS":"FAILURE");

            gatt->DisplayRemoteDeviceProperties(0, &(EventData->EventData.RemoteDevicePropertiesStatusEventData.RemoteDeviceProperties));
            break;
        case detRemoteDeviceServicesStatus:
            gatt->BD_ADDRToStr(EventData->EventData.RemoteDeviceServicesStatusEventData.RemoteDeviceAddress, Buffer);

            BOT_NOTIFY_DEBUG("Remote Device %s Services Status: %s, %s.\r\n", Buffer, (EventData->EventData.RemoteDeviceServicesStatusEventData.StatusFlags & DEVM_REMOTE_DEVICE_SERVICES_STATUS_FLAGS_LOW_ENERGY)?"LE":"BR/EDR", (EventData->EventData.RemoteDeviceServicesStatusEventData.StatusFlags & DEVM_REMOTE_DEVICE_SERVICES_STATUS_FLAGS_SUCCESS)?"SUCCESS":"FAILURE");
            break;
        case detRemoteDevicePairingStatus:
            gatt->BD_ADDRToStr(EventData->EventData.RemoteDevicePairingStatusEventData.RemoteDeviceAddress, Buffer);

            BOT_NOTIFY_DEBUG("%s Remote Device Pairing Status: %s, %s (0x%02X)\r\n", ((EventData->EventData.RemoteDevicePairingStatusEventData.AuthenticationStatus & DEVM_REMOTE_DEVICE_PAIRING_STATUS_FLAGS_LOW_ENERGY)?"LE":"BR/EDR"), Buffer, (EventData->EventData.RemoteDevicePairingStatusEventData.Success)?"SUCCESS":"FAILURE", EventData->EventData.RemoteDevicePairingStatusEventData.AuthenticationStatus);
            break;
        case detRemoteDeviceAuthenticationStatus:
            gatt->BD_ADDRToStr(EventData->EventData.RemoteDeviceAuthenticationStatusEventData.RemoteDeviceAddress, Buffer);

            BOT_NOTIFY_DEBUG("Remote Device Authentication Status: %s, %d (%s)\r\n", Buffer, EventData->EventData.RemoteDeviceAuthenticationStatusEventData.Status, (EventData->EventData.RemoteDeviceAuthenticationStatusEventData.Status)?ERR_ConvertErrorCodeToString(EventData->EventData.RemoteDeviceAuthenticationStatusEventData.Status):"SUCCESS");
            break;
        case detRemoteDeviceEncryptionStatus:
            gatt->BD_ADDRToStr(EventData->EventData.RemoteDeviceEncryptionStatusEventData.RemoteDeviceAddress, Buffer);

            BOT_NOTIFY_DEBUG("Remote Device Encryption Status: %s, %d (%s)\r\n", Buffer, EventData->EventData.RemoteDeviceEncryptionStatusEventData.Status, (EventData->EventData.RemoteDeviceEncryptionStatusEventData.Status)?ERR_ConvertErrorCodeToString(EventData->EventData.RemoteDeviceEncryptionStatusEventData.Status):"SUCCESS");
            break;
        case detRemoteDeviceConnectionStatus:
            gatt->BD_ADDRToStr(EventData->EventData.RemoteDeviceConnectionStatusEventData.RemoteDeviceAddress, Buffer);

            BOT_NOTIFY_DEBUG("Remote Device Connection Status: %s, %d (%s)\r\n", Buffer, EventData->EventData.RemoteDeviceConnectionStatusEventData.Status, (EventData->EventData.RemoteDeviceConnectionStatusEventData.Status)?ERR_ConvertErrorCodeToString(EventData->EventData.RemoteDeviceConnectionStatusEventData.Status):"SUCCESS");
            break;
        case detConnectionParameterUpdateResponse:
            BOT_NOTIFY_DEBUG("Connection Parameter Update Response\r\n");

            gatt->BD_ADDRToStr(EventData->EventData.RemoteDeviceConnectionParameterUpdateResponseEventData.RemoteDeviceAddress, Buffer);
            BOT_NOTIFY_DEBUG("   Status:              %s.\r\n", (EventData->EventData.RemoteDeviceConnectionParameterUpdateResponseEventData.Accepted)?"Accepted":"Declined");
            BOT_NOTIFY_DEBUG("   BD_ADDR:             %s.\r\n", Buffer);

            break;
        case detConnectionParametersUpdated:
            gatt->BD_ADDRToStr(EventData->EventData.RemoteDeviceConnectionParametersUpdatedEventData.RemoteDeviceAddress, Buffer);
            BOT_NOTIFY_DEBUG("Connection Parameters Updated\r\n");
            BOT_NOTIFY_DEBUG("   Status             : %d\r\n", EventData->EventData.RemoteDeviceConnectionParametersUpdatedEventData.Status);
            BOT_NOTIFY_DEBUG("   BD_ADDR            : %s\r\n", Buffer);
            if (EventData->EventData.RemoteDeviceConnectionParametersUpdatedEventData.Status == 0)
            {
                BOT_NOTIFY_DEBUG("   Connection Interval: %d\r\n", EventData->EventData.RemoteDeviceConnectionParametersUpdatedEventData.Current_Connection_Parameters.Connection_Interval);
                BOT_NOTIFY_DEBUG("   Slave Latency      : %d\r\n", EventData->EventData.RemoteDeviceConnectionParametersUpdatedEventData.Current_Connection_Parameters.Slave_Latency);
                BOT_NOTIFY_DEBUG("   Supervision Timeout: %d\r\n", EventData->EventData.RemoteDeviceConnectionParametersUpdatedEventData.Current_Connection_Parameters.Supervision_Timeout);
            }
            break;
        case detAuthenticationPayloadTimeoutExpired:
            gatt->BD_ADDRToStr(EventData->EventData.RemoteDeviceAuthenticatedPayloadTimeoutExpiredEventData.RemoteDeviceAddress, Buffer);

            BOT_NOTIFY_DEBUG("Authenticated Payload Timeout Expired received from BD_ADDR: %s\r\n", Buffer);
            break;
        default:
            BOT_NOTIFY_DEBUG("Unknown Device Manager Event Received: 0x%08X, Length: 0x%08X.\r\n", (unsigned int)EventData->EventType, EventData->EventLength);
            break;
        }
    }
    else
        BOT_NOTIFY_WARNING("\r\nDEVM Event Data is NULL.\r\n");

    /* Make sure the output is displayed to the user.                    */
#ifdef TEST_CONSOLE_WRITE
    fflush(stdout);
#endif
}

/* The following function is the Device Manager Authentication Event */
/* Callback function that is Registered with the Device Manager.     */
/* This callback is responsible for processing all Device Manager    */
/* Authentication Request Events.                                    */
static void BTPSAPI DEVM_Authentication_Callback(DEVM_Authentication_Information_t *AuthenticationRequestInformation, void *CallbackParameter __attribute__ ((unused)))
{
    int     Result;
    char    Buffer[32];
    Boolean_t    LowEnergy;
    GattSrv *gatt = (GattSrv *) GattSrv::getInstance();
    DEVM_Authentication_Information_t AuthenticationResponseInformation;

    if (AuthenticationRequestInformation)
    {
        BOT_NOTIFY_DEBUG("\nAuthenticationRequestInformation\r\n");

        gatt->BD_ADDRToStr(AuthenticationRequestInformation->BD_ADDR, Buffer);

        BOT_NOTIFY_DEBUG("Authentication Request received for %s.\r\n", Buffer);

        /* Check to see if this is an LE event.                           */
        if (AuthenticationRequestInformation->AuthenticationAction & DEVM_AUTHENTICATION_ACTION_LOW_ENERGY_OPERATION_MASK)
        {
            AuthenticationRequestInformation->AuthenticationAction &= ~DEVM_AUTHENTICATION_ACTION_LOW_ENERGY_OPERATION_MASK;

            LowEnergy = TRUE;
        }
        else
            LowEnergy = FALSE;

        switch(AuthenticationRequestInformation->AuthenticationAction)
        {
        case DEVM_AUTHENTICATION_ACTION_PIN_CODE_REQUEST:
            BOT_NOTIFY_DEBUG("PIN Code Request.\r\n.");

            /* Note the current Remote BD_ADDR that is requesting the   */
            /* PIN Code.                                                */
            gatt->SetCurrentRemoteBD_ADDR(AuthenticationRequestInformation->BD_ADDR);

            /* Inform the user that they will need to respond with a PIN*/
            /* Code Response.                                           */
            BOT_NOTIFY_DEBUG("\r\nRespond with the command: PINCodeResponse\r\n");
            break;
        case DEVM_AUTHENTICATION_ACTION_USER_CONFIRMATION_REQUEST:
            BOT_NOTIFY_DEBUG("User Confirmation Request %s.\r\n", (LowEnergy?"LE":"BR/EDR"));

            /* Note the current Remote BD_ADDR that is requesting the   */
            /* User Confirmation.                                       */
            gatt->SetCurrentRemoteBD_ADDR(AuthenticationRequestInformation->BD_ADDR);

            /* Update the globla parameter to LE mode or BR/EDR mode.   */
            gatt->SetCurrentLowEnergy(LowEnergy);

            /* Check which method we need to use.                       */
            if ((gatt->GetIOCapability() == icDisplayYesNo) && (gatt->GetCurrentLowEnergy() == FALSE))
            {
                /* If the device is using BR/EDR mode and DisplayYesNo  */
                /* IO Capability that means that we are using numeric   */
                /* comparison method and the user must enter an input.  */

                BOT_NOTIFY_DEBUG("User Confirmation: %lu\r\n", (unsigned long)AuthenticationRequestInformation->AuthenticationData.Passkey);

                /* Inform the user that they will need to respond with  */
                /* a PIN Code Response.                                 */
                BOT_NOTIFY_DEBUG("\r\nRespond with the command: UserConfirmationResponse\r\n");
            }
            else
            {
                /* Invoke Just works.                                   */

                BOT_NOTIFY_DEBUG("\r\nAuto Accepting: %lu\r\n", (unsigned long)AuthenticationRequestInformation->AuthenticationData.Passkey);

                BTPS_MemInitialize(&AuthenticationResponseInformation, 0, sizeof(AuthenticationResponseInformation));

                AuthenticationResponseInformation.BD_ADDR                         = AuthenticationRequestInformation->BD_ADDR;
                AuthenticationResponseInformation.AuthenticationAction            = DEVM_AUTHENTICATION_ACTION_USER_CONFIRMATION_RESPONSE;
                AuthenticationResponseInformation.AuthenticationDataLength        = sizeof(AuthenticationResponseInformation.AuthenticationData.Confirmation);

                AuthenticationResponseInformation.AuthenticationData.Confirmation = (Boolean_t)TRUE;

                if (gatt->GetCurrentLowEnergy())
                {
                    /* Flag that this is LE Pairing.                        */
                    AuthenticationResponseInformation.AuthenticationAction |= DEVM_AUTHENTICATION_ACTION_LOW_ENERGY_OPERATION_MASK;
                }

                if ((Result = DEVM_AuthenticationResponse(gatt->GetAuthenticationCallbackID(), &AuthenticationResponseInformation)) >= 0)
                    BOT_NOTIFY_INFO("DEVM_AuthenticationResponse() Success.\r\n");
                else
                    BOT_NOTIFY_DEBUG("DEVM_AuthenticationResponse() Failure: %d, %s.\r\n", Result, ERR_ConvertErrorCodeToString(Result));

                /* Flag that there is no longer a current               */
                /* Authentication procedure in progress.                */
                BD_ADDR_t addr;
                ASSIGN_BD_ADDR(addr, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00);
                gatt->SetCurrentRemoteBD_ADDR(addr);
            }
            break;
        case DEVM_AUTHENTICATION_ACTION_PASSKEY_REQUEST:
            BOT_NOTIFY_DEBUG("Passkey Request %s.\r\n", (LowEnergy?"LE":"BR/EDR"));

            /* Note the current Remote BD_ADDR that is requesting the   */
            /* Passkey.                                                 */
            gatt->SetCurrentRemoteBD_ADDR(AuthenticationRequestInformation->BD_ADDR);
            gatt->SetCurrentLowEnergy(LowEnergy);

            /* Inform the user that they will need to respond with a    */
            /* Passkey Response.                                        */
            BOT_NOTIFY_DEBUG("\r\nRespond with the command: PassKeyResponse\r\n");
            break;
        case DEVM_AUTHENTICATION_ACTION_PASSKEY_INDICATION:
            BOT_NOTIFY_DEBUG("PassKey Indication %s.\r\n", (LowEnergy?"LE":"BR/EDR"));

            BOT_NOTIFY_DEBUG("PassKey: %lu\r\n", (unsigned long)AuthenticationRequestInformation->AuthenticationData.Passkey);
            break;
        case DEVM_AUTHENTICATION_ACTION_NUMERIC_COMPARISON_REQUEST:
            /* Flag that this is LE Pairing.                         */
            gatt->SetCurrentLowEnergy(TRUE);

            BOT_NOTIFY_DEBUG("Numeric Comparison Request %s.\r\n", (LowEnergy?"LE":"BR/EDR"));

            BOT_NOTIFY_DEBUG("User Confirmation: %lu\r\n", (unsigned long)AuthenticationRequestInformation->AuthenticationData.Passkey);

            /* Inform the user they will need to respond                */
            /* with a user confirmation response command                */
            /* that will verify the numeric values matches.             */
            BOT_NOTIFY_DEBUG("\r\nRespond with the command: UserConfirmationResponse\r\n");
            break;
        case DEVM_AUTHENTICATION_ACTION_KEYPRESS_INDICATION:
            BOT_NOTIFY_DEBUG("mKeypress Indication.\r\n");

            BOT_NOTIFY_DEBUG("mKeypress: %d\r\n", (int)AuthenticationRequestInformation->AuthenticationData.Keypress);
            break;
        case DEVM_AUTHENTICATION_ACTION_OUT_OF_BAND_DATA_REQUEST:
            BOT_NOTIFY_DEBUG("Out of Band Data Request: %s.\r\n", (LowEnergy?"LE":"BR/EDR"));

            /* This application does not support OOB data so respond    */
            /* with a data length of Zero to force a negative reply.    */
            BTPS_MemInitialize(&AuthenticationResponseInformation, 0, sizeof(AuthenticationResponseInformation));

            AuthenticationResponseInformation.BD_ADDR                  = AuthenticationRequestInformation->BD_ADDR;
            AuthenticationResponseInformation.AuthenticationAction     = DEVM_AUTHENTICATION_ACTION_OUT_OF_BAND_DATA_RESPONSE;
            AuthenticationResponseInformation.AuthenticationDataLength = 0;

            if (LowEnergy)
            {
                AuthenticationResponseInformation.AuthenticationAction |= DEVM_AUTHENTICATION_ACTION_LOW_ENERGY_OPERATION_MASK;

                /* We should respond with different parametrs in case of LE SC */
                /* This application support OOB only in case of LE SC.  */
                if ((Boolean_t)AuthenticationRequestInformation->SC == TRUE)
                {
                    /* In debug mode, we don't have the remotes random and confirmation values, use the local ones */
                    gatt->CopyRandomValues();
                    gatt->CopyConfirmValues();
                    /* 							End of debug mode									   */

                    gatt->CopyRandomValues(&(AuthenticationResponseInformation.AuthenticationData.LESCOutOfBandData.RemoteRand));
                    gatt->CopyConfirmValues(&(AuthenticationResponseInformation.AuthenticationData.LESCOutOfBandData.RemoteConfirmation));

                    AuthenticationResponseInformation.SC = TRUE;
                    AuthenticationResponseInformation.AuthenticationDataLength = sizeof(AuthenticationResponseInformation.AuthenticationData.LESCOutOfBandData);;
                }
            }

            if ((Result = DEVM_AuthenticationResponse(gatt->GetAuthenticationCallbackID(), &AuthenticationResponseInformation)) >= 0)
                BOT_NOTIFY_INFO("DEVM_AuthenticationResponse() Success.\r\n");
            else
                BOT_NOTIFY_DEBUG("DEVM_AuthenticationResponse() Failure: %d, %s.\r\n", Result, ERR_ConvertErrorCodeToString(Result));
            break;
        case DEVM_AUTHENTICATION_ACTION_IO_CAPABILITIES_REQUEST:
            BOT_NOTIFY_DEBUG("I/O Capability Request: %s.\r\n", (LowEnergy?"LE":"BR/EDR"));

            /* Note the current Remote BD_ADDR that is requesting the   */
            /* Passkey.                                                 */
            gatt->SetCurrentRemoteBD_ADDR(AuthenticationRequestInformation->BD_ADDR);
            gatt->SetCurrentLowEnergy(LowEnergy);

            /* Respond with the currently configured I/O Capabilities.  */
            BTPS_MemInitialize(&AuthenticationResponseInformation, 0, sizeof(AuthenticationResponseInformation));

            AuthenticationResponseInformation.BD_ADDR              = AuthenticationRequestInformation->BD_ADDR;
            AuthenticationResponseInformation.AuthenticationAction = DEVM_AUTHENTICATION_ACTION_IO_CAPABILITIES_RESPONSE;

            if (gatt->GetCurrentLowEnergy())
            {
                AuthenticationResponseInformation.AuthenticationDataLength                                    = sizeof(AuthenticationResponseInformation.AuthenticationData.IOCapabilities);

                AuthenticationResponseInformation.AuthenticationData.IOCapabilities.IO_Capability             = (GAP_IO_Capability_t)gatt->GetIOCapability();
                //xxx Check if MITM should be set
                AuthenticationResponseInformation.AuthenticationData.IOCapabilities.MITM_Protection_Required  = gatt->GetMITMProtection();
                AuthenticationResponseInformation.AuthenticationData.IOCapabilities.OOB_Data_Present          = gatt->GetOOBSupport();

                //xxx Here check if this will default to "just works" or auto accept.
            }
            else
            {
                AuthenticationResponseInformation.AuthenticationAction                                       |= DEVM_AUTHENTICATION_ACTION_LOW_ENERGY_OPERATION_MASK;
                AuthenticationResponseInformation.AuthenticationDataLength                                    = sizeof(AuthenticationResponseInformation.AuthenticationData.LEIOCapabilities);
                AuthenticationResponseInformation.AuthenticationData.LEIOCapabilities.IO_Capability           = (GAP_LE_IO_Capability_t)gatt->GetLEIOCapability();
                AuthenticationResponseInformation.AuthenticationData.LEIOCapabilities.Bonding_Type            = gatt->GetBondingType();
                AuthenticationResponseInformation.AuthenticationData.LEIOCapabilities.MITM                    = gatt->GetLEMITMProtection();
                AuthenticationResponseInformation.AuthenticationData.LEIOCapabilities.SC                      = gatt->GetSC();
                AuthenticationResponseInformation.AuthenticationData.LEIOCapabilities.P256DebugMode           = gatt->GetP256DebugMode();
                AuthenticationResponseInformation.AuthenticationData.LEIOCapabilities.OOB_Present             = gatt->GetOOBSupport();
                AuthenticationResponseInformation.AuthenticationData.LEIOCapabilities.Keypress                = gatt->GetKeypress();
            }

            if ((Result = DEVM_AuthenticationResponse(gatt->GetAuthenticationCallbackID(), &AuthenticationResponseInformation)) >= 0)
                BOT_NOTIFY_INFO("DEVM_AuthenticationResponse() Success.\r\n");
            else
                BOT_NOTIFY_DEBUG("DEVM_AuthenticationResponse() Failure: %d, %s.\r\n", Result, ERR_ConvertErrorCodeToString(Result));
            break;
        case DEVM_AUTHENTICATION_ACTION_IO_CAPABILITIES_RESPONSE:
            BOT_NOTIFY_DEBUG("I/O Capability Response.\r\n");

            /* Inform the user of the Remote I/O Capablities.           */
            BOT_NOTIFY_DEBUG("Remote I/O Capabilities: %s, MITM Protection: %s.\r\n", GattSrv::IOCapabilitiesStrings[AuthenticationRequestInformation->AuthenticationData.IOCapabilities.IO_Capability], AuthenticationRequestInformation->AuthenticationData.IOCapabilities.MITM_Protection_Required?"TRUE":"FALSE");
            break;
        case DEVM_AUTHENTICATION_ACTION_AUTHENTICATION_STATUS_RESULT:
            BOT_NOTIFY_DEBUG("Authentication Status: .\r\n");

            BOT_NOTIFY_DEBUG("Status: %d\r\n", AuthenticationRequestInformation->AuthenticationData.AuthenticationStatus);
            break;
        case DEVM_AUTHENTICATION_ACTION_AUTHENTICATION_START:
            BOT_NOTIFY_DEBUG("Authentication Start.\r\n");
            break;
        case DEVM_AUTHENTICATION_ACTION_AUTHENTICATION_END:
            BOT_NOTIFY_DEBUG("Authentication End.\r\n");
            break;
        default:
            BOT_NOTIFY_DEBUG("Unknown Device Manager Authentication Event Received: 0x%08X, Length: 0x%08X.\r\n", (unsigned int)AuthenticationRequestInformation->AuthenticationAction, AuthenticationRequestInformation->AuthenticationDataLength);
            break;
        }
    }
    else
        BOT_NOTIFY_DEBUG("\r\nDEVM Authentication Request Data is NULL.\r\n");

    /* Make sure the output is displayed to the user.                    */
#ifdef TEST_CONSOLE_WRITE
    fflush(stdout);
#endif
}

/* The following function is the GATM Event Callback function that is*/
/* Registered with the Generic Attribute Profile Manager.  This      */
/* callback is responsible for processing all GATM Events.           */
static void BTPSAPI GATM_Event_Callback(GATM_Event_Data_t *EventData, void *CallbackParameter __attribute__ ((unused)))
{
    char Buffer[128];
    GattSrv *gatt = (GattSrv *) GattSrv::getInstance();

    if (EventData)
    {
        switch(EventData->EventType)
        {
        /* GATM Connection Events.                                     */
        case getGATTConnected:
            BOT_NOTIFY_DEBUG("\r\nGATT Connection Event\r\n");

            gatt->BD_ADDRToStr(EventData->EventData.ConnectedEventData.RemoteDeviceAddress, Buffer);

            BOT_NOTIFY_DEBUG("    Connection Type: %s\r\n", (EventData->EventData.ConnectedEventData.ConnectionType == gctLE)?"LE":"BR/EDR");
            BOT_NOTIFY_DEBUG("    Remote Address:  %s\r\n", Buffer);
            BOT_NOTIFY_DEBUG("    MTU:             %u\r\n", EventData->EventData.ConnectedEventData.MTU);
            break;
        case getGATTDisconnected:
            BOT_NOTIFY_DEBUG("\r\nGATT Disconnect Event\r\n");

            gatt->BD_ADDRToStr(EventData->EventData.DisconnectedEventData.RemoteDeviceAddress, Buffer);

            BOT_NOTIFY_DEBUG("    Connection Type: %s\r\n", (EventData->EventData.DisconnectedEventData.ConnectionType == gctLE)?"LE":"BR/EDR");
            BOT_NOTIFY_DEBUG("    Remote Address:  %s\r\n", Buffer);
            break;
        case getGATTConnectionMTUUpdate:
            BOT_NOTIFY_DEBUG("\r\nGATT Connection MTU Update Event\r\n");

            gatt->BD_ADDRToStr(EventData->EventData.ConnectionMTUUpdateEventData.RemoteDeviceAddress, Buffer);

            BOT_NOTIFY_DEBUG("    Connection Type: %s\r\n", (EventData->EventData.ConnectionMTUUpdateEventData.ConnectionType == gctLE)?"LE":"BR/EDR");
            BOT_NOTIFY_DEBUG("    Remote Address:  %s\r\n", Buffer);
            BOT_NOTIFY_DEBUG("    New MTU:         %u\r\n", EventData->EventData.ConnectionMTUUpdateEventData.MTU);
            break;
        case getGATTHandleValueData:
            BOT_NOTIFY_DEBUG("\r\nGATT Handle Value Data Event\r\n");

            gatt->BD_ADDRToStr(EventData->EventData.HandleValueDataEventData.RemoteDeviceAddress, Buffer);

            BOT_NOTIFY_DEBUG("    Connection Type:  %s\r\n", (EventData->EventData.HandleValueDataEventData.ConnectionType == gctLE)?"LE":"BR/EDR");
            BOT_NOTIFY_DEBUG("    Remote Address:   %s\r\n", Buffer);
            BOT_NOTIFY_DEBUG("    Type:             %s\r\n", (EventData->EventData.HandleValueDataEventData.HandleValueIndication?"Indication":"Notification"));
            BOT_NOTIFY_DEBUG("    Attribute Handle: 0x%04X (%u)\r\n", EventData->EventData.HandleValueDataEventData.AttributeHandle, EventData->EventData.HandleValueDataEventData.AttributeHandle);
            BOT_NOTIFY_DEBUG("    Value Length:     %u\r\n", EventData->EventData.HandleValueDataEventData.AttributeValueLength);
            BOT_NOTIFY_DEBUG("    Value:            \r\n");
            gatt->DumpData(FALSE, EventData->EventData.HandleValueDataEventData.AttributeValueLength, EventData->EventData.HandleValueDataEventData.AttributeValue);
            break;

            /* GATM Server Events.                                         */
        case getGATTWriteRequest:
            BOT_NOTIFY_DEBUG("\r\nGATT Write Request Event\r\n");

            gatt->BD_ADDRToStr(EventData->EventData.WriteRequestData.RemoteDeviceAddress, Buffer);

            BOT_NOTIFY_DEBUG("    Connection Type:  %s\r\n", (EventData->EventData.WriteRequestData.ConnectionType == gctLE)?"LE":"BR/EDR");
            BOT_NOTIFY_DEBUG("    Remote Address:   %s\r\n", Buffer);
            BOT_NOTIFY_DEBUG("    Service ID:       %u\r\n", EventData->EventData.WriteRequestData.ServiceID);
            BOT_NOTIFY_DEBUG("    Request ID:       %u\r\n", EventData->EventData.WriteRequestData.RequestID);
            BOT_NOTIFY_DEBUG("    Attribute Offset: %u\r\n", EventData->EventData.WriteRequestData.AttributeOffset);
            BOT_NOTIFY_DEBUG("    Data Length:      %u\r\n", EventData->EventData.WriteRequestData.DataLength);
            BOT_NOTIFY_DEBUG("    Value:            \r\n");
            gatt->DumpData(FALSE, EventData->EventData.WriteRequestData.DataLength, EventData->EventData.WriteRequestData.Data);
            BOT_NOTIFY_DEBUG("\r\n");

            /* Go ahead and process the Write Request.                  */
            gatt->ProcessWriteRequestEvent(&(EventData->EventData.WriteRequestData));
            break;
        case getGATTSignedWrite:
            BOT_NOTIFY_DEBUG("\r\nGATT Signed Write Event\r\n");

           gatt-> BD_ADDRToStr(EventData->EventData.SignedWriteData.RemoteDeviceAddress, Buffer);

            BOT_NOTIFY_DEBUG("    Connection Type:  %s\r\n", (EventData->EventData.SignedWriteData.ConnectionType == gctLE)?"LE":"BR/EDR");
            BOT_NOTIFY_DEBUG("    Remote Address:   %s\r\n", Buffer);
            BOT_NOTIFY_DEBUG("    Service ID:       %u\r\n", EventData->EventData.SignedWriteData.ServiceID);
            BOT_NOTIFY_DEBUG("    Signature:        %s\r\n", (EventData->EventData.SignedWriteData.ValidSignature?"VALID":"INVALID"));
            BOT_NOTIFY_DEBUG("    Attribute Offset: %u\r\n", EventData->EventData.SignedWriteData.AttributeOffset);
            BOT_NOTIFY_DEBUG("    Data Length:      %u\r\n", EventData->EventData.SignedWriteData.DataLength);
            BOT_NOTIFY_DEBUG("    Value:            \r\n");
            gatt->DumpData(FALSE, EventData->EventData.SignedWriteData.DataLength, EventData->EventData.SignedWriteData.Data);
            BOT_NOTIFY_DEBUG("\r\n");

            /* If the signature is valid go ahead and process the signed*/
            /* write command.                                           */
            if (EventData->EventData.SignedWriteData.ValidSignature)
                gatt->ProcessSignedWriteEvent(&(EventData->EventData.SignedWriteData));
            break;
        case getGATTReadRequest:
            BOT_NOTIFY_DEBUG("\r\nGATT Read Request Event\r\n");

            gatt->BD_ADDRToStr(EventData->EventData.ReadRequestData.RemoteDeviceAddress, Buffer);

            BOT_NOTIFY_DEBUG("    Connection Type:        %s\r\n", (EventData->EventData.ReadRequestData.ConnectionType == gctLE)?"LE":"BR/EDR");
            BOT_NOTIFY_DEBUG("    Remote Address:         %s\r\n", Buffer);
            BOT_NOTIFY_DEBUG("    Service ID:             %u\r\n", EventData->EventData.ReadRequestData.ServiceID);
            BOT_NOTIFY_DEBUG("    Request ID:             %u\r\n", EventData->EventData.ReadRequestData.RequestID);
            BOT_NOTIFY_DEBUG("    Attribute Offset:       %u\r\n", EventData->EventData.ReadRequestData.AttributeOffset);
            BOT_NOTIFY_DEBUG("    Attribute Value Offset: %u\r\n", EventData->EventData.ReadRequestData.AttributeValueOffset);

            /* Go ahead and process the Read Request.                   */
            gatt->ProcessReadRequestEvent(&(EventData->EventData.ReadRequestData));
            break;
        case getGATTPrepareWriteRequest:
            BOT_NOTIFY_DEBUG("\r\nGATT Prepare Write Request Event\r\n");

            gatt->BD_ADDRToStr(EventData->EventData.PrepareWriteRequestEventData.RemoteDeviceAddress, Buffer);

            BOT_NOTIFY_DEBUG("    Connection Type:        %s\r\n", (EventData->EventData.PrepareWriteRequestEventData.ConnectionType == gctLE)?"LE":"BR/EDR");
            BOT_NOTIFY_DEBUG("    Remote Address:         %s\r\n", Buffer);
            BOT_NOTIFY_DEBUG("    Service ID:             %u\r\n", EventData->EventData.PrepareWriteRequestEventData.ServiceID);
            BOT_NOTIFY_DEBUG("    Request ID:             %u\r\n", EventData->EventData.PrepareWriteRequestEventData.RequestID);
            BOT_NOTIFY_DEBUG("    Attribute Offset:       %u\r\n", EventData->EventData.PrepareWriteRequestEventData.AttributeOffset);
            BOT_NOTIFY_DEBUG("    Attribute Value Offset: %u\r\n", EventData->EventData.PrepareWriteRequestEventData.AttributeValueOffset);
            BOT_NOTIFY_DEBUG("    Data Length:            %u\r\n", EventData->EventData.PrepareWriteRequestEventData.DataLength);
            BOT_NOTIFY_DEBUG("    Value:                  \r\n");
            gatt->DumpData(FALSE, EventData->EventData.PrepareWriteRequestEventData.DataLength, EventData->EventData.PrepareWriteRequestEventData.Data);
            BOT_NOTIFY_DEBUG("\r\n");

            /* Go ahead and process the Prepare Write Request.          */
            gatt->ProcessPrepareWriteRequestEvent(&(EventData->EventData.PrepareWriteRequestEventData));
            break;
        case getGATTCommitPrepareWrite:
            BOT_NOTIFY_DEBUG("\r\nGATT Commit Prepare Write Event\r\n");

            gatt->BD_ADDRToStr(EventData->EventData.CommitPrepareWriteEventData.RemoteDeviceAddress, Buffer);

            BOT_NOTIFY_DEBUG("    Connection Type:        %s\r\n", (EventData->EventData.CommitPrepareWriteEventData.ConnectionType == gctLE)?"LE":"BR/EDR");
            BOT_NOTIFY_DEBUG("    Remote Address:         %s\r\n", Buffer);
            BOT_NOTIFY_DEBUG("    Service ID:             %u\r\n", EventData->EventData.CommitPrepareWriteEventData.ServiceID);
            BOT_NOTIFY_DEBUG("    Commit Writes?:         %s\r\n", (EventData->EventData.CommitPrepareWriteEventData.CommitWrites?"YES":"NO"));

            /* Go ahead and process the Execute Write Request.          */
            gatt->ProcessCommitPrepareWriteEvent(&(EventData->EventData.CommitPrepareWriteEventData));
            break;
        case getGATTHandleValueConfirmation:
            BOT_NOTIFY_DEBUG("\r\nGATT Handle-Value Confirmation Event\r\n");

            gatt->BD_ADDRToStr(EventData->EventData.HandleValueConfirmationEventData.RemoteDeviceAddress, Buffer);

            BOT_NOTIFY_DEBUG("    Connection Type:  %s\r\n", (EventData->EventData.HandleValueConfirmationEventData.ConnectionType == gctLE)?"LE":"BR/EDR");
            BOT_NOTIFY_DEBUG("    Remote Address:   %s\r\n", Buffer);
            BOT_NOTIFY_DEBUG("    Service ID:       %u\r\n", EventData->EventData.HandleValueConfirmationEventData.ServiceID);
            BOT_NOTIFY_DEBUG("    Request ID:       %u\r\n", EventData->EventData.HandleValueConfirmationEventData.TransactionID);
            BOT_NOTIFY_DEBUG("    Attribute Offset: %u\r\n", EventData->EventData.HandleValueConfirmationEventData.AttributeOffset);
            BOT_NOTIFY_DEBUG("    Status:           %u\r\n", EventData->EventData.HandleValueConfirmationEventData.Status);
            break;
        default:
            BOT_NOTIFY_DEBUG("\r\nUnhandled event.\r\n");
            break;
        }

        puts("\r\n");
    }
    else
        BOT_NOTIFY_DEBUG("\r\nGATM Event Data is NULL.\r\n");

    /* Make sure the output is displayed to the user.                    */
#ifdef TEST_CONSOLE_WRITE
    fflush(stdout);
#endif
}

/////////////////////////////// Helper funtions ///////////////////////////////////
/////////////////////////////// Helper funtions ///////////////////////////////////
/////////////////////////////// Helper funtions ///////////////////////////////////
/////////////////////////////// Helper funtions ///////////////////////////////////

/* The following function is a utility function that exists to       */
/* display either the entire Local Device Property information (first*/
/* parameter is zero) or portions that have changed.                 */
void GattSrv::DisplayLocalDeviceProperties(unsigned long UpdateMask, DEVM_Local_Device_Properties_t *LocalDeviceProperties)
{
    char Buffer[64];

    if (LocalDeviceProperties)
    {
        /* First, display any information that is not part of any update  */
        /* mask.                                                          */
        if (!UpdateMask)
        {
            BD_ADDRToStr(LocalDeviceProperties->BD_ADDR, Buffer);

            BOT_NOTIFY_DEBUG("BD_ADDR:      %s\r\n", Buffer);
            BOT_NOTIFY_DEBUG("HCI Ver:      0x%04X\r\n", (Word_t)LocalDeviceProperties->HCIVersion);
            BOT_NOTIFY_DEBUG("HCI Rev:      0x%04X\r\n", (Word_t)LocalDeviceProperties->HCIRevision);
            BOT_NOTIFY_DEBUG("LMP Ver:      0x%04X\r\n", (Word_t)LocalDeviceProperties->LMPVersion);
            BOT_NOTIFY_DEBUG("LMP Sub Ver:  0x%04X\r\n", (Word_t)LocalDeviceProperties->LMPSubVersion);
            BOT_NOTIFY_DEBUG("Device Man:   0x%04X (%s)\r\n", (Word_t)LocalDeviceProperties->DeviceManufacturer, DEVM_ConvertManufacturerNameToString(LocalDeviceProperties->DeviceManufacturer));
            BOT_NOTIFY_DEBUG("Device Flags: 0x%08lX\r\n", LocalDeviceProperties->LocalDeviceFlags);
        }
        else
        {
            if (UpdateMask & DEVM_LOCAL_DEVICE_PROPERTIES_CHANGED_DEVICE_FLAGS)
                BOT_NOTIFY_DEBUG("Device Flags: 0x%08lX\r\n", LocalDeviceProperties->LocalDeviceFlags);
        }

        if ((!UpdateMask) || (UpdateMask & DEVM_LOCAL_DEVICE_PROPERTIES_CHANGED_BLE_ADDRESS))
        {
            switch(LocalDeviceProperties->BLEAddressType)
            {
            case atPublic:
                BOT_NOTIFY_DEBUG("BLE Address Type: %s\r\n", "Public");
                break;
            case atStatic:
                BOT_NOTIFY_DEBUG("BLE Address Type: %s\r\n", "Static");
                break;
            case atPrivate_Resolvable:
                BOT_NOTIFY_DEBUG("BLE Address Type: %s\r\n", "Resolvable Random");
                break;
            case atPrivate_NonResolvable:
                BOT_NOTIFY_DEBUG("BLE Address Type: %s\r\n", "Non-Resolvable Random");
                break;
            }

            BD_ADDRToStr(LocalDeviceProperties->BLEBD_ADDR, Buffer);

            BOT_NOTIFY_DEBUG("BLE BD_ADDR:      %s\r\n", Buffer);
        }

        if ((!UpdateMask) || (UpdateMask & DEVM_LOCAL_DEVICE_PROPERTIES_CHANGED_CLASS_OF_DEVICE))
            BOT_NOTIFY_DEBUG("COD:          0x%02X%02X%02X\r\n", LocalDeviceProperties->ClassOfDevice.Class_of_Device0, LocalDeviceProperties->ClassOfDevice.Class_of_Device1, LocalDeviceProperties->ClassOfDevice.Class_of_Device2);

        if ((!UpdateMask) || (UpdateMask & DEVM_LOCAL_DEVICE_PROPERTIES_CHANGED_DEVICE_NAME))
            BOT_NOTIFY_DEBUG("Device Name:  %s\r\n", (LocalDeviceProperties->DeviceNameLength)?LocalDeviceProperties->DeviceName:"");

        if ((!UpdateMask) || (UpdateMask & DEVM_LOCAL_DEVICE_PROPERTIES_CHANGED_DISCOVERABLE_MODE))
            BOT_NOTIFY_DEBUG("Disc. Mode:   %s, 0x%08X\r\n", LocalDeviceProperties->DiscoverableMode?"TRUE ":"FALSE", LocalDeviceProperties->DiscoverableModeTimeout);

        if ((!UpdateMask) || (UpdateMask & DEVM_LOCAL_DEVICE_PROPERTIES_CHANGED_CONNECTABLE_MODE))
            BOT_NOTIFY_DEBUG("Conn. Mode:   %s, 0x%08X\r\n", LocalDeviceProperties->ConnectableMode?"TRUE ":"FALSE", LocalDeviceProperties->ConnectableModeTimeout);

        if ((!UpdateMask) || (UpdateMask & DEVM_LOCAL_DEVICE_PROPERTIES_CHANGED_PAIRABLE_MODE))
            BOT_NOTIFY_DEBUG("Pair. Mode:   %s, 0x%08X\r\n", LocalDeviceProperties->PairableMode?"TRUE ":"FALSE", LocalDeviceProperties->PairableModeTimeout);

        if ((!UpdateMask) || (UpdateMask & DEVM_LOCAL_DEVICE_PROPERTIES_CHANGED_DEVICE_FLAGS))
        {
            BOT_NOTIFY_DEBUG("LE Scan Mode:    %s, 0x%08X\r\n", (LocalDeviceProperties->LocalDeviceFlags & DEVM_LOCAL_DEVICE_FLAGS_LE_SCANNING_IN_PROGRESS)?"TRUE":"FALSE", LocalDeviceProperties->ScanTimeout);
            BOT_NOTIFY_DEBUG("LE Adv Mode:     %s, 0x%08X\r\n", (LocalDeviceProperties->LocalDeviceFlags & DEVM_LOCAL_DEVICE_FLAGS_LE_ADVERTISING_IN_PROGRESS)?"TRUE":"FALSE", LocalDeviceProperties->AdvertisingTimeout);
        }
    }
}

/* The following function is a utility function that exists to       */
/* display either the entire Remote Device Property information      */
/* (first parameter is zero) or portions that have changed.          */
void GattSrv::DisplayRemoteDeviceProperties(unsigned long UpdateMask, DEVM_Remote_Device_Properties_t *RemoteDeviceProperties)
{
    char          Buffer[64];
    Boolean_t     SingleMode;
    unsigned long LEFlags;

    if (RemoteDeviceProperties)
    {
        /* First, display any information that is not part of any update  */
        /* mask.                                                          */
        BD_ADDRToStr(RemoteDeviceProperties->BD_ADDR, Buffer);

        /* Determine what type of device this is.                         */
        LEFlags    = (RemoteDeviceProperties->RemoteDeviceFlags & (DEVM_REMOTE_DEVICE_FLAGS_DEVICE_SUPPORTS_BR_EDR | DEVM_REMOTE_DEVICE_FLAGS_DEVICE_SUPPORTS_LOW_ENERGY));
        SingleMode = (LEFlags == DEVM_REMOTE_DEVICE_FLAGS_DEVICE_SUPPORTS_LOW_ENERGY);

        /* Print the BR/EDR + LE Common Information.                      */
        BOT_NOTIFY_DEBUG("BD_ADDR:             %s\r\n", Buffer);

        if (LEFlags & DEVM_REMOTE_DEVICE_FLAGS_DEVICE_SUPPORTS_LOW_ENERGY)
        {
            /* Print the address type.                                     */
            switch(RemoteDeviceProperties->BLEAddressType)
            {
            default:
            case atPublic:
                BOT_NOTIFY_DEBUG("Address Type:        %s\r\n", "Public");
                break;
            case atStatic:
                BOT_NOTIFY_DEBUG("Address Type:        %s\r\n", "Static");
                break;
            case atPrivate_Resolvable:
                BOT_NOTIFY_DEBUG("Address Type:        %s\r\n", "Resolvable Random Address.");
                break;
            case atPrivate_NonResolvable:
                BOT_NOTIFY_DEBUG("Address Type:        %s\r\n", "Non-resolvable Random Address.");
                break;
            }
        }

        if ((!UpdateMask) || (UpdateMask & DEVM_REMOTE_DEVICE_PROPERTIES_CHANGED_DEVICE_NAME))
            BOT_NOTIFY_DEBUG("Device Name:         %s\r\n", (RemoteDeviceProperties->DeviceNameLength)?RemoteDeviceProperties->DeviceName:"");

        if (LEFlags & DEVM_REMOTE_DEVICE_FLAGS_DEVICE_SUPPORTS_LOW_ENERGY)
            BOT_NOTIFY_DEBUG("LE Type:             %s\r\n", (!SingleMode)?"Dual Mode":"Single Mode");

        if ((!UpdateMask) || (UpdateMask & DEVM_REMOTE_DEVICE_PROPERTIES_CHANGED_DEVICE_FLAGS))
            BOT_NOTIFY_DEBUG("Device Flags:        0x%08lX\r\n", RemoteDeviceProperties->RemoteDeviceFlags);

        /* Print the LE Information.                                      */
        if (LEFlags & DEVM_REMOTE_DEVICE_FLAGS_DEVICE_SUPPORTS_LOW_ENERGY)
        {
            if (((!UpdateMask) && (!COMPARE_NULL_BD_ADDR(RemoteDeviceProperties->PriorResolvableBD_ADDR))) || (UpdateMask & DEVM_REMOTE_DEVICE_PROPERTIES_CHANGED_PRIOR_RESOLVABLE_ADDRESS))
            {
                BD_ADDRToStr(RemoteDeviceProperties->PriorResolvableBD_ADDR, Buffer);
                BOT_NOTIFY_DEBUG("Resolv. BD_ADDR:     %s\r\n\r\n", Buffer);
            }

            if (((!UpdateMask) && (RemoteDeviceProperties->RemoteDeviceFlags & DEVM_REMOTE_DEVICE_FLAGS_DEVICE_LE_APPEARANCE_KNOWN)) || (UpdateMask & DEVM_REMOTE_DEVICE_PROPERTIES_CHANGED_DEVICE_APPEARANCE))
                BOT_NOTIFY_DEBUG("Device Appearance:   %u\r\n", RemoteDeviceProperties->DeviceAppearance);

            if ((!UpdateMask) || (UpdateMask & DEVM_REMOTE_DEVICE_PROPERTIES_CHANGED_LE_RSSI))
                BOT_NOTIFY_DEBUG("LE RSSI:             %d\r\n", RemoteDeviceProperties->LE_RSSI);

            if ((!UpdateMask) && (RemoteDeviceProperties->RemoteDeviceFlags & DEVM_REMOTE_DEVICE_FLAGS_DEVICE_LE_TX_POWER_KNOWN))
                BOT_NOTIFY_DEBUG("LE Trans. Power:     %d\r\n", RemoteDeviceProperties->LETransmitPower);

            if ((!UpdateMask) || (UpdateMask & DEVM_REMOTE_DEVICE_PROPERTIES_CHANGED_LE_PAIRING_STATE))
                BOT_NOTIFY_DEBUG("LE Paired State :    %s\r\n", (RemoteDeviceProperties->RemoteDeviceFlags & DEVM_REMOTE_DEVICE_FLAGS_DEVICE_CURRENTLY_PAIRED_OVER_LE)?"TRUE":"FALSE");

            if ((!UpdateMask) || (UpdateMask & DEVM_REMOTE_DEVICE_PROPERTIES_CHANGED_LE_CONNECTION_STATE))
                BOT_NOTIFY_DEBUG("LE Connect State:    %s\r\n", (RemoteDeviceProperties->RemoteDeviceFlags & DEVM_REMOTE_DEVICE_FLAGS_DEVICE_CURRENTLY_CONNECTED_OVER_LE)?"TRUE":"FALSE");

            if ((!UpdateMask) || (UpdateMask & DEVM_REMOTE_DEVICE_PROPERTIES_CHANGED_LE_ENCRYPTION_STATE))
                BOT_NOTIFY_DEBUG("LE Encrypt State:    %s\r\n", (RemoteDeviceProperties->RemoteDeviceFlags & DEVM_REMOTE_DEVICE_FLAGS_DEVICE_LE_LINK_CURRENTLY_ENCRYPTED)?"TRUE":"FALSE");

            if ((!UpdateMask) || (UpdateMask & DEVM_REMOTE_DEVICE_PROPERTIES_CHANGED_LE_SERVICES_STATE))
                BOT_NOTIFY_DEBUG("GATT Services Known: %s\r\n", (RemoteDeviceProperties->RemoteDeviceFlags & DEVM_REMOTE_DEVICE_FLAGS_DEVICE_LE_SERVICES_KNOWN)?"TRUE":"FALSE");
        }

        /* Print the BR/EDR Only information.                             */
        if (!SingleMode)
        {
            if ((!UpdateMask) || (UpdateMask & DEVM_REMOTE_DEVICE_PROPERTIES_CHANGED_RSSI))
                BOT_NOTIFY_DEBUG("RSSI:                %d\r\n", RemoteDeviceProperties->RSSI);

            if ((!UpdateMask) && (RemoteDeviceProperties->RemoteDeviceFlags & DEVM_REMOTE_DEVICE_FLAGS_DEVICE_TX_POWER_KNOWN))
                BOT_NOTIFY_DEBUG("Trans. Power:        %d\r\n", RemoteDeviceProperties->TransmitPower);

            if (((!UpdateMask) || ((UpdateMask & DEVM_REMOTE_DEVICE_PROPERTIES_CHANGED_APPLICATION_DATA) && (RemoteDeviceProperties->RemoteDeviceFlags & DEVM_REMOTE_DEVICE_FLAGS_DEVICE_APPLICATION_DATA_VALID))))
            {
                BOT_NOTIFY_DEBUG("Friendly Name:       %s\r\n", (RemoteDeviceProperties->ApplicationData.FriendlyNameLength)?RemoteDeviceProperties->ApplicationData.FriendlyName:"");

                BOT_NOTIFY_DEBUG("App. Info:   :       %08lX\r\n", RemoteDeviceProperties->ApplicationData.ApplicationInfo);
            }

            if ((!UpdateMask) || (UpdateMask & DEVM_REMOTE_DEVICE_PROPERTIES_CHANGED_PAIRING_STATE))
                BOT_NOTIFY_DEBUG("Paired State :       %s\r\n", (RemoteDeviceProperties->RemoteDeviceFlags & DEVM_REMOTE_DEVICE_FLAGS_DEVICE_CURRENTLY_PAIRED)?"TRUE":"FALSE");

            if ((!UpdateMask) || (UpdateMask & DEVM_REMOTE_DEVICE_PROPERTIES_CHANGED_CONNECTION_STATE))
                BOT_NOTIFY_DEBUG("Connect State:       %s\r\n", (RemoteDeviceProperties->RemoteDeviceFlags & DEVM_REMOTE_DEVICE_FLAGS_DEVICE_CURRENTLY_CONNECTED)?"TRUE":"FALSE");

            if ((!UpdateMask) || (UpdateMask & DEVM_REMOTE_DEVICE_PROPERTIES_CHANGED_ENCRYPTION_STATE))
                BOT_NOTIFY_DEBUG("Encrypt State:       %s\r\n", (RemoteDeviceProperties->RemoteDeviceFlags & DEVM_REMOTE_DEVICE_FLAGS_DEVICE_LINK_CURRENTLY_ENCRYPTED)?"TRUE":"FALSE");

            if (((!UpdateMask) || (UpdateMask & DEVM_REMOTE_DEVICE_PROPERTIES_CHANGED_SNIFF_STATE)))
            {
                if (RemoteDeviceProperties->RemoteDeviceFlags & DEVM_REMOTE_DEVICE_FLAGS_DEVICE_LINK_CURRENTLY_SNIFF_MODE)
                    BOT_NOTIFY_DEBUG("Sniff State  :       TRUE (%u ms)\r\n", RemoteDeviceProperties->SniffInterval);
                else
                    BOT_NOTIFY_DEBUG("Sniff State  :       FALSE\r\n");
            }

            if (((!UpdateMask) || (UpdateMask & DEVM_REMOTE_DEVICE_PROPERTIES_CHANGED_CLASS_OF_DEVICE)))
                BOT_NOTIFY_DEBUG("COD:                 0x%02X%02X%02X\r\n", RemoteDeviceProperties->ClassOfDevice.Class_of_Device0, RemoteDeviceProperties->ClassOfDevice.Class_of_Device1, RemoteDeviceProperties->ClassOfDevice.Class_of_Device2);

            if ((!UpdateMask) || (UpdateMask & DEVM_REMOTE_DEVICE_PROPERTIES_CHANGED_SERVICES_STATE))
                BOT_NOTIFY_DEBUG("SDP Serv. Known :    %s\r\n", (RemoteDeviceProperties->RemoteDeviceFlags & DEVM_REMOTE_DEVICE_FLAGS_DEVICE_SERVICES_KNOWN)?"TRUE":"FALSE");
        }
    }
}

/* The following function is a utility function that is used to      */
/* dispay a GATT UUID.                                               */
void GattSrv::DisplayGATTUUID(GATT_UUID_t *UUID, const char *Prefix, unsigned int Level)
{
    if ((UUID) && (Prefix))
    {
        switch (UUID->UUID_Type)
        {
        case guUUID_16:
        {
            BOT_NOTIFY_DEBUG("%*s %s: %02X%02X\r\n", (Level*INDENT_LENGTH), "", Prefix, UUID->UUID.UUID_16.UUID_Byte1, UUID->UUID.UUID_16.UUID_Byte0);
            break;
        }
        case guUUID_32:
        {
            BOT_NOTIFY_DEBUG("%*s %s: %02X%02X%02X%02X\r\n", (Level*INDENT_LENGTH), "", Prefix,
                   UUID->UUID.UUID_128.UUID_Byte3,  UUID->UUID.UUID_128.UUID_Byte2,
                   UUID->UUID.UUID_128.UUID_Byte1,  UUID->UUID.UUID_128.UUID_Byte0);
            break;
        }
        case guUUID_128:
        {
            BOT_NOTIFY_DEBUG("%*s %s: %02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X\r\n", (Level*INDENT_LENGTH), "", Prefix,
                   UUID->UUID.UUID_128.UUID_Byte15, UUID->UUID.UUID_128.UUID_Byte14, UUID->UUID.UUID_128.UUID_Byte13,
                   UUID->UUID.UUID_128.UUID_Byte12, UUID->UUID.UUID_128.UUID_Byte11, UUID->UUID.UUID_128.UUID_Byte10,
                   UUID->UUID.UUID_128.UUID_Byte9,  UUID->UUID.UUID_128.UUID_Byte8,  UUID->UUID.UUID_128.UUID_Byte7,
                   UUID->UUID.UUID_128.UUID_Byte6,  UUID->UUID.UUID_128.UUID_Byte5,  UUID->UUID.UUID_128.UUID_Byte4,
                   UUID->UUID.UUID_128.UUID_Byte3,  UUID->UUID.UUID_128.UUID_Byte2,  UUID->UUID.UUID_128.UUID_Byte1,
                   UUID->UUID.UUID_128.UUID_Byte0);
            break;
        }
        }
    }
}

/* The following function is responsible for displaying the contents */
/* of Parsed Remote Device Services Data to the display.             */
void GattSrv::DisplayParsedGATTServiceData(DEVM_Parsed_Services_Data_t *ParsedGATTData)
{
    unsigned int Index;
    unsigned int Index1;
    unsigned int Index2;

    /* First, check to see if Service Records were returned.             */
    if (ParsedGATTData)
    {
        /* Print the number of GATT Services in the Record.               */
        BOT_NOTIFY_DEBUG("Number of Services: %u\r\n", ParsedGATTData->NumberServices);

        for (Index=0;Index<ParsedGATTData->NumberServices;Index++)
        {
            DisplayGATTUUID(&(ParsedGATTData->GATTServiceDiscoveryIndicationData[Index].ServiceInformation.UUID), "Service UUID", 0);

            BOT_NOTIFY_DEBUG("%*s Start Handle: 0x%04X (%d)\r\n", (1*INDENT_LENGTH), "", ParsedGATTData->GATTServiceDiscoveryIndicationData[Index].ServiceInformation.Service_Handle, ParsedGATTData->GATTServiceDiscoveryIndicationData[Index].ServiceInformation.Service_Handle);
            BOT_NOTIFY_DEBUG("%*s End Handle:   0x%04X (%d)\r\n", (1*INDENT_LENGTH), "", ParsedGATTData->GATTServiceDiscoveryIndicationData[Index].ServiceInformation.End_Group_Handle, ParsedGATTData->GATTServiceDiscoveryIndicationData[Index].ServiceInformation.End_Group_Handle);

            /* Check to see if there are included services.                */
            if (ParsedGATTData->GATTServiceDiscoveryIndicationData[Index].NumberOfIncludedService)
            {
                for (Index1=0;Index1<ParsedGATTData->GATTServiceDiscoveryIndicationData[Index].NumberOfIncludedService;Index1++)
                {
                    DisplayGATTUUID(&(ParsedGATTData->GATTServiceDiscoveryIndicationData[Index].IncludedServiceList[Index1].UUID), "Included Service UUID", 2);

                    BOT_NOTIFY_DEBUG("%*s Start Handle: 0x%04X (%d)\r\n", (2*INDENT_LENGTH), "", ParsedGATTData->GATTServiceDiscoveryIndicationData[Index].IncludedServiceList[Index1].Service_Handle, ParsedGATTData->GATTServiceDiscoveryIndicationData[Index].IncludedServiceList[Index1].Service_Handle);
                    BOT_NOTIFY_DEBUG("%*s End Handle:   0x%04X (%d)\r\n", (2*INDENT_LENGTH), "", ParsedGATTData->GATTServiceDiscoveryIndicationData[Index].IncludedServiceList[Index1].End_Group_Handle, ParsedGATTData->GATTServiceDiscoveryIndicationData[Index].IncludedServiceList[Index1].End_Group_Handle);
                    BOT_NOTIFY_DEBUG("\r\n");
                }
            }

            /* Check to see if there are characteristics.                  */
            if (ParsedGATTData->GATTServiceDiscoveryIndicationData[Index].NumberOfCharacteristics)
            {
                for (Index1=0;Index1<ParsedGATTData->GATTServiceDiscoveryIndicationData[Index].NumberOfCharacteristics;Index1++)
                {
                    DisplayGATTUUID(&(ParsedGATTData->GATTServiceDiscoveryIndicationData[Index].CharacteristicInformationList[Index1].Characteristic_UUID), "Characteristic UUID", 2);

                    BOT_NOTIFY_DEBUG("%*s Handle:     0x%04X (%d)\r\n", (2*INDENT_LENGTH), "", ParsedGATTData->GATTServiceDiscoveryIndicationData[Index].CharacteristicInformationList[Index1].Characteristic_Handle, ParsedGATTData->GATTServiceDiscoveryIndicationData[Index].CharacteristicInformationList[Index1].Characteristic_Handle);
                    BOT_NOTIFY_DEBUG("%*s Properties: 0x%02X\r\n", (2*INDENT_LENGTH), "", ParsedGATTData->GATTServiceDiscoveryIndicationData[Index].CharacteristicInformationList[Index1].Characteristic_Properties);

                    /* Loop through the descriptors for this characteristic. */
                    for (Index2=0;Index2<ParsedGATTData->GATTServiceDiscoveryIndicationData[Index].CharacteristicInformationList[Index1].NumberOfDescriptors;Index2++)
                    {
                        if (Index2==0)
                            BOT_NOTIFY_DEBUG("\r\n");

                        DisplayGATTUUID(&(ParsedGATTData->GATTServiceDiscoveryIndicationData[Index].CharacteristicInformationList[Index1].DescriptorList[Index2].Characteristic_Descriptor_UUID), "Descriptor UUID", 3);
                        BOT_NOTIFY_DEBUG("%*s Handle:     0x%04X (%d)\r\n", (3*INDENT_LENGTH), "", ParsedGATTData->GATTServiceDiscoveryIndicationData[Index].CharacteristicInformationList[Index1].DescriptorList[Index2].Characteristic_Descriptor_Handle, ParsedGATTData->GATTServiceDiscoveryIndicationData[Index].CharacteristicInformationList[Index1].DescriptorList[Index2].Characteristic_Descriptor_Handle);
                    }

                    if (Index2>0)
                        BOT_NOTIFY_DEBUG("\r\n");
                }
            }
        }
    }
    else
        BOT_NOTIFY_DEBUG("No GATT Service Records Found.\r\n");
}

/* The following function is responsible for displaying the contents */
/* of Parsed Remote Device Services Data to the display.             */
void GattSrv::DisplayParsedSDPServiceData(DEVM_Parsed_SDP_Data_t *ParsedSDPData)
{
    unsigned int Index;

    /* First, check to see if Service Records were returned.             */
    if ((ParsedSDPData) && (ParsedSDPData->NumberServiceRecords))
    {
        /* Loop through all returned SDP Service Records.                 */
        for (Index=0;Index<ParsedSDPData->NumberServiceRecords;Index++)
        {
            /* First display the number of SDP Service Records we are      */
            /* currently processing.                                       */
            BOT_NOTIFY_DEBUG("Service Record: %u:\r\n", (Index + 1));

            /* Call Display SDPAttributeResponse for all SDP Service       */
            /* Records received.                                           */
            DisplaySDPAttributeResponse(&(ParsedSDPData->SDPServiceAttributeResponseData[Index]), 1);
        }
    }
    else
        BOT_NOTIFY_DEBUG("No SDP Service Records Found.\r\n");
}

/* The following function is responsible for Displaying the contents */
/* of an SDP Service Attribute Response to the display.              */
void GattSrv::DisplaySDPAttributeResponse(SDP_Service_Attribute_Response_Data_t *SDPServiceAttributeResponse, unsigned int InitLevel)
{
    int Index;

    /* First, check to make sure that there were Attributes returned.    */
    if (SDPServiceAttributeResponse->Number_Attribute_Values)
    {
        /* Loop through all returned SDP Attribute Values.                */
        for (Index = 0; Index < SDPServiceAttributeResponse->Number_Attribute_Values; Index++)
        {
            /* First Print the Attribute ID that was returned.             */
            BOT_NOTIFY_DEBUG("%*s Attribute ID 0x%04X\r\n", (InitLevel*INDENT_LENGTH), "", SDPServiceAttributeResponse->SDP_Service_Attribute_Value_Data[Index].Attribute_ID);

            /* Now Print out all of the SDP Data Elements that were        */
            /* returned that are associated with the SDP Attribute.        */
            DisplayDataElement(SDPServiceAttributeResponse->SDP_Service_Attribute_Value_Data[Index].SDP_Data_Element, (InitLevel + 1));
        }
    }
    else
        BOT_NOTIFY_DEBUG("No SDP Attributes Found.\r\n");
}

/* The following function is responsible for actually displaying an  */
/* individual SDP Data Element to the Display.  The Level Parameter  */
/* is used in conjunction with the defined INDENT_LENGTH constant to */
/* make readability easier when displaying Data Element Sequences    */
/* and Data Element Alternatives.  This function will recursively    */
/* call itself to display the contents of Data Element Sequences and */
/* Data Element Alternatives when it finds these Data Types (and     */
/* increments the Indent Level accordingly).                         */
void GattSrv::DisplayDataElement(SDP_Data_Element_t *SDPDataElement, unsigned int Level)
{
    unsigned int Index;
    char         Buffer[256];

    switch(SDPDataElement->SDP_Data_Element_Type)
    {
    case deNIL:
        /* Display the NIL Type.                                       */
        BOT_NOTIFY_DEBUG("%*s Type: NIL\r\n", (Level*INDENT_LENGTH), "");
        break;
    case deNULL:
        /* Display the NULL Type.                                      */
        BOT_NOTIFY_DEBUG("%*s Type: NULL\r\n", (Level*INDENT_LENGTH), "");
        break;
    case deUnsignedInteger1Byte:
        /* Display the Unsigned Integer (1 Byte) Type.                 */
        BOT_NOTIFY_DEBUG("%*s Type: Unsigned Int = 0x%02X\r\n", (Level*INDENT_LENGTH), "", SDPDataElement->SDP_Data_Element.UnsignedInteger1Byte);
        break;
    case deUnsignedInteger2Bytes:
        /* Display the Unsigned Integer (2 Bytes) Type.                */
        BOT_NOTIFY_DEBUG("%*s Type: Unsigned Int = 0x%04X\r\n", (Level*INDENT_LENGTH), "", SDPDataElement->SDP_Data_Element.UnsignedInteger2Bytes);
        break;
    case deUnsignedInteger4Bytes:
        /* Display the Unsigned Integer (4 Bytes) Type.                */
        BOT_NOTIFY_DEBUG("%*s Type: Unsigned Int = 0x%08X\r\n", (Level*INDENT_LENGTH), "", (unsigned int)SDPDataElement->SDP_Data_Element.UnsignedInteger4Bytes);
        break;
    case deUnsignedInteger8Bytes:
        /* Display the Unsigned Integer (8 Bytes) Type.                */
        BOT_NOTIFY_DEBUG("%*s Type: Unsigned Int = 0x%02X%02X%02X%02X%02X%02X%02X%02X\r\n", (Level*INDENT_LENGTH), "", SDPDataElement->SDP_Data_Element.UnsignedInteger8Bytes[7],
                SDPDataElement->SDP_Data_Element.UnsignedInteger8Bytes[6],
                SDPDataElement->SDP_Data_Element.UnsignedInteger8Bytes[5],
                SDPDataElement->SDP_Data_Element.UnsignedInteger8Bytes[4],
                SDPDataElement->SDP_Data_Element.UnsignedInteger8Bytes[3],
                SDPDataElement->SDP_Data_Element.UnsignedInteger8Bytes[2],
                SDPDataElement->SDP_Data_Element.UnsignedInteger8Bytes[1],
                SDPDataElement->SDP_Data_Element.UnsignedInteger8Bytes[0]);
        break;
    case deUnsignedInteger16Bytes:
        /* Display the Unsigned Integer (16 Bytes) Type.               */
        BOT_NOTIFY_DEBUG("%*s Type: Unsigned Int = 0x%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X\r\n", (Level*INDENT_LENGTH), "", SDPDataElement->SDP_Data_Element.UnsignedInteger16Bytes[15],
                SDPDataElement->SDP_Data_Element.UnsignedInteger16Bytes[14],
                SDPDataElement->SDP_Data_Element.UnsignedInteger16Bytes[13],
                SDPDataElement->SDP_Data_Element.UnsignedInteger16Bytes[12],
                SDPDataElement->SDP_Data_Element.UnsignedInteger16Bytes[11],
                SDPDataElement->SDP_Data_Element.UnsignedInteger16Bytes[10],
                SDPDataElement->SDP_Data_Element.UnsignedInteger16Bytes[9],
                SDPDataElement->SDP_Data_Element.UnsignedInteger16Bytes[8],
                SDPDataElement->SDP_Data_Element.UnsignedInteger16Bytes[7],
                SDPDataElement->SDP_Data_Element.UnsignedInteger16Bytes[6],
                SDPDataElement->SDP_Data_Element.UnsignedInteger16Bytes[5],
                SDPDataElement->SDP_Data_Element.UnsignedInteger16Bytes[4],
                SDPDataElement->SDP_Data_Element.UnsignedInteger16Bytes[3],
                SDPDataElement->SDP_Data_Element.UnsignedInteger16Bytes[2],
                SDPDataElement->SDP_Data_Element.UnsignedInteger16Bytes[1],
                SDPDataElement->SDP_Data_Element.UnsignedInteger16Bytes[0]);
        break;
    case deSignedInteger1Byte:
        /* Display the Signed Integer (1 Byte) Type.                   */
        BOT_NOTIFY_DEBUG("%*s Type: Signed Int = 0x%02X\r\n", (Level*INDENT_LENGTH), "", SDPDataElement->SDP_Data_Element.SignedInteger1Byte);
        break;
    case deSignedInteger2Bytes:
        /* Display the Signed Integer (2 Bytes) Type.                  */
        BOT_NOTIFY_DEBUG("%*s Type: Signed Int = 0x%04X\r\n", (Level*INDENT_LENGTH), "", SDPDataElement->SDP_Data_Element.SignedInteger2Bytes);
        break;
    case deSignedInteger4Bytes:
        /* Display the Signed Integer (4 Bytes) Type.                  */
        BOT_NOTIFY_DEBUG("%*s Type: Signed Int = 0x%08X\r\n", (Level*INDENT_LENGTH), "", (unsigned int)SDPDataElement->SDP_Data_Element.SignedInteger4Bytes);
        break;
    case deSignedInteger8Bytes:
        /* Display the Signed Integer (8 Bytes) Type.                  */
        BOT_NOTIFY_DEBUG("%*s Type: Signed Int = 0x%02X%02X%02X%02X%02X%02X%02X%02X\r\n", (Level*INDENT_LENGTH), "", SDPDataElement->SDP_Data_Element.SignedInteger8Bytes[7],
                SDPDataElement->SDP_Data_Element.SignedInteger8Bytes[6],
                SDPDataElement->SDP_Data_Element.SignedInteger8Bytes[5],
                SDPDataElement->SDP_Data_Element.SignedInteger8Bytes[4],
                SDPDataElement->SDP_Data_Element.SignedInteger8Bytes[3],
                SDPDataElement->SDP_Data_Element.SignedInteger8Bytes[2],
                SDPDataElement->SDP_Data_Element.SignedInteger8Bytes[1],
                SDPDataElement->SDP_Data_Element.SignedInteger8Bytes[0]);
        break;
    case deSignedInteger16Bytes:
        /* Display the Signed Integer (16 Bytes) Type.                 */
        BOT_NOTIFY_DEBUG("%*s Type: Signed Int = 0x%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X\r\n", (Level*INDENT_LENGTH), "", SDPDataElement->SDP_Data_Element.SignedInteger16Bytes[15],
                SDPDataElement->SDP_Data_Element.SignedInteger16Bytes[14],
                SDPDataElement->SDP_Data_Element.SignedInteger16Bytes[13],
                SDPDataElement->SDP_Data_Element.SignedInteger16Bytes[12],
                SDPDataElement->SDP_Data_Element.SignedInteger16Bytes[11],
                SDPDataElement->SDP_Data_Element.SignedInteger16Bytes[10],
                SDPDataElement->SDP_Data_Element.SignedInteger16Bytes[9],
                SDPDataElement->SDP_Data_Element.SignedInteger16Bytes[8],
                SDPDataElement->SDP_Data_Element.SignedInteger16Bytes[7],
                SDPDataElement->SDP_Data_Element.SignedInteger16Bytes[6],
                SDPDataElement->SDP_Data_Element.SignedInteger16Bytes[5],
                SDPDataElement->SDP_Data_Element.SignedInteger16Bytes[4],
                SDPDataElement->SDP_Data_Element.SignedInteger16Bytes[3],
                SDPDataElement->SDP_Data_Element.SignedInteger16Bytes[2],
                SDPDataElement->SDP_Data_Element.SignedInteger16Bytes[1],
                SDPDataElement->SDP_Data_Element.SignedInteger16Bytes[0]);
        break;
    case deTextString:
        /* First retrieve the Length of the Text String so that we can */
        /* copy the Data into our Buffer.                              */
        Index = (SDPDataElement->SDP_Data_Element_Length < sizeof(Buffer))?SDPDataElement->SDP_Data_Element_Length:(sizeof(Buffer)-1);

        /* Copy the Text String into the Buffer and then NULL terminate*/
        /* it.                                                         */
        memcpy(Buffer, SDPDataElement->SDP_Data_Element.TextString, Index);
        Buffer[Index] = '\0';

        BOT_NOTIFY_DEBUG("%*s Type: Text String = %s\r\n", (Level*INDENT_LENGTH), "", Buffer);
        break;
    case deBoolean:
        BOT_NOTIFY_DEBUG("%*s Type: boolean = %s\r\n", (Level*INDENT_LENGTH), "", (SDPDataElement->SDP_Data_Element.Boolean)?"TRUE":"FALSE");
        break;
    case deURL:
        /* First retrieve the Length of the URL String so that we can  */
        /* copy the Data into our Buffer.                              */
        Index = (SDPDataElement->SDP_Data_Element_Length < sizeof(Buffer))?SDPDataElement->SDP_Data_Element_Length:(sizeof(Buffer)-1);

        /* Copy the URL String into the Buffer and then NULL terminate */
        /* it.                                                         */
        memcpy(Buffer, SDPDataElement->SDP_Data_Element.URL, Index);
        Buffer[Index] = '\0';

        BOT_NOTIFY_DEBUG("%*s Type: URL = %s\r\n", (Level*INDENT_LENGTH), "", Buffer);
        break;
    case deUUID_16:
        BOT_NOTIFY_DEBUG("%*s Type: UUID_16 = 0x%02X%02X\r\n", (Level*INDENT_LENGTH), "", SDPDataElement->SDP_Data_Element.UUID_16.UUID_Byte0,
               SDPDataElement->SDP_Data_Element.UUID_16.UUID_Byte1);
        break;
    case deUUID_32:
        BOT_NOTIFY_DEBUG("%*s Type: UUID_32 = 0x%02X%02X%02X%02X\r\n", (Level*INDENT_LENGTH), "", SDPDataElement->SDP_Data_Element.UUID_32.UUID_Byte0,
               SDPDataElement->SDP_Data_Element.UUID_32.UUID_Byte1,
               SDPDataElement->SDP_Data_Element.UUID_32.UUID_Byte2,
               SDPDataElement->SDP_Data_Element.UUID_32.UUID_Byte3);
        break;
    case deUUID_128:
        BOT_NOTIFY_DEBUG("%*s Type: UUID_128 = 0x%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X\r\n", (Level*INDENT_LENGTH), "", SDPDataElement->SDP_Data_Element.UUID_128.UUID_Byte0,
               SDPDataElement->SDP_Data_Element.UUID_128.UUID_Byte1,
               SDPDataElement->SDP_Data_Element.UUID_128.UUID_Byte2,
               SDPDataElement->SDP_Data_Element.UUID_128.UUID_Byte3,
               SDPDataElement->SDP_Data_Element.UUID_128.UUID_Byte4,
               SDPDataElement->SDP_Data_Element.UUID_128.UUID_Byte5,
               SDPDataElement->SDP_Data_Element.UUID_128.UUID_Byte6,
               SDPDataElement->SDP_Data_Element.UUID_128.UUID_Byte7,
               SDPDataElement->SDP_Data_Element.UUID_128.UUID_Byte8,
               SDPDataElement->SDP_Data_Element.UUID_128.UUID_Byte9,
               SDPDataElement->SDP_Data_Element.UUID_128.UUID_Byte10,
               SDPDataElement->SDP_Data_Element.UUID_128.UUID_Byte11,
               SDPDataElement->SDP_Data_Element.UUID_128.UUID_Byte12,
               SDPDataElement->SDP_Data_Element.UUID_128.UUID_Byte13,
               SDPDataElement->SDP_Data_Element.UUID_128.UUID_Byte14,
               SDPDataElement->SDP_Data_Element.UUID_128.UUID_Byte15);
        break;
    case deSequence:
        /* Display that this is a SDP Data Element Sequence.           */
        BOT_NOTIFY_DEBUG("%*s Type: Data Element Sequence\r\n", (Level*INDENT_LENGTH), "");

        /* Loop through each of the SDP Data Elements in the SDP Data  */
        /* Element Sequence.                                           */
        for (Index = 0; Index < SDPDataElement->SDP_Data_Element_Length; Index++)
        {
            /* Call this function again for each of the SDP Data        */
            /* Elements in this SDP Data Element Sequence.              */
            DisplayDataElement(&(SDPDataElement->SDP_Data_Element.SDP_Data_Element_Sequence[Index]), (Level + 1));
        }
        break;
    case deAlternative:
        /* Display that this is a SDP Data Element Alternative.        */
        BOT_NOTIFY_DEBUG("%*s Type: Data Element Alternative\r\n", (Level*INDENT_LENGTH), "");

        /* Loop through each of the SDP Data Elements in the SDP Data  */
        /* Element Alternative.                                        */
        for (Index = 0; Index < SDPDataElement->SDP_Data_Element_Length; Index++)
        {
            /* Call this function again for each of the SDP Data        */
            /* Elements in this SDP Data Element Alternative.           */
            DisplayDataElement(&(SDPDataElement->SDP_Data_Element.SDP_Data_Element_Alternative[Index]), (Level + 1));
        }
        break;
    default:
        BOT_NOTIFY_DEBUG("%*s Unknown SDP Data Element Type\r\n", (Level*INDENT_LENGTH), "");
        break;
    }
}

/* The following function is the Callback function that is installed */
/* to be notified when any local IPC connection to the server has    */
/* been lost.  This case only occurs when the Server exits.  This    */
/* callback allows the application mechanism to be notified so that  */
/* all resources can be cleaned up (i.e.  call BTPM_Cleanup().       */
void GattSrv::BTPSAPI ServerUnRegistrationCallback(void *CallbackParameter __attribute__ ((unused)))
{
    BOT_NOTIFY_DEBUG("Server has been Un-Registered.\r\n");

    GattSrv *gatt = (GattSrv *) GattSrv::instance;
    if (gatt != NULL)
    {
        /* Reset the GATM Event Callback ID.                                 */
        gatt->mGATMCallbackID = 0;

        /* Wait on access to the Service Info List Mutex.                    */
        if (BTPS_WaitMutex(gatt->mServiceMutex, BTPS_INFINITE_WAIT))
        {
            /* Free the prepare write list.                                   */
            gatt->FreePrepareWriteEntryList(&gatt->mPrepareWriteList);
            gatt->FreeServerList();

            /* Release the mutex that was previously acquired.                */
            BTPS_ReleaseMutex(gatt->mServiceMutex);
        }
    }

    /* Make sure the output is displayed to the user.                    */
#ifdef TEST_CONSOLE_WRITE
    fflush(stdout);
#endif
}

/* The following function is responsible for converting data of type */
/* BD_ADDR to a string.  The first parameter of this function is the */
/* BD_ADDR to be converted to a string.  The second parameter of this*/
/* function is a pointer to the string in which the converted BD_ADDR*/
/* is to be stored.                                                  */
void GattSrv::BD_ADDRToStr(BD_ADDR_t Board_Address, char *BoardStr)
{
    sprintf(BoardStr, "%02X%02X%02X%02X%02X%02X", Board_Address.BD_ADDR5,
            Board_Address.BD_ADDR4,
            Board_Address.BD_ADDR3,
            Board_Address.BD_ADDR2,
            Board_Address.BD_ADDR1,
            Board_Address.BD_ADDR0);
}

/* The following function is responsible for the specified string    */
/* into data of type BD_ADDR.  The first parameter of this function  */
/* is the BD_ADDR string to be converted to a BD_ADDR.  The second   */
/* parameter of this function is a pointer to the BD_ADDR in which   */
/* the converted BD_ADDR String is to be stored.                     */
void GattSrv::StrToBD_ADDR(char *BoardStr, BD_ADDR_t *Board_Address)
{
    unsigned int Address[sizeof(BD_ADDR_t)];

    if ((BoardStr) && (strlen(BoardStr) == sizeof(BD_ADDR_t)*2) && (Board_Address))
    {
        sscanf(BoardStr, "%02X%02X%02X%02X%02X%02X", &(Address[5]), &(Address[4]), &(Address[3]), &(Address[2]), &(Address[1]), &(Address[0]));

        Board_Address->BD_ADDR5 = (Byte_t)Address[5];
        Board_Address->BD_ADDR4 = (Byte_t)Address[4];
        Board_Address->BD_ADDR3 = (Byte_t)Address[3];
        Board_Address->BD_ADDR2 = (Byte_t)Address[2];
        Board_Address->BD_ADDR1 = (Byte_t)Address[1];
        Board_Address->BD_ADDR0 = (Byte_t)Address[0];
    }
    else
    {
        if (Board_Address)
            BTPS_MemInitialize(Board_Address, 0, sizeof(BD_ADDR_t));
    }
}

/* The following function is responsible for converting the specified*/
/* string into data of type SDP_UUID_Entry_t. The first parameter    */
/* of this function is the UUID string to be converted to an SDP     */
/* UUID Entry. The second parameter of this function is a pointer to */
/* the SDP_UUID_Entry_t in which the converted UUID String is to be  */
/* stored.                                                           */
void GattSrv::StrToUUIDEntry(char *UUIDStr, SDP_UUID_Entry_t *UUIDEntry)
{
    Byte_t       *UUIDBuffer;
    unsigned int  RemainingDigits;

    if ((UUIDStr) && (UUIDEntry))
    {
        BTPS_MemInitialize(UUIDEntry, 0, sizeof(SDP_UUID_Entry_t));

        /* We always treat UUIDs as hex, so skip any "hex format" prefix. */
        if ((strlen(UUIDStr) >= 2) && (UUIDStr[0] == '0') && ((UUIDStr[1] == 'x') || (UUIDStr[1] == 'X')))
            UUIDStr += 2;

        switch(strlen(UUIDStr))
        {
        case 4:
            UUIDEntry->SDP_Data_Element_Type = deUUID_16;
            UUIDBuffer                       = (Byte_t *)(&UUIDEntry->UUID_Value.UUID_16);
            RemainingDigits                  = 4;
            break;
        case 8:
            UUIDEntry->SDP_Data_Element_Type = deUUID_32;
            UUIDBuffer                       = (Byte_t *)(&UUIDEntry->UUID_Value.UUID_32);
            RemainingDigits                  = 8;
            break;
        case 32:
        case 36:
            UUIDEntry->SDP_Data_Element_Type = deUUID_128;
            UUIDBuffer                       = (Byte_t *)(&UUIDEntry->UUID_Value.UUID_128);
            RemainingDigits                  = 32;
            break;
        default:
            UUIDEntry->SDP_Data_Element_Type = deNULL;
            UUIDBuffer                       = NULL;
            RemainingDigits                  = 0;
            break;
        }

        while((UUIDStr) && (UUIDBuffer) && (*UUIDStr != '\0') && (RemainingDigits > 0))
        {
            if ((*UUIDStr == '-') && (UUIDEntry->SDP_Data_Element_Type == deUUID_128))
            {
                /* Skip the dashes that appear in full-length UUID values.  */
                UUIDStr++;
            }
            else
            {
                if ((*UUIDStr >= '0') && (*UUIDStr <= '9'))
                {
                    *UUIDBuffer += (*UUIDStr - '0');
                    RemainingDigits--;
                }
                else
                {
                    if ((*UUIDStr >= 'a') && (*UUIDStr <= 'f'))
                    {
                        *UUIDBuffer += (*UUIDStr - 'a' + 10);
                        RemainingDigits--;
                    }
                    else
                    {
                        if ((*UUIDStr >= 'A') && (*UUIDStr <= 'F'))
                        {
                            *UUIDBuffer += (*UUIDStr - 'A' + 10);
                            RemainingDigits--;
                        }
                        else
                        {
                            /* Invalid hex character. Abort the conversion.    */
                            UUIDEntry->SDP_Data_Element_Type = deNULL;
                        }
                    }
                }

                UUIDStr++;

                if ((RemainingDigits % 2) == 0)
                {
                    /* A full byte was just completed. Move to the next byte */
                    /* in the UUID.                                          */
                    UUIDBuffer++;
                }
                else
                {
                    /* The first half of a byte was processed. Shift the new */
                    /* nibble into the upper-half of the byte.               */
                    *UUIDBuffer = (*UUIDBuffer << 4);
                }
            }
        }
    }
}

/* The following function is provided to allow a means of dumping    */
/* data to the console.                                              */
void GattSrv::DumpData(Boolean_t String, unsigned int Length, Byte_t *Data)
{
    int  i, offset, ascidx;
    char Buf[256];
    char Ascii[256];

    if ((Length) && (Data))
    {
        /* Initialize the temporary buffer.                               */
        BTPS_MemInitialize(Buf, 0, (sizeof(Buf)/sizeof(char)));
        BTPS_MemInitialize(Ascii, 0, (sizeof(Ascii)/sizeof(char)));

        offset = 0;
        i      = 0;
        ascidx = 0;
        int first_line_len = 0;

        while(Length-i)
        {
            if (!String)
            {
                Ascii[ascidx++] = ((Data[i] >= ' ') && (Data[i] < 0x7F)) ? Data[i] : ' ';
                offset += sprintf(&Buf[offset], "%02X ", Data[i]);
            }
            else
            {
                offset += sprintf(&Buf[offset], "%c", (char)Data[i]);
            }

            if (!(++i % 16))
            {
                offset = 0;
                BOT_NOTIFY_DEBUG("%s | ", Buf);
                BOT_NOTIFY_DEBUG("%s\n", Ascii);

                ascidx = 0;
                BTPS_MemInitialize(Ascii, 0, (sizeof(Ascii)/sizeof(char)));
                first_line_len  = strlen(Buf);
            }
        }

        if (i % 16)
        {
            BOT_NOTIFY_DEBUG("%s", Buf);
            for (int j = 0; j < (first_line_len - (int ) strlen(Buf)); BOT_NOTIFY_DEBUG(" "), j++);
            BOT_NOTIFY_DEBUG(" | %s\n", Ascii);
        }
    }
}

/* The following function is used to search the Service Info list to */
/* return an attribute based on the ServiceID and the                */
/* AttributeOffset.  This function returns a pointer to the attribute*/
/* or NULL if not found.                                             */
AttributeInfo_t* GattSrv::SearchServiceListByOffset(unsigned int ServiceID, unsigned int AttributeOffset)
{
    unsigned int     Index;
    unsigned int     Index1;
    AttributeInfo_t *AttributeInfo = NULL;

    /* Verify that the input parameters are semi-valid.                  */
    if ((ServiceID) && (AttributeOffset) && mServiceTable)
    {
        /* Loop through the services and find the correct attribute.      */
        for (Index=0;(AttributeInfo==NULL)&&(Index<mServiceCount);Index++)
        {
            if (mServiceTable[Index].ServiceID == ServiceID)
            {
                /* Loop through the attribute list for this service and     */
                /* locate the correct attribute based on the Attribute      */
                /* Offset.                                                  */
                for (Index1=0;(AttributeInfo==NULL)&&(Index1<mServiceTable[Index].NumberAttributes);Index1++)
                {
                    if (mServiceTable[Index].AttributeList[Index1].AttributeOffset == AttributeOffset)
                        AttributeInfo = &(mServiceTable[Index].AttributeList[Index1]);
                }
            }
        }
    }

    return(AttributeInfo);
}

void GattSrv::FreeServerList()
{
    /* Walk the server list and mark that no services are registered. */
    for (unsigned int Index=0; Index < mServiceCount && mServiceTable; Index++)
    {
        mServiceTable[Index].ServiceID                          = 0;
        mServiceTable[Index].ServiceHandleRange.Starting_Handle = 0;
        mServiceTable[Index].ServiceHandleRange.Ending_Handle   = 0;
    }
}

/* The following function is used to cleanup the specified service   */
/* list and free any allocated memory.                               */
void GattSrv::CleanupServiceList(void)
{
    unsigned int          Index;
    unsigned int          Index1;
    DescriptorInfo_t     *DescriptorInfo;
    CharacteristicInfo_t *CharacteristicInfo;

    /* Wait on access to the service list mutex.                         */
    if (BTPS_WaitMutex(mServiceMutex, BTPS_INFINITE_WAIT))
    {
        /* Loop through the services and perform the necessary cleanup.   */
        for (Index=0;mServiceTable && Index<mServiceCount;Index++)
        {
            /* If this service has a registered Persistent UID go ahead and*/
            /* un-register it.                                             */
            if (mServiceTable[Index].PersistentUID)
                GATM_UnRegisterPersistentUID(mServiceTable[Index].PersistentUID);

            /* Loop through the attribute list for this service and free   */
            /* any attributes with dynamically allocated buffers.          */
            for (Index1=0;Index1<mServiceTable[Index].NumberAttributes;Index1++)
            {
                switch(mServiceTable[Index].AttributeList[Index1].AttributeType)
                {
                case atCharacteristic:
                    if ((CharacteristicInfo = (CharacteristicInfo_t *)mServiceTable[Index].AttributeList[Index1].Attribute) != NULL)
                    {
                        if ((CharacteristicInfo->AllocatedValue) && (CharacteristicInfo->Value))
                        {
                            BTPS_FreeMemory(CharacteristicInfo->Value);

                            CharacteristicInfo->Value          = NULL;
                            CharacteristicInfo->AllocatedValue = FALSE;
                        }
                    }
                    break;
                case atDescriptor:
                    if ((DescriptorInfo = (DescriptorInfo_t *)mServiceTable[Index].AttributeList[Index1].Attribute) != NULL)
                    {
                        if ((DescriptorInfo->AllocatedValue) && (DescriptorInfo->Value))
                        {
                            BTPS_FreeMemory(DescriptorInfo->Value);

                            DescriptorInfo->Value          = NULL;
                            DescriptorInfo->AllocatedValue = FALSE;
                        }
                    }
                    break;
                default:
                    /* Do nothing.                                        */
                    break;
                }
            }
        }

        /* Release the mutex that was acquired previously.                */
        BTPS_ReleaseMutex(mServiceMutex);
    }
}

/* The following function is a utility function which is used to     */
/* calculate the number of attributes that are needed to register the*/
/* specified service in the GATT database.                           */
unsigned int GattSrv::CalculateNumberAttributes(ServiceInfo_t *ServiceInfo)
{
    unsigned int Index;
    unsigned int NumberAttributes = 0;

    /* Verify that the input parameter is semi-valid.                    */
    if (ServiceInfo)
    {
        for (Index=0;Index<ServiceInfo->NumberAttributes;Index++)
        {
            switch(ServiceInfo->AttributeList[Index].AttributeType)
            {
            case atInclude:
            case atDescriptor:
                NumberAttributes += 1;
                break;
            case atCharacteristic:
                NumberAttributes += 2;
                break;
            }
        }
    }

    return(NumberAttributes);
}

/* The following function is a utility function which is used to     */
/* process a GATM Write Request Event.                               */
void GattSrv::ProcessWriteRequestEvent(GATM_Write_Request_Event_Data_t *WriteRequestData)
{
    int               Result;
    Byte_t          **Value;
    Byte_t            ErrorCode;
    Boolean_t        *AllocatedMemory;
    unsigned int     *ValueLength;
    unsigned int      MaximumValueLength;
    AttributeInfo_t  *AttributeInfo;

    /* Verify that the input parameter is semi-valid.                    */
    if (WriteRequestData)
    {
        /* Initialize the error code.                                     */
        ErrorCode = 0;

        /* Wait on access to the Service Info List Mutex.                 */
        if (BTPS_WaitMutex(mServiceMutex, BTPS_INFINITE_WAIT))
        {
            /* Search for the Attribute for this write request (which must */
            /* be a characteristic or descriptor).                         */
            AttributeInfo = SearchServiceListByOffset(WriteRequestData->ServiceID, WriteRequestData->AttributeOffset);
            if ((AttributeInfo) && ((AttributeInfo->AttributeType == atCharacteristic) || (AttributeInfo->AttributeType == atDescriptor)))
            {
                /* Handle the read based on the type of attribute.          */
                switch(AttributeInfo->AttributeType)
                {
                case atCharacteristic:
                    MaximumValueLength = ((CharacteristicInfo_t *)AttributeInfo->Attribute)->MaximumValueLength;
                    AllocatedMemory    = &(((CharacteristicInfo_t *)AttributeInfo->Attribute)->AllocatedValue);
                    ValueLength        = &(((CharacteristicInfo_t *)AttributeInfo->Attribute)->ValueLength);
                    Value              = &(((CharacteristicInfo_t *)AttributeInfo->Attribute)->Value);
                    break;
                case atDescriptor:
                    MaximumValueLength = ((DescriptorInfo_t *)AttributeInfo->Attribute)->MaximumValueLength;
                    AllocatedMemory    = &(((DescriptorInfo_t *)AttributeInfo->Attribute)->AllocatedValue);
                    ValueLength        = &(((DescriptorInfo_t *)AttributeInfo->Attribute)->ValueLength);
                    Value              = &(((DescriptorInfo_t *)AttributeInfo->Attribute)->Value);
                    break;
                default:
                    /* Do nothing.                                        */
                    break;
                }

                /* Verify that the length of the write is less than the     */
                /* maximum length of this characteristic.                   */
                if (WriteRequestData->DataLength <= MaximumValueLength)
                {
                    /* Free any previously existing buffer.                  */
                    if ((*AllocatedMemory) && (*Value))
                    {
                        BTPS_FreeMemory(*Value);

                        *AllocatedMemory = FALSE;
                        *ValueLength     = 0;
                        *Value           = NULL;
                    }

                    /* Allocate a buffer to hold this new value (note we will*/
                    /* always allocate a worst case buffer for this attribute*/
                    /* to make handling prepare writes later easier).        */
                    if ((WriteRequestData->DataLength == 0) || ((*Value = (Byte_t *)BTPS_AllocateMemory(MaximumValueLength)) != NULL))
                    {
                        /* Store the information on the write.                */
                        if (WriteRequestData->DataLength)
                        {
                            /* Copy the value over into the allocated buffer.  */
                            BTPS_MemCopy(*Value, WriteRequestData->Data, WriteRequestData->DataLength);

                            /* Save the Value Length and flag that a buffer is */
                            /* allocated for this attribute.                   */
                            *AllocatedMemory = TRUE;
                            *ValueLength     = WriteRequestData->DataLength;
                        }
                        else
                        {
                            /* Simply reset the state since this is a 0 byte   */
                            /* write.                                          */
                            *AllocatedMemory = FALSE;
                            *ValueLength     = 0;
                            *Value           = NULL;
                        }

                        /* If we need to respond to this request go ahead and */
                        /* do so.                                             */
                        if (WriteRequestData->RequestID)
                        {
                            if ((Result = GATM_WriteResponse(WriteRequestData->RequestID)) == 0)
                                BOT_NOTIFY_INFO("GATM_WriteResponse() success.\n");
                            else
                                BOT_NOTIFY_DEBUG("Error - GATM_WriteResponse() %d, %s\n", Result, ERR_ConvertErrorCodeToString(Result));
                        }
                    }
                    else
                        ErrorCode = ATT_PROTOCOL_ERROR_CODE_INSUFFICIENT_RESOURCES;
                }
                else
                    ErrorCode = ATT_PROTOCOL_ERROR_CODE_INVALID_ATTRIBUTE_VALUE_LENGTH;
            }
            else
                ErrorCode = ATT_PROTOCOL_ERROR_CODE_INVALID_HANDLE;

            /* Release the mutex that was previously acquired.             */
            BTPS_ReleaseMutex(mServiceMutex);
        }
        else
        {
            /* Simply send an error response.                              */
            ErrorCode = ATT_PROTOCOL_ERROR_CODE_UNLIKELY_ERROR;
        }

        /* If requested go ahead and issue an error response to the       */
        /* request (if a response is required for this request).          */
        if ((ErrorCode) && (WriteRequestData->RequestID))
        {
            if ((Result = GATM_ErrorResponse(WriteRequestData->RequestID, ErrorCode)) == 0)
                BOT_NOTIFY_INFO("GATM_ErrorResponse() success.\n");
            else
                BOT_NOTIFY_DEBUG("Error - GATM_ErrorResponse() %d, %s\n", Result, ERR_ConvertErrorCodeToString(Result));
        }
    }
}

/* The following function is a utility function which is used to     */
/* process a GATM Signed Write Request Event.                        */
void GattSrv::ProcessSignedWriteEvent(GATM_Signed_Write_Event_Data_t *SignedWriteData)
{
    Byte_t          **Value;
    Boolean_t        *AllocatedMemory;
    unsigned int     *ValueLength;
    unsigned int      MaximumValueLength;
    AttributeInfo_t  *AttributeInfo;

    /* Verify that the input parameter is semi-valid.                    */
    if (SignedWriteData)
    {
        /* Wait on access to the Service Info List Mutex.                 */
        if (BTPS_WaitMutex(mServiceMutex, BTPS_INFINITE_WAIT))
        {
            /* Search for the Attribute for this signed write (which must  */
            /* be a characteristic or descriptor).                         */
            AttributeInfo = SearchServiceListByOffset(SignedWriteData->ServiceID, SignedWriteData->AttributeOffset);
            if ((AttributeInfo) && ((AttributeInfo->AttributeType == atCharacteristic) || (AttributeInfo->AttributeType == atDescriptor)))
            {
                /* Handle the read based on the type of attribute.          */
                switch(AttributeInfo->AttributeType)
                {
                case atCharacteristic:
                    MaximumValueLength = ((CharacteristicInfo_t *)AttributeInfo->Attribute)->MaximumValueLength;
                    AllocatedMemory    = &(((CharacteristicInfo_t *)AttributeInfo->Attribute)->AllocatedValue);
                    ValueLength        = &(((CharacteristicInfo_t *)AttributeInfo->Attribute)->ValueLength);
                    Value              = &(((CharacteristicInfo_t *)AttributeInfo->Attribute)->Value);
                    break;
                case atDescriptor:
                    MaximumValueLength = ((DescriptorInfo_t *)AttributeInfo->Attribute)->MaximumValueLength;
                    AllocatedMemory    = &(((DescriptorInfo_t *)AttributeInfo->Attribute)->AllocatedValue);
                    ValueLength        = &(((DescriptorInfo_t *)AttributeInfo->Attribute)->ValueLength);
                    Value              = &(((DescriptorInfo_t *)AttributeInfo->Attribute)->Value);
                    break;
                default:
                    /* Do nothing.                                        */
                    break;
                }

                /* Verify that the length of the write is less than the     */
                /* maximum length of this characteristic.                   */
                if (SignedWriteData->DataLength <= MaximumValueLength)
                {
                    /* Free any previously existing buffer.                  */
                    if ((*AllocatedMemory) && (*Value))
                    {
                        BTPS_FreeMemory(*Value);

                        *AllocatedMemory = FALSE;
                        *ValueLength     = 0;
                        *Value           = NULL;
                    }

                    /* Allocate a buffer to hold this new value (note we will*/
                    /* always allocate a worst case buffer for this attribute*/
                    /* to make handling prepare writes later easier).        */
                    if ((SignedWriteData->DataLength == 0) || ((*Value = (Byte_t *)BTPS_AllocateMemory(MaximumValueLength)) != NULL))
                    {
                        /* Store the information on the write.                */
                        if (SignedWriteData->DataLength)
                        {
                            /* Copy the value over into the allocated buffer.  */
                            BTPS_MemCopy(*Value, SignedWriteData->Data, SignedWriteData->DataLength);

                            /* Save the Value Length and flag that a buffer is */
                            /* allocated for this attribute.                   */
                            *AllocatedMemory = TRUE;
                            *ValueLength     = SignedWriteData->DataLength;
                        }
                        else
                        {
                            /* Simply reset the state since this is a 0 byte   */
                            /* write.                                          */
                            *AllocatedMemory = FALSE;
                            *ValueLength     = 0;
                            *Value           = NULL;
                        }
                    }
                }
            }

            /* Release the mutex that was previously acquired.             */
            BTPS_ReleaseMutex(mServiceMutex);
        }
    }
}

/* The following function is a utility function which is used to     */
/* process a GATM Prepare Write Request Event.                       */
void GattSrv::ProcessPrepareWriteRequestEvent(GATM_Prepare_Write_Request_Event_Data_t *PrepareWriteRequestData)
{
    int                  Result;
    Byte_t               ErrorCode;
    unsigned int         MaximumValueLength;
    AttributeInfo_t     *AttributeInfo;
    PrepareWriteEntry_t *PrepareWriteEntry;

    /* Verify that the input parameter is semi-valid.                    */
    if (PrepareWriteRequestData)
    {
        /* Initialize the error code.                                     */
        ErrorCode = 0;

        /* Wait on access to the Service Info List Mutex.                 */
        if (BTPS_WaitMutex(mServiceMutex, BTPS_INFINITE_WAIT))
        {
            /* Search for the Attribute for this prepare write request     */
            /* (which must be a characteristic or descriptor).             */
            AttributeInfo = SearchServiceListByOffset(PrepareWriteRequestData->ServiceID, PrepareWriteRequestData->AttributeOffset);
            if ((AttributeInfo) && ((AttributeInfo->AttributeType == atCharacteristic) || (AttributeInfo->AttributeType == atDescriptor)))
            {
                /* Handle the read based on the type of attribute.          */
                switch(AttributeInfo->AttributeType)
                {
                case atCharacteristic:
                    MaximumValueLength = ((CharacteristicInfo_t *)AttributeInfo->Attribute)->MaximumValueLength;
                    break;
                case atDescriptor:
                    MaximumValueLength = ((DescriptorInfo_t *)AttributeInfo->Attribute)->MaximumValueLength;
                    break;
                default:
                    /* Do nothing.                                        */
                    break;
                }

                /* Attempt to combine this compare write with other pending */
                /* prepare writes for this client.                          */
                if ((PrepareWriteEntry = CombinePrepareWrite(&mPrepareWriteList, PrepareWriteRequestData)) == NULL)
                {
                    /* This prepare write could not be combined with any     */
                    /* other oustanding prepare write for this client so go  */
                    /* ahead and allocate a new prepare write (with a buffer */
                    /* of the maximum attribute size) and add it to the list.*/
                    if ((PrepareWriteEntry = (PrepareWriteEntry_t *)BTPS_AllocateMemory(sizeof(PrepareWriteEntry_t) + (MaximumValueLength*BYTE_SIZE))) != NULL)
                    {
                        /* Configure the Prepare Write Entry.                 */
                        BTPS_MemInitialize(PrepareWriteEntry, 0, sizeof(PrepareWriteEntry_t));

                        PrepareWriteEntry->ConnectionType       = PrepareWriteRequestData->ConnectionType;
                        PrepareWriteEntry->RemoteDeviceAddress  = PrepareWriteRequestData->RemoteDeviceAddress;
                        PrepareWriteEntry->ServiceID            = PrepareWriteRequestData->ServiceID;
                        PrepareWriteEntry->AttributeOffset      = PrepareWriteRequestData->AttributeOffset;
                        PrepareWriteEntry->AttributeValueOffset = PrepareWriteRequestData->AttributeValueOffset;
                        PrepareWriteEntry->MaximumValueLength   = MaximumValueLength;
                        PrepareWriteEntry->Value                = (Byte_t *)(((Byte_t *)PrepareWriteEntry) + ((unsigned int)sizeof(PrepareWriteEntry_t)));

                        /* Attempt to add the list entry to the prepare write */
                        /* list.                                              */
                        if (!AddPrepareWriteEntry(&mPrepareWriteList, PrepareWriteEntry))
                        {
                            /* Failed to add entry to list so just free the    */
                            /* memory and flag that no request entry was added.*/
                            BTPS_FreeMemory(PrepareWriteEntry);

                            PrepareWriteEntry = NULL;
                        }
                    }
                }

                /* Only continue if we have a request entry for this        */
                /* request.                                                 */
                if (PrepareWriteEntry)
                {
                    /* Verify that the offset is valid.                      */
                    if (PrepareWriteRequestData->AttributeValueOffset < PrepareWriteEntry->MaximumValueLength)
                    {
                        /* Verify that the length is valid for this request.  */
                        if ((PrepareWriteRequestData->AttributeValueOffset + PrepareWriteRequestData->DataLength) <=  PrepareWriteEntry->MaximumValueLength)
                        {
                            /* Verify that we will not write more data into the*/
                            /* queue then is allowed.                          */
                            if ((PrepareWriteEntry->ValueLength + PrepareWriteRequestData->DataLength) <= PrepareWriteEntry->MaximumValueLength)
                            {
                                /* Data is valid so go ahead and copy the data  */
                                /* into the request entry.                      */
                                BTPS_MemCopy(&(PrepareWriteEntry->Value[PrepareWriteEntry->ValueLength]), PrepareWriteRequestData->Data, PrepareWriteRequestData->DataLength);

                                PrepareWriteEntry->ValueLength += PrepareWriteRequestData->DataLength;

                                /* Go ahead and return success to this request. */
                                GATM_WriteResponse(PrepareWriteRequestData->RequestID);
                            }
                            else
                                ErrorCode = ATT_PROTOCOL_ERROR_CODE_INVALID_ATTRIBUTE_VALUE_LENGTH;
                        }
                        else
                            ErrorCode = ATT_PROTOCOL_ERROR_CODE_INVALID_ATTRIBUTE_VALUE_LENGTH;

                        if (ErrorCode == ATT_PROTOCOL_ERROR_CODE_INVALID_ATTRIBUTE_VALUE_LENGTH)
                        {
                            BOT_NOTIFY_DEBUG("Maximum Value Length is     %u\r\n", PrepareWriteEntry->MaximumValueLength);
                            BOT_NOTIFY_DEBUG("Prepare Write Offset:       %u\r\n", PrepareWriteRequestData->AttributeValueOffset);
                            BOT_NOTIFY_DEBUG("Prepare Write Length:       %u\r\n", PrepareWriteRequestData->DataLength);
                            BOT_NOTIFY_DEBUG("Prepare Queue Entry Length: %u\r\n", PrepareWriteEntry->ValueLength);
                        }
                    }
                    else
                        ErrorCode = ATT_PROTOCOL_ERROR_CODE_INVALID_OFFSET;

                    /* If an error occurred go ahead and free the entry that */
                    /* was allocated.                                        */
                    if (ErrorCode)
                    {
                        if ((PrepareWriteEntry = DeletePrepareWriteEntryByPtr(&mPrepareWriteList, PrepareWriteEntry)) != NULL)
                            FreePrepareWriteEntryEntryMemory(PrepareWriteEntry);
                    }
                }
                else
                    ErrorCode = ATT_PROTOCOL_ERROR_CODE_INSUFFICIENT_RESOURCES;
            }
            else
                ErrorCode = ATT_PROTOCOL_ERROR_CODE_INVALID_HANDLE;

            /* Release the mutex that was previously acquired.             */
            BTPS_ReleaseMutex(mServiceMutex);
        }
        else
        {
            /* Simply send an error response.                              */
            ErrorCode = ATT_PROTOCOL_ERROR_CODE_UNLIKELY_ERROR;
        }

        /* If requested go ahead and issue an error response to the       */
        /* request.                                                       */
        if (ErrorCode)
        {
            if ((Result = GATM_ErrorResponse(PrepareWriteRequestData->RequestID, ErrorCode)) == 0)
                BOT_NOTIFY_INFO("GATM_ErrorResponse() success.\n");
            else
                BOT_NOTIFY_DEBUG("Error - GATM_ErrorResponse() %d, %s\n", Result, ERR_ConvertErrorCodeToString(Result));
        }
    }
}

/* The following function is a utility function which is used to     */
/* process a GATM Commit Prepare Write Event.                        */
void GattSrv::ProcessCommitPrepareWriteEvent(GATM_Commit_Prepare_Write_Event_Data_t *CommitPrepareWriteData)
{
    Byte_t              **Value;
    Byte_t               *TempBuffer;
    Boolean_t            *AllocatedMemory;
    unsigned int         *ValueLength;
    unsigned int          MaximumValueLength;
    unsigned int          QueuedValueLength;
    AttributeInfo_t      *AttributeInfo;
    PrepareWriteEntry_t  *PrepareWriteEntry;

    /* Verify that the input parameter is semi-valid.                    */
    if (CommitPrepareWriteData)
    {
        /* Wait on access to the Service Info List Mutex.                 */
        if (BTPS_WaitMutex(mServiceMutex, BTPS_INFINITE_WAIT))
        {
            /* Walk through the Prepare Write list for this client and     */
            /* delete all prepare queue entries (since we can now either   */
            /* commit the prepared writes or flush them from the queue.    */
            while((PrepareWriteEntry = DeletePrepareWriteEntry(&mPrepareWriteList, CommitPrepareWriteData->ConnectionType, CommitPrepareWriteData->RemoteDeviceAddress, CommitPrepareWriteData->ServiceID)) != NULL)
            {
                /* Check to see if we are committing the prepared writes for*/
                /* this client.                                             */
                if (CommitPrepareWriteData->CommitWrites)
                {
                    /* Search for the Attribute for this signed write (which */
                    /* must be a characteristic or descriptor).              */
                    AttributeInfo = SearchServiceListByOffset(PrepareWriteEntry->ServiceID, PrepareWriteEntry->AttributeOffset);
                    if ((AttributeInfo) && ((AttributeInfo->AttributeType == atCharacteristic) || (AttributeInfo->AttributeType == atDescriptor)))
                    {
                        /* Handle the request based on the type of attribute. */
                        switch(AttributeInfo->AttributeType)
                        {
                        case atCharacteristic:
                            MaximumValueLength = ((CharacteristicInfo_t *)AttributeInfo->Attribute)->MaximumValueLength;
                            AllocatedMemory    = &(((CharacteristicInfo_t *)AttributeInfo->Attribute)->AllocatedValue);
                            ValueLength        = &(((CharacteristicInfo_t *)AttributeInfo->Attribute)->ValueLength);
                            Value              = &(((CharacteristicInfo_t *)AttributeInfo->Attribute)->Value);
                            break;
                        case atDescriptor:
                            MaximumValueLength = ((DescriptorInfo_t *)AttributeInfo->Attribute)->MaximumValueLength;
                            AllocatedMemory    = &(((DescriptorInfo_t *)AttributeInfo->Attribute)->AllocatedValue);
                            ValueLength        = &(((DescriptorInfo_t *)AttributeInfo->Attribute)->ValueLength);
                            Value              = &(((DescriptorInfo_t *)AttributeInfo->Attribute)->Value);
                            break;
                        default:
                            /* Do nothing.                                  */
                            break;
                        }

                        /* If we have not allocated a buffer for this entry go*/
                        /* ahead and do so (of the maximum length of this     */
                        /* attribute.                                         */
                        if (*AllocatedMemory == FALSE)
                        {
                            if ((TempBuffer = (Byte_t*) BTPS_AllocateMemory(MaximumValueLength)) != NULL)
                            {
                                BTPS_MemInitialize(TempBuffer, 0, MaximumValueLength);
                                BTPS_MemCopy(TempBuffer, *Value, *ValueLength);

                                *AllocatedMemory = TRUE;
                                *Value           = TempBuffer;
                            }
                            else
                                Value = NULL;
                        }

                        /* Only continue if we have an allocated buffer to    */
                        /* use.                                               */
                        if ((Value) && (*Value))
                        {
                            /* Verify that we will not exceed the maximum      */
                            /* length of the allocated buffer.                 */
                            QueuedValueLength = (PrepareWriteEntry->AttributeValueOffset + PrepareWriteEntry->ValueLength);
                            if (QueuedValueLength <= MaximumValueLength)
                            {
                                /* Copy the data into the value.                */
                                BTPS_MemCopy(&((*Value)[PrepareWriteEntry->AttributeValueOffset]), PrepareWriteEntry->Value, PrepareWriteEntry->ValueLength);

                                /* Update the value length only if necessary.   */
                                /* Since prepare writes allow multiple requests */
                                /* to be done to the same attribute at different*/
                                /* offsets we may not always need to update the */
                                /* length.                                      */
                                if (QueuedValueLength > *ValueLength)
                                    *ValueLength = QueuedValueLength;
                            }
                        }
                    }
                }

                /* Free the memory allocated for this entry.                */
                FreePrepareWriteEntryEntryMemory(PrepareWriteEntry);
            }

            /* Release the mutex that was previously acquired.             */
            BTPS_ReleaseMutex(mServiceMutex);
        }
    }
}

/* This function frees the specified Prepare Write Entry member.  No */
/* check is done on this entry other than making sure it NOT NULL.   */
void GattSrv::FreePrepareWriteEntryEntryMemory(PrepareWriteEntry_t *EntryToFree)
{
    if (EntryToFree)
        BTPS_FreeMemory(EntryToFree);
}

/* The following function deletes (and free's all memory) every      */
/* element of the specified Prepare Write Entry List.  Upon return of*/
/* this function, the Head Pointer is set to NULL.                   */
void GattSrv::FreePrepareWriteEntryList(PrepareWriteEntry_t **ListHead)
{
    PrepareWriteEntry_t *EntryToFree;
    PrepareWriteEntry_t *tmpEntry;

    /* Let's make sure the parameters appear to be semi-valid.           */
    if (ListHead)
    {
        /* Simply traverse the list and free every element present.       */
        EntryToFree = *ListHead;

        while(EntryToFree)
        {
            tmpEntry    = EntryToFree;
            EntryToFree = EntryToFree->NextPrepareWriteEntryPtr;

            FreePrepareWriteEntryEntryMemory(tmpEntry);
        }

        /* Make sure the List appears to be empty.                        */
        *ListHead = NULL;
    }
}

/* The following function is a utility function which exists to      */
/* attempt to combine the specified prepare write with a prepare     */
/* write entry to the same attribute already in the list.  This      */
/* function returns a pointer to the combined entry or NULL on       */
/* failure.                                                          */
PrepareWriteEntry_t *GattSrv::CombinePrepareWrite(PrepareWriteEntry_t **ListHead, GATM_Prepare_Write_Request_Event_Data_t *PrepareWriteRequestData)
{
    PrepareWriteEntry_t *PrepareWriteEntry = NULL;

    /* Let's make sure the parameters appear to be semi-valid.           */
    if ((ListHead) && (PrepareWriteRequestData))
    {
        /* Walk the list and attempt to combine the prepare writes.       */
        PrepareWriteEntry = *ListHead;
        while(PrepareWriteEntry)
        {
            /* First we need to make sure that this prepare write is from  */
            /* the same client to the same service.                        */
            if ((PrepareWriteEntry->ConnectionType == PrepareWriteRequestData->ConnectionType) && (COMPARE_BD_ADDR(PrepareWriteEntry->RemoteDeviceAddress, PrepareWriteRequestData->RemoteDeviceAddress)) && (PrepareWriteEntry->ServiceID == PrepareWriteRequestData->ServiceID))
            {
                /* Next check to see that the prepare writes are to the same*/
                /* attribute (i.e.  the Attribute Offset in this Service's  */
                /* table match).                                            */
                if (PrepareWriteEntry->AttributeOffset == PrepareWriteRequestData->AttributeOffset)
                {
                    /* Okay so next check to see if this request is          */
                    /* contiguous to a previous prepare write request.       */
                    if ((PrepareWriteEntry->AttributeValueOffset + PrepareWriteEntry->ValueLength) == PrepareWriteRequestData->AttributeValueOffset)
                    {
                        /* We can combine these entries so go ahead and exit  */
                        /* the loop.                                          */
                        break;
                    }
                }
            }

            PrepareWriteEntry = PrepareWriteEntry->NextPrepareWriteEntryPtr;
        }
    }

    return(PrepareWriteEntry);
}

/* The following function adds the specified Entry to the specified  */
/* List.  This function allocates and adds an entry to the list that */
/* has the same attributes as the Entry passed into this function.   */
/* This function will return FALSE if NO Entry was added, otherwise  */
/* it will return TRUE.  This can occur if the element passed in was */
/* deemed invalid or the actual List Head was invalid.               */
Boolean_t GattSrv::AddPrepareWriteEntry(PrepareWriteEntry_t **ListHead, PrepareWriteEntry_t *EntryToAdd)
{
    Boolean_t            ret_val = FALSE;
    PrepareWriteEntry_t *tmpEntry;

    /* First let's verify that values passed in are semi-valid.          */
    if ((ListHead) && (EntryToAdd))
    {
        /* Make sure that the element that we are adding seems semi-valid.*/
        if ((EntryToAdd->ServiceID) && (EntryToAdd->AttributeOffset))
        {
            /* Now Add it to the end of the list.                          */
            EntryToAdd->NextPrepareWriteEntryPtr = NULL;

            /* First, let's check to see if there are any elements already */
            /* present in the List that was passed in.                     */
            if ((tmpEntry = *ListHead) != NULL)
            {
                /* Head Pointer was not NULL, so we will traverse the list  */
                /* until we reach the last element.                         */
                while(tmpEntry)
                {
                    /* OK, we need to see if we are at the last element of   */
                    /* the List.                                             */
                    if (tmpEntry->NextPrepareWriteEntryPtr)
                        tmpEntry = tmpEntry->NextPrepareWriteEntryPtr;
                    else
                    {
                        /* Last element found, simply Add the entry.          */
                        tmpEntry->NextPrepareWriteEntryPtr = EntryToAdd;

                        /* Return success to the caller.                      */
                        ret_val                            = TRUE;
                        break;
                    }
                }
            }
            else
            {
                /* Go ahead and add it to the head of the list.             */
                *ListHead = EntryToAdd;

                /* Return success to the caller.                            */
                ret_val   = TRUE;
            }
        }
    }

    return ret_val;
}

/* The following function searches the specified List for the        */
/* specified entry and removes it from the List.  This function      */
/* returns NULL if either the List Head is invalid or the specified  */
/* entry was NOT present in the list.  The entry returned will have  */
/* the Next Entry field set to NULL, and the caller is responsible   */
/* for deleting the memory associated with this entry by calling     */
/* FreePrepareWriteEntryEntryMemory().                               */
PrepareWriteEntry_t *GattSrv::DeletePrepareWriteEntry(PrepareWriteEntry_t **ListHead, GATT_Connection_Type_t ConnectionType, BD_ADDR_t RemoteDeviceAddress, unsigned int ServiceID)
{
    PrepareWriteEntry_t *FoundEntry = NULL;
    PrepareWriteEntry_t *LastEntry  = NULL;

    /* Let's make sure the List and search parameters to search for      */
    /* appear to be semi-valid.                                          */
    if ((ListHead) && ((ConnectionType == gctLE) || (ConnectionType == gctBR_EDR)) && (!COMPARE_NULL_BD_ADDR(RemoteDeviceAddress)) && (ServiceID))
    {
        /* Now, let's search the list until we find the correct entry.    */
        FoundEntry = *ListHead;

        while((FoundEntry) && ((FoundEntry->ConnectionType != ConnectionType) || (!COMPARE_BD_ADDR(FoundEntry->RemoteDeviceAddress, RemoteDeviceAddress)) || (FoundEntry->ServiceID != ServiceID)))
        {
            LastEntry  = FoundEntry;
            FoundEntry = FoundEntry->NextPrepareWriteEntryPtr;
        }

        /* Check to see if we found the specified entry.                  */
        if (FoundEntry)
        {
            /* OK, now let's remove the entry from the list.  We have to   */
            /* check to see if the entry was the first entry in the list.  */
            if (LastEntry)
            {
                /* Entry was NOT the first entry in the list.               */
                LastEntry->NextPrepareWriteEntryPtr = FoundEntry->NextPrepareWriteEntryPtr;
            }
            else
                *ListHead = FoundEntry->NextPrepareWriteEntryPtr;

            FoundEntry->NextPrepareWriteEntryPtr = NULL;
        }
    }

    return(FoundEntry);
}

/* The following function searches the specified List for the        */
/* specified entry and removes it from the List.  This function      */
/* returns NULL if either the List Head is invalid or the specified  */
/* entry was NOT present in the list.  The entry returned will have  */
/* the Next Entry field set to NULL, and the caller is responsible   */
/* for deleting the memory associated with this entry by calling     */
/* FreePrepareWriteEntryEntryMemory().                               */
PrepareWriteEntry_t* GattSrv::DeletePrepareWriteEntryByPtr(PrepareWriteEntry_t **ListHead, PrepareWriteEntry_t *PrepareWriteEntry)
{
    PrepareWriteEntry_t *FoundEntry = NULL;
    PrepareWriteEntry_t *LastEntry  = NULL;

    /* Let's make sure the List and search parameters to search for      */
    /* appear to be semi-valid.                                          */
    if ((ListHead) && (PrepareWriteEntry))
    {
        /* Now, let's search the list until we find the correct entry.    */
        FoundEntry = *ListHead;

        while((FoundEntry) && (FoundEntry != PrepareWriteEntry))
        {
            LastEntry  = FoundEntry;
            FoundEntry = FoundEntry->NextPrepareWriteEntryPtr;
        }

        /* Check to see if we found the specified entry.                  */
        if (FoundEntry)
        {
            /* OK, now let's remove the entry from the list.  We have to   */
            /* check to see if the entry was the first entry in the list.  */
            if (LastEntry)
            {
                /* Entry was NOT the first entry in the list.               */
                LastEntry->NextPrepareWriteEntryPtr = FoundEntry->NextPrepareWriteEntryPtr;
            }
            else
                *ListHead = FoundEntry->NextPrepareWriteEntryPtr;

            FoundEntry->NextPrepareWriteEntryPtr = NULL;
        }
    }

    return(FoundEntry);
}
