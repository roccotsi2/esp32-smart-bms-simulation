# esp32-smart-bms-simulation
A simple simulation of the Daly Smart BMS for ESP32 via bluetooth BLE (with Arduino). Is purpose is to use this for retrieving current values from Smart BMS without having one running.
Only same sample values are sent as response, not all things works (especially no write actions).

The number of batteries, min/max values can be apated in the "#define" section.

I used Arduino IDE for development and installed "esp32" board using board manager.

To test this simulation, you can use the "Smart BMS App" and connect via bluetooth to "DL-40D63C3223A2" (bluetooth and location must be activated): https://play.google.com/store/apps/details?id=com.inuker.bluetooth.daliy&hl=de&gl=US

Needed libraries:
- Board: esp32 (version 1.0.4 used)

