This repo is an archive of a project done in 2019 as a student.

The target platform is an ESP8266, with some required lib MQTT and SSL.

The memory map is detailedin IOTProtect.h

Note : SSL is disabled, due to size of the SSL Certificate.
       An older implementation tied to compress the certificate in the ROM, but 
       the performance was way not sufficient.
      
       Also, SSL is a bit overkill for a project like this.

