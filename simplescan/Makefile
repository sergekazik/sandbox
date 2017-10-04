# Makefile for hciconfig shared library and test_ble
BUILD_PATH=$(shell pwd)

CC = gcc
CFLAGS = -fPIC -Wall -Wextra -O2 -g

$(info RING_NAME = [${RING_NAME}])
ifeq ($(RING_NAME), s2lm_ironman)
CFLAGS += -Ds2lm_ironman
else ifeq ($(RING_NAME), hpcam2)
CFLAGS += -Dhpcam2
else
CFLAGS += -DLinux_x86_64
$(info build for Linux_x86_64)
endif

LDFLAGS_LIB = -shared -lbluetooth
LDFLAGS_APP = -lbluetooth -L$(BUILD_PATH)/ -lRingBle

RM = rm -f
TARGET_LIB = libRingBle.so
TARGET_APP = test_ble

SRCS_LIB = RingBleApi.cpp
SRCS_APP = test_ble.c ble_api.cpp hcitools.c

OBJS_LIB = RingBleApi.o
OBJS_APP = test_ble.o ble_api.o hcitools.o

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

