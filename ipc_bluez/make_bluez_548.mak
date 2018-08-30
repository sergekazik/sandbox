BLUEZ_TOOLS_SUPPORT = 1

# Makefile for Bluez 5.48 library used by ring

BUILD_PATH=$(shell pwd)

CC = gcc
RM = rm -f

################################################################################
## Third party path/defs/stabs
################################################################################
BLUEZ_548  = ./bluez-5.48

CFLAGS =    -fPIC -Wall -Wextra -O2 -g -std=c++14	\
	    -Wno-format-zero-length -Wno-write-striings	\
	    -Wno-missing-field-initializers		\
	    -Wno-unused-result				\
	    -I$(BLUEZ_548) -I./BCM43			\
	    -DBLUEZ_TOOLS_SUPPORT=1

CXXFLAGS = $(CFLAGS)

LDFLAGS_LIB = -shared -lbluetooth -lpthread
TARGET_BLZ = libBluez548.so

SRCS_BLZ =  $(BLUEZ_548)/lib/bluetooth.c \
	    $(BLUEZ_548)/lib/hci.c	\
	    $(BLUEZ_548)/lib/sdp.c	\
	    $(BLUEZ_548)/lib/uuid.c	\
	    $(BLUEZ_548)/src/uuid-helper.c	\
	    $(BLUEZ_548)/src/shared/gatt-db.c	\
	    $(BLUEZ_548)/src/shared/att.c	\
	    $(BLUEZ_548)/src/shared/btsnoop.c   \
	    $(BLUEZ_548)/src/shared/ecc.c	\
	    $(BLUEZ_548)/src/shared/gap.c	\
	    $(BLUEZ_548)/src/shared/gatt-helpers.c	\
	    $(BLUEZ_548)/src/shared/gatt-server.c	\
	    $(BLUEZ_548)/src/shared/hfp.c	\
	    $(BLUEZ_548)/src/shared/io-mainloop.c	\
	    $(BLUEZ_548)/src/shared/mainloop.c  \
	    $(BLUEZ_548)/src/shared/mgmt.c	\
	    $(BLUEZ_548)/src/shared/pcap.c	\
	    $(BLUEZ_548)/src/shared/queue.c	\
	    $(BLUEZ_548)/src/shared/ringbuf.c   \
	    $(BLUEZ_548)/src/shared/timeout-mainloop.c  \
	    $(BLUEZ_548)/src/shared/util.c	\
	    $(BLUEZ_548)/src/shared/crypto.c

OBJ_BLZ =   $(BLUEZ_548)/lib/bluetooth.o \
	    $(BLUEZ_548)/lib/hci.o	\
	    $(BLUEZ_548)/lib/sdp.o	\
	    $(BLUEZ_548)/lib/uuid.o	\
	    $(BLUEZ_548)/src/uuid-helper.o	\
	    $(BLUEZ_548)/src/shared/gatt-db.o	\
	    $(BLUEZ_548)/src/shared/att.o	\
	    $(BLUEZ_548)/src/shared/btsnoop.o   \
	    $(BLUEZ_548)/src/shared/ecc.o	\
	    $(BLUEZ_548)/src/shared/gap.o	\
	    $(BLUEZ_548)/src/shared/gatt-helpers.o	\
	    $(BLUEZ_548)/src/shared/gatt-server.o	\
	    $(BLUEZ_548)/src/shared/hfp.o	\
	    $(BLUEZ_548)/src/shared/io-mainloop.o	\
	    $(BLUEZ_548)/src/shared/mainloop.o  \
	    $(BLUEZ_548)/src/shared/mgmt.o	\
	    $(BLUEZ_548)/src/shared/pcap.o	\
	    $(BLUEZ_548)/src/shared/queue.o	\
	    $(BLUEZ_548)/src/shared/ringbuf.o   \
	    $(BLUEZ_548)/src/shared/timeout-mainloop.o  \
	    $(BLUEZ_548)/src/shared/util.o	\
	    $(BLUEZ_548)/src/shared/crypto.o

.PHONY: all
all: ${TARGET_BLZ}

$(TARGET_BLZ): $(OBJ_BLZ)
	$(CC) $(OBJ_BLZ) $(LDFLAGS_LIB) -o $@

.PHONY: clean
clean_obj:
	-${RM} ${OBJS_BLZ}

clean:
	-${RM} ${TARGET_BLZ} ${OBJ_BLZ}

