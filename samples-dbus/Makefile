
CFLAGS_DBUS = $(shell pkg-config --cflags --libs dbus-1)
CFLAGS_DBUS_GLIB = $(shell pkg-config --cflags --libs dbus-glib-1)
CFLAGS_GIO  = $(shell pkg-config --cflags --libs gio-2.0)

CFLAGS = -g -Werror -I/usr/include/dbus-1.0 -I/usr/lib/x86_64-linux-gnu/dbus-1.0/include


all: dbus-server dbus-client dbblue

dbus-server: dbus-server.c
	gcc $< -o $@ $(CFLAGS) $(CFLAGS_DBUS) $(CFLAGS_DBUS_GLIB)

dbus-client: dbus-client.c
	gcc $< -o $@ $(CFLAGS) $(CFLAGS_GIO)

dbblue:	dbblue.c
	gcc $< -o $@ $(CFLAGS)  -ldbus-1 $(CFLAGS_GIO)

clean:
	rm -f dbus-server
	rm -f dbus-client
	rm -f dbblue


.PHONY: all clean
