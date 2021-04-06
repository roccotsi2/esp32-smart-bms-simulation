// General format (big-endian):
// D203        
// HEADER_READ CONTENT_LENGTH CONTENT CRC
// <-----------strGoodData-------------->
// <-----------CRC relevant--------->
// Content length (byte): 124 -> Run Info
// Content length (byte): 32  -> Run Info Last Battery Value
// Content length (byte): 82  -> Set Info
// Content length (byte):buffer 64  -> Version
// Content length (byte): 6   -> PWD

// Format send by Smart BMS APP:
// D203        0000    003E       D7B9
// HEADER_READ LOC No  MaxLength  CRC

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

#define RECEIVE_BUFFER_SIZE 500
#define READ_PACKET_HEADER_LENGTH 3 // the number of bytes for header of read packets (header, content length)
#define READ_PACKET_CRC_LENGTH 2 // the number of bytes for CRC of read packets
#define READ_PACKET_OVERHEAD_LENGTH  READ_PACKET_HEADER_LENGTH + READ_PACKET_CRC_LENGTH // the number of bytes for overhead (header, content length, CRC)

byte smartBmsReceiveBuffer[RECEIVE_BUFFER_SIZE];
uint16_t indexSmartBmsReceiveBuffer = 0;

void smartbmsutilGetCRC(byte *crcArray, byte *sourceByteArray, int crcRelevantDataLength) {
  int CRC = 65535;
  for (int i = 0; i < crcRelevantDataLength; i++) {
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

void smartbmsutilCreateVersionResponse(byte *buffer, String appVersion, String mcuVersion, String machineVersion) {
  // Content length: 40 (HEX)
  // Content: <App Version: 16 byte> (reversed), <MCU Version: 16 byte>, <Machine Version: 32 byte>
   
  int lengthApp = appVersion.length();
  int lengthMcu = mcuVersion.length();
  int lengthMachine = machineVersion.length();
  buffer[0] = 0xD2;
  buffer[1] = 0x03;
  buffer[2] = 0x40; // content length "40" = 64
  for (int i = READ_PACKET_HEADER_LENGTH; i < READ_PACKET_HEADER_LENGTH + 16; i++) {
    // app version is reversed
    if (lengthApp >= READ_PACKET_HEADER_LENGTH + 16 - i) {
      buffer[i] = appVersion[READ_PACKET_HEADER_LENGTH + 15 - i];
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
  smartbmsutilGetCRC(crcBuffer, buffer, 67);
  buffer[67] = crcBuffer[0];
  buffer[68] = crcBuffer[1];
}

/*void smartbmsutilCreateRunInfoResponse(byte *buffer, int currentV, int currentA) {
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
  // Position 49: count battery volt
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
      hexutilSetIntValueToArray(buffer, READ_PACKET_HEADER_LENGTH + 2*i, batteryMilliVolt);
    } else {
      hexutilSetIntValueToArray(buffer, READ_PACKET_HEADER_LENGTH + 2*i, 0);
    }
    batteryMilliVolt = batteryMilliVolt + 10;
  }

  int batteryTemperature = 60;
  for (int i = 0; i < 8; i++) {
    if (COUNT_BATTERIES >= i + 1) {
      hexutilSetIntValueToArray(buffer, READ_PACKET_HEADER_LENGTH + 64 + 2*i, batteryTemperature);
      batteryTemperature++;
    } else {
      hexutilSetIntValueToArray(buffer, READ_PACKET_HEADER_LENGTH + 64 + 2*i, 0);
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

  hexutilSetIntValueToArray(buffer, 83, currentV * 10); // current V (in 0,1V)
  hexutilSetIntValueToArray(buffer, 85, 30000 + (currentA * 10)); // current A (offset: 30000 in 0,1A)
  hexutilSetIntValueToArray(buffer, 87, currentLoad); // NowValue in Percent (in 0,1 percent)
  hexutilSetIntValueToArray(buffer, 89, maxCellVoltage); // max cell voltage (in mV)
  hexutilSetIntValueToArray(buffer, 91, MIN_CELL_VOLTAGE); // min cell voltage (in mV)
  hexutilSetIntValueToArray(buffer, 93, 0); // ???
  hexutilSetIntValueToArray(buffer, 95, 0); // ???
  hexutilSetIntValueToArray(buffer, 97, 0); // ???
  hexutilSetIntValueToArray(buffer, 99, 0); // ???
  hexutilSetIntValueToArray(buffer, 101, COUNT_BATTERIES); // count battery volt
  hexutilSetIntValueToArray(buffer, 103, COUNT_BATTERIES); // count battery temperatures (max. 8)
  hexutilSetIntValueToArray(buffer, 105, 5); // cycle
  hexutilSetIntValueToArray(buffer, 107, 1); // JH on/off
  hexutilSetIntValueToArray(buffer, 109, 1); // CDMOS on/off (1 = on, everything else off)
  hexutilSetIntValueToArray(buffer, 111, 1); // FDMOS on/off
  hexutilSetIntValueToArray(buffer, 113, averageCellVoltage); // average voltage (in mV)
  hexutilSetIntValueToArray(buffer, 115, 4000); // differential voltage (in mV)
  hexutilSetIntValueToArray(buffer, 117, currentKw); // current KW (in W)

  // alarm 1
  buffer[119] = 0;
  buffer[120] = 0;

  // alarm 2
  buffer[121] = 0;
  buffer[122] = 0;

  // alarm 3
  buffer[123] = 0;
  buffer[124] = 0;

  // alarm 4
  buffer[125] = 0;
  buffer[126] = 0;
  
  byte crcBuffer[2];
  smartbmsutilGetCRC(crcBuffer, buffer, 127);
  buffer[127] = crcBuffer[0];
  buffer[128] = crcBuffer[1];
}*/

void smartbmsutilCreateRunInfoLastBatteryValueResponse(byte *buffer) {
  // Content length RunInfoLastBatteryValue: 32 byte (all values in 2 byte)
  // 16 values: Battery volt (in mV)
  
  buffer[0] = 0xD2;
  buffer[1] = 0x03;
  buffer[2] = 0x20; // content length: "20" = 32

  int batteryMilliVolt = MIN_CELL_VOLTAGE;
  for (int i = 0; i < 16; i++) {
    if (COUNT_BATTERIES >= i + 1) {
      batteryMilliVolt = batteryMilliVolt + 10 * i;
      hexutilSetIntValueToArray(buffer, READ_PACKET_HEADER_LENGTH + 2*i, batteryMilliVolt);
    } else {
      hexutilSetIntValueToArray(buffer, READ_PACKET_HEADER_LENGTH + 2*i, 0);
    }
  }
  
  byte crcBuffer[2];
  smartbmsutilGetCRC(crcBuffer, buffer, 35);
  buffer[35] = crcBuffer[0];
  buffer[36] = crcBuffer[1];
}

// returns true if CRC is OK
bool smartbmsutilCheckCrc(byte *buffer, int size) {
  int contentLength = buffer[2];
  if (size < READ_PACKET_OVERHEAD_LENGTH + contentLength) {
    Serial.print("smartbmsutilCheckCrc: Array length too small: ");
    Serial.println(size);
    return false;
  }
  
  byte crcBuffer[2];
  smartbmsutilGetCRC(crcBuffer, buffer, READ_PACKET_HEADER_LENGTH + contentLength);
  if (crcBuffer[0] != buffer[READ_PACKET_HEADER_LENGTH + contentLength] || crcBuffer[1] != buffer[READ_PACKET_HEADER_LENGTH + contentLength + 1]) {
    Serial.println("smartbmsutilCheckCrc: CRC does not match");
    return false;
  }
  return true;
}

// return true if packet is valid (full packet with correct header, correct crc)
bool smartbmsutilIsValidPacket(byte *buffer, int size) {
  if (size < READ_PACKET_HEADER_LENGTH) {
    Serial.print("smartbmsutilIsValidPacket: Array length too small: ");
    Serial.println(size);
    return false;
  }
  if (buffer[0] != 0xD2) {
    return false;
  }
  if (buffer[1] != 0x03) {
    return false;
  }
  return smartbmsutilCheckCrc(buffer, size);
}

// swap endians of int16_t (starting from 4th byte)
void smartbmsutilSwapBmsBytesEndian(byte *buffer, int size) {
  byte tmpValue;
  for (int i = 0; i < ((size - READ_PACKET_HEADER_LENGTH) / 2); i++) {
    tmpValue = buffer[READ_PACKET_HEADER_LENGTH + 2*i];
    buffer[READ_PACKET_HEADER_LENGTH + 2*i] = buffer[READ_PACKET_HEADER_LENGTH + 2*i + 1];
    buffer[READ_PACKET_HEADER_LENGTH + 2*i + 1] = tmpValue;
  }
}

void smartbmsutilDataReceived(byte *pData, size_t length) {
  if (length == 0) {
    // nothing to do
    return;
  }
  if (indexSmartBmsReceiveBuffer + length > RECEIVE_BUFFER_SIZE) {
    Serial.println("smartBmsReceiveBuffer too small");
    return;
  }

  for (int i = 0; i < length; i++) {
    smartBmsReceiveBuffer[indexSmartBmsReceiveBuffer] = pData[i];
    indexSmartBmsReceiveBuffer++;
  }

  Serial.print("indexSmartBmsReceiveBuffer: ");
  Serial.println(indexSmartBmsReceiveBuffer);

  Serial.print("length: ");
  Serial.println(length);

  bool isReadHeader = smartBmsReceiveBuffer[0] == 0xD2 && smartBmsReceiveBuffer[1] == 0x03;

  if (isReadHeader) {
    int contentLength = smartBmsReceiveBuffer[2];
    if (contentLength + READ_PACKET_OVERHEAD_LENGTH == indexSmartBmsReceiveBuffer) {
      Serial.println("Packet is complete");
      // packet is complete
      if (smartbmsutilIsValidPacket(smartBmsReceiveBuffer, indexSmartBmsReceiveBuffer)) {
        Serial.println("Packet is valid");
        // packet is valid
        if (smartBmsReceiveBuffer[2] == 0x7C) {
          // Packet is RunInfo
          SmartbmsutilRunInfo runInfo = smartbmsutilGetRunInfo(smartBmsReceiveBuffer, indexSmartBmsReceiveBuffer);
          smartbmsutilPrintRunInfo(runInfo);
        }
      }

      // reset smartBmsReceiveBuffer
      memset(smartBmsReceiveBuffer, 0, sizeof(smartBmsReceiveBuffer));
      indexSmartBmsReceiveBuffer = 0;
      Serial.println("Buffer reset");
    }
  }
}

// this method assumes that packet is valid (CRC checked)
SmartbmsutilRunInfo smartbmsutilGetRunInfo(byte *buffer, int size) {
  // use copy of buffer to not change original buffer for endian conversion
  byte tmpBuffer[size];
  memcpy(tmpBuffer, buffer, size);

  // swap bytes to little endian (as structs are organized in little endian in ESP32)
  smartbmsutilSwapBmsBytesEndian(tmpBuffer, size);

  // copy tmpBuffer to struct
  SmartbmsutilRunInfo result;
  memcpy(&result, tmpBuffer, sizeof(result));  
  return result;
}

void smartbmsutilWriteSmartbmsutilRunInfoToBuffer(byte *buffer, int size, SmartbmsutilRunInfo *smartbmsutilRunInfo) {
  if (size < sizeof(SmartbmsutilRunInfo)) {
    Serial.println("smartbmsutilWriteSmartbmsutilRunInfoToBuffer: buffer too small");
    return;
  }
  memcpy(buffer, smartbmsutilRunInfo, sizeof(SmartbmsutilRunInfo));

  // swap bytes to little endian (as structs are organized in little endian in ESP32)
  smartbmsutilSwapBmsBytesEndian(buffer, sizeof(SmartbmsutilRunInfo));
}

void smartbmsutilPrintRunInfo(SmartbmsutilRunInfo runInfo) {
  Serial.print("Battery voltages: ");
  for (int i = 0; i < runInfo.countBatteryVoltages; i++) {
    Serial.print(runInfo.batteryVoltages[i] / 1000.0, 3);
    Serial.print("V ");
  }
  Serial.println();

  Serial.print("Battery temps: ");
  for (int i = 0; i < runInfo.countBatteryTemp; i++) {
    Serial.print(runInfo.batteryTemp[i] - 40);
    Serial.print("°C ");
  }
  Serial.println();

  Serial.print("Max Cell voltage: ");
  Serial.println(runInfo.maxCellVoltage / 1000.0, 3);

  Serial.print("Min Cell voltage: ");
  Serial.println(runInfo.minCellVoltage / 1000.0, 3);

  Serial.print("Current voltage: ");
  Serial.println(runInfo.currentV / 10.0);

  Serial.print("Current A: ");
  Serial.println((runInfo.currentA - 30000) / 10.0);

  Serial.print("Current KW: ");
  Serial.println(runInfo.currentKw / 1000.0, 3);
}
