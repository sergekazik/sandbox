#ifndef BLEAPI_H
#define BLEAPI_H

#include <stdio.h>
#include <stdlib.h>

#if defined(hpcam2) // || defined(Linux_x86_64)
#include "gatt_api.h"
#endif

namespace Ring { namespace Ble {

class BleApi
{
   public:
       static BleApi* getInstance( ) {
           if (instance == NULL)
               instance = new BleApi();
           return instance;
       }

       ~BleApi( );
   private:
       BleApi( );
       static BleApi* instance;
};

} } /* namespace Ring::Ble */


#endif // BLEAPI_H
