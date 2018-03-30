#ifndef _RING_BLE_PAIRING_H_
#define _RING_BLE_PAIRING_H_

#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <functional>

#include "RingBleApi.hh"
#ifdef PAIRING_ENABLE_CRYPTO
#include "crypto/RingCrypto.hh"
#endif

namespace Ring { namespace Ble {

#define RING_CHARACTERISTICS_MAX         24
#define RING_SERVICES_MAX                5

class BlePairing
{
public:
    static BlePairing* getInstance(); // singleton
    static char mMacAddress[DEV_MAC_ADDR_LEN];
#ifdef PAIRING_ENABLE_CRYPTO
    static Crypto::Server *mCrypto;
#endif

private:
    BlePairing();
    static BlePairing* instance;
    BleApi *mBleApi;

public:
    int Initialize(char *aDeviceName = NULL, unsigned char * mac = NULL);
    int StartAdvertising(int aTimeout = 0);
    int StopAdvertising();
    int Shutdown();
    bool isAdvertisingRequested() { return mAdvertisingRequested; }
    const ServiceInfo_t *GetServiceTable() {return &mServiceTable[RING_PAIRING_SVC_IDX]; }

    // status debug
    int PrintStatus();
    // to pass data to ringnm
    int registerRingDataCallback(std::function<int(int, void*, int)> callback);
    int updateAttribute(int attr_idx, const char * str_data, int len = 0);
    int updateServiceTable(int attr_idx, const char * str_data, int len = 0);

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
