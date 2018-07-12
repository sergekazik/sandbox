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

SRCS_SERVER = pb_receive.cpp
SRCS_CLIENT = pb_send.cpp   \
	      RingIPDetector.cpp

OBJS_SERVER = pb_receive.o
OBJS_CLIENT = pb_send.o RingIPDetector.o

#pb_receive.o: pb_receive.cpp pb_common.h
#	$(CC) -c $(CFLAGS) -DPB_SERVER pb_receive.cpp -o $@
#pb_send.o: pb_send.cpp pb_common.h
#	$(CC) -c $(CFLAGS) -DPB_CLIENT pb_send.cpp -o $@

.PHONY: all
all: ${TARGET_SERVER} $(TARGET_CLIENT)

$(TARGET_SERVER): $(OBJS_SERVER)
	$(CC) $(OBJS_SERVER) $(LDFLAGS) -o $@

$(TARGET_CLIENT): $(OBJS_CLIENT)
	$(CC) $(OBJS_CLIENT) $(LDFLAGS) -o $@

cpportblock:
	@echo "---------------- updated files: ---------------------"
	@cp -u -v *.cpp *.h *.hh ../../chime_pro-firmware/app/diag/.
	@echo "-----------------------------------------------------"


.PHONY: clean
clean:
	-${RM} ${TARGET_SERVER} ${TARGET_CLIENT} ${OBJS_SERVER} ${OBJS_CLIENT} *.o *.d
