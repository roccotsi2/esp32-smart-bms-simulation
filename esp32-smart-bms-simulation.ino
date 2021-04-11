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
#define UPDATE_INTERVAL_MILLIS 5000
#define COUNT_BATTERIES 4
#define MIN_CELL_VOLTAGE 4127
#define MIN_CURRENT_VOLT 12
#define MIN_CURRENT_AMP 1
#define MAX_CURRENT_VOLT 14
#define MAX_CURRENT_AMP 100
#define STEPS_VOLT 1
#define STEPS_AMP 1
String APP_VERSION = "App v6.11";
String MCU_VERSION = "MCU v6.12";
String MACHINE_VERSION = "Machine v6.13";

#define READ_PACKET_OVERHEAD_LENGTH 5

// variables
int currentA = MIN_CURRENT_AMP;
int currentV = MIN_CURRENT_VOLT;
SmartbmsutilRunInfo currentSmartbmsutilRunInfo;
SmartbmsutilVersionInfo currentSmartbmsutilVersionInfo;
SmartbmsutilRunInfoLastBatteryValue currentSmartbmsutilRunInfoLastBatteryValue;

// fills currentSmartbmsutilRunInfo with next simulation values
void fillSmartbmsutilRunInfo(SmartbmsutilRunInfo *currentSmartbmsutilRunInfo) {
  currentSmartbmsutilRunInfo->header1 = 0xD2;
  currentSmartbmsutilRunInfo->header2 = 0x03;
  currentSmartbmsutilRunInfo->contentLength = sizeof(SmartbmsutilRunInfo) - READ_PACKET_OVERHEAD_LENGTH;

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
  smartbmsutilSwapBmsBytesEndian(buffer, sizeof(SmartbmsutilRunInfo) - 2);
  smartbmsutilGetCRC(crcBuffer, buffer, sizeof(SmartbmsutilRunInfo) - 2);
  currentSmartbmsutilRunInfo->crcHigh = crcBuffer[0];
  currentSmartbmsutilRunInfo->crcLow = crcBuffer[1];
}

// fills currentSmartbmsutilVersionInfo with next simulation values
void fillSmartbmsutilVersionInfo(SmartbmsutilVersionInfo *currentSmartbmsutilVersionInfo) {
  currentSmartbmsutilVersionInfo->header1 = 0xD2;
  currentSmartbmsutilVersionInfo->header2 = 0x03;
  currentSmartbmsutilVersionInfo->contentLength = sizeof(SmartbmsutilVersionInfo) - READ_PACKET_OVERHEAD_LENGTH;

  int lengthApp = APP_VERSION.length();
  int lengthMcu = MCU_VERSION.length();
  int lengthMachine = MACHINE_VERSION.length();  //currentSmartbmsutilVersionInfo->crc = (crcBuffer[0] << 8) + crcBuffer[1];
  for (int i = 0; i < 16; i++) {
    // app version is reversed
    if (lengthApp >= 16 - i) {
      currentSmartbmsutilVersionInfo->appVersionText[i] = (byte) APP_VERSION[15 - i];
    } else {
      currentSmartbmsutilVersionInfo->appVersionText[i] = 0;
    }
  }
  for (int i = 0; i < 16; i++) {
    if (lengthMcu >= i + 1) {
      currentSmartbmsutilVersionInfo->mcuVersionText[i] = (byte) MCU_VERSION[i];
    } else {
      currentSmartbmsutilVersionInfo->mcuVersionText[i] = 0;
    }
  }
  for (int i = 0; i < 32; i++) {
    if (lengthMachine >= i + 1) {
      currentSmartbmsutilVersionInfo->machineVersionText[i] = (byte) MACHINE_VERSION[i];
    } else {
      currentSmartbmsutilVersionInfo->machineVersionText[i] = 0;
    }
  }

  byte crcBuffer[2];
  byte buffer[sizeof(SmartbmsutilVersionInfo)];
  memcpy(buffer, currentSmartbmsutilVersionInfo, sizeof(SmartbmsutilVersionInfo));  
  smartbmsutilGetCRC(crcBuffer, buffer, sizeof(SmartbmsutilVersionInfo) - 2);
  currentSmartbmsutilVersionInfo->crcHigh = crcBuffer[0];
  currentSmartbmsutilVersionInfo->crcLow = crcBuffer[1];
}

// fills currentSmartbmsutilRunInfoLastBatteryValuewith next simulation values
void fillSmartbmsutilRunInfoLastBatteryValue(SmartbmsutilRunInfoLastBatteryValue *currentSmartbmsutilRunInfoLastBatteryValue) {
  currentSmartbmsutilRunInfoLastBatteryValue->header1 = 0xD2;
  currentSmartbmsutilRunInfoLastBatteryValue->header2 = 0x03;
  currentSmartbmsutilRunInfoLastBatteryValue->contentLength = sizeof(SmartbmsutilRunInfoLastBatteryValue) - READ_PACKET_OVERHEAD_LENGTH;

  int batteryMilliVolt = MIN_CELL_VOLTAGE;
  for (int i = 0; i < 16; i++) {
    if (COUNT_BATTERIES >= i + 1) {
      batteryMilliVolt = batteryMilliVolt + 10 * i;
      currentSmartbmsutilRunInfoLastBatteryValue->lastBatteryValue[i] = batteryMilliVolt;
    } else {
      currentSmartbmsutilRunInfoLastBatteryValue->lastBatteryValue[i] = 0;
    }
  }

  byte crcBuffer[2];
  byte buffer[sizeof(SmartbmsutilRunInfoLastBatteryValue)];
  memcpy(buffer, currentSmartbmsutilRunInfoLastBatteryValue, sizeof(SmartbmsutilRunInfoLastBatteryValue));  
  smartbmsutilSwapBmsBytesEndian(buffer, sizeof(SmartbmsutilRunInfoLastBatteryValue) - 2);
  smartbmsutilGetCRC(crcBuffer, buffer, sizeof(SmartbmsutilRunInfoLastBatteryValue) - 2);
  currentSmartbmsutilRunInfoLastBatteryValue->crcHigh = crcBuffer[0];
  currentSmartbmsutilRunInfoLastBatteryValue->crcLow = crcBuffer[1];
}

void setup() {
  Serial.begin(115200);  
  bluetoothSetupServer();  
}

void loop() {
  if (bluetoothIsDeviceConnected) {  
    Serial.println("Sending VersionInfo");  
    byte bufferVersion[sizeof(SmartbmsutilVersionInfo)];
    fillSmartbmsutilVersionInfo(&currentSmartbmsutilVersionInfo);
    smartbmsutilWriteSmartbmsutilVersionInfoToBuffer(bufferVersion, sizeof(bufferVersion), &currentSmartbmsutilVersionInfo);
    bluetoothSendByteArray(bufferVersion, sizeof(bufferVersion));
    
    delay(1000);
    Serial.println("Sending RunInfo");
    byte bufferRunInfo[sizeof(SmartbmsutilRunInfo)];
    fillSmartbmsutilRunInfo(&currentSmartbmsutilRunInfo);
    smartbmsutilWriteSmartbmsutilRunInfoToBuffer(bufferRunInfo, sizeof(bufferRunInfo), &currentSmartbmsutilRunInfo);
    bluetoothSendByteArray(bufferRunInfo, sizeof(bufferRunInfo));
    
    /*delay(1000);
    Serial.println("Sending RunInfoLastBatteryValue");
    byte bufferRunInfoLastBatteryValue[sizeof(SmartbmsutilRunInfoLastBatteryValue)];
    fillSmartbmsutilRunInfoLastBatteryValue(&currentSmartbmsutilRunInfoLastBatteryValue);
    smartbmsutilWriteSmartbmsutilRunInfoLastBatteryValueToBuffer(bufferRunInfoLastBatteryValue, sizeof(bufferRunInfoLastBatteryValue), &currentSmartbmsutilRunInfoLastBatteryValue);
    bluetoothSendByteArray(bufferRunInfoLastBatteryValue, sizeof(bufferRunInfoLastBatteryValue));
    delay(1000);*/

    delay(UPDATE_INTERVAL_MILLIS);
  }
}
