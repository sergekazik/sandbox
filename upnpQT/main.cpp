#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <unistd.h>

#include <upnp/upnp.h>
#include "../libupnp-1.3.1/upnp/sample/common/sample_util.h"

extern "C" {
    #include "../libupnp-1.3.1/upnp/sample/tvctrlpt/upnp_tv_ctrlpt.h"
    #include "../libupnp-1.3.1/ixml/src/inc/ixmlparser.h"
}

int CallbackFxn( Upnp_EventType EventType, void* Event, void* Cookie );
int gbTimeout = FALSE;

int main()
{
    int rc = 0;

    const char *deviceType = "ssdp:all"; //"upnp:rootdevice";

    rc = UpnpInit(NULL /*"10.190.85.14"*/, 0 /*any port*/);
    if (rc != UPNP_E_SUCCESS)
    {
        printf("failed with error %d\n", rc);
        goto quit;
    }

    // UpnpClient_Handle ctrlpt_handle;
    rc = UpnpRegisterClient( CallbackFxn, &ctrlpt_handle, &ctrlpt_handle );
    if (UPNP_E_SUCCESS != rc)
    {
        printf("Error registering CP: %d\n", rc);
        goto quit;
    }

    gbTimeout = FALSE;
    // search for something
    rc = UpnpSearchAsync( ctrlpt_handle, 5, deviceType, NULL );
    if (UPNP_E_SUCCESS != rc) {
        SampleUtil_Print("Error sending search request = %d", rc );
        goto quit;
    }

    while (!gbTimeout)
    {
        sleep(1);
    }

    printf("------------------------------------------\n");
    for (int ch = 'l'; ch != 'q'; ch = getchar())
    {
        switch (ch) {
        case 10: // LF (Line feed, 0x0A, 10 in decimal)
        case 13: // CR (Carriage return, 0x0D, 13 in decimal)
            break;

        case 'l':
            TvCtrlPointPrintList(TRUE);
            printf("\n");
        default:
            printf("waiting for client notifications.....\n");
            printf("\t\"q\"\t- to quit\n");
            printf("\t\"l\"\t- to print device list\n");
            printf(">");
            break;
        }
    }


    // release all
quit:
    printf("finished with rc = %d\n", rc);

    TvCtrlPointStop();

    return rc;
}

