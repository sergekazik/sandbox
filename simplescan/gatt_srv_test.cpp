/*-----------------------------------------------------------------------
 *      ===================    ----------------------                   *
 *      # gatt_srv_test.c #    | Ring App Setup.cpp |                   *
 *      ===================    ----------------------                   *
 *                  |                     |                             *
 *                  |       ------------------------------              *
 *                  |       |   RingBleApi.cpp abstract  |              *
 *                  |       ------------------------------              *
 *                  |       | TI Impl.cpp | BCM Impl.cpp |              *
 *                  |       ------------------------------              *
 *                  |             |                                     *
 *               ------------------------    ---------------            *
 *               |      gatt_api.c      |    | hcitools.c  |            *
 *               ------------------------    ---------------            *
 *                          |                       |                   *
 *               ------------------------    ---------------            *
 *               | TI WiLink18xx BlueTP |    |   BlueZ     |            *
 *               ------------------------    ---------------            *
 *----------------------------------------------------------------------*/
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <signal.h>
#include <unistd.h>

extern "C" {
#include "SS1BTPM.h"          /* BTPM Application Programming Interface.      */
#include "BTPMMAPI.h"          /* BTPM Application Programming Interface.      */
} // #ifdef __cplusplus

#include "RingGattApi.hh"
#include "gatt_srv_test.h"    /* Main Application Prototypes and Constants.   */

using namespace Ring::Ble;

/* Internal Variables to this Module (Remember that all variables   */
/* declared static are initialized to 0 automatically by the        */
/* compiler as part of standard C/C++).                             */

static unsigned int        NumberCommands;
static CommandTable_t      CommandTable[MAX_SUPPORTED_COMMANDS];

/* TODO: (skazik) - orginize passing data it as parameter
 *                - verify notifications with a gatt clinet
 */
static char DataString[] = "Hello World from gatt_srv_test";

/* Internal function prototypes.                                    */
static void UserInterface(void);
static int ReadLine(char *Buffer, unsigned int BufferSize);
static unsigned int StringToUnsignedInteger(char *StringInteger);
static char *StringParser(char *String);
static int CommandParser(UserCommand_t *TempCommand, char *UserInput);
static int CommandInterpreter(UserCommand_t *TempCommand);
static int AddCommand(char *CommandName, CommandFunction_t CommandFunction);
static CommandFunction_t FindCommand(char *Command);
static void ClearCommands(void);

static int DisplayHelp(ParameterList_t *TempParam __attribute__ ((unused)));

static GattSrv* gGattSrvInst = NULL;
#define RING_BLE_DEF_CPP_WRAPPER
#include "gatt_srv_defs.h"

/*********************************************************************
 * Temp. "HARDCODED" Services Declaration.
 *********************************************************************/

static CharacteristicInfo_t Srv1Attr1=
{
    /* Characteristic Properties.                                        */
    GATM_CHARACTERISTIC_PROPERTIES_READ | GATM_CHARACTERISTIC_PROPERTIES_WRITE | GATM_CHARACTERISTIC_PROPERTIES_INDICATE,

    /* Characteristic Security Properties.                               */
    GATM_SECURITY_PROPERTIES_AUTHENTICATED_ENCRYPTION_WRITE,

    /* Characteristic UUID.                                              */
    { 0x6A, 0xFE, 0x4D, 0x41, 0x6C, 0x52, 0x10, 0x14, 0x8B, 0x7A, 0x95, 0x02, 0x01, 0xB8, 0x65, 0xBC  },

    /* Value is allocated dynamically.                                   */
    FALSE,

    /* Maximum value length.                                             */
    117,

    /* Current value length.                                             */
    117,

    /* Current value.                                                    */
    (Byte_t *)"VUHhfCQDLvB~!%^&*()-_+={}[]|:;.<>?/#uoPXonUEjisFQjbWGlXmAoQsSSqJwexZsugfFtamHcTun~!%^&*()-_+={}[]|:;.<>?/#AfjLKCeuMia"
};

static CharacteristicInfo_t Srv1Attr2=
{
    /* Characteristic Properties.                                        */
    GATM_CHARACTERISTIC_PROPERTIES_READ | GATM_CHARACTERISTIC_PROPERTIES_WRITE | GATM_CHARACTERISTIC_PROPERTIES_NOTIFY | GATM_CHARACTERISTIC_PROPERTIES_AUTHENTICATED_SIGNED_WRITES,

    /* Characteristic Security Properties.                               */
    GATM_SECURITY_PROPERTIES_UNAUTHENTICATED_ENCRYPTION_READ | GATM_SECURITY_PROPERTIES_UNAUTHENTICATED_ENCRYPTION_WRITE | GATM_SECURITY_PROPERTIES_AUTHENTICATED_SIGNED_WRITES,

    /* Characteristic UUID.                                              */
    { 0x6A, 0xFE, 0x51, 0x96, 0x6C, 0x52, 0x10, 0x14, 0x8B, 0x7A, 0x95, 0x02, 0x01, 0xB8, 0x65, 0xBC  },

    /* Value is allocated dynamically.                                   */
    FALSE,

    /* Maximum value length.                                             */
    45,

    /* Current value length.                                             */
    0,

    /* Current value.                                                    */
    (Byte_t *)NULL
};

static CharacteristicInfo_t Srv1Attr3=
{
    /* Characteristic Properties.                                        */
    GATM_CHARACTERISTIC_PROPERTIES_WRITE,

    /* Characteristic Security Properties.                               */
    GATM_SECURITY_PROPERTIES_NO_SECURITY,

    /* Characteristic UUID.                                              */
    { 0x6A, 0xFE, 0x52, 0x8B, 0x6C, 0x52, 0x10, 0x14, 0x8B, 0x7A, 0x95, 0x02, 0x01, 0xB8, 0x65, 0xBC  },

    /* Value is allocated dynamically.                                   */
    FALSE,

    /* Maximum value length.                                             */
    50,

    /* Current value length.                                             */
    0,

    /* Current value.                                                    */
    (Byte_t *)NULL
};


/* The following array is the array containing attributes that are   */
/* registered by this service.                                       */
static AttributeInfo_t SrvTable1[] =
{
    { atCharacteristic, 1, (void *)&Srv1Attr1 },
    { atCharacteristic, 3, (void *)&Srv1Attr2 },
    { atCharacteristic, 5, (void *)&Srv1Attr3 }
};

/*********************************************************************/
/* End of Service Declaration.                                       */
/*********************************************************************/

/*********************************************************************/
/* Service Declaration.                                              */
/*********************************************************************/

static CharacteristicInfo_t Srv2Attr2=
{
    /* Characteristic Properties.                                        */
    GATM_CHARACTERISTIC_PROPERTIES_READ | GATM_CHARACTERISTIC_PROPERTIES_WRITE_WO_RESP | GATM_CHARACTERISTIC_PROPERTIES_WRITE,

    /* Characteristic Security Properties.                               */
    GATM_SECURITY_PROPERTIES_AUTHENTICATED_ENCRYPTION_READ | GATM_SECURITY_PROPERTIES_UNAUTHENTICATED_ENCRYPTION_WRITE,

    /* Characteristic UUID.                                              */
    { 0x6A, 0xFE, 0x55, 0xA3, 0x6C, 0x52, 0x10, 0x14, 0x8B, 0x7A, 0x95, 0x02, 0x01, 0xB8, 0x65, 0xBC  },

    /* Value is allocated dynamically.                                   */
    FALSE,

    /* Maximum value length.                                             */
    464,

    /* Current value length.                                             */
    0,

    /* Current value.                                                    */
    (Byte_t *)NULL
};

static CharacteristicInfo_t Srv2Attr3=
{
    /* Characteristic Properties.                                        */
    GATM_CHARACTERISTIC_PROPERTIES_READ | GATM_CHARACTERISTIC_PROPERTIES_WRITE_WO_RESP,

    /* Characteristic Security Properties.                               */
    GATM_SECURITY_PROPERTIES_NO_SECURITY,

    /* Characteristic UUID.                                              */
    { 0x6A, 0xFE, 0x56, 0x89, 0x6C, 0x52, 0x10, 0x14, 0x8B, 0x7A, 0x95, 0x02, 0x01, 0xB8, 0x65, 0xBC  },

    /* Value is allocated dynamically.                                   */
    FALSE,

    /* Maximum value length.                                             */
    36,

    /* Current value length.                                             */
    36,

    /* Current value.                                                    */
    (Byte_t *)"rlgzzRztTCqnCBBrsm~!%^&*()-_+={}[]|"
};

static CharacteristicInfo_t Srv2Attr4=
{
    /* Characteristic Properties.                                        */
    GATM_CHARACTERISTIC_PROPERTIES_READ | GATM_CHARACTERISTIC_PROPERTIES_WRITE,

    /* Characteristic Security Properties.                               */
    GATM_SECURITY_PROPERTIES_UNAUTHENTICATED_ENCRYPTION_READ | GATM_SECURITY_PROPERTIES_UNAUTHENTICATED_ENCRYPTION_WRITE,

    /* Characteristic UUID.                                              */
    { 0x6A, 0xFE, 0x5C, 0x40, 0x6C, 0x52, 0x10, 0x14, 0x8B, 0x7A, 0x95, 0x02, 0x01, 0xB8, 0x65, 0xBC  },

    /* Value is allocated dynamically.                                   */
    FALSE,

    /* Maximum value length.                                             */
    62,

    /* Current value length.                                             */
    0,

    /* Current value.                                                    */
    (Byte_t *)NULL
};

static DescriptorInfo_t Srv2Attr5=
{
    /* Descriptor Properties.                                            */
    GATM_DESCRIPTOR_PROPERTIES_READ,

    /* Descriptor Security Properties.                                   */
    GATM_SECURITY_PROPERTIES_UNAUTHENTICATED_ENCRYPTION_READ,

    /* Descriptor UUID.                                                  */
    { 0x6A, 0xFE, 0x5D, 0x2A, 0x6C, 0x52, 0x10, 0x14, 0x8B, 0x7A, 0x95, 0x02, 0x01, 0xB8, 0x65, 0xBC  },

    /* Value is allocated dynamically.                                   */
    FALSE,

    /* Maximum value length.                                             */
    264,

    /* Current value length.                                             */
    10,

    /* Current value.                                                    */
    (Byte_t *)"HelloWorld"
};

static CharacteristicInfo_t Srv2Attr6=
{
    /* Characteristic Properties.                                        */
    GATM_CHARACTERISTIC_PROPERTIES_READ | GATM_CHARACTERISTIC_PROPERTIES_WRITE,

    /* Characteristic Security Properties.                               */
    GATM_SECURITY_PROPERTIES_UNAUTHENTICATED_ENCRYPTION_READ | GATM_SECURITY_PROPERTIES_UNAUTHENTICATED_ENCRYPTION_WRITE,

    /* Characteristic UUID.                                              */
    { 0x6A, 0xFE, 0x5E, 0x32, 0x6C, 0x52, 0x10, 0x14, 0x8B, 0x7A, 0x95, 0x02, 0x01, 0xB8, 0x65, 0xBC  },

    /* Value is allocated dynamically.                                   */
    FALSE,

    /* Maximum value length.                                             */
    321,

    /* Current value length.                                             */
    321,

    /* Current value.                                                    */
    (Byte_t *)"qioFsYUBoESuvABqUCDyOXWmevWjxJsaYYMvpnoY~!%^&*()-_+={}[]|:;.<>?/#ShTtUGfemhchQnTAuZFTUwBSSSNhjDcBlASzZaMDsqWkCVAWCYIiqvIWhUcWUHwPIqKTLiYzQFNugrkvZWwqjRexoJeAiwEZrqjxHIukLiAWhUUoV~!%^&*()-_+={}[]|:;.<>?/#tgvnqYVCfywRZgBTARae~!%^&*()-_+={}[]|:;.<>?/#vmBGQSf~!%^&*()-_+={}[]|:;.<>?/#iJMviBnTpRvLjOTVwKUoAHbOdMTWRlvNXSrRNOyhAaFXRXBGxzOyMigBvOTHCNsuSgxscaBucAhmtuRocF~!%^&*()-_+={}[]|:;.<>?/#AAJbaYbMvoMzYspPdXMjPBGxv~!%^&*()-_+={}[]|:;.<>?/#fTTpc~!%^&*()-_+={}[]|:;.<>?/#mHfzsCpjnhdubiwGAhidAV"
};

static CharacteristicInfo_t Srv2Attr7=
{
    /* Characteristic Properties.                                        */
    GATM_CHARACTERISTIC_PROPERTIES_READ | GATM_CHARACTERISTIC_PROPERTIES_WRITE_WO_RESP,

    /* Characteristic Security Properties.                               */
    GATM_SECURITY_PROPERTIES_UNAUTHENTICATED_ENCRYPTION_READ | GATM_SECURITY_PROPERTIES_AUTHENTICATED_ENCRYPTION_WRITE,

    /* Characteristic UUID.                                              */
    { 0x6A, 0xFE, 0x63, 0x01, 0x6C, 0x52, 0x10, 0x14, 0x8B, 0x7A, 0x95, 0x02, 0x01, 0xB8, 0x65, 0xBC  },

    /* Value is allocated dynamically.                                   */
    FALSE,

    /* Maximum value length.                                             */
    186,

    /* Current value length.                                             */
    186,

    /* Current value.                                                    */
    (Byte_t *)"kGmFzGcmIWrFLGfeYBwzWRwXCKHlXYdnVPugxwJ~!%^&*()-_+={}[]|:;.<>?/#ZaimuLdaHNDALPEzzCtEaHSZyKHfuSAIa~!%^&*()-_+={}[]|:;.<>?/#nKxCfeXahNzeegKGZBErbDHiYciGOqy~!%^&*()-_+={}[]|:;.<>?/#~!%^&*()-_+={}[]|:;.<>?/#KomahJaknrBClz~!%^&*()-_+={}[]|:;.<>?/#cKQSngsQCDjPABccaYNstELvfkWzxMbIrZtzcxvoTxtSAlDfoysREDZLzW~!%^&*()-_+={}[]|:;.<>?/#CUtI~!%^&*()-_+={}[]|:;.<>?/#"
};

static CharacteristicInfo_t Srv2Attr8=
{
    /* Characteristic Properties.                                        */
    GATM_CHARACTERISTIC_PROPERTIES_WRITE,

    /* Characteristic Security Properties.                               */
    GATM_SECURITY_PROPERTIES_UNAUTHENTICATED_ENCRYPTION_WRITE,

    /* Characteristic UUID.                                              */
    { 0x6A, 0xFE, 0x66, 0xCA, 0x6C, 0x52, 0x10, 0x14, 0x8B, 0x7A, 0x95, 0x02, 0x01, 0xB8, 0x65, 0xBC  },

    /* Value is allocated dynamically.                                   */
    FALSE,

    /* Maximum value length.                                             */
    120,

    /* Current value length.                                             */
    0,

    /* Current value.                                                    */
    (Byte_t *)NULL
};

static DescriptorInfo_t Srv2Attr9=
{
    /* Descriptor Properties.                                            */
    GATM_DESCRIPTOR_PROPERTIES_READ,

    /* Descriptor Security Properties.                                   */
    GATM_SECURITY_PROPERTIES_NO_SECURITY,

    /* Descriptor UUID.                                                  */
    { 0x6A, 0xFE, 0x67, 0x9A, 0x6C, 0x52, 0x10, 0x14, 0x8B, 0x7A, 0x95, 0x02, 0x01, 0xB8, 0x65, 0xBC  },

    /* Value is allocated dynamically.                                   */
    FALSE,

    /* Maximum value length.                                             */
    172,

    /* Current value length.                                             */
    10,

    /* Current value.                                                    */
    (Byte_t *)"HelloWorld"
};


/* The following array is the array containing attributes that are   */
/* registered by this service.                                       */
static AttributeInfo_t SrvTable2[] =
{
    { atInclude, 1, NULL },
    { atCharacteristic, 2, (void *)&Srv2Attr2 },
    { atCharacteristic, 4, (void *)&Srv2Attr3 },
    { atCharacteristic, 6, (void *)&Srv2Attr4 },
    { atDescriptor, 8, (void *)&Srv2Attr5 },
    { atCharacteristic, 9, (void *)&Srv2Attr6 },
    { atCharacteristic, 11, (void *)&Srv2Attr7 },
    { atCharacteristic, 13, (void *)&Srv2Attr8 },
    { atDescriptor, 15, (void *)&Srv2Attr9 }
};

/*********************************************************************/
/* End of Service Declaration.                                       */
/*********************************************************************/

/*********************************************************************/
/* Service Declaration.                                              */
/*********************************************************************/

static CharacteristicInfo_t Srv3Attr2=
{
    /* Characteristic Properties.                                        */
    GATM_CHARACTERISTIC_PROPERTIES_READ,

    /* Characteristic Security Properties.                               */
    GATM_SECURITY_PROPERTIES_NO_SECURITY,

    /* Characteristic UUID.                                              */
    { 0x6A, 0xFE, 0x69, 0x37, 0x6C, 0x52, 0x10, 0x14, 0x8B, 0x7A, 0x95, 0x02, 0x01, 0xB8, 0x65, 0xBC  },

    /* Value is allocated dynamically.                                   */
    FALSE,

    /* Maximum value length.                                             */
    229,

    /* Current value length.                                             */
    10,

    /* Current value.                                                    */
    (Byte_t *)"HelloWorld"
};

static CharacteristicInfo_t Srv3Attr3=
{
    /* Characteristic Properties.                                        */
    GATM_CHARACTERISTIC_PROPERTIES_WRITE,

    /* Characteristic Security Properties.                               */
    GATM_SECURITY_PROPERTIES_NO_SECURITY,

    /* Characteristic UUID.                                              */
    { 0x6A, 0xFE, 0x69, 0xF2, 0x6C, 0x52, 0x10, 0x14, 0x8B, 0x7A, 0x95, 0x02, 0x01, 0xB8, 0x65, 0xBC  },

    /* Value is allocated dynamically.                                   */
    FALSE,

    /* Maximum value length.                                             */
    165,

    /* Current value length.                                             */
    0,

    /* Current value.                                                    */
    (Byte_t *)NULL
};

static CharacteristicInfo_t Srv3Attr4=
{
    /* Characteristic Properties.                                        */
    GATM_CHARACTERISTIC_PROPERTIES_WRITE,

    /* Characteristic Security Properties.                               */
    GATM_SECURITY_PROPERTIES_NO_SECURITY,

    /* Characteristic UUID.                                              */
    { 0x6A, 0xFE, 0x6A, 0xA7, 0x6C, 0x52, 0x10, 0x14, 0x8B, 0x7A, 0x95, 0x02, 0x01, 0xB8, 0x65, 0xBC  },

    /* Value is allocated dynamically.                                   */
    FALSE,

    /* Maximum value length.                                             */
    495,

    /* Current value length.                                             */
    0,

    /* Current value.                                                    */
    (Byte_t *)NULL
};

static DescriptorInfo_t Srv3Attr5=
{
    /* Descriptor Properties.                                            */
    GATM_DESCRIPTOR_PROPERTIES_READ | GATM_DESCRIPTOR_PROPERTIES_WRITE,

    /* Descriptor Security Properties.                                   */
    GATM_SECURITY_PROPERTIES_AUTHENTICATED_ENCRYPTION_READ,

    /* Descriptor UUID.                                                  */
    { 0x6A, 0xFE, 0x6B, 0x65, 0x6C, 0x52, 0x10, 0x14, 0x8B, 0x7A, 0x95, 0x02, 0x01, 0xB8, 0x65, 0xBC  },

    /* Value is allocated dynamically.                                   */
    FALSE,

    /* Maximum value length.                                             */
    243,

    /* Current value length.                                             */
    227,

    /* Current value.                                                    */
    (Byte_t *)"SDZSWqRxoK~!%^&*()-_+={}[]|:;.<>?/#BDsKpSyoekWZShYdZmPJEnZPJGCnqAzYGnSewzqxNKLQwHgZVVOevQkuBoZXLeIksvfaRwEjjEBcmVvjdTmvuEoTnDMSfSNaAKlkGLzkyOTzmsW~!%^&*()-_+={}[]|:;.<>?/#czlMJCADkIIDWJAqBnkcUowtmEa~!%^&*()-_+={}[]|:;.<>?/#xkhbaBCPvFBVodzsKqaoqCMDOXjLINajTIJUHoUWJAlAWKDeYsBfqBv~!%^&*()-_+={}[]|:;.<>?/#xyMxzvKpDSB~!%^&*()-_+={}[]|:;.<>?/#jnGiLaKg"
};


/* The following array is the array containing attributes that are   */
/* registered by this service.                                       */
static AttributeInfo_t SrvTable3[] =
{
    { atInclude, 1, NULL },
    { atCharacteristic, 2, (void *)&Srv3Attr2 },
    { atCharacteristic, 4, (void *)&Srv3Attr3 },
    { atCharacteristic, 6, (void *)&Srv3Attr4 },
    { atDescriptor, 8, (void *)&Srv3Attr5 }
};

/*********************************************************************/
/* End of Service Declaration.                                       */
/*********************************************************************/

/*********************************************************************/
/* Service Declaration.                                              */
/*********************************************************************/

static CharacteristicInfo_t Srv4Attr2=
{
    /* Characteristic Properties.                                        */
    GATM_CHARACTERISTIC_PROPERTIES_WRITE,

    /* Characteristic Security Properties.                               */
    GATM_SECURITY_PROPERTIES_NO_SECURITY,

    /* Characteristic UUID.                                              */
    { 0x6A, 0xFE, 0x6F, 0x22, 0x6C, 0x52, 0x10, 0x14, 0x8B, 0x7A, 0x95, 0x02, 0x01, 0xB8, 0x65, 0xBC  },

    /* Value is allocated dynamically.                                   */
    FALSE,

    /* Maximum value length.                                             */
    307,

    /* Current value length.                                             */
    0,

    /* Current value.                                                    */
    (Byte_t *)NULL
};

static CharacteristicInfo_t Srv4Attr3=
{
    /* Characteristic Properties.                                        */
    GATM_CHARACTERISTIC_PROPERTIES_WRITE_WO_RESP | GATM_CHARACTERISTIC_PROPERTIES_WRITE,

    /* Characteristic Security Properties.                               */
    GATM_SECURITY_PROPERTIES_AUTHENTICATED_ENCRYPTION_WRITE,

    /* Characteristic UUID.                                              */
    { 0x6A, 0xFE, 0x6F, 0xFE, 0x6C, 0x52, 0x10, 0x14, 0x8B, 0x7A, 0x95, 0x02, 0x01, 0xB8, 0x65, 0xBC  },

    /* Value is allocated dynamically.                                   */
    FALSE,

    /* Maximum value length.                                             */
    426,

    /* Current value length.                                             */
    0,

    /* Current value.                                                    */
    (Byte_t *)NULL
};

static DescriptorInfo_t Srv4Attr4=
{
    /* Descriptor Properties.                                            */
    GATM_DESCRIPTOR_PROPERTIES_READ | GATM_DESCRIPTOR_PROPERTIES_WRITE,

    /* Descriptor Security Properties.                                   */
    GATM_SECURITY_PROPERTIES_AUTHENTICATED_ENCRYPTION_READ | GATM_SECURITY_PROPERTIES_UNAUTHENTICATED_ENCRYPTION_WRITE,

    /* Descriptor UUID.                                                  */
    { 0x6A, 0xFE, 0x70, 0xE7, 0x6C, 0x52, 0x10, 0x14, 0x8B, 0x7A, 0x95, 0x02, 0x01, 0xB8, 0x65, 0xBC  },

    /* Value is allocated dynamically.                                   */
    FALSE,

    /* Maximum value length.                                             */
    490,

    /* Current value length.                                             */
    469,

    /* Current value.                                                    */
    (Byte_t *)"~!%^&*()-_+={}[]|:;.<>?/#urpPyrtiaBXDTzKNyWgozZuCTpXYlkOCVWCbUbZIUrqjUWkNflnkUthgfAJtXYOGsINpAoVBOQwHGcjSEviprvXzjzljZtALcMGPbenLdbHpBObmL~!%^&*()-_+={}[]|:;.<>?/#eigmIow~!%^&*()-_+={}[]|:;.<>?/#UGTUGlhZREyIGDZCqoqmFxYkmrXgKCWBeRypnRDUjJfwmRAJRIWLvzIPUnLBYOFllTUvgQaGwXBQLIwuF~!%^&*()-_+={}[]|:;.<>?/#KGqLVpsnKcXPdOKHDlvLXZukFDtHnnfGiESZCqYEtwGVOiabkWTCHxVgzzUMrAPThqbxnjewzCBOYYHOMRQKhgVrCCGrPfkYMLuJxQfQhBKzFwgByxcaoPnrCVskADR~!%^&*()-_+={}[]|:;.<>?/#vSZOsRyMHCN~!%^&*()-_+={}[]|:;.<>?/#AwbVVsUIGWVDxTwBjjqaOWzfXAEdHaOLnBVtUunpEDSszUNWnOlWQrZzaNbDdwx~!%^&*()-_+={}[]|:;.<>?/#hXxn~!%^&*()-_+={}[]|:;.<>?/#yAWs~!%^&*()-_+={}[]|:;.<>?/#dWmDMwXVkDPDkigimWc~!%^&*()-_+={}[]|:;.<>?/#WzsBmVhLGaRgFxAJrupOWlkrpNFWPO"
};

static CharacteristicInfo_t Srv4Attr5=
{
    /* Characteristic Properties.                                        */
    GATM_CHARACTERISTIC_PROPERTIES_WRITE,

    /* Characteristic Security Properties.                               */
    GATM_SECURITY_PROPERTIES_AUTHENTICATED_ENCRYPTION_WRITE,

    /* Characteristic UUID.                                              */
    { 0x6A, 0xFE, 0x77, 0x51, 0x6C, 0x52, 0x10, 0x14, 0x8B, 0x7A, 0x95, 0x02, 0x01, 0xB8, 0x65, 0xBC  },

    /* Value is allocated dynamically.                                   */
    FALSE,

    /* Maximum value length.                                             */
    487,

    /* Current value length.                                             */
    0,

    /* Current value.                                                    */
    (Byte_t *)NULL
};

static CharacteristicInfo_t Srv4Attr6=
{
    /* Characteristic Properties.                                        */
    GATM_CHARACTERISTIC_PROPERTIES_WRITE,

    /* Characteristic Security Properties.                               */
    GATM_SECURITY_PROPERTIES_AUTHENTICATED_ENCRYPTION_WRITE,

    /* Characteristic UUID.                                              */
    { 0x6A, 0xFE, 0x78, 0x46, 0x6C, 0x52, 0x10, 0x14, 0x8B, 0x7A, 0x95, 0x02, 0x01, 0xB8, 0x65, 0xBC  },

    /* Value is allocated dynamically.                                   */
    FALSE,

    /* Maximum value length.                                             */
    267,

    /* Current value length.                                             */
    0,

    /* Current value.                                                    */
    (Byte_t *)NULL
};

static DescriptorInfo_t Srv4Attr7=
{
    /* Descriptor Properties.                                            */
    GATM_DESCRIPTOR_PROPERTIES_WRITE,

    /* Descriptor Security Properties.                                   */
    GATM_SECURITY_PROPERTIES_NO_SECURITY,

    /* Descriptor UUID.                                                  */
    { 0x6A, 0xFE, 0x78, 0xFF, 0x6C, 0x52, 0x10, 0x14, 0x8B, 0x7A, 0x95, 0x02, 0x01, 0xB8, 0x65, 0xBC  },

    /* Value is allocated dynamically.                                   */
    FALSE,

    /* Maximum value length.                                             */
    189,

    /* Current value length.                                             */
    0,

    /* Current value.                                                    */
    (Byte_t *)NULL
};


/* The following array is the array containing attributes that are   */
/* registered by this service.                                       */
static AttributeInfo_t SrvTable4[] =
{
    { atInclude, 1, NULL },
    { atCharacteristic, 2, (void *)&Srv4Attr2 },
    { atCharacteristic, 4, (void *)&Srv4Attr3 },
    { atDescriptor, 6, (void *)&Srv4Attr4 },
    { atCharacteristic, 7, (void *)&Srv4Attr5 },
    { atCharacteristic, 9, (void *)&Srv4Attr6 },
    { atDescriptor, 11, (void *)&Srv4Attr7 }
};

/*********************************************************************/
/* End of Service Declaration.                                       */
/*********************************************************************/

/*********************************************************************/
/* Service Declaration.                                              */
/*********************************************************************/

static CharacteristicInfo_t Srv5Attr1=
{
    /* Characteristic Properties.                                        */
    GATM_CHARACTERISTIC_PROPERTIES_READ | GATM_CHARACTERISTIC_PROPERTIES_WRITE_WO_RESP | GATM_CHARACTERISTIC_PROPERTIES_WRITE,

    /* Characteristic Security Properties.                               */
    GATM_SECURITY_PROPERTIES_AUTHENTICATED_ENCRYPTION_READ | GATM_SECURITY_PROPERTIES_AUTHENTICATED_ENCRYPTION_WRITE,

    /* Characteristic UUID.                                              */
    { 0x6A, 0xFE, 0x7A, 0xCE, 0x6C, 0x52, 0x10, 0x14, 0x8B, 0x7A, 0x95, 0x02, 0x01, 0xB8, 0x65, 0xBC  },

    /* Value is allocated dynamically.                                   */
    FALSE,

    /* Maximum value length.                                             */
    326,

    /* Current value length.                                             */
    0,

    /* Current value.                                                    */
    (Byte_t *)NULL
};

static CharacteristicInfo_t Srv5Attr2=
{
    /* Characteristic Properties.                                        */
    GATM_CHARACTERISTIC_PROPERTIES_READ | GATM_CHARACTERISTIC_PROPERTIES_WRITE,

    /* Characteristic Security Properties.                               */
    GATM_SECURITY_PROPERTIES_AUTHENTICATED_ENCRYPTION_READ,

    /* Characteristic UUID.                                              */
    { 0x6A, 0xFE, 0x7B, 0xD8, 0x6C, 0x52, 0x10, 0x14, 0x8B, 0x7A, 0x95, 0x02, 0x01, 0xB8, 0x65, 0xBC  },

    /* Value is allocated dynamically.                                   */
    FALSE,

    /* Maximum value length.                                             */
    233,

    /* Current value length.                                             */
    233,

    /* Current value.                                                    */
    (Byte_t *)"VRWGsXStNGTcZJTqQkQxnTXZBdGjtjBKaAWWGvMFGxDleYXtztTYBvyQquoo~!%^&*()-_+={}[]|:;.<>?/#XuKgBLoyu~!%^&*()-_+={}[]|:;.<>?/#LuaLebVuyVIYaAgAEpEBJnkOoaRPz~!%^&*()-_+={}[]|:;.<>?/#jGwjjXaZrXuSkgJdHzDelfBzFRntFNZQNrxbgcU~!%^&*()-_+={}[]|:;.<>?/#PhdYpBIyxqaibS~!%^&*()-_+={}[]|:;.<>?/#xJtAOtvIUZwCToViALctNDDguIKjWgQMPJPaMSHbGwtCDrJOvnylIZMPWrXvIRibFImIHiEaqwroS"
};

static CharacteristicInfo_t Srv5Attr3=
{
    /* Characteristic Properties.                                        */
    GATM_CHARACTERISTIC_PROPERTIES_READ | GATM_CHARACTERISTIC_PROPERTIES_WRITE,

    /* Characteristic Security Properties.                               */
    GATM_SECURITY_PROPERTIES_UNAUTHENTICATED_ENCRYPTION_WRITE,

    /* Characteristic UUID.                                              */
    { 0x6A, 0xFE, 0x7F, 0xD9, 0x6C, 0x52, 0x10, 0x14, 0x8B, 0x7A, 0x95, 0x02, 0x01, 0xB8, 0x65, 0xBC  },

    /* Value is allocated dynamically.                                   */
    FALSE,

    /* Maximum value length.                                             */
    220,

    /* Current value length.                                             */
    220,

    /* Current value.                                                    */
    (Byte_t *)"abERkFtyw~!%^&*()-_+={}[]|:;.<>?/#PFyhGjfDuAdCqIgBllooqMAxQcvoVIOzwPI~!%^&*()-_+={}[]|:;.<>?/#KDfuLyHqdPlbonxkrtMcsHyrDKuIFHNFctzEsbdDlYyhpTs~!%^&*()-_+={}[]|:;.<>?/#uhHpehiGaLXhvqzGDcZjGqkYinUWovoDGXPHUoxErJTaLoIkESbgRMeHJAmEfwHsCzxF~!%^&*()-_+={}[]|:;.<>?/#fwecsjvwosyRUJxjZVoauSlBRglfi~!%^&*()-_+={}[]|:;.<>?/#MBXtEdkCzlrLhyYKkMcaeLnNaqO"
};

static CharacteristicInfo_t Srv5Attr4=
{
    /* Characteristic Properties.                                        */
    GATM_CHARACTERISTIC_PROPERTIES_WRITE_WO_RESP | GATM_CHARACTERISTIC_PROPERTIES_WRITE,

    /* Characteristic Security Properties.                               */
    GATM_SECURITY_PROPERTIES_AUTHENTICATED_ENCRYPTION_WRITE,

    /* Characteristic UUID.                                              */
    { 0x6A, 0xFE, 0x83, 0x6B, 0x6C, 0x52, 0x10, 0x14, 0x8B, 0x7A, 0x95, 0x02, 0x01, 0xB8, 0x65, 0xBC  },

    /* Value is allocated dynamically.                                   */
    FALSE,

    /* Maximum value length.                                             */
    296,

    /* Current value length.                                             */
    0,

    /* Current value.                                                    */
    (Byte_t *)NULL
};

static CharacteristicInfo_t Srv5Attr5=
{
    /* Characteristic Properties.                                        */
    GATM_CHARACTERISTIC_PROPERTIES_READ | GATM_CHARACTERISTIC_PROPERTIES_WRITE_WO_RESP,

    /* Characteristic Security Properties.                               */
    GATM_SECURITY_PROPERTIES_UNAUTHENTICATED_ENCRYPTION_WRITE,

    /* Characteristic UUID.                                              */
    { 0x6A, 0xFE, 0x84, 0x56, 0x6C, 0x52, 0x10, 0x14, 0x8B, 0x7A, 0x95, 0x02, 0x01, 0xB8, 0x65, 0xBC  },

    /* Value is allocated dynamically.                                   */
    FALSE,

    /* Maximum value length.                                             */
    494,

    /* Current value length.                                             */
    0,

    /* Current value.                                                    */
    (Byte_t *)NULL
};

static DescriptorInfo_t Srv5Attr6=
{
    /* Descriptor Properties.                                            */
    GATM_DESCRIPTOR_PROPERTIES_READ,

    /* Descriptor Security Properties.                                   */
    GATM_SECURITY_PROPERTIES_NO_SECURITY,

    /* Descriptor UUID.                                                  */
    { 0x6A, 0xFE, 0x85, 0x21, 0x6C, 0x52, 0x10, 0x14, 0x8B, 0x7A, 0x95, 0x02, 0x01, 0xB8, 0x65, 0xBC  },

    /* Value is allocated dynamically.                                   */
    FALSE,

    /* Maximum value length.                                             */
    417,

    /* Current value length.                                             */
    349,

    /* Current value.                                                    */
    (Byte_t *)"ZLyNTJKNaxZcH~!%^&*()-_+={}[]|:;.<>?/#EEIealwBbtwOnKTCRafurXYsYTnhaBeUWgqmeIcFdcoQRFDKsLdOmq~!%^&*()-_+={}[]|:;.<>?/#NQEDYGgBuJrftgIHUXJNrISkqOJkjEYwqxGfJUVVuQBqSgDwiJzCJSptJykLhVBffIJXqaBEgZggyioVUYJhCJrjXtGNCWirdaAefsirCxyFiAjfxfCZSEVdXewuByBoUFWvIw~!%^&*()-_+={}[]|:;.<>?/#oyjZNkHouTeuwRkzTsUY~!%^&*()-_+={}[]|:;.<>?/#zAUBeqLLSUMYvMh~!%^&*()-_+={}[]|:;.<>?/#ihbCdBBLYkbVFLNZfpYfTBNLPFIxKwuFTKxQINtabCfRWiqEufdurxVaXCDb~!%^&*()-_+={}[]|:;.<>?/#goQypeNfKntfqMLrxfZCvYknOefYToAGvqLqVcwFZGMAJBd"
};

static CharacteristicInfo_t Srv5Attr7=
{
    /* Characteristic Properties.                                        */
    GATM_CHARACTERISTIC_PROPERTIES_READ | GATM_CHARACTERISTIC_PROPERTIES_WRITE_WO_RESP,

    /* Characteristic Security Properties.                               */
    GATM_SECURITY_PROPERTIES_AUTHENTICATED_ENCRYPTION_READ,

    /* Characteristic UUID.                                              */
    { 0x6A, 0xFE, 0x8A, 0x37, 0x6C, 0x52, 0x10, 0x14, 0x8B, 0x7A, 0x95, 0x02, 0x01, 0xB8, 0x65, 0xBC  },

    /* Value is allocated dynamically.                                   */
    FALSE,

    /* Maximum value length.                                             */
    374,

    /* Current value length.                                             */
    10,

    /* Current value.                                                    */
    (Byte_t *)"HelloWorld"
};

static CharacteristicInfo_t Srv5Attr8=
{
    /* Characteristic Properties.                                        */
    GATM_CHARACTERISTIC_PROPERTIES_READ | GATM_CHARACTERISTIC_PROPERTIES_WRITE_WO_RESP,

    /* Characteristic Security Properties.                               */
    GATM_SECURITY_PROPERTIES_UNAUTHENTICATED_ENCRYPTION_WRITE,

    /* Characteristic UUID.                                              */
    { 0x6A, 0xFE, 0x8A, 0xFF, 0x6C, 0x52, 0x10, 0x14, 0x8B, 0x7A, 0x95, 0x02, 0x01, 0xB8, 0x65, 0xBC  },

    /* Value is allocated dynamically.                                   */
    FALSE,

    /* Maximum value length.                                             */
    213,

    /* Current value length.                                             */
    10,

    /* Current value.                                                    */
    (Byte_t *)"HelloWorld"
};

static CharacteristicInfo_t Srv5Attr9=
{
    /* Characteristic Properties.                                        */
    GATM_CHARACTERISTIC_PROPERTIES_READ | GATM_CHARACTERISTIC_PROPERTIES_WRITE_WO_RESP | GATM_CHARACTERISTIC_PROPERTIES_WRITE,

    /* Characteristic Security Properties.                               */
    GATM_SECURITY_PROPERTIES_AUTHENTICATED_ENCRYPTION_WRITE,

    /* Characteristic UUID.                                              */
    { 0x6A, 0xFE, 0x8B, 0xCD, 0x6C, 0x52, 0x10, 0x14, 0x8B, 0x7A, 0x95, 0x02, 0x01, 0xB8, 0x65, 0xBC  },

    /* Value is allocated dynamically.                                   */
    FALSE,

    /* Maximum value length.                                             */
    418,

    /* Current value length.                                             */
    0,

    /* Current value.                                                    */
    (Byte_t *)NULL
};


/* The following array is the array containing attributes that are   */
/* registered by this service.                                       */
static AttributeInfo_t SrvTable5[] =
{
    { atCharacteristic, 1, (void *)&Srv5Attr1 },
    { atCharacteristic, 3, (void *)&Srv5Attr2 },
    { atCharacteristic, 5, (void *)&Srv5Attr3 },
    { atCharacteristic, 7, (void *)&Srv5Attr4 },
    { atCharacteristic, 9, (void *)&Srv5Attr5 },
    { atDescriptor, 11, (void *)&Srv5Attr6 },
    { atCharacteristic, 12, (void *)&Srv5Attr7 },
    { atCharacteristic, 14, (void *)&Srv5Attr8 },
    { atCharacteristic, 16, (void *)&Srv5Attr9 }
};

/*********************************************************************/
/* End of Service Declaration.                                       */
/*********************************************************************/

/*********************************************************************/
/* Service Declaration.                                              */
/*********************************************************************/

static CharacteristicInfo_t Srv6Attr2=
{
    /* Characteristic Properties.                                        */
    GATM_CHARACTERISTIC_PROPERTIES_WRITE,

    /* Characteristic Security Properties.                               */
    GATM_SECURITY_PROPERTIES_NO_SECURITY,

    /* Characteristic UUID.                                              */
    { 0x6A, 0xFE, 0x8D, 0xCB, 0x6C, 0x52, 0x10, 0x14, 0x8B, 0x7A, 0x95, 0x02, 0x01, 0xB8, 0x65, 0xBC  },

    /* Value is allocated dynamically.                                   */
    FALSE,

    /* Maximum value length.                                             */
    374,

    /* Current value length.                                             */
    0,

    /* Current value.                                                    */
    (Byte_t *)NULL
};

static CharacteristicInfo_t Srv6Attr3=
{
    /* Characteristic Properties.                                        */
    GATM_CHARACTERISTIC_PROPERTIES_READ | GATM_CHARACTERISTIC_PROPERTIES_WRITE_WO_RESP,

    /* Characteristic Security Properties.                               */
    GATM_SECURITY_PROPERTIES_UNAUTHENTICATED_ENCRYPTION_WRITE,

    /* Characteristic UUID.                                              */
    { 0x6A, 0xFE, 0x8E, 0xB5, 0x6C, 0x52, 0x10, 0x14, 0x8B, 0x7A, 0x95, 0x02, 0x01, 0xB8, 0x65, 0xBC  },

    /* Value is allocated dynamically.                                   */
    FALSE,

    /* Maximum value length.                                             */
    255,

    /* Current value length.                                             */
    255,

    /* Current value.                                                    */
    (Byte_t *)"JHRgqzMWRhsbGOsxWrszFcyPpOzv~!%^&*()-_+={}[]|:;.<>?/#gJQbSFvpLKbBktISgdCfOgAgDcAOEMMsrEajXMvKkAPBxxoKTIFRAnWfPpegGfCzFDjRgZlyskytEIQDihRKbzPGJzXkwinWguoxkdQOLoJqbjjgjcRdzBeewuZKkEhZjrGpINilcPBxNhepqxHeJnFfotxgNcxnmnYuJTc~!%^&*()-_+={}[]|:;.<>?/#muyF~!%^&*()-_+={}[]|:;.<>?/#LmriVMHhxlGLoaIJnoGXAxCcokMphszDikMuGW~!%^&*()-_+={}[]|:;.<>?/#YXASuzLtfXImfc"
};


/* The following array is the array containing attributes that are   */
/* registered by this service.                                       */
static AttributeInfo_t SrvTable6[] =
{
    { atInclude, 1, NULL },
    { atCharacteristic, 2, (void *)&Srv6Attr2 },
    { atCharacteristic, 4, (void *)&Srv6Attr3 }
};

/*********************************************************************/
/* End of Service Declaration.                                       */
/*********************************************************************/

/* The following array is the array containing attributes that are   */
/* registered by this service.                                       */
static ServiceInfo_t ServiceTable[] =
{
    {
        /* Service Flags.                                                 */
        0,

        /* Service ID.                                                    */
        0,

        /* PersistentUID.                                                 */
        0,

        /* Service UUID.                                                  */
        { 0x6A, 0xFE, 0x54, 0x81, 0x6C, 0x52, 0x10, 0x14, 0x8B, 0x7A, 0x95, 0x02, 0x01, 0xB8, 0x65, 0xBC  },

        /* Service Handle Range.                                          */
        {0, 0},

        /* Number of Service Table Entries.                               */
        3,

        /* Service Attribute List.                                        */
        SrvTable1
    },
    {
        /* Service Flags.                                                 */
        SERVICE_TABLE_FLAGS_USE_PERSISTENT_UID,

        /* Service ID.                                                    */
        0,

        /* PersistentUID.                                                 */
        0,

        /* Service UUID.                                                  */
        { 0x6A, 0xFE, 0x68, 0x65, 0x6C, 0x52, 0x10, 0x14, 0x8B, 0x7A, 0x95, 0x02, 0x01, 0xB8, 0x65, 0xBC  },

        /* Service Handle Range.                                          */
        {0, 0},

        /* Number of Service Table Entries.                               */
        9,

        /* Service Attribute List.                                        */
        SrvTable2
    },
    {
        /* Service Flags.                                                 */
        SERVICE_TABLE_FLAGS_USE_PERSISTENT_UID,

        /* Service ID.                                                    */
        0,

        /* PersistentUID.                                                 */
        0,

        /* Service UUID.                                                  */
        { 0x6A, 0xFE, 0x6E, 0x1B, 0x6C, 0x52, 0x10, 0x14, 0x8B, 0x7A, 0x95, 0x02, 0x01, 0xB8, 0x65, 0xBC  },

        /* Service Handle Range.                                          */
        {0, 0},

        /* Number of Service Table Entries.                               */
        5,

        /* Service Attribute List.                                        */
        SrvTable3
    },
    {
        /* Service Flags.                                                 */
        0,

        /* Service ID.                                                    */
        0,

        /* PersistentUID.                                                 */
        0,

        /* Service UUID.                                                  */
        { 0x6A, 0xFE, 0x79, 0xCE, 0x6C, 0x52, 0x10, 0x14, 0x8B, 0x7A, 0x95, 0x02, 0x01, 0xB8, 0x65, 0xBC  },

        /* Service Handle Range.                                          */
        {0, 0},

        /* Number of Service Table Entries.                               */
        7,

        /* Service Attribute List.                                        */
        SrvTable4
    },
    {
        /* Service Flags.                                                 */
        SERVICE_TABLE_FLAGS_SECONDARY_SERVICE | SERVICE_TABLE_FLAGS_USE_PERSISTENT_UID,

        /* Service ID.                                                    */
        0,

        /* PersistentUID.                                                 */
        0,

        /* Service UUID.                                                  */
        { 0x6A, 0xFE, 0x8C, 0xB1, 0x6C, 0x52, 0x10, 0x14, 0x8B, 0x7A, 0x95, 0x02, 0x01, 0xB8, 0x65, 0xBC  },

        /* Service Handle Range.                                          */
        {0, 0},

        /* Number of Service Table Entries.                               */
        9,

        /* Service Attribute List.                                        */
        SrvTable5
    },
    {
        /* Service Flags.                                                 */
        SERVICE_TABLE_FLAGS_USE_PERSISTENT_UID,

        /* Service ID.                                                    */
        0,

        /* PersistentUID.                                                 */
        0,

        /* Service UUID.                                                  */
        { 0x6A, 0xFE, 0x92, 0x3A, 0x6C, 0x52, 0x10, 0x14, 0x8B, 0x7A, 0x95, 0x02, 0x01, 0xB8, 0x65, 0xBC  },

        /* Service Handle Range.                                          */
        {0, 0},

        /* Number of Service Table Entries.                               */
        3,

        /* Service Attribute List.                                        */
        SrvTable6
    }
};


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
#ifdef Linux_x86_64
                    sleep(1);
#else
                    BTPS_Delay(500);
#endif
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
    unsigned int ret_val = BleApi::BleApi::NO_ERROR;

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
            ret_val = BleApi::NO_ERROR;

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

                    ret_val = BleApi::NO_ERROR;
                }
                else
                {
                    /* Be sure we exit out of the Loop.                      */
                    StringLength = 0;

                    ret_val = BleApi::TO_MANY_PARAMS;
                }
            }

            /* Set the number of parameters in the User Command to the     */
            /* number of found parameters                                  */
            TempCommand->Parameters.NumberofParameters = Count;
        }
        else
        {
            /* No command was specified                                    */
            ret_val = BleApi::PARSER_ERROR;
        }
    }
    else
    {
        /* One or more of the passed parameters appear to be invalid.     */
        ret_val = BleApi::INVALID_PARAMETERS_ERROR;
    }

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
    ret_val = BleApi::INVALID_COMMAND_ERROR;

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
                if(memcmp(TempCommand->Command, "INITIALIZE", strlen("INITIALIZE")) == 0)
                {
                    TempCommand->Parameters.Params[1].intParam = NUMBER_OF_SERVICES_MAX;
                    TempCommand->Parameters.Params[2].strParam =(char*) &ServiceTable;
                }
                /* The command was found in the table so call the command.  */
                if(!((*CommandFunction)(&TempCommand->Parameters)))
                {
                    /* Return success to the caller.                         */
                    ret_val = BleApi::NO_ERROR;
                }
                else
                    ret_val = BleApi::FUNCTION_ERROR;
            }
        }
        else
        {
            /* The command entered is exit, set return value to BleApi::EXIT_CODE  */
            /* and return.                                                 */
            ret_val = BleApi::EXIT_CODE;
        }
    }
    else
        ret_val = BleApi::INVALID_PARAMETERS_ERROR;

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
    /* Note the order they are listed here *MUST* match the order in     */
    /* which then are added to the Command Table.                        */
    printf("******************************************************************\r\n");
    printf("* Command Options: 1) Initialize                                 *\r\n");
    printf("*                  2) Cleanup                                    *\r\n");
    printf("*                  3) QueryDebugZoneMask                         *\r\n");
    printf("*                  4) SetDebugZoneMask                           *\r\n");
    printf("*                  5) SetDebugZoneMaskPID                        *\r\n");
    printf("*                  6) ShutdownService                            *\r\n");
    printf("*                  7) RegisterEventCallback,                     *\r\n");
    printf("*                  8) UnRegisterEventCallback,                   *\r\n");
    printf("*                  9) QueryDevicePower                           *\r\n");
    printf("*                  10)SetDevicePower                             *\r\n");
    printf("*                  11)QueryLocalDeviceProperties                 *\r\n");
    printf("*                  12)SetLocalDeviceName                         *\r\n");
    printf("*                  13)SetLocalClassOfDevice                      *\r\n");
    printf("*                  14)SetDiscoverable                            *\r\n");
    printf("*                  15)SetConnectable                             *\r\n");
    printf("*                  16)SetPairable                                *\r\n");
    printf("*                  17)StartDeviceDiscovery                       *\r\n");
    printf("*                  18)StopDeviceDiscovery                        *\r\n");
    printf("*                  19)QueryRemoteDeviceList                      *\r\n");
    printf("*                  20)QueryRemoteDeviceProperties                *\r\n");
    printf("*                  21)AddRemoteDevice                            *\r\n");
    printf("*                  22)DeleteRemoteDevice                         *\r\n");
    printf("*                  23)UpdateRemoteDeviceAppData                  *\r\n");
    printf("*                  24)DeleteRemoteDevices                        *\r\n");
    printf("*                  25)PairWithRemoteDevice                       *\r\n");
    printf("*                  26)CancelPairWithRemoteDevice                 *\r\n");
    printf("*                  27)UnPairRemoteDevice                         *\r\n");
    printf("*                  28)QueryRemoteDeviceServices                  *\r\n");
    printf("*                  29)QueryRemoteDeviceServiceSupported          *\r\n");
    printf("*                  30)QueryRemoteDevicesForService               *\r\n");
    printf("*                  31)QueryRemoteDeviceServiceClasses            *\r\n");
    printf("*                  32)AuthenticateRemoteDevice                   *\r\n");
    printf("*                  33)EncryptRemoteDevice                        *\r\n");
    printf("*                  34)ConnectWithRemoteDevice                    *\r\n");
    printf("*                  35)DisconnectRemoteDevice                     *\r\n");
    printf("*                  36)SetRemoteDeviceLinkActive                  *\r\n");
    printf("*                  37)CreateSDPRecord                            *\r\n");
    printf("*                  38)DeleteSDPRecord                            *\r\n");
    printf("*                  39)AddSDPAttribute                            *\r\n");
    printf("*                  40)DeleteSDPAttribute                         *\r\n");
    printf("*                  41)EnableBluetoothDebug                       *\r\n");
    printf("*                  42)RegisterAuthentication                     *\r\n");
    printf("*                  43)UnRegisterAuthentication                   *\r\n");
    printf("*                  44)PINCodeResponse                            *\r\n");
    printf("*                  45)PassKeyResponse                            *\r\n");
    printf("*                  46)UserConfirmationResponse                   *\r\n");
    printf("*                  47)ChangeSimplePairingParameters              *\r\n");
    printf("*                  48)RegisterGATTCallback                       *\r\n");
    printf("*                  49)UnRegisterGATTCallback                     *\r\n");
    printf("*                  50)QueryGATTConnections                       *\r\n");
    printf("*                  51)SetLocalDeviceAppearance                   *\r\n");
    printf("*                  52)StartAdvertising                           *\r\n");
    printf("*                  53)StopAdvertising                            *\r\n");
    printf("*                  54)RegisterService                            *\r\n");
    printf("*                  55)UnRegisterService                          *\r\n");
    printf("*                  56)IndicateCharacteristic                     *\r\n");
    printf("*                  57)NotifyCharacteristic                       *\r\n");
    printf("*                  58)ListCharacteristics                        *\r\n");
    printf("*                  59)ListDescriptors                            *\r\n");
    printf("*                  60)QueryPublishedServices                     *\r\n");
    printf("*                  61)SetAdvertisingInterval                     *\r\n");
    printf("*                  62)SetAndUpdateConnectionAndScanBLEParameters *\r\n");
    printf("*                  63)SetAuthenticatedPayloadTimeout             *\r\n");
    printf("*                  64)QueryAuthenticatedPayloadTimeout           *\r\n");
    printf("*                  65)EnableSCOnly                               *\r\n");
    printf("*                  66)RegenerateP256LocalKeys                    *\r\n");
    printf("*                  67)OOBGenerateParameters                      *\r\n");
    printf("*                  68)ChangeLEPairingParameters                  *\r\n");
    printf("*                  Help, Quit.                                   *\r\n");
    printf("******************************************************************\r\n");

    return(0);
}

/*****************************************************************************
 * SECTION of server Init and command line user interface for tests purposes
 *****************************************************************************/

static void GATM_Init(void)
{
    struct sigaction SignalAction;

    static int bInited = FALSE;
    if (bInited)
        return;
    bInited = TRUE;

    /* First let's make sure that we start on new line.                  */
    printf("\r\n");

    /* Next display the available commands.                              */
    DisplayHelp(NULL);

    ClearCommands();
    #define RING_BLE_DEF_ADD_COMMAND
    #include "gatt_srv_defs.h"

    AddCommand((char*) "HELP", DisplayHelp);

    /* Configure the SIGTTIN signal so that this application can run in  */
    /* the background.                                                   */

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
}
/* This function is responsible for taking the input from the user   */
/* and dispatching the appropriate Command Function.  First, this    */
/* function retrieves a String of user input, parses the user input  */
/* into Command and Parameters, and finally executes the Command or  */
/* Displays an Error Message if the input is not a valid Command.    */
static void UserInterface(void)
{
    UserCommand_t TempCommand;
    int  Result = !BleApi::EXIT_CODE;
    char UserInput[MAX_COMMAND_LENGTH];

    GATM_Init();

    /* This is the main loop of the program.  It gets user input from the*/
    /* command window, make a call to the command parser, and command    */
    /* interpreter.  After the function has been ran it then check the   */
    /* return value and displays an error message when appropriate. If   */
    /* the result returned is ever the BleApi::EXIT_CODE the loop will exit      */
    /* leading the the exit of the program.                              */
    while(Result != BleApi::EXIT_CODE)
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

                /* The string input by the user contains a value, now run   */
                /* the string through the Command Parser.                   */
                if(CommandParser(&TempCommand, UserInput) >= 0)
                {
                    /* The Command was successfully parsed, run the Command. */
                    Result = CommandInterpreter(&TempCommand);

                    switch(Result)
                    {
                    case BleApi::INVALID_COMMAND_ERROR:
                        printf("Invalid Command.\r\n");
                        break;
                    case BleApi::FUNCTION_ERROR:
                        printf("Function Error.\r\n");
                        break;
                    }
                }
                else
                    printf("Invalid Input.\r\n");
            }
        }
        else
            Result = BleApi::EXIT_CODE;
    }
}

static int ValidateAndExecCommand(char *UserInput)
{
    int  Result = !BleApi::EXIT_CODE;
    UserCommand_t TempCommand;

    /* The string input by the user contains a value, now run   */
    /* the string through the Command Parser.                   */
    if(CommandParser(&TempCommand, UserInput) >= 0)
    {
        /* The Command was successfully parsed, run the Command. */
        Result = CommandInterpreter(&TempCommand);

        switch(Result)
        {
        case BleApi::INVALID_COMMAND_ERROR:
            printf("Invalid Command.\r\n");
            break;
        case BleApi::FUNCTION_ERROR:
            printf("Function Error.\r\n");
            break;
        default:
            printf("Command executed with res=%d\n", Result);
        }
    }
    else
    {
        printf("Invalid Input.\r\n");
    }
    return Result;
}

///
/// \brief gatt_server_start
/// \param arguments - expected NULL or "--autoinit"
/// \return errno
///
extern "C" int gatt_server_start(const char* arguments)
{
    static const StartUpCommands_t gStartUpCommandList[] =
    {
        {"Initialize 1",                        2},
        {"SetDevicePower 1",                    2},
        {"SetLocalDeviceName Ampak-WL18",       1},
        {"SetLocalClassOfDevice  0x280430",     1},
        {"SetAdvertisingInterval 2000 3000",    1},
        {"RegisterGATTCallback",                1},
        {"RegisterService 0",                   1},
        {"RegisterService 1",                   1},
        {"ListCharacteristics",                 1},
        {"ListDescriptors",                     1},
        {"RegisterAuthentication",              1},
        {"ChangeSimplePairingParameters 0 0",   1},
        {"StartAdvertising 118 300",            1},
        {"QueryLocalDeviceProperties",          1},
    };

#ifndef Linux_x86_64
    gGattSrvInst = (GattSrv*) GattSrv::getInstance();
    if (gGattSrvInst == NULL)
    {
        printf("error creating gGattSrvInst!! Abort. \n");
        return -666;
    }
#endif

    GATM_Init();

    if (arguments != NULL)
    {
        if (!strcmp(arguments, "--autoinit"))
        {
            char UserInput[MAX_COMMAND_LENGTH];
            int nCmds = sizeof(gStartUpCommandList)/sizeof(StartUpCommands_t);

            printf("---starting in a sec...---\n");
            sleep(2);

            int nRepeat = 2;

            for (int i = 0; i < nCmds; i++)
            {
                sprintf(UserInput, "%s", gStartUpCommandList[i].mCmd);
                printf("cmd[%d]: [%s]\n", i+1, UserInput);

                int ret = ValidateAndExecCommand(UserInput);
                printf("cmd[%d]: [%s] ret = %d\n", i+1, UserInput, ret);
                sleep(gStartUpCommandList[i].mDelay);

                if (ret != 0)
                {
                    if (nRepeat-- > 0)
                    {
                        // try one more time same operation
                        i--;
                    }
                    else
                    {
                        // abort sequence
                        printf("cmd[%d]: %s failed - Abort the series.\n", i+1, UserInput);
                        break;
                    }
                }
            }
        }
    }

    UserInterface();

    return 0;
}

