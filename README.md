
# Integration of Touchdetect sensor with Universal robot

This project introduces the code used to integrate the touchdetect sensor by PowerOn GmbH with Universal robot. 


## Structure

This project has 4 files:
- Sensor_read.ino : This is the code used to read the data from Touchdetect sensor to the arduino.
- Sensor_mod_mega.ino : This is the code used in reading the sensor data by the arduino and sending it to the Universal robot via modbus.
- Sensitive_grip.script : This is the URScript code used for sensitive gripping on the UR.
- adjust.script : This is the URScript code used for orientation adjusting on the UR.


## Acknowledgements

 This project include code from various github respitories, great thanks to: 
 - https://github.com/felis/USB%5C_Host%5C_Shield
 - https://github.com/bang-olufsen/yahdlc
 - https://github.com/poweron-gmbh/touch_detect_sdk/tree/main
 - https://github.com/epsilonrt/modbus-arduino

