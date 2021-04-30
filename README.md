This repo is an archive of a project done in 2019 as a student.

The Goal was to use an ESP8266 to regularly send a status with provided sensor (°C / Gyro/ GPS* / Battery**)
The ESP8266 use the MQTT Protocol and the included WiFi chip to transmit data.

And because of me, I did *a bit* over-complicated the project (for my side at leat).
A Android MQTT Client was also done (not present here tho).

This project some third-party libs, listed bellow :<br>
<br>
CRC32                 : Arduino Lib<br>
DallasTemperature     : Arduino Lib<br>
EEPROM                : Arduino Lib<br>
ESP8266WiFi           : Arduino Lib<br>
MQTT                  : Arduino Lib<br>
OneWire               : Arduino Lib<br>
SHA3                  : Arduino Lib (?)<br>
SparkFunLSM6DS3       : https://github.com/sparkfun/SparkFun_LSM6DS3_Arduino_Library<br>
Wire                  : Arduino Lib <br>
<br>

The memory map is detailedin IOTProtect.h

Note : 

       SSL is disabled, due perfomances.
       Also, SSL is a bit overkill for a project like this.
       /!\ DO NOT USE THE SSL CERTIFIATE PROVIDED AS SAMPLE

*   GPS : The GPS module was used in one of my project parner solution (using a Onion Omega 2)
**  The battery status check is a fake °C, easier to implement
