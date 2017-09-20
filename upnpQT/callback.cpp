/********************************************************************************
 * CallbackFxn
 *
 * Description:
 *       The callback handler registered with the SDK while registering
 *       the control point.  Detects the type of callback, and passes the
 *       request on to the appropriate function.
 *
 * Parameters:
 *   EventType -- The type of callback event
 *   Event -- Data structure containing event data
 *   Cookie -- Optional data specified during callback registration
 *
 ********************************************************************************/
#define CALLBACKFXN_TEST 1

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <upnp/upnp.h>

extern "C" {
    #include "../libupnp-1.3.1/upnp/sample/tvctrlpt/upnp_tv_ctrlpt.h"
    #include "../libupnp-1.3.1/ixml/src/inc/ixmlparser.h"
}

static const char* gUpnp_EventType_Str[] =
{
  "UPNP_CONTROL_ACTION_REQUEST",
  "UPNP_CONTROL_ACTION_COMPLETE",
  "UPNP_CONTROL_GET_VAR_REQUEST",
  "UPNP_CONTROL_GET_VAR_COMPLETE",
  "UPNP_DISCOVERY_ADVERTISEMENT_ALIVE",
  "UPNP_DISCOVERY_ADVERTISEMENT_BYEBYE",
  "UPNP_DISCOVERY_SEARCH_RESULT",
  "UPNP_DISCOVERY_SEARCH_TIMEOUT",
  "UPNP_EVENT_SUBSCRIPTION_REQUEST",
  "UPNP_EVENT_RECEIVED",
  "UPNP_EVENT_RENEWAL_COMPLETE",
  "UPNP_EVENT_SUBSCRIBE_COMPLETE",
  "UPNP_EVENT_UNSUBSCRIBE_COMPLETE",
  "UPNP_EVENT_AUTORENEWAL_FAILED",
  "UPNP_EVENT_SUBSCRIPTION_EXPIRED"
};

#define GET_EVENT_TYPE_STRING(_ev) (_ev <=  UPNP_EVENT_SUBSCRIPTION_EXPIRED ? gUpnp_EventType_Str[_ev] : "unknown")
extern int gbTimeout;

int default_handling( Upnp_EventType EventType, void *Event, void *Cookie )
{
    printf("Callback got event type %d [%s]\n", EventType, GET_EVENT_TYPE_STRING(EventType));

    (void) Cookie;


    switch ( EventType ) {
            /*
               SSDP Stuff
             */
        case UPNP_DISCOVERY_ADVERTISEMENT_ALIVE:
        case UPNP_DISCOVERY_SEARCH_RESULT:
            {
                struct Upnp_Discovery *d_event =
                    ( struct Upnp_Discovery * )Event;
                IXML_Document *DescDoc = NULL;
                int ret;

                if( d_event->ErrCode != UPNP_E_SUCCESS ) {
                    SampleUtil_Print( "Error in Discovery Callback -- %d",
                                      d_event->ErrCode );
                }

                if( ( ret =
                      UpnpDownloadXmlDoc( d_event->Location,
                                          &DescDoc ) ) !=
                    UPNP_E_SUCCESS ) {
                    SampleUtil_Print
                        ( "Error obtaining device description from %s -- error = %d",
                          d_event->Location, ret );
                } else {
                    TvCtrlPointAddDevice( DescDoc, d_event->Location,
                                          d_event->Expires );
                }

                if( DescDoc )
                    ixmlDocument_free( DescDoc );

                TvCtrlPointPrintList(FALSE);
                break;
            }

        case UPNP_DISCOVERY_SEARCH_TIMEOUT:
            /*
               Nothing to do here...
             */
            break;

        case UPNP_DISCOVERY_ADVERTISEMENT_BYEBYE:
            {
                struct Upnp_Discovery *d_event =
                    ( struct Upnp_Discovery * )Event;

                if( d_event->ErrCode != UPNP_E_SUCCESS ) {
                    SampleUtil_Print
                        ( "Error in Discovery ByeBye Callback -- %d",
                          d_event->ErrCode );
                }

                SampleUtil_Print( "Received ByeBye for Device: %s",
                                  d_event->DeviceId );
                TvCtrlPointRemoveDevice( d_event->DeviceId );

                SampleUtil_Print( "After byebye:" );
                TvCtrlPointPrintList(FALSE);

                break;
            }

            /*
               SOAP Stuff
             */
        case UPNP_CONTROL_ACTION_COMPLETE:
            {
                struct Upnp_Action_Complete *a_event =
                    ( struct Upnp_Action_Complete * )Event;

                if( a_event->ErrCode != UPNP_E_SUCCESS ) {
                    SampleUtil_Print
                        ( "Error in  Action Complete Callback -- %d",
                          a_event->ErrCode );
                }

                /*
                   No need for any processing here, just print out results.  Service state
                   table updates are handled by events.
                 */

                break;
            }

        case UPNP_CONTROL_GET_VAR_COMPLETE:
            {
                struct Upnp_State_Var_Complete *sv_event =
                    ( struct Upnp_State_Var_Complete * )Event;

                if( sv_event->ErrCode != UPNP_E_SUCCESS ) {
                    SampleUtil_Print
                        ( "Error in Get Var Complete Callback -- %d",
                          sv_event->ErrCode );
                } else {
                    TvCtrlPointHandleGetVar( sv_event->CtrlUrl,
                                             sv_event->StateVarName,
                                             sv_event->CurrentVal );
                }

                break;
            }

            /*
               GENA Stuff
             */
        case UPNP_EVENT_RECEIVED:
            {
                struct Upnp_Event *e_event = ( struct Upnp_Event * )Event;

                TvCtrlPointHandleEvent( e_event->Sid, e_event->EventKey,
                                        e_event->ChangedVariables );
                break;
            }

        case UPNP_EVENT_SUBSCRIBE_COMPLETE:
        case UPNP_EVENT_UNSUBSCRIBE_COMPLETE:
        case UPNP_EVENT_RENEWAL_COMPLETE:
            {
                struct Upnp_Event_Subscribe *es_event =
                    ( struct Upnp_Event_Subscribe * )Event;

                if( es_event->ErrCode != UPNP_E_SUCCESS ) {
                    SampleUtil_Print
                        ( "Error in Event Subscribe Callback -- %d",
                          es_event->ErrCode );
                } else {
                    TvCtrlPointHandleSubscribeUpdate( es_event->
                                                      PublisherUrl,
                                                      es_event->Sid,
                                                      es_event->TimeOut );
                }

                break;
            }

        case UPNP_EVENT_AUTORENEWAL_FAILED:
        case UPNP_EVENT_SUBSCRIPTION_EXPIRED:
            {
                int TimeOut = default_timeout;
                Upnp_SID newSID;
                int ret;

                struct Upnp_Event_Subscribe *es_event =
                    ( struct Upnp_Event_Subscribe * )Event;

                ret =
                    UpnpSubscribe( ctrlpt_handle, es_event->PublisherUrl,
                                   &TimeOut, newSID );

                if( ret == UPNP_E_SUCCESS ) {
                    SampleUtil_Print( "Subscribed to EventURL with SID=%s",
                                      newSID );
                    TvCtrlPointHandleSubscribeUpdate( es_event->
                                                      PublisherUrl, newSID,
                                                      TimeOut );
                } else {
                    SampleUtil_Print
                        ( "Error Subscribing to EventURL -- %d", ret );
                }
                break;
            }

            /*
               ignore these cases, since this is not a device
             */
        case UPNP_EVENT_SUBSCRIPTION_REQUEST:
        case UPNP_CONTROL_GET_VAR_REQUEST:
        case UPNP_CONTROL_ACTION_REQUEST:
            break;
    }
    return 0;
}

int CallbackFxn( Upnp_EventType EventType, void *Event, void *Cookie )
{
    (void) Cookie;
    int bNewDeviceFound = FALSE;

    switch ( EventType ) {
        case UPNP_DISCOVERY_ADVERTISEMENT_ALIVE:
        case UPNP_DISCOVERY_SEARCH_RESULT:
        {
            struct Upnp_Discovery *d_event = (
                        struct Upnp_Discovery * ) Event;
            IXML_Document *DescDoc=NULL;
            int ret;
            if ((ret = UpnpDownloadXmlDoc( d_event->Location, &DescDoc ))
                    != UPNP_E_SUCCESS) {
                /* ... */
            } else {
                bNewDeviceFound = TvCtrlPointAddDevice( DescDoc, d_event->Location, d_event->Expires );
            }

            if (bNewDeviceFound)
            {
                if (DescDoc)
                {
                    IXML_NodeList *list = NULL;
                    char tagname[] = "friendlyName";
                    ixmlNode_getElementsByTagName( &DescDoc->n, tagname, &list);

                    if (list != NULL)
                    {
                        if (list->nodeItem != NULL)
                            printf("found: %s | %s [%s]\n", list->nodeItem->nodeName, list->nodeItem->firstChild->nodeValue, d_event->Location);
                        ixmlNodeList_free(list);
                    }

                    ixmlDocument_free( DescDoc );
                }
                TvCtrlPointPrintList(FALSE);
            }

            break;
        }
        case UPNP_DISCOVERY_SEARCH_TIMEOUT:
            printf("Callback got event type %d [%s]\n", EventType, GET_EVENT_TYPE_STRING(EventType));
            gbTimeout = TRUE;
            break;

        case UPNP_DISCOVERY_ADVERTISEMENT_BYEBYE:
        {
            printf("Callback got event type %d [%s]\n", EventType, GET_EVENT_TYPE_STRING(EventType));
            struct Upnp_Discovery *d_event =
                    (struct Upnp_Discovery * ) Event;
                    TvCtrlPointRemoveDevice(d_event->DeviceId);
                    TvCtrlPointPrintList(FALSE);

            break;
        }
        default:
            default_handling( EventType, Event, Cookie);
            printf("Callback got event type %d [%s]\n", EventType, GET_EVENT_TYPE_STRING(EventType));
            break;
    }
    return 0;
}
