# Makefile for hciconfig shared library and test_ble
BUILD_PATH=$(shell pwd)

CC = gcc
TIBLE_PATH = $(HOME)/workspace/low-powered-doorbell-fw/ambarella/ring/ble
CFLAGS = -fPIC -Wall -Wextra -O2 -g \
	-I$(TIBLE_PATH)

RING_NAME = Linux_x86_64
$(info RING_NAME = [${RING_NAME}])
CFLAGS += -D$(RING_NAME)

LDFLAGS_LIB = -shared -lbluetooth
LDFLAGS_APP = -lbluetooth -L$(BUILD_PATH)/ -lRingBle

RM = rm -f
TARGET_LIB = libRingBle.so
TARGET_APP = test_ble

SRCS_LIB = RingBleApi.cpp hcitools.c	    # $(TIBLE_PATH)/gatt_api.cpp
SRCS_APP = test_ble.c

OBJS_LIB = RingBleApi.o hcitools.o	    # $(TIBLE_PATH)/gatt_api.o
OBJS_APP = test_ble.o

.PHONY: all
all: ${TARGET_LIB} $(TARGET_APP)

$(TARGET_LIB): $(OBJS_LIB)
	$(CC) $(OBJS_LIB) $(LDFLAGS_LIB) -o $@

$(TARGET_APP): $(OBJS_APP)
	$(CC) $(OBJS_APP) $(LDFLAGS_APP) -o $@

.PHONY: clean
clean_obj:
	-${RM} ${OBJS_LIB} ${OBJS_APP} *.o *.d

clean:
	-${RM} ${TARGET_LIB} ${TARGET_APP} ${OBJS_LIB} ${OBJS_APP} *.o *.d

