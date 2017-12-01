/* ----------------------------------------------------------------------------------------
 *  Note: When building Ring App, the LDFLAGS include -lRingIPC for real Bot_Notifier
 * implementation via IPC, for example:
 * LOCAL_LDFLAGS+:= -L$(AMB_BOARD_OUT)/ring/ipc -lRingIPC
 *
 * For standalone test the "faked" Bot_Notify is defined below in this file
 * which redirects all levels of debug to stdout with standard "printf"
 *
 * Since the RingBle.a is static lib, all Bot_Notify calls in RingBle/GattAPI calsses will
 * be resolved depending on what it is linked with - either libRingIPC.so or test_stubs.cpp
 * ----------------------------------------------------------------------------------------*/
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <string.h>
#include <stdarg.h>

#include "Bot_Notifier.h"
#ifdef Linux_x86_64
extern "C" {
#include "SS1BTPM.h"          /* BTPM Application Programming Interface.      */
#include "BTPMMAPI.h"          /* BTPM Application Programming Interface.      */
} // #ifdef __cplusplus
#endif

#define BUF_LEN 1024
#define CASE_FORM_TYPE(__type) case BOT_NOTIFY_##__type##_T: sprintf(typeString, #__type ":"); break;

BOT_RET_CODE_T Bot_Notify(BOT_NOTIFY_TYPE_T type, uint32_t line, char const *fnName, char const *fileName, char const *format, ...)
{
    (void) line;
    (void) fnName;
    (void) fileName;

    va_list argPtr;
    char typeString[10];
    char argBuffer[BUF_LEN];

    switch (type) {
        CASE_FORM_TYPE(TRACE)
        CASE_FORM_TYPE(DEBUG)
        CASE_FORM_TYPE(INFO)
        CASE_FORM_TYPE(WARNING)
        CASE_FORM_TYPE(ERROR)
        CASE_FORM_TYPE(SILENT)
            default:
            sprintf(typeString, "-?-:");
    }
    va_start(argPtr, format);
    vsnprintf(argBuffer, BUF_LEN, format, argPtr);
    va_end(argPtr);

    printf("%-9s %s%s", typeString, argBuffer, argBuffer[strlen(argBuffer)-1]=='\n'?"":"\r\n");
    fflush(stdout);

    return BOT_OK;
}

#ifdef Linux_x86_64
BTPSAPI_DECLARATION void *BTPSAPI BTPS_AllocateMemory(unsigned long MemorySize)
{
    return malloc(MemorySize);
}

BTPSAPI_DECLARATION void BTPSAPI BTPS_FreeMemory(void *MemoryPointer)
{
    return free(MemoryPointer);
}
#endif
