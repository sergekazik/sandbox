BUILD_PATH=$(shell pwd)

CC = gcc
RM = rm -f

################################################################################
CFLAGS =    -fPIC -Wall -Wextra -O0 -g -std=c++11 -pthread \
	    -Wunused -Wunreachable-code	-Wunused-function   \
	    -Wno-format-zero-length -Wno-write-strings -Wno-missing-field-initializers -Wno-unused-result  \
	    -I.

CXXFLAGS = $(CFLAGS)

LDFLAGS = -lpthread -lstdc++

TARGET_SERVER = test_server_portblock
TARGET_CLIENT = test_client_portblock

SRCS_SERVER = test_pb_server.cpp
SRCS_CLIENT = test_pb_client.cpp   \
	      RingIPDetector.cpp

OBJS_SERVER = test_pb_server.o
OBJS_CLIENT = test_pb_client.o RingIPDetector.o

.PHONY: all
all: ${TARGET_SERVER} $(TARGET_CLIENT)

$(TARGET_SERVER): $(OBJS_SERVER)
	$(CC) $(OBJS_SERVER) $(LDFLAGS) -o $@

$(TARGET_CLIENT): $(OBJS_CLIENT)
	$(CC) $(OBJS_CLIENT) $(LDFLAGS) -o $@

cpportblock:
	@echo "---------------- updated files: ---------------------"
	@cp -u -v *.cpp *.hh ../../chime_pro-firmware/app/diag/.
	@echo "-----------------------------------------------------"


.PHONY: clean
clean:
	-${RM} ${TARGET_SERVER} ${TARGET_CLIENT} ${OBJS_SERVER} ${OBJS_CLIENT} *.o *.d
