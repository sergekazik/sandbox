TEMPLATE = app
CONFIG += console
CONFIG -= app_bundle
CONFIG -= qt

SOURCES += main.cpp \
    callback.cpp \
    ../libupnp-1.3.1/ixml/src/ixmlparser.c \
    ../libupnp-1.3.1/ixml/src/node.c \
    ../libupnp-1.3.1/upnp/sample/tvctrlpt/upnp_tv_ctrlpt.c \
    ../libupnp-1.3.1/upnp/sample/common/sample_util.c

include(deployment.pri)
qtcAddDeployment()

DISTFILES +=

HEADERS += \
    ../libupnp-1.3.1/upnp/inc/upnp.h \
    ../libupnp-1.3.1/upnp/inc/upnpconfig.h \
    ../libupnp-1.3.1/upnp/inc/upnpconfig.h.in \
    ../libupnp-1.3.1/upnp/inc/upnpdebug.h \
    ../libupnp-1.3.1/upnp/inc/upnptools.h \
    ../libupnp-1.3.1/upnp/src/inc/client_table.h \
    ../libupnp-1.3.1/upnp/src/inc/config.h \
    ../libupnp-1.3.1/upnp/src/inc/gena.h \
    ../libupnp-1.3.1/upnp/src/inc/gena_ctrlpt.h \
    ../libupnp-1.3.1/upnp/src/inc/gena_device.h \
    ../libupnp-1.3.1/upnp/src/inc/global.h \
    ../libupnp-1.3.1/upnp/src/inc/gmtdate.h \
    ../libupnp-1.3.1/upnp/src/inc/http_client.h \
    ../libupnp-1.3.1/upnp/src/inc/httpparser.h \
    ../libupnp-1.3.1/upnp/src/inc/httpreadwrite.h \
    ../libupnp-1.3.1/upnp/src/inc/md5.h \
    ../libupnp-1.3.1/upnp/src/inc/membuffer.h \
    ../libupnp-1.3.1/upnp/src/inc/miniserver.h \
    ../libupnp-1.3.1/upnp/src/inc/netall.h \
    ../libupnp-1.3.1/upnp/src/inc/parsetools.h \
    ../libupnp-1.3.1/upnp/src/inc/server.h \
    ../libupnp-1.3.1/upnp/src/inc/service_table.h \
    ../libupnp-1.3.1/upnp/src/inc/soaplib.h \
    ../libupnp-1.3.1/upnp/src/inc/sock.h \
    ../libupnp-1.3.1/upnp/src/inc/ssdplib.h \
    ../libupnp-1.3.1/upnp/src/inc/statcodes.h \
    ../libupnp-1.3.1/upnp/src/inc/statuscodes.h \
    ../libupnp-1.3.1/upnp/src/inc/strintmap.h \
    ../libupnp-1.3.1/upnp/src/inc/sysdep.h \
    ../libupnp-1.3.1/upnp/src/inc/unixutil.h \
    ../libupnp-1.3.1/upnp/src/inc/upnpapi.h \
    ../libupnp-1.3.1/upnp/src/inc/upnpclosesocket.h \
    ../libupnp-1.3.1/upnp/src/inc/upnp_timeout.h \
    ../libupnp-1.3.1/upnp/src/inc/uri.h \
    ../libupnp-1.3.1/upnp/src/inc/urlconfig.h \
    ../libupnp-1.3.1/upnp/src/inc/util.h \
    ../libupnp-1.3.1/upnp/src/inc/utilall.h \
    ../libupnp-1.3.1/upnp/src/inc/uuid.h \
    ../libupnp-1.3.1/upnp/src/inc/webserver.h \
    ../libupnp-1.3.1/ixml/src/inc/ixmlmembuf.h \
    ../libupnp-1.3.1/ixml/src/inc/ixmlparser.h \
    ../libupnp-1.3.1/upnp/sample/tvctrlpt/upnp_tv_ctrlpt.h \
    ../libupnp-1.3.1/upnp/sample/common/sample_util.h

unix:!macx: LIBS += -lupnp -lixml -lpthread

QMAKE_CXXFLAGS += -I../libupnp-1.3.1/upnp/sample/common \
		  -I../libupnp-1.3.1/upnp/inc	\
		  -I../libupnp-1.3.1/threadutil/inc/ \
		  -I../libupnp-1.3.1/ixml/inc/ \
		  -I../libupnp-1.3.1/ixml/src/inc/ \
		  -I../libupnp-1.3.1/upnp/sample/common/ \
		  -I../libupnp-1.3.1/upnp/sample/tvctrlpt/	\

QMAKE_CFLAGS += -I../libupnp-1.3.1/upnp/sample/common \
		  -I../libupnp-1.3.1/upnp/inc	\
		  -I../libupnp-1.3.1/threadutil/inc/ \
		  -I../libupnp-1.3.1/ixml/inc/ \
		  -I../libupnp-1.3.1/ixml/src/inc/ \
		  -I../libupnp-1.3.1/upnp/sample/common/ \
		  -I../libupnp-1.3.1/upnp/sample/tvctrlpt/	\


