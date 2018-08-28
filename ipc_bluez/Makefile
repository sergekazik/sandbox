BUILD_PATH=$(shell pwd)

CC = gcc
RM = rm -f

################################################################################
CFLAGS =    -fPIC -Wall -Wextra -O0 -g -std=c++11 -pthread \
	    -Wunused -Wunreachable-code	-Wunused-function   \
	    -Wno-format-zero-length -Wno-write-strings -Wno-missing-field-initializers -Wno-unused-result  \
	    -I. -DDEBUG_ENABLED

CXXFLAGS = $(CFLAGS)

LDFLAGS = -lpthread -lstdc++ -L./ -lbluez

TARGET_SERVER = iserver
TARGET_CLIENT = iclient

SRCS_SERVER = ipcluez.cpp
SRCS_CLIENT = ipclint.cpp

OBJS_SERVER = ipcluez.o
OBJS_CLIENT = ipclint.o

.PHONY: all
all: ${TARGET_SERVER} $(TARGET_CLIENT)

$(TARGET_SERVER): $(OBJS_SERVER)
	$(CC) $(OBJS_SERVER) $(LDFLAGS) -o $@

$(TARGET_CLIENT): $(OBJS_CLIENT)
	$(CC) $(OBJS_CLIENT) $(LDFLAGS) -o $@

cpipcbluez:
	@echo "---------------- updated files: ---------------------"
	@cp -u -v *.cpp *.hh test.script ../../chime_pro-firmware/app/diag/.
	@echo "-----------------------------------------------------"


.PHONY: clean
clean:
	-${RM} ${TARGET_SERVER} ${TARGET_CLIENT} ${OBJS_SERVER} ${OBJS_CLIENT} *.o *.d
