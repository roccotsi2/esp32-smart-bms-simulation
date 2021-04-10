# esp32-smart-bms-simulation
A simple simulation of the Daly Smart BMS for ESP32 via bluetooth BLE (with Arduino). Its purpose is to use this for retrieving current values from Smart BMS without having one running. Only same sample values are sent as response, not all things works (especially no write actions).

Following data is sent within this simulation:
- Run Info
- Run Info Last Battery Value
- Version Info

The number of batteries, min/max values can be apated in the "#define" section.

I used Arduino IDE for development and installed "esp32" board using board manager.

To test this simulation, you can use the "Smart BMS App" and connect via bluetooth to "DL-40D63C3223A2" (bluetooth and location must be activated): https://play.google.com/store/apps/details?id=com.inuker.bluetooth.daliy&hl=de&gl=US

## Needed libraries:
- Board: esp32 (version 1.0.6 used)

## Data format of used Smart BMS from Daly:
This section does not contain the full data format for Daly Smart BMS (there seems to be different Smart BMS that uses another data format, anyway), because I found no official documentation on this and therefore I analyzed the data packets. The results of my analysis are documented in this section. It seems to be the case that Smart BMS is waiting for some data (because Android App does send commands to my simulation while connected to it), but I do not know how to interpret these data yet.
This section will be updated if I get new insights on the data format.

Daly Smart BMS uses the following format to send data (data from Smart BMS to a client):

```
<HEADER_READ><CONTENT_LENGTH><CONTENT><CRC>
<-----------CRC relevant------------->
```
- HEADER_READ: always 0xD203
- CONTENT_LENGTH: content length in bytes
- CONTENT: the content (depends on the information which is sent)
- CRC: checksum (2 bytes). The checksum is calculated over HEADER_READ, CONTENT_LENGTH and CONTENT. See function "getCRC" to see how CRC is calculated

The content lengths are in bytes:
- Run Info: 124
- Run Info Last Battery Value: 32
- Set Info: 82
- Version Info: 64
- PWD: 6

### Content format Run Info
All positions needs 2 bytes:
- Position 0-31: Battery volt (in mV)batteryTemperature
- Position 32-39: Battery temperature in Celsius (offset: 40)
- Position 40: current V (in 0,1V)
- Position 41: current A (offset: 30000 in 0,1A)
- Position 42: NowValue in Percent (in 0,1 percent)
- Position 43: max cell voltage (in mV)
- Position 44: min cell voltage (in mV)
- Position 45: ???
- Position 46: ???
- Position 47: ???
- Position 48: ???
- Position 49: count battery volt
- Position 50: count battery temperatures (max. 8)
- Position 51: cycle
- Position 52: JH on/off
- Position 53: CDMOS on/off (1 = on, everything else off)
- Position 54: FDMOS on/off
- Position 55: average voltage (in mV)
- Position 56: differential voltage (in mV)   
- Position 57: current KW (in W)
- Position 58: Alarm1
- Position 59: Alarm2
- Position 60: Alarm3
- Position 61: Alarm4

### Content format Version Info
<App Version: 16 bytes as String> (reversed), <MCU Version: 16 bytes as String>, <Machine Version: 32 bytes as String>

### Content format Run Info Last Battery Value
16 values (each 2 byte): Battery volt (in mV)
