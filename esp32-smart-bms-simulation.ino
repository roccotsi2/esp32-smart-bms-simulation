// General format:
// D203        
// HEADER_READ CONTENT_LENGTH CONTENT CRC
// <-----------strGoodData-------------->
// <-----------CRC relevant--------->
// Content length (byte): 124 -> Run Info
// Content length (byte): 32  -> Run Info Last Battery Value
// Content length (byte): 82  -> Set Info
// Content length (byte): 64  -> Version
// Content length (byte): 6   -> PWD

// Format send by Smart BMS APP:
// D203        0000    003E       D7B9
// HEADER_READ LOC No  MaxLength  CRC

#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>
#include "datatypes.h"

// define simulation constants
#define UPDATE_INTERVAL_MILLIS 10000

void setup() {
  Serial.begin(115200);  
  bluetoothSetupServer();  
}

void loop() {
  if (bluetoothIsDeviceConnected()) {  
    delay(2000);
    smartbmsdemoSendRunInfo();

    delay(2000);
    smartbmsdemoSendVersionInfo();
    
    /*delay(2000);
    smartbmsutilSendRunInfoLastBatteryValue();*/

    delay(UPDATE_INTERVAL_MILLIS);
  }
}
