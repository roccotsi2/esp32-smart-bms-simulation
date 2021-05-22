//#pragma pack(1)
typedef struct smartbmsutilRunInfo {
   int8_t header1;
   int8_t header2;
   int8_t contentLength;
   int16_t batteryVoltages[32];
   int16_t batteryTemp[8];
   int16_t currentV;
   int16_t currentA;
   int16_t chargePercent;
   int16_t maxCellVoltage;
   int16_t minCellVoltage;
   int16_t dummy1;
   int16_t dummy2;
   int16_t dummy3;
   int16_t dummy4;
   int16_t countBatteryVoltages;
   int16_t countBatteryTemp;
   int16_t cycle;
   int16_t jh;
   int16_t cdmos;
   int16_t fdmos;
   int16_t avgVoltage;
   int16_t diffVoltage;
   int16_t currentKw;
   int16_t alarm1;
   int16_t alarm2;
   int16_t alarm3;
   int16_t alarm4;
   int8_t crcHigh;
   int8_t crcLow;
} __attribute__ ((packed)) SmartbmsutilRunInfo;

//#pragma pack(1)
typedef struct smartbmsutilVersionInfo {
   int8_t header1;
   int8_t header2;
   int8_t contentLength;
   int8_t appVersionText[16];
   int8_t mcuVersionText[16];
   int8_t machineVersionText[32];
   int8_t crcHigh;
   int8_t crcLow;
} __attribute__ ((packed)) SmartbmsutilVersionInfo;

//#pragma pack(1)
typedef struct smartbmsutilRunInfoLastBatteryValue {
   int8_t header1;
   int8_t header2;
   int8_t contentLength;
   int16_t lastBatteryValue[16];
   int8_t crcHigh;
   int8_t crcLow;
} __attribute__ ((packed)) SmartbmsutilRunInfoLastBatteryValue;
