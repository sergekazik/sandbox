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


# Ring Service, UUID: 9760FACEA23446869E00FCBBEE3373F7
# SET_PUBLIC_KEY    ,Characteristic, UUID: 9760FACEA23446869E20D087333C2C01
# GET_PUBLIC_PAYLOAD,Characteristic, UUID: 9760FACEA23446869E20D087333C2C02
# GET_PUBLICPLD_DESC,Descriptor, UUID: 0000290200001000800000805F9B34FB
# GET_AP_LIST       ,Characteristic, UUID: 9760FACEA23446869E20D087333C2C03
# SET_PROVISION     ,Characteristic, UUID: 9760FACEA23446869E20D087333C2C04
# SET_AP_SCAN_START ,Characteristic, UUID: 9760FACEA23446869E20D087333C2C05
# SET_COUNTRY_CODE  ,Characteristic, UUID: 9760FACEA23446869E20D087333C2C06
# GET_PAIRING_STATE ,Characteristic, UUID: 9760FACEA23446869E20D087333C2C07
# GET_PAIRINGST_DESC,Descriptor, UUID: 0000290200001000800000805F9B34FB
# SET_LANGUAGE      ,Characteristic, UUID: 9760FACEA23446869E20D087333C2C08
# SET_ZIPCODE       ,Characteristic, UUID: 9760FACEA23446869E20D087333C2C09
# RESERVED          ,Characteristic, UUID: 9760FACEA23446869E20D087333C2C0A
# GET_SSID_WIFI     ,Characteristic, UUID: 9760FACEA23446869E20D087333C2C0B
# GET_SERIAL_NUMBER ,Characteristic, UUID: 9760FACEA23446869E20D087333C2C0C
# GET_MAC_ADDRESS   ,Characteristic, UUID: 9760FACEA23446869E20D087333C2C0D
# SET_ETHERNET      ,Characteristic, UUID: 9760FACEA23446869E20D087333C2C0E
# SET_REG_DOMAIN    ,Characteristic, UUID: 9760FACEA23446869E20D087333C2C0F
# GET_NET_INFO      ,Characteristic, UUID: 9760FACEA23446869E20D087333C2C10
