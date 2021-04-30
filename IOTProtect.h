#ifndef IOTPROTECT
#define IOTPROTECT
#include "Arduino.h"
#include <ESP8266WiFi.h>
#include <MQTT.h>

#define __VERSION__ "0.2_BETA"

#ifndef IOTPROTECT_COMMAND_FLAG
#define IOTPROTECT_COMMAND_FLAG

#define FLAG_COMMAND_NULNUL 0b00000000 
//                          MODE            NORMAL           ; ALTERNATIVE 
#define FLAG_COMMAND_RSTHLP 0b1000000000000000 //!< NORM :     RESET ; ALT :      HELP
#define FLAG_COMMAND_EXTSYN 0b0100000000000000 //!< NORM :      EXIT ; ALT :      SYNC
#define FLAG_COMMAND_SENCLS 0b0010000000000000 //!< NORM :      SEND ; ALT :     CLEAR
#define FLAG_COMMAND_STAFLA 0b0001000000000000 //!< NORM :    STATUS ; ALT :     FLAGS
#define FLAG_COMMAND_RESREB 0b0000100000000000 //!< NORM :   RESTART ; ALT :    REBOOT
#define FLAG_COMMAND_ENADIS 0b0000010000000000 //!< NORM :    ENABLE ; ALT :   DISABLE
#define FLAG_COMMAND_GETSET 0b0000001000000000 //!< NORM :       GET ; ALT :       SET
#define FLAG_COMMAND_UPPALT 0b0000000100000000 //!< UPPER ALTERNATIVE BIT

#define FLAG_COMMAND_FINGER 0b0000000000010110 //!< NORM :FINGERPRINT; ALT :     UNSET
#define FLAG_COMMAND_SLPTIM 0b0000000000010100 //!< NORM :     SLEEP ; ALT :   TIMEOUT
#define FLAG_COMMAND_MQTI2C 0b0000000000010010 //!< NORM :      MQTT ; ALT :       I2C
#define FLAG_COMMAND_LIPLOW 0b0000000000010000 //!< NORM :  LOCAL IP ; ALT :      PORT
#define FLAG_COMMAND_SNSPIN 0b0000000000001110 //!< NORM :   SENSORS ; ALT :       PIN
#define FLAG_COMMAND_TRIEVE 0b0000000000001100 //!< NORM :   TRIGGER ; ALT :    EVENTS
#define FLAG_COMMAND_DEPHYB 0b0000000000001010 //!< NORM : DEEPSLEEP ; ALT :    HYBRID
#define FLAG_COMMAND_STRLOG 0b0000000000001000 //!< NORM :    STRING ; ALT :     LOGON
#define FLAG_COMMAND_CRCBAK 0b0000000000000110 //!< NORM :       CRC ; ALT :     BAKUP
#define FLAG_COMMAND_TELSEC 0b0000000000000100 //!< NORM :    TELNET ; ALT :    SECURE
#define FLAG_COMMAND_SERWIF 0b0000000000000010 //!< NORM :    SERIAL ; ALT :      WIFI
#define FLAG_COMMAND_LOWALT 0b0000000000000001 //!< LOWER ALTERNATIVE BIT

#endif  //IOTPROTECT_COMMAND_FLAG

#ifndef IOTPROTECT_MATCH_FLAG
#define IOTPROTECT_MATCH_FLAG

#define FLAG_MATCH_INVALID 0b00000001
#define FLAG_MATCH_AMBIGUS 0b00000011
#define FLAG_MATCH_STRVOID 0b00000101
#define FLAG_MATCH_DROPPED 0b00000111

#endif  //IOTPROTECT_MATCH_FLAG

#ifndef IOTPROTECT_EVENT_FLAG
#define IOTPROTECT_EVENT_FLAG

#define FLAG_EVENT_ECNOP  0b00000000 //!< Event Code NO ERROR / NO OPERATION
#define FLAG_EVENT_ECFAL  0b10000000 //!< Event Code FAIL
#define FLAG_EVENT_ECWRT  0b01000000 //!< Event Code WRITE
#define FLAG_EVENT_ECTST  0b00100000 //!< Event Code TEST
#define FLAG_EVENT_ECRED  0b00010000 //!< Event Code READ

#define FLAG_EVENT_EFBOOT 0b00000001 //!< Event Flag BOOT
#define FLAG_EVENT_EFMSG_ 0b00000010 //!< Event Flag MESSAGE
#define FLAG_EVENT_EFRST_ 0b00000011 //!< Event Flag RESET
#define FLAG_EVENT_EFSLP_ 0b00000100 //!< Event Flag SLEEP
#define FLAG_EVENT_EFEEPM 0b00000101 //!< Event Flag EEPROM
#define FLAG_EVENT_EFTRIG 0b00000110 //!< Event Flag TRIGGER
#define FLAG_EVENT_EFEVNT 0b00000111 //!< Event Flag EVENT
#define FLAG_EVENT_EFADMN 0b00001000 //!< Event Flag ADMIN
#define FLAG_EVENT_EFBACK 0b00001001 //!< Event Flag BACKUP
#define FLAG_EVENT_EFCNFG 0b00001010 //!< Event Flag CONFIG
#define FLAG_EVENT_EFINTY 0b00001011 //!< Event Flag INTEGRITY

#endif //IOTPROTECT_EVENT_FLAG

#ifndef IOTPROTECT_MEMORY_MAP
#define IOTPROTECT_MEMORY_MAP

#define EEPROM_SIZE_FULL 0xFFF // Full EEPROM   size
#define EEPROM_SIZE_CLOW 0x002 // Common Low    size
#define EEPROM_SIZE_CMED 0x004 // Common Medium size
#define EEPROM_SIZE_CFNG 0x014 // Common Finger size
#define EEPROM_SIZE_CSTR 0x020 // Common String size

#define EEPROM_HEAD_CONF  0x000 //!< 0x000 --- REFERENCE ---                 +SERVICE CONFIG AREA+-------------------+
#define EEPROM_ADDR_STAT  0x000 //!< 0x000 -> 0x003 Status Flag              |                   |                   |
#define EEPROM_ADDR_EVCN  0x004 //!< 0x004 -> 0x005 Event  Counter           |                   |                   |
#define EEPROM_ADDR_BKCN  0x006 //!< 0x006 -> 0x007 Backup  Counter          |                   |                   |
#define EEPROM_ADDR_HRES  0x008 //!< 0x008 -> 0x009 RESERVED                 |                   |                   |

#define EEPROM_HEAD_CRC   0x010 //!< 0x010 --- REFERENCE ---                 + SERVICE CRC AREA  +
#define EEPROM_ADDR_OCRC  0x010 //!< 0x010 -> 0x013 Old CRC                  |                   |                   |

#define EEPROM_HEAD_LOWS  0x014 //!< 0x014 --- REFERENCE ---                 +   LOW SIZE  AREA  +-------------------+
#define EEPROM_ADDR_SSPO  0x014 //!< 0x014 -> 0x015 Server Service Port      |                   |SERVER SERVICE PORT|
#define EEPROM_ADDR_SMPO  0x016 //!< 0x016 -> 0x017 Server Maintenance Port  |                   |SERVER SERVICE PORT|
#define EEPROM_ADDR_SLPO  0x018 //!< 0x018 -> 0x019 Server SSL Port          |                   |SERVER SERVICE PORT|
#define EEPROM_ADDR_STSD  0x01A //!< 0x01A -> 0x01B Sercice Trace Sleep Delay|                   |SERVICE TRACE DELAY|
#define EEPROM_ADDR_MTOT  0x01C //!< 0x01C -> 0x01D MQTT Timeout             |                   |SERVICE TRACE DELAY|
#define EEPROM_ADDR_PINS  0x01E //!< 0x01E -> 0x01F PIN Enable               |                   |-------------------|
#define EEPROM_TAIL_LOWS  0x01F //!< 0x01F --- REFERENCE ---                 |                   |                   |

#define EEPROM_HEAD_STR   0x020 //!< 0x020 --- REFERENCE ---                 |                   |                   |
#define EEPROM_HEAD_HSEC  0x020 //!< 0x020 --- REFERENCE ---                 +-------------------+-------------------+
#define EEPROM_ADDR_MIDN  0x020 //!< 0x020 -> 0x03F MQTT Identifier          |                   |                   |
#define EEPROM_ADDR_SADR  0x040 //!< 0x040 -> 0x05F Server Address           |                   |                   |
#define EEPROM_ADDR_MPSS  0x060 //!< 0x060 -> 0x07F MQTT Pass                |                   |                   |
#define EEPROM_ADDR_LWPS  0x080 //!< 0x080 -> 0x09F Local WiFi Pass          |                   |                   |
#define EEPROM_ADDR_MLIN  0x0A0 //!< 0x0A0 -> 0x0BF MQTT Login               |                   |                   |
#define EEPROM_ADDR_LSID  0x0C0 //!< 0x0C0 -> 0x0DF Local WiFi SSID          |                   |                   |
#define EEPROM_ADDR_MTOP  0x0E0 //!< 0x0E0 -> 0x0FF (Main) Topic MQTT        |                   |                   |
#define EEPROM_TAIL_HSEC  0x0FF //!< 0x0FF --- REFERENCE ---                 +-------------------+-------------------+

#define EEPROM_HEAD_SPTS  0x100 //!< 0x100 --- REFERENCE ---                 +-------------------+-------------------+
#define EEPROM_ADDR_SP00  0x100 //!< 0x100 -> 0x11F SENSOR PIN0 TOPIC        |                   |                   |
#define EEPROM_ADDR_SP01  0x120 //!< 0x120 -> 0x13F SENSOR PIN1 TOPIC        |                   |                   |
#define EEPROM_ADDR_SP02  0x140 //!< 0x140 -> 0x15F SENSOR PIN2 TOPIC        |                   |                   |
#define EEPROM_ADDR_SP03  0x160 //!< 0x160 -> 0x17F SENSOR PIN3 TOPIC        |                   |                   |
#define EEPROM_ADDR_SP04  0x180 //!< 0x180 -> 0x19F SENSOR PIN4 TOPIC        |                   |                   |
#define EEPROM_ADDR_SP05  0x1A0 //!< 0x1A0 -> 0x1BF SENSOR PIN5 TOPIC        |                   |                   |
#define EEPROM_ADDR_SP06  0x1C0 //!< 0x1C0 -> 0x1DF SENSOR PIN6 TOPIC        |                   |                   |
#define EEPROM_ADDR_SP07  0x1E0 //!< 0x1E0 -> 0x1FF SENSOR PIN7 TOPIC        |                   |                   |
#define EEPROM_TAIL_SPTS  0x1FF //!< 0x1FF --- REFERENCE ---                 +-------------------+-------------------+

#define EEPROM_HEAD_CRIT  0x200 //!< 0x200 --- REFERENCE ---                 +-------------------+-------------------+
#define EEPROM_ADDR_ISHA  0x200 //!< 0x200 -> 0x21F Internal Pass    (SHA256)|                   |                    |
#define EEPROM_TAIL_CRIT  0x21F //!< 0x21F --- REFERENCE ---                 +-------------------+-------------------+
#define EEPROM_TAIL_STR   0x21F //!< 0x21F --- REFERENCE ---                 +-------------------+-------------------+
#define EEPROM_TAIL_CRC   0x21F //!< 0x21F --- REFERENCE ---                 +-------------------+-------------------+
#define EEPROM_TAIL_CONF  0x21F //!< 0x21F --- REFERENCE ---                 +-------------------+-------------------+

#define EEPROM_HEAD_FING  0x220 //!< 0x220 --- REFERENCE ---                 +-------------------+-------------------+
#define EEPROM_ADDR_FPRT  0x220 //!< 0x220 -> 0x233 Finger Print             |                   |                   |
#define EEPROM_TAIL_FING  0x234 //!< 0x234 --- REFERENCE ---                 +-------------------+-------------------+

#define EEPROM_HEAD_MEDS  0x234 //!< 0x220 --- REFERENCE ---                 +-------------------+-------------------+
#define EEPROM_ADDR_ICRC  0x234 //!< 0x234 -> 0x237 CRC                      |                   |                   |
#define EEPROM_ADDR_MRES  0x238 //!< 0x238 -> 0x2FF RESERVED                 |                   |-------------------|
#define EEPROM_TAIL_MEDS  0x2FF //!< 0x23F --- REFERENCE ---                 +-------------------+-------------------+

#define EEPROM_HEAD_TRIG  0x300 //!< 0x300 --- REFERENCE ---                 +-------------------+-------------------+
#define EEPROM_ADDR_TMAP  0x300 //!< 0x300 -> 0x4FF Trigger Map              |32 BYTES PER ENTITY| 16 MAX ENTITY     |
#define EEPROM_TAIL_TRIG  0x4FF //!< 0x4FF --- REFERENCE ---                 +-------------------+-------------------+

#define EEPROM_HEAD_BFSD  0x500 //!< 0x500 --- REFERENCE ---                 +-------------------+-------------------+
#define EEPROM_ADDR_BFSD  0x500 //!< 0x500 -> 0x6FF Bakup Failed Message     | 3 BYTES PER ENTITY| 170 MAX ENTITY    |
#define EEPROM_TAIL_BFSD  0x6FF //!< 0x6FF --- REFERENCE ---                 +-------------------+-------------------+

#define EEPROM_HEAD_EVNT  0x700 //!< 0x700 --- REFERENCE ---                 +-------------------+-------------------+
#define EEPROM_ADDR_EMAP  0x700 //!< 0x700 -> 0xFFF Event Map                | 8 BYTES PER ENTITY| 288 MAX ENTITY    |
#define EEPROM_TAIL_EVNT  0xFFF //!< 0xFFF --- REFERENCE ---                 +-------------------+-------------------+

#endif //IOTPROTECT_MEMORY_MAP

#ifndef IOTPROTECT_TRIGGER
#define IOTPROTECT_TRIGGER

#define FLAG_TRIGGER_GTHN 0b0001 //!< UNSET : NONE     ; SET : GRATER THAN * if equal is set and less -> <=
#define FLAG_TRIGGER_LTHN 0b0010 //!< UNSET : NONE     ; SET : LESS THAN   * if equal is set and grater is set -> >=
#define FLAG_TRIGGER_EQTY 0b0100 //!< UNSET : NONE     ; SET : EQUAL
#define FLAG_TRIGGER_XNOT 0b1000 //!< UNSET : NONE     ; SET : NOT
#define FLAG_TRIGGER_DISD 0b1111 //!< UNSET : NONE     ; SET : DISABLED
 
#endif //IOTPROTECT_TRIGGER

#ifndef IOTPROTECT_STATUS_FLAG
#define IOTPROTECT_STATUS_FLAG

#define FLAG_STATUS_SYSET 0b100000000000000000000000 //!< System    Mode (System override for system command                                    )
#define FLAG_STATUS_ADMIN 0b010000000000000000000000 //!< Admin     Mode (Administrator Mode ONLY IN CONFIG MODE !                              )
#define FLAG_STATUS_SAFEM 0b001000000000000000000000 //!< Safe      Mode (Default behaviour plus logs, Serial and Telnet interface enabled      )
#define FLAG_STATUS_CONFG 0b000100000000000000000000 //!< Config Enabled (Enable Config  WARNING : FLAG SET AUTOMATICALLY IF CONFIG FAILED      )

#define FLAG_STATUS_ICSET 0b000000000010000000000000 //!< I2C    Enabled (Enable I2C          WARNING : DEPENDENT OF SNSET                      )
#define FLAG_STATUS_RESET 0b000000000001000000000000 //!< Reset  Enabled (Enable Reset        WARNING : EFFECT TRIGGER IN BOOT ONLY             )
#define FLAG_STATUS_SNSET 0b000000000000100000000000 //!< Sensor Enabled (Enable Sensors      WARNING : IF DISABLED, ONLY BSSID WILL BE SENT    )
#define FLAG_STATUS_WFSET 0b000000000000010000000000 //!< WiFi   Enabled (Enable WiFi         WARNING : IF DISABLED, MQTT UNAVAIBLE SEE EFFECT  )
#define FLAG_STATUS_SMSET 0b000000000000001000000000 //!< Secure Enabled (Enable Secure MQTT  WARNING : FINGER PRINT NEEDED                     )
#define FLAG_STATUS_MQSET 0b000000000000000100000000 //!< MQTT   Enabled (Enable MQTT         WARNING : IF DISABLED, USE INTERFACES AND EEPROM  )
#define FLAG_STATUS_HDSET 0b000000000000000010000000 //!< Hybrid Enabled (Enable behaviour swapping                                             )
#define FLAG_STATUS_DPSET 0b000000000000000001000000 //!< Deepsl Enabled (Enable DeepSleep Behaviour NOTE : DEFAULT IS ECO MODE (NO FLAG)       )
#define FLAG_STATUS_MMSET 0b000000000000000000100000 //!< Memlog Service (Enable Message log WARNING : ONLY IN CASE OF MESSAGE FAILURE          )
#define FLAG_STATUS_SLSET 0b000000000000000000010000 //!< SSL    Service (Enable SSL    Interface WARNING : TELNET INTERFACE DISABLED           )
#define FLAG_STATUS_TNSET 0b000000000000000000001000 //!< Telnet Service (Enable Telnet Interface WARNING : RAW BYTES STREAM                    )
#define FLAG_STATUS_SRSET 0b000000000000000000000100 //!< Serial Service (Enable Serial Interface                                               )
#define FLAG_STATUS_TRSET 0b000000000000000000000010 //!< Trigger    Set
#define FLAG_STATUS_WAKEP 0b000000000000000000000001 //!< Wakup      Set

#endif //IOTPROTECT_STATUS_FLAG

#ifndef IOTPROTECT_XDATA
#define IOTPROTECT_XDATA

const uint8_t  PINMAP[8] PROGMEM = {D1, D2, D3, D4, D5, D6, D7, D8};

const uint16_t EEPROM_AREA_CONF[2] PROGMEM = {EEPROM_HEAD_CONF, EEPROM_TAIL_CONF}; //!< Area Config
const uint16_t EEPROM_AREA_CRC[2]  PROGMEM = {EEPROM_HEAD_CRC,  EEPROM_TAIL_CRC }; //!< Area CRC
const uint16_t EEPROM_AREA_LOWS[2] PROGMEM = {EEPROM_HEAD_LOWS, EEPROM_TAIL_LOWS}; //!< Area Low Size
const uint16_t EEPROM_AREA_STR[2]  PROGMEM = {EEPROM_HEAD_STR,  EEPROM_TAIL_STR }; //!< Area String
const uint16_t EEPROM_AREA_HSEC[2] PROGMEM = {EEPROM_HEAD_HSEC, EEPROM_TAIL_HSEC}; //!< Area Secure
const uint16_t EEPROM_AREA_MEDS[2] PROGMEM = {EEPROM_HEAD_MEDS, EEPROM_TAIL_MEDS}; //!< Area Medium Size
const uint16_t EEPROM_AREA_TRIG[2] PROGMEM = {EEPROM_HEAD_TRIG, EEPROM_TAIL_TRIG}; //!< Area Triggers
const uint16_t EEPROM_AREA_EVNT[2] PROGMEM = {EEPROM_HEAD_EVNT, EEPROM_TAIL_EVNT}; //!< Area Events
const uint16_t EEPROM_AREA_BFSD[2] PROGMEM = {EEPROM_HEAD_BFSD, EEPROM_TAIL_BFSD}; //!< Area Backup Field
const uint16_t EEPROM_AREA_FING[2] PROGMEM = {EEPROM_HEAD_FING, EEPROM_TAIL_FING}; //!< Area Finger print

const char STR_MSG_ENABLED[ ] PROGMEM = "ENABLED ";
const char STR_MSG_DISABLE[ ] PROGMEM = "DISABLED";
const char STR_MSG_SUCCESS[ ] PROGMEM = "SUCCESS";
const char STR_MSG_FAILURE[ ] PROGMEM = "FAILURE";
const char STR_MSG_STATUS_[ ] PROGMEM = "WAKUP  ,\0TRIGGER,\0SERIAL ,\0TELNET ,\0SSL    ,\0MEMLOG ,\0DPSLEEP,\0HYBRID ,\0MQTT   ,\0S MQTT ,\0WIFI   ,\0SENSOR ,\0RESET  ,\0I2C    ,\0       ,\0       ,\0       ,\0       ,\0       ,\0       ,\0CONFIG ,\0SF MODE,\0ADMIN  ,\0SYSTEM ,\0";
const char STR_MSG_SESSION[ ] PROGMEM = "SERIAL \0TELNET_\0SECURE_";

const char STR_CMD_HEADERS[ ] PROGMEM = "clear;disable;enable;flags;get;reboot;restart;send;set;status;logon;help;reset;exit;sync;swap;";
//         UNBOUND    COMMAND : clear, flags, help, reboot, send
//         PRIMAL     COMMAND : set, get
const char STR_CMD_HPRIMAL[ ] PROGMEM = "backup;i2c;sensors;pin;crc;logon;port;sleep;ip;string;timeout;trigger;event;fingerprint;";
//         UNBOUND   OPTIONS  : backup
//         STRING    OPTIONS 
const char STR_CMD_OPTSSTR[ ] PROGMEM = "address;pass;ssid;mqident;mqpass;mqtopic;mquser;";
//         PORTS     OPTIONS
const char STR_CMD_OPTPORT[ ] PROGMEM = "maintenance;service;secure;";
//         SECONDARY  COMMAND : disable, enable restart, start, status, stop
const char STR_CMD_HSECOND[ ] PROGMEM = "backup;i2c;sensors;pin;deepsleep;hybrid;secure;mqtt;serial;telnet;wifi;mqsecure;";
//         UNBOUND   OPTIONS  : 

const char STR_SSL_CERT[ ] PROGMEM = R"EOF(
-----BEGIN CERTIFICATE-----
MIID5DCCAsygAwIBAgIJAJqSN+SdX6L3MA0GCSqGSIb3DQEBCwUAMIGGMQswCQYD
VQQGEwJGUjEPMA0GA1UECAwGRnJhbmNlMQ4wDAYDVQQHDAVQYXJpczEPMA0GA1UE
CgwGSU9UU1VOMREwDwYDVQQLDAhERVZMRVZFTDENMAsGA1UEAwwERlFETjEjMCEG
CSqGSIb3DQEJARYUZGV2LWxldmVsQGlvdHN1bi5idHMwHhcNMTkwNTIzMDgyMzE3
WhcNMjIwNTIyMDgyMzE3WjCBhjELMAkGA1UEBhMCRlIxDzANBgNVBAgMBkZyYW5j
ZTEOMAwGA1UEBwwFUGFyaXMxDzANBgNVBAoMBklPVFNVTjERMA8GA1UECwwIREVW
TEVWRUwxDTALBgNVBAMMBEZRRE4xIzAhBgkqhkiG9w0BCQEWFGRldi1sZXZlbEBp
b3RzdW4uYnRzMIIBIjANBgkqhkiG9w0BAQEFAAOCAQ8AMIIBCgKCAQEAzlITWUz0
YyJGSk3pOwOBHP0Ruf+mb2DLJg7TvUufUfdGleys73j4bs6WewHcpWOleMbocgUG
BkSWpbKXx8pkVyBxKZPE0ZidE4vEUNkKkmaKJnZ0fr53xFBvXvtLPMHCKbzdGu+e
sRz8tLr3SMP7Zgr9aZWVJ1KcUpuXDqaXZ88SoMxiafXcy9UGmF43gpotLzq+cIfr
NT6zw8Kk3XzNN2qfc+HE2HG8Vvvc6/Fi9JDw5xGhw6DWVSthGGyoR9CJuCJiYQoC
/jMfWuHCL4WOARXihMZze7qGxPys+cGco4l5WrsU0IvrsPgoOAexbu0ObVHbTOlf
s/fCoabKZzTk7QIDAQABo1MwUTAdBgNVHQ4EFgQUUXIazdtDMFaZwe/7/g8OZgRa
iBMwHwYDVR0jBBgwFoAUUXIazdtDMFaZwe/7/g8OZgRaiBMwDwYDVR0TAQH/BAUw
AwEB/zANBgkqhkiG9w0BAQsFAAOCAQEAtLl+3smw0d54dEtG2VKCtAFhQOaxjgq7
prkUON5X/xKDHbJwuNJhGZ1gl2qaLN7cyr0aH/BTDLVKcI2XCpX0hm6g02lQjKl+
yBUEpY3FR2839voNNjcdRCzNoI9qw4WP6rTQgGs7BLY2Eg2F9Eth2kCOerwY0D5Y
daBtyV6AnC4zLy/jbcktK1x75Jh0J0uWnq6HBTewKx9HOl/Tgm366myLwjgB61c9
YPnTCbJGAusPQ3NTGSkyQHBFRjVBFISb/g3LwI80OpDWrHGnW9ear9nSZFvyAzQz
dklLifm9bgI8Olj26qROz+dS+jb+EAuGXuqlFibAlb63cQcDs+IBZg==
-----END CERTIFICATE-----)EOF";

const char STR_SSL_PKEY[ ] PROGMEM = R"EOF(
-----BEGIN PRIVATE KEY-----
MIIEwAIBADANBgkqhkiG9w0BAQEFAASCBKowggSmAgEAAoIBAQDOUhNZTPRjIkZK
Tek7A4Ec/RG5/6ZvYMsmDtO9S59R90aV7KzvePhuzpZ7AdylY6V4xuhyBQYGRJal
spfHymRXIHEpk8TRmJ0Ti8RQ2QqSZoomdnR+vnfEUG9e+0s8wcIpvN0a756xHPy0
uvdIw/tmCv1plZUnUpxSm5cOppdnzxKgzGJp9dzL1QaYXjeCmi0vOr5wh+s1PrPD
wqTdfM03ap9z4cTYcbxW+9zr8WL0kPDnEaHDoNZVK2EYbKhH0Im4ImJhCgL+Mx9a
4cIvhY4BFeKExnN7uobE/Kz5wZyjiXlauxTQi+uw+Cg4B7Fu7Q5tUdtM6V+z98Kh
pspnNOTtAgMBAAECggEBAK899WNHQtik9xELxgsy+cqWhST3qPU3QLWtqFlDTyp+
nnyfT3ADzvfHDveh6DiuP5ErWanm7GwMe+x3mgW/uaRrUNLgyS7BssE0WOXWw+z/
nV34BRDWVMHxE/eX+Bq99F4hJahIWQDUvyv/FiS/Gdxdf0rcG5kPyOk9cHuoMjef
myax0n2vrMoPAlL2WHGqROyJYVVjimaQdcLC1/SspspBzHI9zGvRHsNkLwtX9mpi
QSd3Gyv52Xuq+r/VCUEb0ASTY5Bihv0wM+0voi3E2lpK0wOqtGEHyW6eud2P1bIE
aeu2rZZ0SZUsJjYcMr2dsJNzNQDQYdfGCozDc6eMSP0CgYEA5yY+/DdAC4fct+ZS
nLGWCMfpAJaNYqBxHdxnE1+02/lVgD9MR8NRR3vDDCnNObufgSfSfX/JQqh7WCEY
VFLMdDAbFCKSNSocotMAqiNXEBqZAv3TdvnmqlcjySBXgYvmxib+MFA7xPajeQ5h
OeMyOwh7xdSpkWTO9HlQVYJ9kfsCgYEA5IB8Lz0IuZY34/2zDN/ujoGyimsBvbQA
v01o9QdF0okqrIRQjCVUubDgCRfL5jbbUVcUV0tiXunDl6dZqGERV572KizyEwaC
ffWSE+8tCuYJSV1pr05Swxd28Igl4kvNArLkJdLvAfiUJKtE/j2M6Lfx2HgjGKZ+
ab/zct6xGDcCgYEA31yc5pn5IGtUBLzjPZl5ctjCthfFXfXN+N3/GMCWAy7nyTrh
WkNKKnpUm09ViiFc27CAfVdbQzeNoTaxzOIJwYiu0gEqKYedDlY5QS9PNTeOfk6K
5mBL3nTAFt1S0dM/2GDbphIR/52ZkCQuHYMHCfaDrEJ47Q9t0N3dnBOxhbsCgYEA
3DUOeJ/DJscjua4mvYOY7PEE1Oxm0yEDj1jUiz6lBU53N6aT0VuwmPtiDKFIOOYo
2eynh7D4biut+RqoBuL0yZJB/UcvSzJ801Kzok5fBB/IV1GUpgM/MxAL6aXrjHgs
bCtmPyVWfHnImsYLSVCb2TJb+Zddi0X4kYUQ4mTGGmUCgYEAvU9G/V7+dT7xhoDh
H6jBnbe/duInrOAg+4kePCPQkwO7CghRN4gNQgCbKy0XtQ54+RDeQVOlQHM0rg5A
CXGcQ2RkH+q2Jtf4qWO25v3ioY4lQZJyRVIDZsC1M/fvDP0R+qm6J3l3oumPhQPz
n9h+BjTOMNGIrAQY8d1A7BZujeQ=
-----END PRIVATE KEY-----)EOF";

const char STR_MSG_WELCOME[ ] PROGMEM ="\n\r\
╬═════════════════════════════════════════════════════════════════════════════╬\n\r\
║░░░░░░▒▒▒▒▒▒▒░░░░░░▓▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒║\n\r\
║░░░░▓▓▓▓▓▓▓▓▓▓▓░░░░▓▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒║\n\r\
║░░▓▓▓▓▒▒░░░▒▒▓▓▓▓░░▓▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒║\n\r\
║░░░░░░░░░░░░░░░░░░░▓▒▒██████▒▒▒████░▒▒██████░▒▒▒█████▒▒▒▒█░▒▒▒▒█░░▒▒█▒▒▒▒▒█░▒║\n\r\
║░░░░░░░▒▒▒▒▒░░░░░░░▓▒▒▒▒██░░▒▒█▌▒░▐█░▒▒▒██░░▒▒▒█░░▒▒▒▒▒▒▒█░▒▒▒▒█░░▒▒██▒▒▒▒█░▒║\n\r\
║░░░░░▓▓▓▓▓▓▓▓▓░░░░░▓▒▒▒▒██░░▒▒█▒░░░█░▒▒▒██░▒▒▒▒█░▒▒▒▒▒▒▒▒█░▒▒▒▒█░░▒▒█░█▒▒▒█░▒║\n\r\
║░░░▓▓▓▒░░░░░▒▓▓▓░░░▓▒▒▒▒██░░▒▒█▒░░░█░▒▒▒██░▒▒▒▒▒█████░▒▒▒█░▒▒▒▒█░░▒▒█▒░█▒▒█░▒║\n\r\
║░░░░░░░░▒▒▒░░░░░░░░▓▒▒▒▒██░░▒▒█▒░░░█░▒▒▒██░▒▒▒▒▒▒▒▒▒▒█░▒▒█░░▒▒▒█░░▒▒█▒▒░█▒█░▒║\n\r\
║░░░░░░░▓▓▓▓▓░░░░░░░▓▒▒▒▒██░░▒▒█▌▒░▐█░▒▒▒██░▒▒▒▒▒▒▒▒▒▒█░▒▒█░░░▒▒█░░▒▒█▒▒▒░██░▒║\n\r\
║░░░░░▓▓▒▒░▒▒▓▓░░░░░▓▒▒██████░▒▒████░▒▒▒▒██░▒▒▒▒▒█████░▒▒▒▒█████░░▒▒▒█▒▒▒▒░█░▒║\n\r\
║░░░░░░░░░░░░░░░░░░░▓▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒║\n\r\
║░░░░░░░▒▓▓▓▒░░░░░░░▓▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒║\n\r\
║░░░░░▒▒▓░▒░▓▒▒░░░░░▓▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒║\n\r\
║░░░░░░░▒▓▓▓▒░░░░░░░▓▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒║\n\r\
╬═════════════════════════════════════════════════════════════════════════════╬\n\r\
VERSION: 0.2 BETA © CLÉMENT RÉMY\n\r";
/*1316 bytes used (Can be reduced to 329 bytes via the folowing CHARSET "\0░▒▓█▐▌═║╬\n\r" (exclusion of the last line)
 *The following algorithme can encode the data:
 *  vars:
 *    CHARSET (str)
 *    CHARMAP (str)
 *    TRANSFORMED (str)
 *  begin:
 *    determine the number of bit needed to encode via CHARSET as BITPERCHAR
 *    set the index for TRANSFORMED to 0
 *    for each char in CHARMAP:
 *      get the char referenced in CHARSET as CURRENTCHARREF
 *      get th last reference into TRANSFORMED, if none then get 0 at LASTCHARREF
 *      for each bit of the CURRENTCHARREF
 *        if LASTCHARREF was assigned bellow BITPERCHAR
 *          do a leftshift assignment into LASTCHARREF current index
 *        else
 *          set the current TRANSFORMED index with LASTCHARREF
 *          add 1 to the index of TRANSFORMED
 *          do a leftshift with assigment into LASTCHARREF current index
 *    return TRANSFORMED
 *   notes:
 *    This algorithme exchange tow constraint : The CPU load and a low Memory (EEPROM/RAM).
 *    Since the ESP8266 has a better CPU and more memory than a basic Arduino, the performance wasen't significant.
 *    Addeding the fact that this is only used by the WELCOME logo, the integation is more.
 *    This is just a possibility.
 */

#endif //IOTPROTECT_XDATA

class IOTProtect {
  public:
    IOTProtect();
    void begin();
    void loop();
    void reset();
    ~IOTProtect();

  protected:
    enum interfaces {
      TELNET_0,TELNET_1,TELNET_2,TELNET_3,
      TELNET_4,TELNET_5,TELNET_6,TELNET_7,
      SERIAL_0 = 128,
      ALL = 255
    };

    enum maxval {
      TRIGGER,EVENT,BACKUP
    };

    typedef struct _backup {
      uint16_t value;//float converted to uint16_t precision loss negligible
      uint8_t  trpin;//Trigger id  + pin(4 bit trigger (0-15), 4 bit pin (0-7 pin, 8-10 acc,11-13 gyro)
    } _backup;
    
    typedef struct _event{//[14 bit h |5bit min | 5bit s | 16 bit event | 24 bit status]
      uint32_t time  ;
      uint32_t status;
    } _event;

    typedef struct _trigger {
      char topic[25]      ;
      uint8_t pin     =  0;
      uint8_t trap    =  0;
      uint16_t valpri =  0;
      uint16_t valsec =  0;
      uint8_t alttime = 10;
    } _trigger;

    typedef struct _session {
      String stream = "";       //Session Command Stream
      uint32_t status;          //Session Status
      WiFiClient client;        //Telnet/SSL Session Client
      bool operator==(_session &session){return (uint64_t) (this) == (uint64_t) (&session);}
    } _session;
    
    float pinbuf[8];
    float i2cbuf[6];
    
    bool clear(uint16_t area[2]);
    bool access(uint16_t size,uint16_t addr,_session *session = NULL);
    bool hashkey(String key,uint8_t *hash);
    void print(String message,_session *session = NULL);
    void println(String message,_session *session = NULL);
    void print(String message,interfaces id);
    void println(String message,interfaces id);
    int8_t flacmp(uint8_t* buffer,uint16_t size,uint16_t addr);
    bool setPin(uint8_t pin);
    uint8_t getPin();

    bool sendBackup();
    bool sendSensors();
    bool readSensors();

    bool setFingerPrint(String &fingerprint);
    String getFingerPrint();
    
    void addBackup(_backup backup);
    _backup getBackup(uint8_t id);
    
    void addEvent(uint8_t event);
    _event getEvent(uint16_t id);
    
    bool addTrigger (_trigger trigg);
    bool ediTrigger (uint8_t  id,_trigger trigg);
    bool testTrigger(_trigger trigg,bool read = false);
    _trigger getTrigger(uint8_t id);
    
    bool setString(uint16_t addr,String  str);
    bool setLow(uint16_t value,uint16_t addr);
    bool setMed(uint32_t value,uint16_t addr);
    bool setEEPROM(uint8_t *buffer,uint16_t size,uint16_t addr);
    
    String getString(uint16_t addr);
    uint32_t  getMed(uint16_t addr);
    uint16_t  getLow(uint16_t addr);
    void getEEPROM(uint8_t *buffer,uint16_t size,uint16_t addr);
    
    void checkCRC();
    void checkConf();
    bool checkPass(String passwd);
    bool checkEEPROM();

    bool setCRC();
    uint32_t getCRC();
    
  private:
    _session externSession[8];
    WiFiServerSecure secureServer = WiFiServerSecure(22);
    WiFiServer telnetServer = WiFiServer(23);
    WiFiClient client;
    WiFiClientSecure secureclient;
    MQTTClient mqtt;
    
    const String   DEFAULT_IDENT = "ESP_DEFAULT";
    const String   DEFAULT_KEY   = "admin";
    const uint16_t DEFAULT_SSPO  = 1883;
    const uint16_t DEFAULT_SMPO  = 23;
    const uint16_t DEFAULT_SLPO  = 22;
    const uint16_t DEFAULT_STSD  = 10;
    const uint16_t DEFAULT_MTOT  = 10;
    
    String _stream;
    uint16_t updatetick =  0;
    uint16_t sleeptime = 10;
    bool eventlock = false;
    bool serverstart = false;
    bool isclientssl = false;
    bool isserverssl = false;
    bool sendstate   = true;
    
    uint8_t activeTrigger = 0;

    void sleep();
    void command(String &stream,_session *session = NULL);
    uint32_t getFlag(_session *session=NULL);
    uint16_t getMax(maxval id);
    uint8_t match(const String &string,const String parser,String &matching);
    bool setInternalKey(String &key);
    bool setInternalKey(uint8_t *hash);
    bool update();
    bool takesystem();
    bool issystem();
    bool dropsystem();
    bool setFlag(uint32_t flag, bool update = true,_session *session=NULL);
    bool addFlag(uint32_t flag, bool update = true,_session *session=NULL);
    bool delFlag(uint32_t flag, bool update = true,_session *session=NULL);
    bool isFlag (uint32_t flag,_session *session=NULL);
    bool hasFlag(uint32_t flag,_session *session=NULL);
};

#endif //IOTPROTECT.H
