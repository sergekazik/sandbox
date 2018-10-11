 #
 # Copyright 2018 Amazon.com, Inc. and its affiliates. All Rights Reserved.
 #
 # This program is free software; you can redistribute it and/or
 # modify it under the terms of the GNU General Public License
 # as published by the Free Software Foundation; version 2
 # of the License.
 #
 # This program is distributed in the hope that it will be useful,
 # but WITHOUT ANY WARRANTY; without even the implied warranty of
 # MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 # GNU General Public License for more details.
 #
 # You should have received a copy of the GNU General Public License
 # along with this program. If not, see <http://www.gnu.org/licenses/gpl-2.0.html>
 #

BUILD_PATH=$(shell pwd)

CC = gcc
RM = rm -f

# set path to Bluez directory
BLUEZ_548  = ./bluez-5.48

CFLAGS =    -fPIC -Wall -Wextra -O0 -g -std=c++11 -pthread \
	    -Wunused -Wunreachable-code	-Wunused-function -Wno-permissive \
	    -Wno-format-zero-length -Wno-write-strings -Wno-missing-field-initializers -Wno-unused-result  \
	    -I. -DDEBUG_ENABLED -I$(BLUEZ_548) -DTEST_HCI1_DEVICE=0 \

##	    -DUUID_BASE_16

CXXFLAGS = $(CFLAGS)
################################################################################
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

bluez: ${TARGET_BLZ}

################################################################################
LDFLAGS = -lpthread -lstdc++

TARGET_SERVER = iserver
TARGET_CLIENT = iclient

SRCS_SERVER = ipcluez.cpp gatt_api.cpp
SRCS_CLIENT = ipclint.cpp

INCS_COMMON  = gatt_api.h icommon.h

OBJS_COMMON  = icommon.o
OBJS_SERVER = ipcluez.o gatt_api.o $(OBJS_COMMON)
OBJS_CLIENT = ipclint.o  $(OBJS_COMMON)

.PHONY: all
all: $(TARGET_BLZ) ${TARGET_SERVER} $(TARGET_CLIENT)

$(TARGET_SERVER): $(OBJS_SERVER) $(INCS_COMMON)
	$(CC) $(OBJS_SERVER) $(LDFLAGS) -L./ -lbluez -o $@

$(TARGET_CLIENT): $(OBJS_CLIENT) $(INCS_COMMON)
	$(CC) $(OBJS_CLIENT) $(LDFLAGS) -o $@

$(TARGET_BLZ): $(OBJ_BLZ)
	$(CC) $(OBJ_BLZ) $(LDFLAGS_LIB) -o $@

.PHONY: clean
clean:
	-${RM} ${TARGET_SERVER} ${TARGET_CLIENT} ${OBJS_SERVER} ${OBJS_CLIENT} *.o *.d

