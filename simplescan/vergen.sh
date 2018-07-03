ver=$(date)
echo "// auto generated data version" > version.h
echo "const char* version_date = \"$ver\";" >> version.h
cp -u -v version.h ~/workspace/low-powered-doorbell-fw/ambarella/ring/ble
cp -u -v version.h ~/workspace/chime_pro-firmware/app/ble/



################################################################################
## test_crypto for LPD
################################################################################
#include $(CLEAR_VARS)
#LOCAL_TARGET := test_crypto

#LOCAL_SRCS := $(LOCAL_PATH)/crypto/test_crypto.cpp

#LOCAL_CFLAGS :=  -I$(LOCAL_PATH)/crypto			    \
#		 -I$(PREBUILD_3RD_PARTY_DIR)/libsodium/include	\
#		 -I$(LOCAL_PATH)			\
#		 -std=c++14				\
#		 -Wall -Werror -Wno-unused-result

#LOCAL_LIBS    := libRingBle.so libRingIPC.so libsodium.so
#LOCAL_LDFLAGS := -Wl,-rpath-link=$(FAKEROOT_DIR)/usr/lib \
#		 -lpthread -lRingIPC -lRingBle \
#		 -L$(FAKEROOT_DIR)/usr/lib -lsodium

#include $(BUILD_APP)
#.PHONY: $(LOCAL_TARGET)
#$(LOCAL_TARGET): $(LOCAL_MODULE)
#	@mkdir -p $(FAKEROOT_DIR)/ring$(BUILD_VER)/bin
#	@cp -dpRf $< $(FAKEROOT_DIR)/ring$(BUILD_VER)/bin/
#	@echo "Build $@ Done."
#$(call add-target-into-build, $(LOCAL_TARGET))
