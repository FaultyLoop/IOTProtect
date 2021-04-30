#include "IOTProtect.h"

/* Actual Version 0.2 beta
 *  
 *  Future 0.3 aplha
 *  - Allow usage of multiple sector (actually only 1 sector of 4096 bytes is used)
 *    + User defined I2C address 
 *    + User defined certificates
 *    + User defined Web Page
 *    + User Reserved sector
 *  - Switch to 160Mhz CPU Frequency
 */


IOTProtect IOT;

void setup(){IOT.begin();}
void loop() {IOT.loop();}
