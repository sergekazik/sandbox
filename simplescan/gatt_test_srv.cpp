/*---------------------------------------------------------------------
 *               ------------------      -----------------            *
 *               | Ring App Setup |      |   test_ble    |            *
 *               ------------------      -----------------            *
 *                          |                |                        *
 *                          |   --------------------                  *
 *                          |   |gatt_src_test.cpp |                  *
 *                          |   --------------------                  *
 *                          |          |                              *
 *                  ---------------------------------                 *
 *                  |    RingBleApi.cpp abstract    |                 *
 *                  ---------------------------------                 *
 *                  |  RingGattSrv  |  RingGattSrv  |                 *
 *                  |   WILINK18    |    BCM43      |                 *
 *                  --------------------------------                  *
 *                        |                  |                        *
 *       ------------------------       ---------------               *
 *       |      TIBT lib        |       |   BlueZ     |               *
 *       ------------------------       ---------------               *
 *                  |                        |                        *
 *       ------------------------       ----------------              *
 *       | TI WiLink18xx BlueTP |       | libbluetooth |              *
 *       ------------------------       ----------------              *
 *--------------------------------------------------------------------*/
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <signal.h>
#include <unistd.h>
#include <sys/param.h>
#include <sys/ioctl.h>
#include <sys/socket.h>

extern "C" {            // Bluetopia SDK API
    #include "SS1BTPM.h"
    #include "BTPMMAPI.h"
}

#include "RingGattApi.hh"
#include "RingBlePairing.hh"
#include "gatt_test_srv.h"

using namespace Ring;
using namespace Ring::Ble;

#define DEVELOPMENT_TEST_NO_SECURITY    1   // for tests only, should be set to 0
#define VALIDATE_AND_EXEC_ARGUMENT(_arg, _val, _cmd ) (!strcmp(_arg, _val)) { execute_hci_cmd(_cmd); }

static BleApi* gBleApi = NULL;

static unsigned int        NumberCommands;
static CommandTable_t      CommandTable[MAX_SUPPORTED_COMMANDS];

// NOTE: this is a section of auto definitions by macros in gatt_test_defs.h
// it may be included multiple times in this file to generate different stuctures correlated by order of definition
#define RING_BLE_DEF_CPP_WRAPPER
#include "gatt_test_defs.h"

#define CMD_LEN_MAX                 44
#define RING_BLE_DEF_CMD
static char gCommands[][CMD_LEN_MAX] = {
    #include "gatt_test_defs.h"
};

/* Helper Functions */
static char* UpperCaseUpdate(int &idx)
{
    for (int i = 0; i < (int) strlen(gCommands[idx]); i++)
    {
        if ((gCommands[idx][i] >= 'a') && (gCommands[idx][i] <= 'z'))
            gCommands[idx][i] = gCommands[idx][i]-'a' + 'A';
    }
    // printf("AddCommand: [%s]\n", gCommands[idx]);
    return gCommands[idx++];
}

#include "gatt_attr_def.h"  // for this test only

/* The following function reads a line from standard input into the  */
/* specified buffer.  This function returns the string length of the */
/* line, including the null character, on success and a negative     */
/* value if an error occurred.                                       */
static int ReadLine(char *Buffer, unsigned int BufferSize)
{
    int       ret_val;
    Boolean_t Done;

    Buffer[0] = '\0';
    Done      = FALSE;

    while(!Done)
    {
        /* Flush the output buffer before reading from standard in so that*/
        /* we don't get out of order input and output in the console.     */
        fflush(stdout);

        /* Read a line from standard input.                               */
        ret_val = read(STDIN_FILENO, Buffer, BufferSize);

        /* Check if the read succeeded.                                   */
        if(ret_val > 0)
        {
            /* The read succeeded, replace the new line character with a   */
            /* null character to delimit the string.                       */
            Buffer[ret_val - 1] = '\0';
            Buffer[ret_val] = '\0';
            /* Stop the loop.                                              */
            Done = TRUE;
        }
        else
        {
            /* The read failed, check the error number to determine if we  */
            /* were interrupted by a signal.  Note that the error number is*/
            /* EIO in the case that we are interrupted by a signal because */
            /* we are blocking SIGTTIN. If we were not blocking SIGTTIN and*/
            /* handling it instead then error number would be EINTR.       */
            if(errno == EIO)
            {
                /* The call was interrupted by a signal which indicates that*/
                /* the app was either suspended or placed into the          */
                /* background, spin until the app has re-entered the        */
                /* foreground.                                              */
                while(getpgrp() != tcgetpgrp(STDOUT_FILENO))
                {
                    sleep(1);
                }
            }
            else
            {
                /* An unexpected error occurred, stop the loop.             */
                Done = TRUE;

                /* Display the error to the user.                           */
                perror("Error: Unexpected return from read()");
            }
        }
    }

    return(ret_val);
}

/* The following function is responsible for converting number       */
/* strings to there unsigned integer equivalent.  This function can  */
/* handle leading and tailing white space, however it does not handle*/
/* signed or comma delimited values.  This function takes as its     */
/* input the string which is to be converted.  The function returns  */
/* zero if an error occurs otherwise it returns the value parsed from*/
/* the string passed as the input parameter.                         */
static unsigned int StringToUnsignedInteger(char *StringInteger)
{
    int          IsHex;
    unsigned int Index;
    unsigned int ret_val = Ble::Error::NONE;

    /* Before proceeding make sure that the parameter that was passed as */
    /* an input appears to be at least semi-valid.                       */
    if((StringInteger) && (strlen(StringInteger)))
    {
        /* Initialize the variable.                                       */
        Index = 0;

        /* Next check to see if this is a hexadecimal number.             */
        if(strlen(StringInteger) > 2)
        {
            if((StringInteger[0] == '0') && ((StringInteger[1] == 'x') || (StringInteger[1] == 'X')))
            {
                IsHex = 1;

                /* Increment the String passed the Hexadecimal prefix.      */
                StringInteger += 2;
            }
            else
                IsHex = 0;
        }
        else
            IsHex = 0;

        /* Process the value differently depending on whether or not a    */
        /* Hexadecimal Number has been specified.                         */
        if(!IsHex)
        {
            /* Decimal Number has been specified.                          */
            while(1)
            {
                /* First check to make sure that this is a valid decimal    */
                /* digit.                                                   */
                if((StringInteger[Index] >= '0') && (StringInteger[Index] <= '9'))
                {
                    /* This is a valid digit, add it to the value being      */
                    /* built.                                                */
                    ret_val += (StringInteger[Index] & 0xF);

                    /* Determine if the next digit is valid.                 */
                    if(((Index + 1) < strlen(StringInteger)) && (StringInteger[Index+1] >= '0') && (StringInteger[Index+1] <= '9'))
                    {
                        /* The next digit is valid so multiply the current    */
                        /* return value by 10.                                */
                        ret_val *= 10;
                    }
                    else
                    {
                        /* The next value is invalid so break out of the loop.*/
                        break;
                    }
                }

                Index++;
            }
        }
        else
        {
            /* Hexadecimal Number has been specified.                      */
            while(1)
            {
                /* First check to make sure that this is a valid Hexadecimal*/
                /* digit.                                                   */
                if(((StringInteger[Index] >= '0') && (StringInteger[Index] <= '9')) || ((StringInteger[Index] >= 'a') && (StringInteger[Index] <= 'f')) || ((StringInteger[Index] >= 'A') && (StringInteger[Index] <= 'F')))
                {
                    /* This is a valid digit, add it to the value being      */
                    /* built.                                                */
                    if((StringInteger[Index] >= '0') && (StringInteger[Index] <= '9'))
                        ret_val += (StringInteger[Index] & 0xF);
                    else
                    {
                        if((StringInteger[Index] >= 'a') && (StringInteger[Index] <= 'f'))
                            ret_val += (StringInteger[Index] - 'a' + 10);
                        else
                            ret_val += (StringInteger[Index] - 'A' + 10);
                    }

                    /* Determine if the next digit is valid.                 */
                    if(((Index + 1) < strlen(StringInteger)) && (((StringInteger[Index+1] >= '0') && (StringInteger[Index+1] <= '9')) || ((StringInteger[Index+1] >= 'a') && (StringInteger[Index+1] <= 'f')) || ((StringInteger[Index+1] >= 'A') && (StringInteger[Index+1] <= 'F'))))
                    {
                        /* The next digit is valid so multiply the current    */
                        /* return value by 16.                                */
                        ret_val *= 16;
                    }
                    else
                    {
                        /* The next value is invalid so break out of the loop.*/
                        break;
                    }
                }

                Index++;
            }
        }
    }

    return(ret_val);
}

/* The following function is responsible for parsing strings into    */
/* components.  The first parameter of this function is a pointer to */
/* the String to be parsed.  This function will return the start of  */
/* the string upon success and a NULL pointer on all errors.         */
static char *StringParser(char *String)
{
    int   Index;
    char *ret_val = NULL;

    /* Before proceeding make sure that the string passed in appears to  */
    /* be at least semi-valid.                                           */
    if((String) && (strlen(String)))
    {
        /* The string appears to be at least semi-valid.  Search for the  */
        /* first space character and replace it with a NULL terminating   */
        /* character.                                                     */
        for(Index=0,ret_val=String;Index < (int) strlen(String);Index++)
        {
            /* Is this the space character.                                */
            if((String[Index] == ' ') || (String[Index] == '\r') || (String[Index] == '\n'))
            {
                /* This is the space character, replace it with a NULL      */
                /* terminating character and set the return value to the    */
                /* beginning character of the string.                       */
                String[Index] = '\0';
                break;
            }
        }
    }

    return(ret_val);
}

/* This function is responsible for taking command strings and       */
/* parsing them into a command, param1, and param2.  After parsing   */
/* this string the data is stored into a UserCommand_t structure to  */
/* be used by the interpreter.  The first parameter of this function */
/* is the structure used to pass the parsed command string out of the*/
/* function.  The second parameter of this function is the string    */
/* that is parsed into the UserCommand structure.  Successful        */
/* execution of this function is denoted by a return value of zero.  */
/* Negative return values denote an error in the parsing of the      */
/* string parameter.                                                 */
static int CommandParser(UserCommand_t *TempCommand, char *UserInput)
{
    int            ret_val;
    int            StringLength;
    char          *LastParameter;
    unsigned int   Count         = 0;

    /* Before proceeding make sure that the passed parameters appear to  */
    /* be at least semi-valid.                                           */
    if((TempCommand) && (UserInput) && (strlen(UserInput)))
    {
        /* Retrieve the first token in the string, this should be the     */
        /* commmand.                                                      */
        TempCommand->Command = StringParser(UserInput);

        /* Flag that there are NO Parameters for this Command Parse.      */
        TempCommand->Parameters.NumberofParameters = 0;

        /* Check to see if there is a Command.                            */
        if(TempCommand->Command)
        {
            /* Initialize the return value to zero to indicate success on  */
            /* commands with no parameters.                                */
            ret_val = Ble::Error::NONE;

            /* Adjust the UserInput pointer and StringLength to remove     */
            /* the Command from the data passed in before parsing the      */
            /* parameters.                                                 */
            UserInput    += strlen(TempCommand->Command)+1;
            StringLength  = strlen(UserInput);

            /* There was an available command, now parse out the parameters*/
            while((StringLength > 0) && ((LastParameter = StringParser(UserInput)) != NULL))
            {
                /* There is an available parameter, now check to see if     */
                /* there is room in the UserCommand to store the parameter  */
                if(Count < (sizeof(TempCommand->Parameters.Params)/sizeof(Parameter_t)))
                {
                    /* Save the parameter as a string.                       */
                    TempCommand->Parameters.Params[Count].strParam = LastParameter;

                    /* Save the parameter as an unsigned int intParam will   */
                    /* have a value of zero if an error has occurred.        */
                    TempCommand->Parameters.Params[Count].intParam = StringToUnsignedInteger(LastParameter);

                    Count++;
                    UserInput    += strlen(LastParameter)+1;
                    StringLength -= strlen(LastParameter)+1;

                    ret_val = Ble::Error::NONE;
                }
                else
                {
                    /* Be sure we exit out of the Loop.                      */
                    StringLength = 0;

                    ret_val = Ble::Error::TOO_MANY_PARAMS;
                }
            }

            /* Set the number of parameters in the User Command to the     */
            /* number of found parameters                                  */
            TempCommand->Parameters.NumberofParameters = Count;
        }
        else
        {
            /* No command was specified                                    */
            ret_val = Ble::Error::PARSER;
        }
    }
    else
    {
        /* One or more of the passed parameters appear to be invalid.     */
        ret_val = Ble::Error::INVALID_PARAMETERS;
    }

    return(ret_val);
}

static CommandFunction_t FindCommand(char *Command)
{
    unsigned int      Index;
    CommandFunction_t ret_val;

    /* First, make sure that the command specified is semi-valid.        */
    if(Command)
    {
        /* Special shortcut: If it's simply a one or two digit number,    */
        /* convert the command directly based on the Command Index.       */
        if((strlen(Command) == 1) && (Command[0] >= '1') && (Command[0] <= '9'))
        {
            Index = atoi(Command);

            if(Index < NumberCommands)
                ret_val = CommandTable[Index - 1].CommandFunction;
            else
                ret_val = NULL;
        }
        else
        {
            if((strlen(Command) == 2) && (Command[0] >= '0') && (Command[0] <= '9') && (Command[1] >= '0') && (Command[1] <= '9'))
            {
                Index = atoi(Command);

                if(Index < NumberCommands)
                    ret_val = CommandTable[Index?(Index-1):Index].CommandFunction;
                else
                    ret_val = NULL;
            }
            else
            {
                /* Now loop through each element in the table to see if     */
                /* there is a match.                                        */
                for(Index=0,ret_val=NULL;((Index<NumberCommands) && (!ret_val));Index++)
                {
                    if((strlen(Command) == strlen(CommandTable[Index].CommandName)) && (memcmp(Command, CommandTable[Index].CommandName, strlen(CommandTable[Index].CommandName)) == 0))
                        ret_val = CommandTable[Index].CommandFunction;
                }
            }
        }
    }
    else
        ret_val = NULL;

    return(ret_val);
}

/* This function is responsible for determining the command in which */
/* the user entered and running the appropriate function associated  */
/* with that command.  The first parameter of this function is a     */
/* structure containing information about the command to be issued.  */
/* This information includes the command name and multiple parameters*/
/* which maybe be passed to the function to be executed.  Successful */
/* execution of this function is denoted by a return value of zero.  */
/* A negative return value implies that that command was not found   */
/* and is invalid.                                                   */
static int CommandInterpreter(UserCommand_t *TempCommand)
{
    int               i;
    int               ret_val;
    CommandFunction_t CommandFunction;

    /* If the command is not found in the table return with an invalid   */
    /* command error                                                     */
    ret_val = Ble::Error::INVALID_COMMAND;

    /* Let's make sure that the data passed to us appears semi-valid.    */
    if((TempCommand) && (TempCommand->Command))
    {
        /* Now, let's make the Command string all upper case so that we   */
        /* compare against it.                                            */
        for(i=0; i < (int) strlen(TempCommand->Command);i++)
        {
            if((TempCommand->Command[i] >= 'a') && (TempCommand->Command[i] <= 'z'))
                TempCommand->Command[i] -= ('a' - 'A');
        }

        /* Check to see if the command which was entered was not exit.        */
        if(memcmp(TempCommand->Command, "QUIT", strlen("QUIT")) != 0)
        {
            /* The command entered is not exit so search for command in    */
            /* table.                                                      */
            if((CommandFunction = FindCommand(TempCommand->Command)) != NULL)
            {
                /* Check if the command is Initialize and requires special handling */
                if ((memcmp(TempCommand->Command, "INITIALIZE", strlen("INITIALIZE")) == 0) ||
                    (memcmp(TempCommand->Command, "1", 1) == 0))
                {
                    TempCommand->Parameters.Params[TempCommand->Parameters.NumberofParameters].intParam = 1;
                    TempCommand->Parameters.Params[TempCommand->Parameters.NumberofParameters].strParam = NULL; // (char*) gServiceTable;
                    TempCommand->Parameters.NumberofParameters++;
                }
                /* The command was found in the table so call the command.  */
                if(!((*CommandFunction)(&TempCommand->Parameters)))
                {
                    /* Return success to the caller.                         */
                    ret_val = Ble::Error::NONE;
                }
                else
                    ret_val = Ble::Error::FUNCTION;
            }
        }
        else
        {
            /* The command entered is exit, set return value to Ble::Error::EXIT_CODE  */
            /* and return.                                                 */
            ret_val = Ble::Error::EXIT_CODE;
        }
    }
    else
        ret_val = Ble::Error::INVALID_PARAMETERS;

    return(ret_val);
}

/* The following function is provided to allow a means to            */
/* programmatically add Commands the Global (to this module) Command */
/* Table.  The Command Table is simply a mapping of Command Name     */
/* (NULL terminated ASCII string) to a command function.  This       */
/* function returns zero if successful, or a non-zero value if the   */
/* command could not be added to the list.                           */
static int AddCommand(char *CommandName, CommandFunction_t CommandFunction)
{
    int ret_val;

    /* First, make sure that the parameters passed to us appear to be    */
    /* semi-valid.                                                       */
    if((CommandName) && (CommandFunction))
    {
        /* Next, make sure that we still have room in the Command Table   */
        /* to add commands.                                               */
        if(NumberCommands < MAX_SUPPORTED_COMMANDS)
        {
            /* Simply add the command data to the command table and        */
            /* increment the number of supported commands.                 */
            CommandTable[NumberCommands].CommandName       = CommandName;
            CommandTable[NumberCommands++].CommandFunction = CommandFunction;

            /* Return success to the caller.                               */
            ret_val                                        = 0;
        }
        else
            ret_val = 1;
    }
    else
        ret_val = 1;

    return(ret_val);
}

/* The following function searches the Command Table for the         */
/* specified Command.  If the Command is found, this function returns*/
/* a NON-NULL Command Function Pointer.  If the command is not found */
/* this function returns NULL.                                       */
/* The following function is provided to allow a means to clear out  */
/* all available commands from the command table.                    */
static void ClearCommands(void)
{
    /* Simply flag that there are no commands present in the table.      */
    NumberCommands = 0;
}

/* The following function is responsible for displaying the current  */
/* Command Options for this Device Manager Sample Application.  The  */
/* input parameter to this function is completely ignored, and only  */
/* needs to be passed in because all Commands that can be entered at */
/* the Prompt pass in the parsed information.  This function displays*/
/* the current Command Options that are available and always returns */
/* zero.                                                             */
static int DisplayHelp(ParameterList_t *TempParam __attribute__ ((unused)))
{

    int idx = 1;
    /* Note the order they are listed here *MUST* match the order in     */
    /* which then are added to the Command Table.                        */
    printf("*******************************************************************\r\n");
    printf("* Command Options:                                                *\r\n");
    #define RING_BLE_PRINT_HELP
    #include "gatt_test_defs.h"
    printf("*                  HH, MM, Help, Quit.                            *\r\n");
    printf("*******************************************************************\r\n");
    return(0);
}
static int AutoHelp(ParameterList_t *p)
{
    (void)p;
    static const char *mmHelp[] = {
        "48 - RegisterGATTCallback",
        "54 - RegisterService 0",
        "58 - ListCharacteristics",
        "59 - ListDescriptors",
        "42 - RegisterAuthentication",
        "47 - ChangeSimplePairingParameters 0 0",
        "52 - StartAdvertising 118 300",
        "11 - QueryLocalDeviceProperties",
        "57 - NotifyCharacteristic 0 offset ADDR STATE_WIFI_SET",
        "41 - EnableBluetoothDebug 1 2"
        "----------------------------",
        "ad, leadv, st, stopad",
    };
    for(int i = 0; i < (int) (sizeof(mmHelp)/sizeof(char*)); i++)
    {
        printf("%s\n", mmHelp[i]);
    }
    return Ble::Error::NONE;
}

/*****************************************************************************
 * SECTION of server Init and command line user interface for tests purposes
 *****************************************************************************/

static void gatt_srv_test_init(bool bPrintHelp)
{
#if !defined(Linux_x86_64) || !defined(WILINK18)
    gBleApi = GattSrv::getInstance();
    if (gBleApi == NULL)
    {
        printf("failed to obtain BleApi instance. Abort.\n");
        return;
    }
#endif

    /* First let's make sure that we start on new line.                  */
    printf("\r\n");

    ClearCommands();
    int idx = 0; // used in macro below
    #define RING_BLE_DEF_ADD_COMMAND
    #include "gatt_test_defs.h"

    AddCommand((char*) "HELP", DisplayHelp);
    AddCommand((char*) "HH", HelpParam);
    AddCommand((char*) "MM", AutoHelp);

    /* Next display the available commands.                              */
    if (bPrintHelp)
        DisplayHelp(NULL);

#ifndef DISABLE_CONFIGURE_SIGTTIN_SIGNAL
    /* Configure the SIGTTIN signal so that this application can run in  */
    /* the background.                                                   */
    struct sigaction SignalAction;
    /* Initialize the signal set to empty.                               */
    sigemptyset(&SignalAction.sa_mask);

    /* Flag that we want to ignore this signal.                          */
    SignalAction.sa_handler = SIG_IGN;

    /* Clear the restart flag so that system calls interrupted by this   */
    /* signal fail and set the error number instead of restarting.       */
    SignalAction.sa_flags = ~SA_RESTART;

    /* Set the SIGTTIN signal's action.  Note that the default behavior  */
    /* for SIGTTIN is to suspend the application.  We want to ignore the */
    /* signal instead so that this doesn't happen.  When the read()      */
    /* function fails the we can check the error number to determine if  */
    /* it was interrupted by a signal, and if it was then we can wait    */
    /* until we have re-entered the foreground before attempting to read */
    /* from standard input again.                                        */
    sigaction(SIGTTIN, &SignalAction, NULL);
#endif
}

/* This function is responsible for taking the input from the user   */
/* and dispatching the appropriate Command Function.  First, this    */
/* function retrieves a String of user input, parses the user input  */
/* into Command and Parameters, and finally executes the Command or  */
/* Displays an Error Message if the input is not a valid Command.    */
static void user_interface(bool bPrintHelp)
{
    UserCommand_t TempCommand;
    int  Result = !Ble::Error::EXIT_CODE;
    char UserInput[MAX_COMMAND_LENGTH];

    gatt_srv_test_init(bPrintHelp);

    /* This is the main loop of the program.  It gets user input from the*/
    /* command window, make a call to the command parser, and command    */
    /* interpreter.  After the function has been ran it then check the   */
    /* return value and displays an error message when appropriate. If   */
    /* the result returned is ever the Ble::Error::EXIT_CODE the loop will exit      */
    /* leading the the exit of the program.                              */
    while(Result != Ble::Error::EXIT_CODE)
    {
        /* Initialize the value of the variable used to store the users   */
        /* input and output "Input: " to the command window to inform the */
        /* user that another command may be entered.                      */
        UserInput[0] = '\0';

        /* Output an Input Shell-type prompt.                             */
        printf("GATM>");

        /* Read the next line of user input.  Note that this command will */
        /* return an error if it is interrupted by a signal, this case is */
        /* handled in the else clause below.                              */
        if(ReadLine(UserInput, sizeof(UserInput)) > 0)
        {
            /* Next, check to see if a command was input by the user.      */
            if(strlen(UserInput))
            {
                /* Start a newline for the results.                         */
                printf("\r\n");

                if (!strcmp(UserInput, "leadv") || !strcmp(UserInput, "ad"))
                    sprintf(UserInput, "StartAdvertising 118 200");
                else if (!strcmp(UserInput, "stopad") || !strcmp(UserInput, "st"))
                    sprintf(UserInput, "StopAdvertising 0");

                /* The string input by the user contains a value, now run   */
                /* the string through the Command Parser.                   */
                if(CommandParser(&TempCommand, UserInput) >= 0)
                {
                    /* The Command was successfully parsed, run the Command. */
                    Result = CommandInterpreter(&TempCommand);

                    switch(Result)
                    {
                    case Ble::Error::INVALID_COMMAND:
                        printf("Invalid Command.\r\n");
                        break;
                    case Ble::Error::FUNCTION:
                        printf("Function Error.\r\n");
                        break;
                    }
                }
                else
                    printf("Invalid Input.\r\n");
            }
        }
        else
            Result = Ble::Error::EXIT_CODE;
    }
}

///
/// \brief execute_hci_cmd
/// \param aCmd
/// \return
///
int execute_hci_cmd(eConfig_cmd_t aCmd)
{
#if defined(BLUEZ_TOOLS_SUPPORT) && (defined(BCM43) || defined(Linux_x86_64))

    int ctl;
    static struct hci_dev_info di;
    bdaddr_t  _BDADDR_ANY = {{0, 0, 0, 0, 0, 0}};

    /* Open HCI socket  */
    if ((ctl = socket(AF_BLUETOOTH, SOCK_RAW, BTPROTO_HCI)) < 0) {
        perror("Can't open HCI socket.");
        return errno;
    }

    if (ioctl(ctl, HCIGETDEVINFO, (void *) &di))
    {
        perror("Can't get device info");
        return errno;
    }

    di.dev_id = 0;

    if (hci_test_bit(HCI_RAW, &di.flags) && !bacmp(&di.bdaddr, &_BDADDR_ANY)) {
        int dd = hci_open_dev(di.dev_id);
        hci_read_bd_addr(dd, &di.bdaddr, 1000);
        hci_close_dev(dd);
    }

    if (gBleApi)
    {
        DeviceConfig_t dc[] = {{.tag = Ble::Config::EOL},{.tag = Ble::Config::EOL}};
        switch (aCmd)
        {
            case eConfig_UP: ((GattSrv*)gBleApi)->HCIup(ctl, di.dev_id); break;
            case eConfig_DOWN: ((GattSrv*)gBleApi)->HCIdown(ctl, di.dev_id); break;
            case eConfig_PISCAN: ((GattSrv*)gBleApi)->HCIscan(ctl, di.dev_id, (char*) "piscan"); break;
            case eConfig_NOSCAN: ((GattSrv*)gBleApi)->HCIscan(ctl, di.dev_id, (char*) "noscan"); break;
            case eConfig_NOLEADV: ((GattSrv*)gBleApi)->HCIno_le_adv(di.dev_id); break;
            case eConfig_ALLUP:
                gBleApi->Initialize();
                break;

            case eConfig_ALLDOWN:
                gBleApi->Shutdown();
                break;

            case eConfig_LEADV:
                dc->tag = Ble::Config::Discoverable;
                gBleApi->Configure(dc);
                break;

            case eConfig_CLASS:
                dc->tag = Ble::Config::LocalClassOfDevice;
                gBleApi->Configure(dc);
                break;

            default:
                printf("wrong command for target %s\n", RING_NAME);
            break;
        }
    }

    close(ctl);
#else
    (void)aCmd;
    printf("hci commands are not available with target %s. Abort\n", RING_NAME);
#endif
    return 0;
}

int main(int argc, char ** argv)
{
    int arg_idx, ret_val = 0;
    bool bStartUI = false;
    BlePairing *Pairing = BlePairing::getInstance();
    if (Pairing == NULL)
    {
        printf("BlePairing instance is needed to define pairing services\n");
        printf("ERROR: failed to obtain BlePairing instance. Abort.\n");
        return -777;
    }

    for (arg_idx = argc-1; arg_idx > 0; arg_idx--)
    {
        if (0) {;} // plaseholder for following "else if"
#ifdef BLUEZ_TOOLS_SUPPORT
        // perform HCI commands:
        else if VALIDATE_AND_EXEC_ARGUMENT(argv[arg_idx], "--up", eConfig_UP)
        else if VALIDATE_AND_EXEC_ARGUMENT(argv[arg_idx], "--down", eConfig_DOWN)
        else if VALIDATE_AND_EXEC_ARGUMENT(argv[arg_idx], "--piscan", eConfig_PISCAN)
        else if VALIDATE_AND_EXEC_ARGUMENT(argv[arg_idx], "--noscan", eConfig_NOSCAN)
        else if VALIDATE_AND_EXEC_ARGUMENT(argv[arg_idx], "--leadv", eConfig_LEADV)
        else if VALIDATE_AND_EXEC_ARGUMENT(argv[arg_idx], "--noleadv", eConfig_NOLEADV)
        else if VALIDATE_AND_EXEC_ARGUMENT(argv[arg_idx], "--class", eConfig_CLASS)
        else if VALIDATE_AND_EXEC_ARGUMENT(argv[arg_idx], "--hciinit", eConfig_ALLUP)
        else if VALIDATE_AND_EXEC_ARGUMENT(argv[arg_idx], "--hcishutdown", eConfig_ALLDOWN)
#endif // BLUEZ_TOOLS_SUPPORT

        else {
            bStartUI = true;
            if (!strcmp(argv[arg_idx], "--autoinit"))
            {
                bStartUI = true;
                printf("---starting in a sec...---\n");
                sleep(2);

                if (Ble::Error::NONE != (ret_val = Pairing->Initialize()))
                {
                    printf("Pairing->Initialize() failed, ret = %d. Abort", ret_val);
                    break;
                }
                if (Ble::Error::NONE != (ret_val = Pairing->StartAdvertising()))
                {
                    printf("Pairing->StartAdvertising failed, ret = %d, Abort.\n", ret_val);
                    break;
                }
            }
        }
    }

    if (bStartUI)
        user_interface(TRUE);
    return ret_val;
}
