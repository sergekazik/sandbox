#ifndef _RING_BLE_PAIRING_H_
#define _RING_BLE_PAIRING_H_

#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <functional>

#include "RingBleApi.hh"

namespace Ring { namespace Ble {

class BlePairing
{
public:
    static BlePairing* getInstance(); // singleton
    static const char* mPayloadReady;
    static const char* mWiFiConnected;
    static const char* mWiFiConnectFailed;
    static const char* mWiFiConfigFile;
    static char mPublicPayload[ATT_MTU_MAX];
    static char mMacAddress[DEV_MAC_ADDR_LEN];
    static char mNetworkInfo[ATT_MTU_MAX];
    static void getValByKeyfromJson(const char* json_str, const char* key, char* val, int len);

private:
    BlePairing();
    static BlePairing* instance;
    BleApi *mBleApi;

public:
    int Initialize(char *aDeviceName = NULL, uint8_t * mac = NULL);
    int StartAdvertising(int aTimeout = 0);
    int StopAdvertising();
    int Shutdown();
    bool isAdvertisingRequested() { return mAdvertisingRequested; }

    // status debug
    int PrintStatus();
    // to pass data to ringnm
    int registerRingDataCallback(std::function<int(int, void*, int)> callback);
    int updateAttribute(int attr_idx, const char * str_data, int len = 0);
private:
    unsigned int   mPairingServiceIndex;
    ServiceInfo_t *mServiceTable;
    unsigned int   mServiceCount;

    char mRingDeviceName[DEV_NAME_LEN];
    unsigned int mLocalClassOfDevice;
    unsigned int mAdvIntervalMin_ms;
    unsigned int mAdvIntervalMax_ms;
    unsigned int mAdvertisingTimeout_sec;
    bool         mAdvertisingRequested;
};

} } /* namespace Ring::Ble */


#endif // _RING_BLE_PAIRING_H_
