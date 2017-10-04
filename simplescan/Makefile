# Makefile for hciconfig shared library and test_ble
BUILD_PATH=$(shell pwd)

CC = gcc
CFLAGS = -fPIC -Wall -Wextra -O2 -g

$(info $$RING_NAME = [${RING_NAME}])
ifeq ($(RING_NAME), s2lm_ironman)
CFLAGS += -Ds2lm_ironman
else ifeq ($(RING_NAME), hpcam2)
CFLAGS += -Dhpcam2
else
CFLAGS += -DLinux_x86_64
endif

LDFLAGS_LIB = -shared  -lbluetooth
LDFLAGS_APP = -lbluetooth -L$(BUILD_PATH)/ -lbleconfig

RM = rm -f
TARGET_LIB = libbleconfig.so
TARGET_APP = test_ble

SRCS_LIB = hcitools.c
SRCS_APP = test_ble.c

OBJS_LIB = $(SRCS_LIB:.c=.o)
OBJS_APP = $(SRCS_APP:.c=.o)

.PHONY: all
all: ${TARGET_LIB} $(TARGET_APP)

$(TARGET_LIB): $(OBJS_LIB)
	$(CC) -o $@ $^ ${LDFLAGS_LIB}

$(TARGET_APP): $(OBJS_APP)
	$(CC) -o $@ $^ ${LDFLAGS_APP}

$(SRCS_LIB:.c=.d):%.d:%.c
	$(CC) $(CFLAGS) -MM $< >$@

$(SRCS_APP:.c=.d):%.d:%.c
	$(CC) $(CFLAGS) -MM $< >$@

#include $(SRCS_LIB:.c=.d)
#include $(SRCS_APP:.c=.d)

.PHONY: clean
clean_obj:
	-${RM} ${OBJS_LIB} ${OBJS_APP} $(SRCS_LIB:.c=.d) $(SRCS_APP:.c=.d)

clean:
	-${RM} ${TARGET_LIB} ${TARGET_APP} ${OBJS_LIB} ${OBJS_APP} $(SRCS_LIB:.c=.d) $(SRCS_APP:.c=.d)

OBJS_LIB = $(SRCS_LIB:.c=.o)
OBJS_APP = $(SRCS_APP:.c=.o)

