RING_NAME = Linux_x86_64
BLUEZ_TOOLS_SUPPORT = 1
$(info RING_NAME = [${RING_NAME}])

# Makefile for hciconfig shared library and test_ble
BUILD_PATH=$(shell pwd)

CC = gcc

################################################################################
## Third party path/defs/stabs
################################################################################
AMB_TOPDIR = $(HOME)/workspace/low-powered-doorbell-fw/ambarella
BLUEZ_INC = -I$(PREBUILD_3RD_PARTY_DIR)/bluez5/include

RING_TOP = $(AMB_TOPDIR)/ring
RING_BLE = $(AMB_TOPDIR)/ring/ble
RING_LOG = $(AMB_TOPDIR)/ring/ding/include

TIBLE_3PARTY = $(AMB_TOPDIR)/prebuild/third-party/TIBle
BTPIA_SDK = $(TIBLE_3PARTY)/BluetopiaPM

BTPIA_SDK_INC = -I$(BTPIA_SDK)/include			    \
		-I$(BTPIA_SDK)/include/client		    \
		-I$(BTPIA_SDK)/include/server		    \
		-I$(BTPIA_SDK)/btpmerr			    \
		-I$(BTPIA_SDK)/btpmvs/wl18xx/		    \
		-I$(BTPIA_SDK)/Bluetopia/include	    \
		-I$(BTPIA_SDK)/Bluetopia/btpskrnl/threaded  \
		-I$(BTPIA_SDK)/Bluetopia/btpscfg

CFLAGS =    -fPIC -Wall -Wextra -O2 -g			    \
	    -D$(RING_NAME)				    \
	    -DBLUEZ_TOOLS_SUPPORT			    \
	    -DTEST_CONSOLE_WRITE			    \
	    -I$(RING_TOP) -I$(RING_BLE) -I$(RING_LOG)	    \
	    -I$(BTPIA_SDK_INC)				    \
	    -I$(HOME)/workspace/low-powered-doorbell-fw/ambarella/prebuild/third-party/TIBle/BluetopiaPM/include

CXXFLAGS = $(CFLAGS)

#LDFLAGS_LIB = -shared -lbluetooth
LDFLAGS_APP = -lbluetooth -L$(BUILD_PATH)/ -lRingBle

RM = rm -f
TARGET_LIB = libRingBle.a
TARGET_APP = test_ble

SRCS_LIB = RingBleApi.cpp RingGattApi.cpp hcitools.c
SRCS_APP = test_ble.c gatt_srv_test.cpp test_stubs.cpp

OBJS_LIB = RingBleApi.o RingGattApi.o hcitools.o
OBJS_APP = test_ble.o gatt_srv_test.o test_stubs.o

.PHONY: all
all: ${TARGET_LIB} $(TARGET_APP)

$(TARGET_LIB): $(OBJS_LIB)
	$(AR) rcs $@ $(OBJS_LIB)

$(TARGET_APP): $(OBJS_APP)
	$(CC) $(OBJS_APP) $(LDFLAGS_APP) -o $@

.PHONY: clean
clean_obj:
	-${RM} ${OBJS_LIB} ${OBJS_APP} *.o *.d

clean:
	-${RM} ${TARGET_LIB} ${TARGET_APP} ${OBJS_LIB} ${OBJS_APP} *.o *.d

