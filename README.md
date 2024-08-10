This project introduces the files used for integration of the Touchdetect sensor by PowerOn GmbH with Universal robot. It's a part of Abdelrhman Attya's bachelor thesis at Schmalkalden university of applied sciences 2024.

This project include work from various other github projects, including:
1- https://github.com/felis/USB\_Host\_Shield 2.0
2- https://github.com/bang-olufsen/yahdlc
3- https://github.com/poweron-gmbh/touch_detect_sdk/tree/main
4- https://github.com/epsilonrt/modbus-arduino

Structure: 
This project comprimises 4 files:
1- sensor_read.ino is the code for reading the touchdetect sensor.
2- sensor_mod_mega.ino is the code for reading the sensor data and ssending it to the UR via modbus.
3- sensitive_grip.script is the code used for sensitive gripping on the UR.
4- adjust.script is the code used for orientation adjusting on the UR.
