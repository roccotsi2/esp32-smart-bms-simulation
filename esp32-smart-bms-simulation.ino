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
#define COUNT_BATTERIES 4
#define MIN_CELL_VOLTAGE 4127
#define MIN_CURRENT_VOLT 12
#define MIN_CURRENT_AMP 1
#define MAX_CURRENT_VOLT 14
#define MAX_CURRENT_AMP 100
#define STEPS_VOLT 1
#define STEPS_AMP 1
#define APP_VERSION "App v5.14"
#define MCU_VERSION "MCU v5.14"
#define MACHINE_VERSION "Machine v5.14"

// variables
int currentA = MIN_CURRENT_AMP;
int currentV = MIN_CURRENT_VOLT;
SmartbmsutilRunInfo currentSmartbmsutilRunInfo;

// fills currentSmartbmsutilRunInfo with next simulation values
void fillSmartbmsutilRunInfo(SmartbmsutilRunInfo *currentSmartbmsutilRunInfo) {
  currentSmartbmsutilRunInfo->header1 = 0xD2;
  currentSmartbmsutilRunInfo->header2 = 0x03;
  currentSmartbmsutilRunInfo->contentLength = 0x7C;

  int batteryMilliVolt = MIN_CELL_VOLTAGE;
  for (int i = 0; i < 32; i++) {
    if (COUNT_BATTERIES >= i + 1) {
      currentSmartbmsutilRunInfo->batteryVoltages[i] = batteryMilliVolt;
    } else {
      currentSmartbmsutilRunInfo->batteryVoltages[i] = 0;
    }
    batteryMilliVolt = batteryMilliVolt + 10;
  }

  int batteryTemperature = 60;
  for (int i = 0; i < 8; i++) {
    if (COUNT_BATTERIES >= i + 1) {
      currentSmartbmsutilRunInfo->batteryTemp[i] = batteryTemperature;
      batteryTemperature++;
    } else {
      currentSmartbmsutilRunInfo->batteryTemp[i] = 0;
    }
  }

  if ((currentV + STEPS_VOLT) > MAX_CURRENT_VOLT) {
    currentV = MIN_CURRENT_VOLT;
  } else {
    currentV = currentV + STEPS_VOLT;
  }

  if ((currentA + STEPS_AMP) > MAX_CURRENT_AMP) {
    currentA = MIN_CURRENT_AMP;
  } else {
    currentA = currentA + STEPS_AMP;
  }

  int currentKw = currentV * currentA;
  int currentLoad = (currentKw / (MAX_CURRENT_VOLT * MAX_CURRENT_AMP * 1.0)) * 1000.0;
  int maxCellVoltage = MIN_CELL_VOLTAGE + ((COUNT_BATTERIES - 1) * 10);
  int averageCellVoltage = (MIN_CELL_VOLTAGE + maxCellVoltage) / 2;

  currentSmartbmsutilRunInfo->currentV = currentV * 10; // current V (in 0,1V)
  currentSmartbmsutilRunInfo->currentA = 30000 + (currentA * 10); // current A (offset: 30000 in 0,1A)
  currentSmartbmsutilRunInfo->nowValuePercent = currentLoad; // NowValue in Percent (in 0,1 percent)
  currentSmartbmsutilRunInfo->maxCellVoltage = maxCellVoltage; // max cell voltage (in mV)
  currentSmartbmsutilRunInfo->minCellVoltage = MIN_CELL_VOLTAGE; // min cell voltage (in mV)
  currentSmartbmsutilRunInfo->dummy1 = 0;
  currentSmartbmsutilRunInfo->dummy2 = 0;
  currentSmartbmsutilRunInfo->dummy3 = 0;
  currentSmartbmsutilRunInfo->dummy4 = 0;
  currentSmartbmsutilRunInfo->countBatteryVoltages = COUNT_BATTERIES; // count battery volt
  currentSmartbmsutilRunInfo->countBatteryTemp = COUNT_BATTERIES; // count battery temperatures (max-> 8)
  currentSmartbmsutilRunInfo->cycle = 5;
  currentSmartbmsutilRunInfo->jh = 1;
  currentSmartbmsutilRunInfo->cdmos = 1;
  currentSmartbmsutilRunInfo->fdmos = 1;
  currentSmartbmsutilRunInfo->avgVoltage = averageCellVoltage; // average voltage (in mV)
  currentSmartbmsutilRunInfo->diffVoltage = 4000; // differential voltage (in mV)
  currentSmartbmsutilRunInfo->currentKw = currentKw; // current KW (in W)
  currentSmartbmsutilRunInfo->alarm1 = 0;
  currentSmartbmsutilRunInfo->alarm2 = 0;
  currentSmartbmsutilRunInfo->alarm3 = 0;
  currentSmartbmsutilRunInfo->alarm4 = 0;
  
  byte crcBuffer[2];
  byte buffer[sizeof(SmartbmsutilRunInfo)];
  memcpy(buffer, currentSmartbmsutilRunInfo, sizeof(SmartbmsutilRunInfo));  
  smartbmsutilSwapBmsBytesEndian(buffer, sizeof(SmartbmsutilRunInfo));
  smartbmsutilGetCRC(crcBuffer, buffer, sizeof(SmartbmsutilRunInfo) - 2);
  currentSmartbmsutilRunInfo->crc = (crcBuffer[0] << 8) + crcBuffer[1];
}

void setup() {
  Serial.begin(115200);  
  bluetoothSetupServer();  
}

void loop() {
  if (bluetoothIsDeviceConnected) {    
    byte bufferVersion[69];
    smartbmsutilCreateVersionResponse(bufferVersion, APP_VERSION, MCU_VERSION, MACHINE_VERSION);
    bluetoothSendByteArray(bufferVersion, sizeof(bufferVersion));
    delay(1000);
    
    byte bufferRunInfo[sizeof(SmartbmsutilRunInfo)];
    fillSmartbmsutilRunInfo(&currentSmartbmsutilRunInfo);
    smartbmsutilWriteSmartbmsutilRunInfoToBuffer(bufferRunInfo, sizeof(bufferRunInfo), &currentSmartbmsutilRunInfo);
    bluetoothSendByteArray(bufferRunInfo, sizeof(bufferRunInfo));
    delay(1000);

    byte bufferRunInfoLastBatteryValue[37];
    smartbmsutilCreateRunInfoLastBatteryValueResponse(bufferRunInfoLastBatteryValue);
    bluetoothSendByteArray(bufferRunInfoLastBatteryValue, sizeof(bufferRunInfoLastBatteryValue));
    delay(1000);
  }
}
