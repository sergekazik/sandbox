#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <unistd.h>

#include <upnp/upnp.h>
#include "sample_util.h"

extern "C" {
#include "../libupnp-1.3.1/upnp/sample/tvctrlpt/upnp_tv_ctrlpt.h"
#include "../libupnp-1.3.1/ixml/src/inc/ixmlparser.h"
}

int CallbackFxn( Upnp_EventType EventType, void* Event, void* Cookie );


//extern "C" int SampleUtil_Print(const char *fmt, ... )
//{
//    #define BUF_LEN 1024
//    va_list ap;
//    char buf[BUF_LEN];
//    int rc;

//    va_start( ap, fmt );
//    rc = vsnprintf( buf, BUF_LEN, fmt, ap );
//    va_end( ap );

////    ithread_mutex_lock( &display_mutex );
////    if( gPrintFun )
////        gPrintFun( buf );
////    ithread_mutex_unlock( &display_mutex );

//    printf("%s\n", buf);
//    return rc;
//}

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

    // search for something
    rc = UpnpSearchAsync( ctrlpt_handle, 5, deviceType, NULL );
    if (UPNP_E_SUCCESS != rc) {
        SampleUtil_Print("Error sending search request = %d", rc );
        goto quit;
    }
    sleep(5);

    // release all
quit:
    printf("finished with rc = %d\n", rc);

    TvCtrlPointStop();

    return rc;
}

