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

CFLAGS =    -fPIC -Wall -Wextra -O2 -g -std=c++14	\
	    -Wno-format-zero-length -Wno-write-strings -Wno-missing-field-initializers -Wno-unused-result  \
	    -D$(RING_NAME) -D$(BLE_CS) -DRING_NAME=\"$(RING_NAME)\"	    \
	    -I. -I$(BLE_CS)					    \
	    -DTEST_CONSOLE_WRITE			    \
	    -I$(RING_TOP) -I$(RING_LOG)	-I$(RING_COM)    \
	     $(BTPIA_SDK_INC)

ifeq ($(BLE_CS), BCM43)
CFLAGS += -DBLUEZ_TOOLS_SUPPORT
endif

CXXFLAGS = $(CFLAGS)

#LDFLAGS_LIB = -shared -lbluetooth
#LDFLAGS_APP = -lbluetooth -L$(BUILD_PATH)/ -lRingBle
LDFLAGS_APP = -lbluetooth

RM = rm -f
TARGET_LIB = libRingBle.a
TARGET_APP = test_ble
TARGET_GATT = test_gatt

CRYPTO_TARGET_LIB = libRingCrypto.so
CRYPTO_TARGET_APP = test_crypto

SRCS_LIB = RingBleApi.cpp ./$(BLE_CS)/RingGattApi.cpp RingBlePairing.cpp
SRCS_APP = test_ble.cpp test_stubs.cpp
SRCS_GATT = gatt_test_srv.cpp test_stubs.cpp

OBJS_LIB = RingBleApi.o RingGattApi.o RingBlePairing.o
OBJS_APP = test_ble.o test_stubs.o
OBJS_GATT = gatt_test_srv.o test_stubs.o

RingGattApi.o: ./$(BLE_CS)/RingGattApi.cpp
	$(CC) -c $(CFLAGS) $(BLE_CS)/RingGattApi.cpp -o $@

.PHONY: all
all: ${TARGET_LIB} $(TARGET_APP) $(TARGET_GATT) $(CRYPTO_TARGET_LIB) $(CRYPTO_TARGET_APP)

$(TARGET_LIB): $(OBJS_LIB)
	$(AR) rcs $@ $(OBJS_LIB)
	#$(CC) $(OBJS_LIB) $(LDFLAGS_LIB) -o $@

$(TARGET_APP): $(OBJS_APP)
	$(CC) $(OBJS_APP) ${TARGET_LIB} $(LDFLAGS_APP) -o $@

$(TARGET_GATT): $(OBJS_GATT)
	$(CC) $(OBJS_GATT) ${TARGET_LIB} $(LDFLAGS_APP) -o $@

$(TARGET_TEST_CRYPTO): $(OBJS_TEST_CRYPTO)
	$(CC) $(OBJS_TEST_CRYPTO) $(LDFLAGS_APP) -o $@

CRYPTO_CFLAGS =    -fPIC -Wall -Wextra -O2 -g -std=c++14 \
		    -I./include -Wno-unused-result

#CXXFLAGS = $(CRYPTO_CFLAGS)

CRYPTO_LDFLAGS_LIB = -shared -L/usr/lib/x86_64-linux-gnu/ -lsodium
CRYPTO_LDFLAGS_APP = -L./ -lRingCrypto -lpthread -L/usr/lib/x86_64-linux-gnu/ -lsodium

RM = rm -f
CC = g++

CRYPTO_SRCS_LIB = test_handshake.cpp RingCrypto.cpp
CRYPTO_SRCS_APP = crypto/test_crypto.cpp

CRYPTO_OBJS_LIB = crypto/test_handshake.o crypto/RingCrypto.o
CRYPTO_OBJS_APP = crypto/test_crypto.o

.PHONY: all
all: ${CRYPTO_TARGET_LIB} $(CRYPTO_TARGET_APP)

$(CRYPTO_TARGET_LIB): $(CRYPTO_OBJS_LIB)
	$(CC) $(CRYPTO_OBJS_LIB) $(CRYPTO_CFLAGS) $(CRYPTO_LDFLAGS_LIB) -o $@

$(CRYPTO_TARGET_APP): $(CRYPTO_OBJS_APP)
	$(CC) $(CRYPTO_OBJS_APP) $(CRYPTO_CFLAGS) $(CRYPTO_LDFLAGS_APP) -o $@

cpringble:
	@echo "---------------- updated files: ---------------------"
	@cp -u *.cpp *.h *.hh -v $(RING_BLE)
	@cp -u BCM43/*.* -v $(RING_BLE)/BCM43
	@cp -u WILINK18/*.* -v $(RING_BLE)/WILINK18
	#@cp -u -v crypto/*.cpp crypto/*.h crypto/*.hh -v $(RING_BLE)/crypto
	@echo "-----------------------------------------------------"

prepro:
	$(CC) -E $(CFLAGS) gatt_test_srv.cpp -o gatt_test_srv.prep

.PHONY: clean
clean_obj:
	-${RM} ${OBJS_LIB} ${OBJS_APP} *.o *.d

clean:
	-${RM} ${TARGET_LIB} ${TARGET_APP} ${TARGET_GATT} ${OBJS_LIB} ${OBJS_APP} $(OBJS_GATT) $(CRYPTO_TARGET_LIB) $(CRYPTO_TARGET_APP) *.o *.d

