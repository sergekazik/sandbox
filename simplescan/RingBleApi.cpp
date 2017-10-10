/*-----------------------------------------------------------------------
 *      ------------------     ----------------------                   *
 *      | gatt_srv_test.c|     | Ring App Setup.cpp |                   *
 *      ------------------     ----------------------                   *
 *                  |                     |                             *
 *                  |       ==============================              *
 *                  |       #   RingBleApi.cpp abstract  #              *
 *                  |       ==============================              *
 *                  |       | GattSrv.cpp | BCM Impl.cpp |              *
 *                  |       ------------------------------              *
 *                  |             |                                     *
 *               ------------------------    ---------------            *
 *               |      gatt_api.c      |    | hcitools.c  |            *
 *               ------------------------    ---------------            *
 *                          |                       |                   *
 *               ------------------------    ---------------            *
 *               | TI WiLink18xx BlueTP |    |   BlueZ     |            *
 *               ------------------------    ---------------            *
 *----------------------------------------------------------------------*/
// #include <ring/console/Logger.hh>

#include "RingBleApi.hh"
using namespace Ring::Ble;

BleApi::BleApi() :
    mInitialized(false)
{
}

/********************************************************************************
 * TODO: (skazik) - define API to add/delete services/characteristics in run time
 *                - add multiple users support
 *                - handle CharacteristicInfo_t value (encrypted?)
 *******************************************************************************/

