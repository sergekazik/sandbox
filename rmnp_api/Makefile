 #
 # Copyright 2018 Amazon.com, Inc. and its affiliates. All Rights Reserved.
 #

BUILD_PATH=$(shell pwd)

CC = gcc
RM = rm -f

################################################################################
CFLAGS =    -fPIC -Wall -Wextra -O0 -g -std=c++11 -pthread \
	    -Wunused -Wunreachable-code	-Wunused-function -Wno-permissive \
	    -Wno-format-zero-length -Wno-write-strings -Wno-missing-field-initializers -Wno-unused-result  \
	    -I. -DDEBUG_ENABLED

CXXFLAGS = $(CFLAGS)
LDFLAGS = -lpthread -lstdc++
LDFLAGS_LIB = -shared $(LDFLAGS)

TARGET_SDKAPI = librmnpapi.so
TARGET_CLIENT = rmnp_test

SRCS_SDKAPI = rmnp_api.cpp
SRCS_CLIENT = rmnp_test.cpp

INCS_COMMON  = rmnp_api.h
OBJS_COMMON  =

OBJS_SDKAPI = rmnp_api.o $(OBJS_COMMON)
OBJS_CLIENT = rmnp_test.o  $(OBJS_COMMON)

.PHONY: all
all: ${TARGET_SDKAPI} $(TARGET_CLIENT)

$(TARGET_SDKAPI): $(OBJS_SDKAPI) $(INCS_COMMON)
	$(CC) $(OBJS_SDKAPI) $(LDFLAGS_LIB) -o $@

$(TARGET_CLIENT): $(OBJS_CLIENT) $(INCS_COMMON)
	$(CC) $(OBJS_CLIENT) $(LDFLAGS) -L ./ -lrmnpapi -o $@

.PHONY: clean
clean:
	-${RM} ${TARGET_SDKAPI} ${TARGET_CLIENT} ${OBJS_SDKAPI} ${OBJS_CLIENT} *.o *.d

