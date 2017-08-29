# Makefile for hciconfig shared library
BUILD_PATH=$(shell pwd)

CC = gcc
CFLAGS = -fPIC -Wall -Wextra -O2 -g

LDFLAGS_LIB = -shared  -lbluetooth
LDFLAGS_APP = -lbluetooth -L$(BUILD_PATH)/ -lbleconfig

RM = rm -f
TARGET_LIB = libbleconfig.so
TARGET_APP = simplescan

SRCS_LIB = hcitools.c csr.c
SRCS_APP = simplescan.c

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

