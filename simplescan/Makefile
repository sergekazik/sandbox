# set the proper target for the test
BLUEZ_TOOLS_SUPPORT = 1
RING_NAME = Linux_x86_64

$(info RING_NAME = [${RING_NAME}])
$(info BLE_CS = [${BLE_CS}])

# Makefile for hciconfig shared library and test_ble
BUILD_PATH=$(shell pwd)

CC = g++

################################################################################
## Third party path/defs/stabs
################################################################################
AMB_TOPDIR = $(HOME)/workspace/low-powered-doorbell-fw/ambarella
BLUEZ_INC = -I$(PREBUILD_3RD_PARTY_DIR)/bluez5/include

RING_TOP = $(AMB_TOPDIR)/ring
RING_BLE = $(AMB_TOPDIR)/ring/ble
RING_COM = $(AMB_TOPDIR)/ring/common
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

CFLAGS =    -fPIC -Wall -Wextra -O2 -g -Wno-format-zero-length -Wno-write-strings -Wno-missing-field-initializers    \
	    -D$(RING_NAME) -D$(BLE_CS) -DRING_NAME=\"$(RING_NAME)\"	    \
	    -I. -I$(BLE_CS)					    \
	    -DBLUEZ_TOOLS_SUPPORT			    \
	    -DTEST_CONSOLE_WRITE			    \
	    -I$(RING_TOP) -I$(RING_LOG)	-I$(RING_COM)    \
	     $(BTPIA_SDK_INC)

CXXFLAGS = $(CFLAGS)

#LDFLAGS_LIB = -shared -lbluetooth
#LDFLAGS_APP = -lbluetooth -L$(BUILD_PATH)/ -lRingBle
LDFLAGS_APP = -lbluetooth

RM = rm -f
TARGET_LIB = libRingBle.a
TARGET_APP = test_ble
TARGET_GATT = test_gatt

SRCS_LIB = RingBleApi.cpp ./$(BLE_CS)/RingGattApi.cpp RingBlePairing.cpp
SRCS_APP = test_ble.cpp test_stubs.cpp
SRCS_GATT = gatt_test_srv.cpp test_stubs.cpp

OBJS_LIB = RingBleApi.o RingGattApi.o RingBlePairing.o
OBJS_APP = test_ble.o test_stubs.o
OBJS_GATT = gatt_test_srv.o test_stubs.o

RingGattApi.o: ./$(BLE_CS)/RingGattApi.cpp
	$(CC) -c $(CFLAGS) $(BLE_CS)/RingGattApi.cpp -o $@

.PHONY: all
all: ${TARGET_LIB} $(TARGET_APP) $(TARGET_GATT)

$(TARGET_LIB): $(OBJS_LIB)
	$(AR) rcs $@ $(OBJS_LIB)
	#$(CC) $(OBJS_LIB) $(LDFLAGS_LIB) -o $@

$(TARGET_APP): $(OBJS_APP)
	$(CC) $(OBJS_APP) ${TARGET_LIB} $(LDFLAGS_APP) -o $@

$(TARGET_GATT): $(OBJS_GATT)
	$(CC) $(OBJS_GATT) ${TARGET_LIB} $(LDFLAGS_APP) -o $@

cpringble:
	@echo "---------------- updated files: ---------------------"
	@cp -u *.cpp *.h *.hh -v $(RING_BLE)
	@cp -u BCM43/*.* -v $(RING_BLE)/BCM43
	@cp -u WILINK18/*.* -v $(RING_BLE)/WILINK18
	@echo "-----------------------------------------------------"

prepro:
	$(CC) -E $(CFLAGS) gatt_test_srv.cpp -o gatt_test_srv.prep

.PHONY: clean
clean_obj:
	-${RM} ${OBJS_LIB} ${OBJS_APP} *.o *.d

clean:
	-${RM} ${TARGET_LIB} ${TARGET_APP} ${TARGET_GATT} ${OBJS_LIB} ${OBJS_APP} $(OBJS_GATT) *.o *.d

