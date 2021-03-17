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

// define Daly Smart BMS UUIDs for bluetooth
#define SERVICE_UUID           "0000fff0-0000-1000-8000-00805f9b34fb"
#define CHARACTERISTIC_UUID_RX "0000fff1-0000-1000-8000-00805f9b34fb"
#define CHARACTERISTIC_UUID_TX "0000fff2-0000-1000-8000-00805f9b34fb"

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

// define bluetooth constants
#define BLE_DATA_CHUNK_SIZE 200

// variables
BLECharacteristic *pCharacteristicWrite;
BLECharacteristic *pCharacteristicRead;
bool deviceConnected = false;
int currentA = MIN_CURRENT_AMP;
int currentV = MIN_CURRENT_VOLT;

char convertCharToHex(char ch)
{
  char returnType;
  switch(ch)
  {
    case '0':
    returnType = 0;
    break;
    case  '1' :
    returnType = 1;
    break;
    case  '2':
    returnType = 2;
    break;
    case  '3':
    returnType = 3;
    break;
    case  '4' :
    returnType = 4;
    break;
    case  '5':
    returnType = 5;
    break;
    case  '6':
    returnType = 6;
    break;
    case  '7':
    returnType = 7;
    break;
    case  '8':
    returnType = 8;
    break;
    case  '9':
    returnType = 9;
    break;
    case  'A':
    returnType = 10;
    break;
    case  'B':
    returnType = 11;
    break;
    case  'C':
    returnType = 12;
    break;
    case  'D':
    returnType = 13;
    break;
    case  'E':
    returnType = 14;
    break;
    case  'F' :
    returnType = 15;
    break;
    default:
    returnType = 0;
    break;
  }
  return returnType;
}

void hexStringToByteArray(byte *byteArray, String hexString, int arrayLength) {
  for(char i = 0; i < arrayLength; i++)
  {
    byte extract;
    char a = hexString[2*i];
    char b = hexString[2*i + 1];
    extract = convertCharToHex(a)<<4 | convertCharToHex(b);
    byteArray[i] = extract;
  }
}

void array_to_string(byte array[], unsigned int len, char buffer[])
{
    for (unsigned int i = 0; i < len; i++)
    {
        byte nib1 = (array[i] >> 4) & 0x0F;
        byte nib2 = (array[i] >> 0) & 0x0F;
        buffer[i*2+0] = nib1  < 0xA ? '0' + nib1  : 'A' + nib1  - 0xA;
        buffer[i*2+1] = nib2  < 0xA ? '0' + nib2  : 'A' + nib2  - 0xA;
    }
    buffer[len*2] = '\0';
}

void printByteArrayInHex(byte *buffer) {
  char str[1000] = "";
  array_to_string(buffer, sizeof(buffer), str);
  Serial.println(str);
}

void setIntValueToArray(byte *buffer, int startPosition, int value) {
  buffer[startPosition] = byte(value >> 8); // higher byte
  buffer[startPosition + 1] = byte(value & 0x00FF); // lower byte
}

void getCRC(byte *crcArray, byte *sourceByteArray, int arrayLength) {
  int CRC = 65535;
  for (int i = 0; i < arrayLength; i++) {
    CRC ^= sourceByteArray[i] & 255;
    for (int j = 0; j < 8; j++) {
      if ((CRC & 1) != 0) {
        CRC = (CRC >> 1) ^ 40961;
      } else {
        CRC >>= 1;
      }
    }
  }
  CRC = ((65280 & CRC) >> 8) | ((CRC & 255) << 8);

  crcArray[0] = byte(CRC >> 8);
  crcArray[1] = byte(CRC & 0x00FF);
}

void createVersionResponse(byte *buffer, String appVersion, String mcuVersion, String machineVersion) {
  // Content length: 40 (HEX)
  // Content: <App Version: 16 byte> (reversed), <MCU Version: 16 byte>, <Machine Version: 32 byte>
   
  int lengthApp = appVersion.length();
  int lengthMcu = mcuVersion.length();
  int lengthMachine = machineVersion.length();
  buffer[0] = 0xD2;
  buffer[1] = 0x03;
  buffer[2] = 0x40; // content length "40" = 64
  for (int i = 3; i < 3 + 16; i++) {
    // app version is reversed
    if (lengthApp >= 3 + 16 - i) {
      buffer[i] = appVersion[3 + 15 - i];
    } else {
      buffer[i] = 0;
    }
  }
  for (int i = 19; i < 19 + 16; i++) {
    if (lengthMcu >= i - 19) {
      buffer[i] = (byte) mcuVersion[i - 19];
    } else {
      buffer[i] = 0;
    }
  }
  for (int i = 35; i < 35 + 32; i++) {
    if (lengthMachine >= i - 35) {
      buffer[i] = (byte) machineVersion[i - 35];
    } else {
      buffer[i] = 0;
    }
  }
  byte crcBuffer[2];
  getCRC(crcBuffer, buffer, 67);
  buffer[67] = crcBuffer[0];
  buffer[68] = crcBuffer[1];
}

void createRunInfoResponse(byte *buffer) {
  // Content length RunDataInfo: 124 byte (all values in 2 byte)
  // 32 values (position 0-31): Battery volt (in mV)
  // 8 values (position 32-39): Battery temperature (offset: 40)
  // Position 40: current V (in 0,1V)
  // Position 41: current A (offset: 30000 in 0,1A)
  // Position 42: NowValue in Percent (in 0,1 percent)
  // Position 43: max cell voltage (in mV)
  // Position 44: min cell voltage (in mV)
  // Position 45: ???
  // Position 46: ???
  // Position 47: ???
  // Position 48: ???
  // Position 49: count battbatteryTemperatureery volt
  // Position 50: count battery temperatures (max. 8)
  // Position 51: cycle
  // Position 52: JH on/off
  // Position 53: CDMOS on/off (1 = on, everything else off)
  // Position 54: FDMOS on/off
  // Position 55: average voltage (in mV)
  // Position 56: differential voltage (in mV)   
  // Position 57: current KW (in W)
  // Position 58: Alarm1
  // Position 59: Alarm2
  // Position 60: Alarm3
  // Position 61: Alarm4
  
  buffer[0] = 0xD2;
  buffer[1] = 0x03;
  buffer[2] = 0x7C; // content length: "7C" = 124

  int batteryMilliVolt = MIN_CELL_VOLTAGE;
  for (int i = 0; i < 32; i++) {
    if (COUNT_BATTERIES >= i + 1) {
      setIntValueToArray(buffer, 3 + 2*i, batteryMilliVolt);
    } else {
      setIntValueToArray(buffer, 3 + 2*i, 0);
    }
    batteryMilliVolt = batteryMilliVolt + 10;
  }

  int batteryTemperature = 60;
  for (int i = 0; i < 8; i++) {
    if (COUNT_BATTERIES >= i + 1) {
      setIntValueToArray(buffer, 3 + 64 + 2*i, batteryTemperature);
      batteryTemperature++;
    } else {
      setIntValueToArray(buffer, 3 + 64 + 2*i, 0);
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

  setIntValueToArray(buffer, 83, currentV * 10); // current V (in 0,1V)
  setIntValueToArray(buffer, 85, 30000 + (currentA * 10)); // current A (offset: 30000 in 0,1A)
  setIntValueToArray(buffer, 87, currentLoad); // NowValue in Percent (in 0,1 percent)
  setIntValueToArray(buffer, 89, maxCellVoltage); // max cell voltage (in mV)
  setIntValueToArray(buffer, 91, MIN_CELL_VOLTAGE); // min cell voltage (in mV)
  setIntValueToArray(buffer, 93, 0); // ???
  setIntValueToArray(buffer, 95, 0); // ???
  setIntValueToArray(buffer, 97, 0); // ???
  setIntValueToArray(buffer, 99, 0); // ???
  setIntValueToArray(buffer, 101, COUNT_BATTERIES); // count battery volt
  setIntValueToArray(buffer, 103, COUNT_BATTERIES); // count battery temperatures (max. 8)
  setIntValueToArray(buffer, 105, 5); // cycle
  setIntValueToArray(buffer, 107, 1); // JH on/off
  setIntValueToArray(buffer, 109, 1); // CDMOS on/off (1 = on, everything else off)
  setIntValueToArray(buffer, 111, 1); // FDMOS on/off
  setIntValueToArray(buffer, 113, averageCellVoltage); // average voltage (in mV)
  setIntValueToArray(buffer, 115, 4000); // differential voltage (in mV)
  setIntValueToArray(buffer, 117, currentKw); // current KW (in W)

  // alarm 1
  buffer[119] = 0;
  buffer[120] = 0;

  // alarm 1
  buffer[121] = 0;
  buffer[122] = 0;

  // alarm 1
  buffer[123] = 0;
  buffer[124] = 0;

  // alarm 1
  buffer[125] = 0;
  buffer[126] = 0;
  
  byte crcBuffer[2];
  getCRC(crcBuffer, buffer, 127);
  buffer[127] = crcBuffer[0];
  buffer[128] = crcBuffer[1];
}

void createRunInfoLastBatteryValueResponse(byte *buffer) {
  // Content length RunInfoLastBatteryValue: 32 byte (all values in 2 byte)
  // 16 values: Battery volt (in mV)
  
  buffer[0] = 0xD2;
  buffer[1] = 0x03;
  buffer[2] = 0x20; // content length: "20" = 32

  int batteryMilliVolt = MIN_CELL_VOLTAGE;
  for (int i = 0; i < 16; i++) {
    if (COUNT_BATTERIES >= i + 1) {
      batteryMilliVolt = batteryMilliVolt + 10 * i;
      setIntValueToArray(buffer, 3 + 2*i, batteryMilliVolt);
    } else {
      setIntValueToArray(buffer, 3 + 2*i, 0);
    }
  }
  
  byte crcBuffer[2];
  getCRC(crcBuffer, buffer, 127);
  buffer[127] = crcBuffer[0];
  buffer[128] = crcBuffer[1];
}

void sendByteArray(byte *buffer, int dataLength) {
  pCharacteristicRead->setValue(buffer, dataLength); 
  pCharacteristicRead->notify();
}

class MyServerCallbacks: public BLEServerCallbacks {
    void onConnect(BLEServer* pServer) {
      deviceConnected = true;
    };

    void onDisconnect(BLEServer* pServer) {
      deviceConnected = false;
    }
};

class MyCallbacks: public BLECharacteristicCallbacks {
    void onWrite(BLECharacteristic *pCharacteristic) {
      std::string rxValue = pCharacteristic->getValue();

      if (rxValue.length() > 0) {
        Serial.println("*********");
        Serial.print("Received Value: ");

        for (int i = 0; i < rxValue.length(); i++) {
          if (rxValue[i]<0x10) {
            Serial.print("0");
          }
          Serial.print(rxValue[i], HEX);
          Serial.print(' ');
        }

        Serial.println();
      }  
    }
};

void setup() {
  Serial.begin(115200);  

  // Create the BLE Device
  BLEDevice::init("DL-40D63C3223A2"); // Give it a name

  // Create the BLE Server
  BLEServer *pServer = BLEDevice::createServer();
  pServer->setCallbacks(new MyServerCallbacks());

  // Create the BLE Service
  BLEService *pService = pServer->createService(SERVICE_UUID);

  // Create a BLE Characteristic
  pCharacteristicWrite = pService->createCharacteristic(
                      CHARACTERISTIC_UUID_TX,
                      BLECharacteristic::PROPERTY_WRITE
                    );

  pCharacteristicWrite->setCallbacks(new MyCallbacks());

  pCharacteristicRead = pService->createCharacteristic(
                                         CHARACTERISTIC_UUID_RX,
                                         BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_NOTIFY
                                       );

  pCharacteristicRead->addDescriptor(new BLE2902());

  // Start the service
  pService->start();  

  // Start advertising
  pServer->getAdvertising()->addServiceUUID(BLEUUID(SERVICE_UUID));
  pServer->getAdvertising()->start();
  Serial.println("Waiting a client connection to notify...");
}

void loop() {
  if (deviceConnected) {    
    byte bufferVersion[69];
    createVersionResponse(bufferVersion, APP_VERSION, MCU_VERSION, MACHINE_VERSION);
    sendByteArray(bufferVersion, sizeof(bufferVersion));
    delay(1000);
    
    byte bufferRunInfo[129];
    createRunInfoResponse(bufferRunInfo);
    sendByteArray(bufferRunInfo, sizeof(bufferRunInfo));
    delay(1000);

    byte bufferRunInfoLastBatteryValue[37];
    createRunInfoLastBatteryValueResponse(bufferRunInfoLastBatteryValue);
    sendByteArray(bufferRunInfoLastBatteryValue, sizeof(bufferRunInfoLastBatteryValue));
    delay(1000);
  }
}
