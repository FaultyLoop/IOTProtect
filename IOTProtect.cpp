#include "IOTProtect.h"
#include "SparkFunLSM6DS3.h"
#include <CRC32.h>
#include <DallasTemperature.h>
#include <EEPROM.h>
#include <ESP8266WiFi.h>
#include <MQTT.h>
#include <OneWire.h>
#include <SHA3.h>
#include <Wire.h>

/* WARNING : Missing commentary bellow, please be aware that this is how i work whenever i am alone on something*/
OneWire oneWire(0);
DallasTemperature sensor(&oneWire);

IOTProtect::IOTProtect(){
  EEPROM.begin(EEPROM_SIZE_FULL);//Init EEPROM (full-size: 4KB)
  sensor.begin();                //Init Sensors
  Wire.begin(D4, D5);            //Init Wire Protocol
  addEvent(FLAG_EVENT_EFBOOT);   //Add Boot Event
  delay(750);                    //Delay (sensor warmup)
}

/***** WORK IN PROGRESS *****/ //<-- Ignore that, it is just an indicator

void IOTProtect::begin() {
 //Hardware Button Check
  const bool aquiresystem = takesystem();//Reserve System
  bool manual = false;
  
  //Initial System Check
  checkCRC();
  
  //No flag, force a soft RESET FLAG_STATUS_RESET
  if (!getFlag()){addFlag(FLAG_STATUS_RESET,false);}
 
  //Interity check : OK
  if(!hasFlag(FLAG_STATUS_RESET)){
    
    //Removing High Status Flag
    delFlag(FLAG_STATUS_SAFEM|FLAG_STATUS_ADMIN|FLAG_STATUS_CONFG|FLAG_STATUS_WAKEP,false);
    
    //LAST SESSION SYS WAS WRITING
    if (!aquiresystem){addFlag(FLAG_STATUS_SAFEM,false);}
    
    //Update (for enabling services)
    update();
    
    //Button Flash Pressed, disable automatic behavior (for this boot sequence)
    if (digitalRead(0) == LOW){
      
      //Enable Serial (telnet cannot push a button)
      addFlag(FLAG_STATUS_SRSET,false);
      
      //Warn user
      println("SETTING CONFIG MODE TO MANUAL",ALL);
      delay(1500);
      
      //After 1.5 sec, if button keep pressed, do HARD_RESET, else open parametred interface
      if (digitalRead(0) == LOW){addFlag(FLAG_STATUS_RESET);}
      else {addFlag(FLAG_STATUS_CONFG|FLAG_STATUS_SRSET);manual = true;}
    } else {checkConf();}//Check Config
  }
  
  //If reset flag set, reset
  if (hasFlag(FLAG_STATUS_RESET)){reset();}
  else {
    //Update (required to sensor 'keep-alive')
    update();
    
    //Only if set to speak openly
    print("BOOT MODE : ",ALL);
    
    if (!manual && ((ESP.getResetInfoPtr()->reason == REASON_DEEP_SLEEP_AWAKE)|| hasFlag(FLAG_STATUS_DPSET))){
      //Set automatic behavior flags
      //HYBRID : can wake anytime (CPU running at low spec)
      //DEEPSLEEP : wake only with interrupt (CPU Timer only)
      addFlag(FLAG_STATUS_WAKEP,false);
      delFlag(FLAG_STATUS_CONFG,false);
      println(String(hasFlag(FLAG_STATUS_HDSET) ? "HYBRID":"DEEPSLEEP"),ALL);
      
    } else if (manual || hasFlag(FLAG_STATUS_CONFG)){
      //Config Failure mean data corruption
      addFlag(FLAG_STATUS_TNSET|FLAG_STATUS_SRSET,false);
      println("CONFIG "+String(manual ? "(MANUAL)":"(FAILURE)"),ALL);
      
    } else {println("TIMED",ALL);}
  }
  
  //Free reserved
  dropsystem();
  
  //Update (required to sensor 'keep-alive')
  update();
}

bool IOTProtect::setFingerPrint(String &fingerprint){
  //SSL Related Stuff
  bool ret = true;
  uint8_t buffer[EEPROM_SIZE_CFNG];
  memset(buffer,0,EEPROM_SIZE_CFNG);
  fingerprint.replace(":" ,"");
  fingerprint.replace(" " ,"");
  fingerprint.replace("0x","");
  fingerprint.trim();
  if (fingerprint.length() != EEPROM_SIZE_CFNG*2){return false;}

  for (uint8_t i(0);ret && i<EEPROM_SIZE_CFNG;i++){
    buffer[i] = (uint8_t) strtoul(String("0x"+fingerprint.substring(0,2)).c_str(),NULL,16);
    fingerprint = fingerprint.substring(2);
  }
  return setEEPROM(buffer,EEPROM_SIZE_CFNG,EEPROM_ADDR_FPRT);
}

String IOTProtect::getFingerPrint(){
  //SSL Related Stuff 
  String finger;
  uint8_t codex[EEPROM_SIZE_CFNG];
  getEEPROM(codex,EEPROM_SIZE_CFNG,EEPROM_ADDR_FPRT);
  for (uint8_t i(0);i<EEPROM_SIZE_CFNG;i++){
    String temp = String(codex[i],HEX);
    while (temp.length() < 2){temp = "0"+temp;}
    finger += temp+":";
  }
  return finger.substring(0,finger.length()-1);
}

void IOTProtect::loop() {
  //Main loop
  if (hasFlag(FLAG_STATUS_TNSET)){
    bool addClient = false;
    WiFiClient _local;
    if (hasFlag(FLAG_STATUS_SLSET) && (_local = secureServer.available())){//Get new client in the secure area
      println("SECURE CLIENT DETECTED",ALL);
      for (uint8_t i(0);i<8&&!addClient;i++){
        if (externSession[i].client.connected()){continue;}
        externSession[i].status = getFlag() & ~(FLAG_STATUS_SYSET|FLAG_STATUS_ADMIN);
        externSession[i].client = _local;
        delay(250);
        externSession[i].client.write(STR_MSG_WELCOME);
        externSession[i].client.write("\n\r>> ");
        addClient = true;
    }
  } else if (!hasFlag(FLAG_STATUS_SLSET) && (_local = telnetServer.available())){//Get new client in the plain area
      println("TELNET CLIENT DETECTED",ALL);
      for (uint8_t i(0);i<8&&!addClient;i++){
        if (externSession[i].client.connected()){continue;}
        externSession[i].status = getFlag() & ~(FLAG_STATUS_SYSET|FLAG_STATUS_ADMIN);
        externSession[i].client = _local;
        delay(250);
        externSession[i].client.write(STR_MSG_WELCOME);
        externSession[i].client.write("\n\r>> ");
        addClient = true;
      }
    }
    if (!addClient && _local){
        _local.write("MAX CLIENT COUNT, DISCONNECTING");
        _local.stop();
        Serial.println("CLIENT DISCONNECTED");
    }
  }
  if (hasFlag(FLAG_STATUS_CONFG) && !issystem()){//If config mode and not system, allow command to be registred
    if (Serial.available()){
      char current = Serial.read();
      if (_stream.length() > 512){_stream = String(&_stream.c_str()[1]);}
      if (current == 10 || current == 0){command(_stream);_stream = "";}
      else {_stream += current;}
    }
    for (uint8_t i(0);i<8 && !issystem();i++){
      if (externSession[i].client.connected() && externSession[i].client.available()){
       char current = externSession[i].client.read();
       if (externSession[i].stream.length() > 512){externSession[i].stream = String(&externSession[i].stream.c_str()[1]);}
       if (current == 10 || current == 0){
          while (!(externSession[i].stream[0] >= 'a' && externSession[i].stream[0] <= 'z') && (externSession[i].stream != "")){externSession[i].stream = externSession[i].stream.substring(1);}
          command(externSession[i].stream,&externSession[i]);
          externSession[i].stream = "";
          externSession[i].client.write("\n\r>> ");
        } else {externSession[i].stream += current;}
      }
    }
    if(updatetick == 12800){update();updatetick=0;}
    else {updatetick ++;}
    if (mqtt.connected()){mqtt.loop();}
    delay(10);
  } else {
    if (updatetick == ((sleeptime)/2)*1000){
      update();updatetick = 0;
      if(readSensors()){for (uint8_t i(0);i<getMax(TRIGGER);i++){testTrigger(getTrigger(i));}}
    } else {updatetick ++;}
    if (((millis()/1000) % sleeptime) == 0){
      if (!sendstate){
        sendstate = true;
        takesystem();
        addFlag(FLAG_STATUS_WAKEP);
        delFlag(FLAG_STATUS_WAKEP,false);
        dropsystem();
      }
    } else if (sendstate) {sendstate = false;}
    if (mqtt.connected()){mqtt.loop();}
    delay((sleeptime)/4);
  }
}

void IOTProtect::reset() {
  //Reset Memory
  uint8_t hash[EEPROM_SIZE_CSTR];
  
  //take system
  takesystem();
  addFlag(FLAG_STATUS_SRSET|FLAG_STATUS_TNSET|FLAG_STATUS_RESET);
  
  //Ensure system flag set
  delay(25);
  
  //Reste Init
  println("INITIALISING RESET",ALL);
  if (setEEPROM(NULL,NULL,NULL)){
    WiFi.begin("","");
    //FLAG_STATUS_SYSET CHECK (if set, system was not taken), when it happend this is bad (easiest solution : reflash)
    if (takesystem()){println("WARNING CONFLICT : SYSTEM FLAG RESERVED",ALL);}
   
    //Clearing Events Memory
    println("CLEARING MEMORY : "+(String(addFlag(FLAG_STATUS_SRSET|FLAG_STATUS_TNSET) ? STR_MSG_SUCCESS:STR_MSG_FAILURE)),ALL);
    
    //Add EEPROM_RESET Event
    addEvent(FLAG_EVENT_EFRST_);
    
    //RESET Specific EEPROM address
    //The memory map (how it is done) ensure that, if we are in a reset
    //and the device lose power, the secure information are still there
    //that ensure that security related info cannot (i hope) be accessed
    println("RESETING MQTT INDENT : "+String(setString(EEPROM_ADDR_MIDN,String(DEFAULT_IDENT)) ? STR_MSG_SUCCESS:STR_MSG_FAILURE),ALL);
    println("RESETING PORT TELNET : "+String(setLow(DEFAULT_SMPO,EEPROM_ADDR_SMPO) ? STR_MSG_SUCCESS:STR_MSG_FAILURE),ALL);
    println("RESETING PORT TIMEOUT: "+String(setLow(DEFAULT_MTOT,EEPROM_ADDR_MTOT) ? STR_MSG_SUCCESS:STR_MSG_FAILURE),ALL);
    println("RESETING PORT TRACER : "+String(setLow(DEFAULT_STSD,EEPROM_ADDR_STSD) ? STR_MSG_SUCCESS:STR_MSG_FAILURE),ALL);
    println("RESETING PORT SERVICE: "+String(setLow(DEFAULT_SSPO,EEPROM_ADDR_SSPO) ? STR_MSG_SUCCESS:STR_MSG_FAILURE),ALL);
    println("RESETING PORT SECURE : "+String(setLow(DEFAULT_SLPO,EEPROM_ADDR_SLPO) ? STR_MSG_SUCCESS:STR_MSG_FAILURE),ALL);
    println("HASHING SERVICE KEY  : "+String(hashkey(DEFAULT_KEY,hash) ? STR_MSG_SUCCESS:STR_MSG_FAILURE),ALL);
    println("SAVEING SERVICE KEY  : "+String(setInternalKey(hash) ? STR_MSG_SUCCESS:STR_MSG_FAILURE),ALL);
    addFlag(FLAG_STATUS_CONFG);
    dropsystem();
    println("DONE !",ALL);
  } else {println("FATAL FAILURE : SYSTEM FLAG ERROR",ALL);}
}

bool IOTProtect::clear(uint16_t area[2]) {
  //Clear an area of memory
  bool aquiresystem = takesystem();
  bool ret = setEEPROM(NULL,area[1]-area[0],area[0]);
  if (aquiresystem){dropsystem();}
  return ret;
}

void IOTProtect::print(String message,IOTProtect::_session *session) {
  //Send message to session 
  if (hasFlag(FLAG_STATUS_SRSET) && (session == NULL)){Serial.print(message);}
  if (hasFlag(FLAG_STATUS_TNSET) && (session != NULL)){
    if (session->client.connected()){
      session->client.write(message.c_str(),message.length());
      session->client.flush();
    }
  }
}

void IOTProtect::print(String message,interfaces id) {
  //Send message to specific session or all session if ALL specified
  if (hasFlag(FLAG_STATUS_SRSET) && (id == SERIAL_0 || id == ALL)){Serial.print(message);}
  if (hasFlag(FLAG_STATUS_TNSET)){
    for (uint8_t i(0);i<8;i++){
      if (id == ALL || i == id){
        if (externSession[i].client.connected()){
          externSession[i].client.write(message.c_str(),message.length());
          externSession[i].client.flush();
        }
      }
    }
  }
}

void IOTProtect::println(String message,interfaces id) {print(message+"\n\r",id);}
void IOTProtect::println(String message,IOTProtect::_session *session) {print(message+"\n\r",session);}

uint8_t IOTProtect::match(const String &string,const String parser,String &matching) {
  //Command Parsing and resolution (only used when using interface)
  if (string == ""){return FLAG_MATCH_STRVOID;}
  matching = "";
  String matchcmd = parser;
  float oldscore = 60.0;
  uint8_t errorno = FLAG_MATCH_INVALID;
  
  while (matchcmd != ""){
    String sentence, lockerror = string, statment = matchcmd.substring(0,matchcmd.indexOf(";"));
    float score = 100;
    if (statment.length() > EEPROM_SIZE_CSTR){matching = statment;return FLAG_MATCH_DROPPED;}
    else if (string.length() > statment.length()){score -= 12.0 * (string.length()-statment.length());}
    else if (string.length() < statment.length()){score -= 4.0 * (statment.length()-string.length());}
    
    for (uint16_t i(0);i<statment.length();i++){
      if (lockerror.indexOf(sentence+statment[i]) == -1){
        if (lockerror[i ? i-1:0] == statment[i])     {score -= (100.0/statment.length()) * ((statment.length()-i)*0.05);}
        else if (lockerror[i] == statment[i ? i-1:0]){score -= (100.0/statment.length()) * ((statment.length()-i)*0.25);}
        else                                         {score -= (100.0/statment.length()) * ((statment.length()-i)*0.15);}
        lockerror[i ? i-1:0] = statment[i ? i-1:0];
      }
      sentence += statment[i];
    }
    //Serial.println("SCORE FOR :"+statment+" : "+String(score));
    if (score >= 100.0)                                 {matching  =         statment;oldscore = 100.0;errorno = 0;}
    else if (score >  (oldscore-00.0) && matching == ""){matching  =         statment;oldscore = score;errorno = FLAG_MATCH_INVALID;}
    else if (score >  (oldscore+15.0) && matching != ""){matching  =         statment;oldscore = score;errorno = FLAG_MATCH_INVALID;}
    else if (score >= (oldscore-05.0) && matching != ""){matching += "' , '"+statment;oldscore = score;errorno = FLAG_MATCH_INVALID | FLAG_MATCH_AMBIGUS;}
    matchcmd = matchcmd.substring(matchcmd.indexOf(";")+1);
  }
  return errorno;
}


void IOTProtect::addBackup(IOTProtect::_backup backup) {
  //Backup status (ensure message is saved, if the connection is lost)
  const bool aquired = takesystem();
  uint8_t id = getLow(EEPROM_ADDR_BKCN);
  addEvent(FLAG_EVENT_EFBACK|FLAG_EVENT_ECWRT|((id<=getMax(BACKUP))&&(setEEPROM((uint8_t *)&backup,sizeof(_backup),EEPROM_ADDR_BFSD+(sizeof(_backup)*id))) ? 0:FLAG_EVENT_ECFAL));
  setLow(++id,EEPROM_ADDR_BKCN);
  if (aquired){dropsystem();}
}

IOTProtect::_backup IOTProtect::getBackup(uint8_t id) {
  //Get a backup
  _backup _local;
  if (id <= getMax(BACKUP)){getEEPROM((uint8_t *) &_local,sizeof(_backup),EEPROM_ADDR_BFSD+(sizeof(_backup)*id));}
  return _local;
}

void IOTProtect::command(String &stream,IOTProtect::_session *session) {
  uint8_t buffer[4];
  uint32_t flag = 0;
  uint16_t command = 0, addr = 0;
  uint8_t failureid = 0;
  bool all = false, have = false;

  String sessionid = STR_MSG_SESSION+(8*((bool)session+hasFlag(FLAG_STATUS_SLSET)));
  String bases,matching,target;
  
  stream.trim();
  if (stream == ""){println("---INTERFACE UP---",session);return;}
  println(stream,session);
  if (stream.indexOf("swap")>-1){
    takesystem();
    delFlag(FLAG_STATUS_CONFG);
    addFlag(FLAG_STATUS_WAKEP);
    dropsystem();
    print("SWAPING TO ",session);
    if (hasFlag(FLAG_STATUS_DPSET)){
      if (hasFlag(FLAG_STATUS_HDSET)){print("HYBRID",session);}
      else {print("DEEPSLEEP",session);}
    }else {print("TIMER");}
    println(" MODE",session);
    return;
  }
  stream += " "; 
  const String origin = stream;

  for (uint8_t i(0);i<3&&!failureid;i++){
    target = "";
    if (!i){target = STR_CMD_HEADERS;}
    else {
      switch (command & (i == 2 ? 0x00ff:0xff00)){
        case FLAG_COMMAND_SENCLS | FLAG_COMMAND_UPPALT:
        case FLAG_COMMAND_GETSET | FLAG_COMMAND_NULNUL:
        case FLAG_COMMAND_GETSET | FLAG_COMMAND_UPPALT:target = STR_CMD_HPRIMAL;break;
        case FLAG_COMMAND_RESREB | FLAG_COMMAND_NULNUL:
        case FLAG_COMMAND_ENADIS | FLAG_COMMAND_NULNUL:
        case FLAG_COMMAND_ENADIS | FLAG_COMMAND_UPPALT:
        case FLAG_COMMAND_STAFLA | FLAG_COMMAND_NULNUL:target = STR_CMD_HSECOND;break;
        
        case FLAG_COMMAND_STRLOG | FLAG_COMMAND_NULNUL:target = STR_CMD_OPTSSTR;break;
        case FLAG_COMMAND_LIPLOW | FLAG_COMMAND_LOWALT:target = STR_CMD_OPTPORT;break;
        
        case FLAG_COMMAND_RSTHLP | FLAG_COMMAND_NULNUL:
          if (session && !hasFlag(FLAG_STATUS_ADMIN,session)){println("DISTANT SESSION NEED ADMIN AUTH TO DO DEVICE RESET");}
          else {reset();}
          return;
        case FLAG_COMMAND_RSTHLP | FLAG_COMMAND_UPPALT:
          if (true){
            String result;
            result = (";"+String(STR_CMD_HEADERS)).substring(0,result.length()-1);
            result.replace(";","\n\t-");
            println("MAIN COMMANDS : "+result,session);
          }
          return;
        case FLAG_COMMAND_EXTSYN | FLAG_COMMAND_NULNUL:
          if (session){println("CLIENT DISCONNECTED",ALL);session->client.stop();}
          else {println("UNAVALAIBLE FOR THIS SESSION",session);}
          return;
        case FLAG_COMMAND_EXTSYN | FLAG_COMMAND_UPPALT:
          if (session){
            if (isFlag(FLAG_STATUS_ADMIN|FLAG_STATUS_CONFG,session) && isFlag(FLAG_STATUS_CONFG) && !isFlag(FLAG_STATUS_SAFEM|FLAG_STATUS_SYSET)){
              println("SYNC "+String(setFlag(getFlag(session)) ? STR_MSG_SUCCESS:STR_MSG_FAILURE),ALL);
            } else {println("SYNC FAILED : UNAUTORISED",session);}
          }
          else {println("UNAVALAIBLE FOR THIS SESSION",session);}
          return;
      }
    }
    if (target == ""){break;}
    bases = stream.substring(0,stream.indexOf(" "));
    bases.toLowerCase();
    failureid = match(bases,target,matching);
    if (!failureid){
      int16_t index = 0;
      for (uint16_t i(0);i<target.indexOf(matching);i++){if (target[i] == ';'){index++;}}
      switch (index){
        case  0:
          if (i == 0){command  = FLAG_COMMAND_SENCLS | FLAG_COMMAND_UPPALT;}
          if (i == 1){command |= FLAG_COMMAND_CRCBAK | FLAG_COMMAND_LOWALT;flag = FLAG_STATUS_MMSET;addr = EEPROM_ADDR_BFSD;}
          if (i == 2){addr = (command & FLAG_COMMAND_STRLOG ? EEPROM_ADDR_SADR:EEPROM_ADDR_SMPO);}
          break;
        case  1:
          if (i == 0){command  = FLAG_COMMAND_ENADIS | FLAG_COMMAND_UPPALT;}
          if (i == 1){command |= FLAG_COMMAND_MQTI2C | FLAG_COMMAND_LOWALT;flag = FLAG_STATUS_ICSET;}
          if (i == 2){addr = (command & FLAG_COMMAND_STRLOG ? EEPROM_ADDR_LWPS:EEPROM_ADDR_SSPO);}
          break;
        case  2:
          if (i == 0){command  = FLAG_COMMAND_ENADIS | FLAG_COMMAND_NULNUL;}
          if (i == 1){command |= FLAG_COMMAND_SNSPIN | FLAG_COMMAND_NULNUL;flag = FLAG_STATUS_SNSET;}
          if (i == 2){addr = (command & FLAG_COMMAND_STRLOG ? EEPROM_ADDR_LSID:EEPROM_ADDR_SLPO);}
          break;
        case  3:
          if (i == 0){command  = FLAG_COMMAND_STAFLA | FLAG_COMMAND_UPPALT;}
          if (i == 1){command |= FLAG_COMMAND_SNSPIN | FLAG_COMMAND_LOWALT;addr = EEPROM_ADDR_SP00 ;}
          if (i == 2){addr = EEPROM_ADDR_MIDN;}
          break;
        case  4:
          if (i == 0){command  = FLAG_COMMAND_GETSET | FLAG_COMMAND_NULNUL;}
          if (i == 1){command |= (command & FLAG_COMMAND_GETSET ? FLAG_COMMAND_CRCBAK:FLAG_COMMAND_DEPHYB) | FLAG_COMMAND_NULNUL;flag = FLAG_STATUS_DPSET;}
          if (i == 2){addr = EEPROM_ADDR_MPSS;}
          break;
        case  5:
          if (i == 0){command  = FLAG_COMMAND_RESREB | FLAG_COMMAND_UPPALT;}
          if (i == 1){command |= (command & FLAG_COMMAND_GETSET ? FLAG_COMMAND_STRLOG:FLAG_COMMAND_DEPHYB) | FLAG_COMMAND_LOWALT;flag = FLAG_STATUS_HDSET;}
          if (i == 2){addr = EEPROM_ADDR_MTOP;}
          break;
        case  6:
          if (i == 0){command  = FLAG_COMMAND_RESREB | FLAG_COMMAND_NULNUL;}
          if (i == 1){command |= (command & FLAG_COMMAND_GETSET ? FLAG_COMMAND_LIPLOW:FLAG_COMMAND_TELSEC) | FLAG_COMMAND_LOWALT;flag = FLAG_STATUS_SLSET;}
          if (i == 2){addr = EEPROM_ADDR_MLIN;}
          break;
        case  7:
          if (i == 0){command  = FLAG_COMMAND_SENCLS | FLAG_COMMAND_NULNUL;}
          if (i == 1){command |= (command & FLAG_COMMAND_GETSET ? FLAG_COMMAND_SLPTIM:FLAG_COMMAND_MQTI2C) | FLAG_COMMAND_NULNUL;flag = FLAG_STATUS_MQSET;addr = EEPROM_ADDR_STSD;}
          break;
        case  8:
          if (i == 0){command  = FLAG_COMMAND_GETSET | FLAG_COMMAND_UPPALT;}
          if (i == 1){command |= (command & FLAG_COMMAND_GETSET ? FLAG_COMMAND_LIPLOW:FLAG_COMMAND_SERWIF) | FLAG_COMMAND_NULNUL;flag = FLAG_STATUS_SRSET;}
          break;
        case  9:
          if (i == 0){command  = FLAG_COMMAND_STAFLA | FLAG_COMMAND_NULNUL;}
          if (i == 1){command |= (command & FLAG_COMMAND_GETSET ? FLAG_COMMAND_STRLOG:FLAG_COMMAND_TELSEC) | FLAG_COMMAND_NULNUL;flag = FLAG_STATUS_TNSET;}
          break;
        case 10:
          if (i == 0){command  = FLAG_COMMAND_STRLOG | FLAG_COMMAND_LOWALT;}
          if (i == 1){command |= (command & FLAG_COMMAND_GETSET ? FLAG_COMMAND_SLPTIM:FLAG_COMMAND_SERWIF) | FLAG_COMMAND_LOWALT;flag = FLAG_STATUS_WFSET;addr = EEPROM_ADDR_MTOT;}
          break;
        case 11:
          if (i == 0){command  = FLAG_COMMAND_RSTHLP | FLAG_COMMAND_UPPALT;}
          if (i == 1){command |= (command & FLAG_COMMAND_GETSET ? FLAG_COMMAND_TRIEVE:FLAG_COMMAND_FINGER) | FLAG_COMMAND_NULNUL;flag = FLAG_STATUS_SMSET;}
          break;
       case 12:
          if (i == 0){command  = FLAG_COMMAND_RSTHLP | FLAG_COMMAND_NULNUL;}
          if (i == 1){command |= FLAG_COMMAND_TRIEVE | FLAG_COMMAND_LOWALT;}
          break;
       case 13:
          if (i == 0){command  = FLAG_COMMAND_EXTSYN | FLAG_COMMAND_NULNUL;}
          if (i == 1){command |= FLAG_COMMAND_FINGER | FLAG_COMMAND_NULNUL;}
          break;
       case 14:
          if (i == 0){command  = FLAG_COMMAND_EXTSYN | FLAG_COMMAND_UPPALT;}
          break;
      }
    }
    stream = stream.substring(stream.indexOf(" ")+1);
  }
  
 
  if (!failureid){
    stream.trim();
    if ((command & 0xff00) == 0){command |= FLAG_COMMAND_GETSET | FLAG_COMMAND_NULNUL;}
    switch(command & 0xff00){
        case FLAG_COMMAND_GETSET | FLAG_COMMAND_NULNUL:
          switch (command & 0x00ff){
            case FLAG_COMMAND_STRLOG | FLAG_COMMAND_NULNUL:
              if (hasFlag(FLAG_STATUS_ADMIN,session)){println("STRING '"+bases+"' : "+getString(addr),session);}
              else {failureid = FLAG_MATCH_DROPPED;}
              break;
            case FLAG_COMMAND_STRLOG | FLAG_COMMAND_LOWALT:
              if (checkPass(stream)){
                while (issystem()){delay(10);}
                takesystem();
                println("PASSWORD OK, "+String(addFlag(FLAG_STATUS_ADMIN,false,session) ? "YOU ARE NOW IN ADMINISTATION MODE":"FLAG SET ERROR"),session);
                dropsystem();
              } else {println("PASSWORD ERROR, EVENT REGISTRED",session);}
              break;
            case FLAG_COMMAND_TRIEVE | FLAG_COMMAND_NULNUL:
              if (stream.toInt() > getMax(TRIGGER)){println("TRIGGER ID INVALID",session);}
              else {
                _trigger _local = getTrigger(stream.toInt());
                println("TOPIC : "+String(_local.topic),session);
                print  ("PIN   : ",session);
                for (uint8_t i(0);i<8;i++){
                  if ((1<<i) & _local.pin){print(String(i)+",",session);}
                }
                println("",session);
                
                for (uint8_t i(0);i<2;i++){
                  print("TRAP "+String(i)+" : " ,session);
                  if (!(_local.trap>>(4*(1-i)) & 0x0f) || (_local.trap>>(4*(1-i)) & 0x0f) == FLAG_TRIGGER_DISD){print(STR_MSG_DISABLE,session);}
                  else {
                    if (_local.trap>>(4*(1-i)) & FLAG_TRIGGER_XNOT){print("!",session);}
                    if (_local.trap>>(4*(1-i)) & FLAG_TRIGGER_GTHN){print(">",session);}
                    if (_local.trap>>(4*(1-i)) & FLAG_TRIGGER_EQTY){print("=",session);}
                    if (_local.trap>>(4*(1-i)) & FLAG_TRIGGER_LTHN){print("<",session);}
                  }
                  println("",session);
                }
                uint32_t val;
                val = _local.valpri << 16;
                println("VALUE 1 : "+String(*((float *) &val)),session);
                val = _local.valsec << 16;
                println("VALUE 2 : "+String(*((float *) &val)),session);
                println("DELAY : "+String(_local.alttime));
                println("TRIGGERED : "+String(testTrigger(_local,true) ? "TRUE":"FALSE"),session);
              }
              break;
            case FLAG_COMMAND_TRIEVE | FLAG_COMMAND_LOWALT:
              flag = -1;
              if (stream.toInt() || stream[0] == '0'){flag = stream.toInt();}
              
              for (uint16_t i(0);i< (stream.indexOf("--force") > -1 ? getMax(EVENT):getLow(EEPROM_ADDR_EVCN));i++){
                if (i != flag && flag < getMax(EVENT)){continue;}
                _event _local = getEvent(i);
                
                const String eventHead = "EVENT-"+String(i);
                const uint32_t flags =  _local.status & 0xffffff;
                const uint16_t event = (_local.time & 0xff)<<8|(_local.status>>24);
                if (!event && !flags){continue;}
                
                for (uint8_t i(0);i<(80-eventHead.length())>>1;i++){print("-",session);}
                print(eventHead,session);
                for (uint8_t i(0);i<(80-eventHead.length())>>1;i++){print("-",session);}
                
                println("\n\rTime   : "+String(_local.time>>19)+":"+String((_local.time>>14) & 63)+":"+String((_local.time>> 9 & 63)),session);
                print("EVENT CODE  : ");
                if (stream.indexOf("-r") > -1 || stream.indexOf("--raw") > -1){println(String(event,BIN));}
                else {
                  if (event & FLAG_EVENT_ECFAL){print("FAIL ",session);}
                  if (event & FLAG_EVENT_ECWRT){print("WRITE ",session);}
                  if (event & FLAG_EVENT_ECRED){print("READ ",session);}
                  if (event & FLAG_EVENT_ECTST){print("TEST ",session);}
                  switch (event &0xf){
                    case (FLAG_EVENT_EFBOOT):println("BOOT",session);break;
                    case (FLAG_EVENT_EFMSG_):println("MESSAGE",session);break;
                    case (FLAG_EVENT_EFRST_):println("RESET",session);break;
                    case (FLAG_EVENT_EFSLP_):println("SLEEP",session);break;
                    case (FLAG_EVENT_EFEEPM):println("EEPROM",session);break;
                    case (FLAG_EVENT_EFTRIG):println("TRIGGER",session);break;
                    case (FLAG_EVENT_EFEVNT):println("EVENT",session);break;
                    case (FLAG_EVENT_EFADMN):println("ADMIN LOG",session);break;
                    case (FLAG_EVENT_EFBACK):println("BACKUP",session);break;
                    case (FLAG_EVENT_EFCNFG):println("CONFIG",session);break;
                    case (FLAG_EVENT_EFINTY):println("INTEGRITY",session);break;
                    default:println("UNKOWN",session);
                  }
                }
                print("STATUS : ",session);
                for (uint8_t i(0);i<24;i++){if (flags & (1<<i)){print(STR_MSG_STATUS_+(9*i),session);}}
                println("",session);
                delay(120);
              }
              if (!getLow(EEPROM_ADDR_EVCN)){println("NO EVENT FOUND",session);}
              println("DONE",session);
              break;
            case FLAG_COMMAND_SNSPIN | FLAG_COMMAND_NULNUL:
              if (hasFlag(FLAG_STATUS_SNSET,session)){
                if (stream.indexOf("-r") > -1 || stream.indexOf("--read") > -1){readSensors();}
                bool force = stream.indexOf("-f") > -1 || stream.indexOf("--force") > -1;
                uint8_t pin = getPin();
                if (stream.indexOf("-r") >-1 || stream.indexOf("--raw") >-1){println("PIN CODE : "+String(pin,BIN));}
                for (uint8_t i(0);i<8;i++){
                  println("PIN  "+String(i)+" : ",session);
                  if (force || ((1<<i) & pin)){println("VALUE  : "+String(pinbuf[i])+(force ? " (FORCED)":""),session);}
                  println("TOPIC  : "+getString(EEPROM_ADDR_SP00+EEPROM_SIZE_CSTR*i),session);
                  println("STATUS : "+String((1<<i)&pin ? STR_MSG_ENABLED:STR_MSG_DISABLE),session);
                }
              } else {println("SENSORS "+String(STR_MSG_DISABLE),session);}
              break;
            case FLAG_COMMAND_SNSPIN | FLAG_COMMAND_LOWALT:
              flag = stream.toInt();
              if (flag > 7){println("INVALID PIN ID",session);}
              else {
                addr += EEPROM_SIZE_CSTR * flag;
                println("PIN "+String(flag)+" : "+getString(addr),session);
              } 
              break;
            case FLAG_COMMAND_LIPLOW | FLAG_COMMAND_NULNUL:
              if (hasFlag(FLAG_STATUS_ADMIN,session)){println("LOCAL IP : "+WiFi.localIP().toString(),session);}
              else {failureid = FLAG_MATCH_DROPPED;}
              break;
            case FLAG_COMMAND_CRCBAK | FLAG_COMMAND_NULNUL:
              println("CRC : "+String(getCRC(),HEX),session);
              break;
            case FLAG_COMMAND_CRCBAK | FLAG_COMMAND_LOWALT:
              if (hasFlag(FLAG_STATUS_ADMIN,session)){
                flag = -1;
                if (stream.toInt() || stream[0] == '0'){flag = stream.toInt();}
                for (uint8_t i(0);i<getLow(EEPROM_ADDR_BKCN);i++){
                  if (i != flag && flag < getMax(BACKUP)){continue;}
                  _backup _local = getBackup(i);
                  uint32_t value = _local.value<<16;
                  println("VALUE   : "+String(*(float *) &value),session);
                  println("TRIGGER : "+String(_local.trpin>>4));
                  println("PINS    : "+String(_local.trpin & 0x0f));
                }
                if (!getLow(EEPROM_ADDR_BKCN)){println("NO BACKUP FOUND",session);}
              } else {failureid = FLAG_MATCH_DROPPED;}
              break;
            case FLAG_COMMAND_LIPLOW | FLAG_COMMAND_LOWALT:
            case FLAG_COMMAND_SLPTIM | FLAG_COMMAND_NULNUL:
            case FLAG_COMMAND_SLPTIM | FLAG_COMMAND_LOWALT:
              if (isFlag(FLAG_STATUS_ADMIN|FLAG_STATUS_CONFG,session)){println("GETTING '"+bases+"' : "+String(getLow(addr)),session);}
              else {failureid = FLAG_MATCH_DROPPED;}
              break;
            case FLAG_COMMAND_MQTI2C | FLAG_COMMAND_LOWALT:
              if (hasFlag(FLAG_STATUS_SNSET,session)){
                if (stream.indexOf("-r") > -1 || stream.indexOf("--read") > -1){readSensors();}
                bool force = stream.indexOf("-f") > -1 || stream.indexOf("--force") > -1;
                if (hasFlag(FLAG_STATUS_ICSET)){
                  const String type[2] = {"ACC", "GYR"};
                  const String id =  "XYZ";
                  for (uint8_t i(0); i < 6; i++) {println(String(type[i >= 3])+" "+String(id[i-3*(i>=3)])+" : "+String(i2cbuf[i]),session);}
                } else {println("I2C "+String(STR_MSG_DISABLE),session);}
              } else {println("SENSORS "+String(STR_MSG_DISABLE),session);}
              break;
            case FLAG_COMMAND_FINGER | FLAG_COMMAND_NULNUL:
              if (hasFlag(FLAG_STATUS_ADMIN,session)){println("FINGERPRINT : "+getFingerPrint(),session);}
              else {failureid = FLAG_MATCH_DROPPED;}
              break;
            default:
              failureid = FLAG_MATCH_DROPPED;
          }
          break;
        case FLAG_COMMAND_GETSET | FLAG_COMMAND_UPPALT:
          if (isFlag(FLAG_STATUS_ADMIN|FLAG_STATUS_CONFG,session) && !hasFlag(FLAG_STATUS_SAFEM)){
          switch (command & 0x00ff){
            case FLAG_COMMAND_STRLOG | FLAG_COMMAND_NULNUL:
              println("STRING '"+bases+"' : "+String(setString(addr,stream) ? STR_MSG_SUCCESS:STR_MSG_FAILURE),session);
              break;
            case FLAG_COMMAND_STRLOG | FLAG_COMMAND_LOWALT:
              println("SETTING NEW KEY : "+String(setInternalKey(stream) ? STR_MSG_SUCCESS:STR_MSG_FAILURE),session);
              break;
            case FLAG_COMMAND_TRIEVE | FLAG_COMMAND_NULNUL:
              if (true){
                String apply = stream.substring(stream.indexOf("--id")+5);
                float temp;
                uint8_t id = apply.substring(0,apply.indexOf(" ")).toInt();
                if (id > getMax(TRIGGER)){println("TRIGGER ID INVALID",session);}
                else {
                  _trigger _local;
                  if (stream.indexOf("--keep")>-1){_local = getTrigger(id);}
                  if (stream.indexOf("--pin")>-1){
                    apply = stream.substring(stream.indexOf("--pin")+6);
                    apply = apply.substring(0,apply.indexOf(" "));
                    for (uint8_t i(0);i<apply.length();i++){
                      if (apply[i] >= '0' && apply[i] <= '7'){_local.pin |= (1<<String(apply[i]).toInt());}
                    }
                  }
  
                  if (stream.indexOf("--topic")>-1){
                    apply = stream.substring(stream.indexOf("--topic" )+8);
                    apply = apply.substring(0,apply.indexOf(" "));
                    strncpy(_local.topic,apply.c_str(),25);
                  }
  
                  if (stream.indexOf("--opa")>-1){
                    apply = stream.substring(stream.indexOf("--opa")+6);
                    apply = apply.substring(0,apply.indexOf(" "));
                    if (apply.indexOf("=") > -1){_local.trap |= (FLAG_TRIGGER_EQTY << 4);}
                    if (apply.indexOf("<") > -1){_local.trap |= (FLAG_TRIGGER_LTHN << 4);}
                    if (apply.indexOf(">") > -1){_local.trap |= (FLAG_TRIGGER_GTHN << 4);}
                    if (apply.indexOf("!") > -1){_local.trap |= (FLAG_TRIGGER_XNOT << 4);}
                  }
                  
                  if (stream.indexOf("--opb")>-1){
                    apply = stream.substring(stream.indexOf("--opb")+6);
                    apply = apply.substring(0,apply.indexOf(" "));
                    if (apply.indexOf("=") > -1){_local.trap |= (FLAG_TRIGGER_EQTY);}
                    if (apply.indexOf("<") > -1){_local.trap |= (FLAG_TRIGGER_LTHN);}
                    if (apply.indexOf(">") > -1){_local.trap |= (FLAG_TRIGGER_GTHN);}
                    if (apply.indexOf("!") > -1){_local.trap |= (FLAG_TRIGGER_XNOT);}
                  }
                  
                  if (stream.indexOf("--valpri")>-1){
                    apply = stream.substring(stream.indexOf("--valpri")+9);
                    apply = apply.substring(0,apply.indexOf(" "));
                    temp = apply.toFloat();
                    _local.valpri = (*((uint32_t *) &temp)) >> 16;
                  }
                  if (stream.indexOf("--valsec")>-1){
                    apply = stream.substring(stream.indexOf("--valsec")+9);
                    apply = apply.substring(0,apply.indexOf(" "));
                    temp = apply.toFloat();
                    _local.valsec = (*((uint32_t *) &temp)) >> 16;
                  }
                  
                  if (stream.indexOf("--time")>-1){
                    apply = stream.substring(stream.indexOf("--time"  )+7);
                    apply = apply.substring(0,apply.indexOf(" "));
                    if (apply.toInt()){_local.alttime = apply.toInt();}
                  }
                  
                  while (issystem()){}
                  takesystem();
                  println("TRIGGER SET WITH "+String(ediTrigger(id,_local) ? STR_MSG_SUCCESS:STR_MSG_FAILURE),session);
                  dropsystem();
                }
              }
              break;
            case FLAG_COMMAND_SNSPIN | FLAG_COMMAND_LOWALT:
              flag = 0;
              if (stream.charAt(0) <= '9' && stream.charAt(0) >= '0'){
                flag = stream.substring(0,1).toInt();
                stream = stream.substring(2);
                stream.trim();
              }
              if (flag > 7){println("INVALID PIN ID",session);}
              else if (isFlag(FLAG_STATUS_ADMIN|FLAG_STATUS_CONFG) && !hasFlag(FLAG_STATUS_SAFEM)){
                addr += EEPROM_SIZE_CSTR * flag;
                getString(addr);
                println("PIN "+String(flag)+" : "+String(getPin() & (1<<flag) ? STR_MSG_SUCCESS:STR_MSG_FAILURE),session);
              } else {failureid = FLAG_MATCH_DROPPED;}
              break;
            case FLAG_COMMAND_LIPLOW | FLAG_COMMAND_LOWALT:
            case FLAG_COMMAND_SLPTIM | FLAG_COMMAND_NULNUL:
            case FLAG_COMMAND_SLPTIM | FLAG_COMMAND_LOWALT:
              println("SETTING "+String(stream.toInt())+" IN '"+bases+"' : "+String(setLow(stream.toInt(),addr) ? STR_MSG_SUCCESS:STR_MSG_FAILURE),session);
              break;
            case FLAG_COMMAND_FINGER | FLAG_COMMAND_NULNUL:
              println("SET FINGERPRINT : "+String(setFingerPrint(stream) ? STR_MSG_SUCCESS:STR_MSG_FAILURE),session);
              break;
          }
        } else {failureid = FLAG_MATCH_DROPPED;}
          break;
        case FLAG_COMMAND_RESREB | FLAG_COMMAND_UPPALT:ESP.restart();delay(9000);break;
        case FLAG_COMMAND_RESREB | FLAG_COMMAND_NULNUL:
        case FLAG_COMMAND_ENADIS | FLAG_COMMAND_UPPALT:
          if (isFlag(FLAG_STATUS_ADMIN|FLAG_STATUS_CONFG,session)){
            if ((command & (FLAG_COMMAND_SNSPIN | FLAG_COMMAND_LOWALT)) == (FLAG_COMMAND_SNSPIN | FLAG_COMMAND_LOWALT)){
              if (stream.charAt(0) >= '0' && stream.charAt(0) <= '9'){
                flag = stream.toInt();
                if (flag > 7){println("INVALID PIN ID",session);}
                else {
                  println("DISABLING PIN '"+String(stream.toInt())+"' : ",session);
                  println(String(setPin(getPin() &~(1<<stream.toInt())) ? STR_MSG_SUCCESS:STR_MSG_FAILURE),session);
                  println("PIN  CODE : "+String(getPin(),BIN),session);
                }
              }
            } else {println("DISABLING '"+bases+"' : "+String(delFlag(flag) ? STR_MSG_SUCCESS:STR_MSG_FAILURE),session);}
          } else {failureid = FLAG_MATCH_DROPPED;}
          if (failureid || !(command & FLAG_COMMAND_RESREB)){break;}
          println("",session);

        case FLAG_COMMAND_ENADIS | FLAG_COMMAND_NULNUL:
          if (isFlag(FLAG_STATUS_ADMIN|FLAG_STATUS_CONFG,session)){
            if ((command & (FLAG_COMMAND_SNSPIN | FLAG_COMMAND_LOWALT)) == (FLAG_COMMAND_SNSPIN | FLAG_COMMAND_LOWALT)){
              if (stream.charAt(0) >= '0' && stream.charAt(0) <= '9'){
                flag = stream.toInt();
                if (flag > 7){println("INVALID PIN ID",session);}
                else {
                  print("ENABLING PIN '"+String(stream.toInt())+"' : ",session);
                  println(String(setPin(getPin() | (1<<stream.toInt())) ? STR_MSG_SUCCESS:STR_MSG_FAILURE),session);
                  println("PIN  CODE : "+String(getPin(),BIN),session);
                }
              }
            } else {println("ENABLING '"+bases+"' : "+String(addFlag(flag) ? STR_MSG_SUCCESS:STR_MSG_FAILURE),session);}
          } else {failureid = FLAG_MATCH_DROPPED;}
          update();
          break;

        case FLAG_COMMAND_STAFLA | FLAG_COMMAND_NULNUL:
          if (isFlag(FLAG_STATUS_ADMIN|FLAG_STATUS_CONFG,session)){
            if ((command & (FLAG_COMMAND_SNSPIN | FLAG_COMMAND_LOWALT)) == (FLAG_COMMAND_SNSPIN | FLAG_COMMAND_LOWALT)){
              if (stream.charAt(0) >= '0' && stream.charAt(0) <= '9'){
                flag = stream.toInt();
                if (flag > 7){println("INVALID PIN ID",session);}
                else {println("PIN '"+String(stream.toInt())+"' IS : "+String((getPin() & (1<<stream.toInt())) ? STR_MSG_ENABLED:STR_MSG_DISABLE),session);}
              }
            } else {println("'"+bases+"' IS : "+String(hasFlag(flag) ? STR_MSG_ENABLED:STR_MSG_DISABLE),session);}
          } else {failureid = FLAG_MATCH_DROPPED;}
          break;
          
        case FLAG_COMMAND_STAFLA | FLAG_COMMAND_UPPALT:
          flag = getFlag(session);
          print("CURRENT FLAGS FOR '"+sessionid+"' : ",session);
          if (stream.indexOf("-r") > -1 || stream.indexOf("--raw") >-1){println(String(flag,BIN),session);}
          else {for (uint8_t i(0);i<24;i++){if (flag & (1<<i)){print(String(STR_MSG_STATUS_+(9*i)),session);}}}
          break;
        case FLAG_COMMAND_SENCLS | FLAG_COMMAND_NULNUL:
          if (stream.indexOf("-s") > -1 || stream.indexOf("--string") > -1){
            stream.replace("--string","");
            stream.replace("-s","");
            if (mqtt.connected()){
              println("SENDING '"+stream+"' AT '"+getString(EEPROM_ADDR_MTOP)+"'",session);
              mqtt.publish(getString(EEPROM_ADDR_MTOP),stream);
            } else {println("BACKUP UNAVAILABE FOR MANUAL SEND",session);}
          } else {
            bool force = false;
            flag = getPin();
            if (sendSensors()){println("SENDING "+String(STR_MSG_SUCCESS),session);}
            else if (hasFlag(FLAG_STATUS_MMSET)){println("BACKUPING "+String(STR_MSG_SUCCESS),session);}
            else {println("SENDING FAILURE ! CHECK "+String(WiFi.isConnected() ? "MQTT":"WIFI")+" CONNEXION",session);}
          }
          break;
        case FLAG_COMMAND_SENCLS | FLAG_COMMAND_UPPALT:
          switch (command & 0x00ff){
            case FLAG_COMMAND_TRIEVE | FLAG_COMMAND_NULNUL:
              if (hasFlag(FLAG_STATUS_ADMIN,session)){
                _trigger nultrigg;
                while (issystem()){delay(150);}
                takesystem();
                for (uint8_t i(0);i<getMax(TRIGGER);i++){ediTrigger(i,nultrigg);}
                dropsystem();
                println("DONE",session);
              } else {println("REQUIRE AUTENTIFICATION",session);};
              break;
            case FLAG_COMMAND_TRIEVE | FLAG_COMMAND_LOWALT:
              if (hasFlag(FLAG_STATUS_ADMIN,session)){
                setLow(0,EEPROM_ADDR_EVCN);
                println("DONE",session);
              } else {println("REQUIRE AUTENTIFICATION",session);};
              break;
            case FLAG_COMMAND_CRCBAK | FLAG_COMMAND_LOWALT:
              if (hasFlag(FLAG_STATUS_ADMIN,session)){
                setLow(0,EEPROM_ADDR_BKCN);
                println("DONE",session);
              } else {println("REQUIRE AUTENTIFICATION",session);};
              break;
              break;
            default:
              println("'"+bases+"' IS NOT CLEARABLE",session);
          }
          break;
    }
  }
  
   if (failureid){
    String result;
    if (bases == ""){bases = " ";}
    if (failureid & FLAG_MATCH_INVALID){
      for (uint8_t i(0);i<origin.lastIndexOf(bases);i++){result += "-";}
      result += "^ COMMAND RESOLUTION ERROR ";
    } else {result += "COMMAND RESOLUTION FOUND : ";}
    switch (failureid){
      case FLAG_MATCH_INVALID:
        result += "'"+bases+"' is invalid";
        if (matching != ""){result += ", do you mean '"+matching+"' ?";}
        break;
      case FLAG_MATCH_AMBIGUS | FLAG_MATCH_INVALID:
        result += "'"+bases+"' ambiguous, matching terms : '"+matching+"'";
        break;
      case FLAG_MATCH_STRVOID | FLAG_MATCH_INVALID:
        target.setCharAt(target.length()-1,'\'');
        target.replace(";","' ,'");
        result += "not enough argument, avalaible arguments are '"+String(target);
        break;
      case FLAG_MATCH_DROPPED | FLAG_MATCH_INVALID:
        result += "current command dropped";
        break;
    }
    println(result,session);
   }
  
}

/***** VERIFIED : FUNCTIONNAL *****/

void IOTProtect::addEvent(uint8_t event) {
  //Add event in memory
  if (eventlock){return;}
  eventlock = true;
  _event _local;
  const uint32_t status = getFlag();
  bool aquiresystem = takesystem();
  uint16_t counter = getLow(EEPROM_ADDR_EVCN);
  
  if (counter > ((EEPROM_AREA_EVNT[1]-EEPROM_AREA_EVNT[0]))/sizeof(_event)){
    setLow(0,EEPROM_ADDR_EVCN);
    counter = 0;
  }
  
  uint32_t time = millis()/1000;
  uint16_t hour = (time/60)/60;
  uint8_t  mins = (time%60)/60;
  uint8_t  secs = (time%60);
  
  _local.time =   (hour<<19)|(mins<<14)|(secs<<9)|(event>>8);
  _local.status = ((event & 0xff) <<24)|(status  & 0xffffff);
  setEEPROM((uint8_t *) &_local,sizeof(_event),EEPROM_ADDR_EMAP+(sizeof(_event)*counter));
  setLow(++counter,EEPROM_ADDR_EVCN);
  
  if(aquiresystem){dropsystem();}
  eventlock = false;
}

IOTProtect::_event IOTProtect::getEvent(uint16_t id) {
  _event _local;
  getEEPROM((uint8_t *) &_local,sizeof(_event),EEPROM_ADDR_EMAP+(sizeof(_event)*id));
  return _local;
}

bool IOTProtect::addTrigger(IOTProtect::_trigger trigg) {
  uint8_t id = 0;
  for (uint8_t i(0);i<getMax(TRIGGER);i++){
     _trigger _local = getTrigger(i);
     if (memcmp(&_local,NULL,sizeof(_trigger)) == 0){return ediTrigger(id, trigg);}
  }
  return false;
}

bool IOTProtect::ediTrigger(uint8_t  id,IOTProtect::_trigger trigg) {
  return setEEPROM((uint8_t *) &trigg,sizeof(_trigger),EEPROM_ADDR_TMAP+(sizeof(_trigger)*id));
}

bool IOTProtect::testTrigger(IOTProtect::_trigger trigg,bool read) {
  if (read){readSensors();}//Read to update values
  
  uint32_t temp[2] = {trigg.valpri<<16,trigg.valsec<<16};
  float val[2] = {*((float *) &temp[0]), *((float *) &temp[1])};//Convert half-precision float
  bool triggered = false;
  for (uint8_t i(0);i<8&&!triggered;i++){
    if (!(1<<i & trigg.pin)){continue;}
    if (trigg.trap>>4 & FLAG_TRIGGER_GTHN && pinbuf[i] >  val[0]){triggered =  true;}
    if (trigg.trap>>4 & FLAG_TRIGGER_EQTY && pinbuf[i] == val[0]){triggered =  true;}
    if (trigg.trap>>4 & FLAG_TRIGGER_LTHN && pinbuf[i] <  val[0]){triggered =  true;}
    if (trigg.trap>>4 & FLAG_TRIGGER_XNOT)                  {triggered = !triggered;}
    if (((trigg.trap & FLAG_TRIGGER_DISD) != FLAG_TRIGGER_DISD) && (trigg.trap & FLAG_TRIGGER_DISD)){
      if (trigg.trap & FLAG_TRIGGER_GTHN){triggered = triggered && pinbuf[i] >  val[1];}
      if (trigg.trap & FLAG_TRIGGER_EQTY){triggered = triggered && pinbuf[i] == val[1];}
      if (trigg.trap & FLAG_TRIGGER_LTHN){triggered = triggered && pinbuf[i] <  val[1];}
      if (trigg.trap & FLAG_TRIGGER_XNOT){triggered = !triggered;}
    }
    if (triggered){activeTrigger = i;addEvent(FLAG_EVENT_EFTRIG|FLAG_EVENT_ECTST);}
  }
  return triggered;
}

IOTProtect::_trigger IOTProtect::getTrigger(uint8_t  id) {
  _trigger _local;
  if (id <= getMax(TRIGGER)){getEEPROM((uint8_t *) &_local,sizeof(_trigger),EEPROM_ADDR_TMAP+(sizeof(_trigger)*id));}
  return _local;
}

uint16_t IOTProtect::getMax(IOTProtect::maxval id) {
  switch(id){
    case TRIGGER:
      return ((EEPROM_AREA_TRIG[1]-EEPROM_AREA_TRIG[0])/sizeof(_trigger));
    case EVENT:
      return ((EEPROM_AREA_EVNT[1]-EEPROM_AREA_EVNT[0])/sizeof(_event));
    case BACKUP:
      return ((EEPROM_AREA_BFSD[1]-EEPROM_AREA_BFSD[0])/sizeof(_backup));
  }
  return 0;
}

bool IOTProtect::readSensors() {
  if (!hasFlag(FLAG_STATUS_SNSET)){return false;}
  LSM6DS3 GYRO( I2C_MODE, 0x6A ); //I2C device address 0x6A
  const uint8_t pin = getPin();
  const bool gyro = !GYRO.begin();
  memset(i2cbuf, 0, 6);
  memset(pinbuf, 0, 8);
  if (hasFlag(FLAG_STATUS_ICSET)){
    if (gyro){
      i2cbuf[0] = GYRO.readFloatAccelX();
      i2cbuf[1] = GYRO.readFloatAccelY();
      i2cbuf[2] = GYRO.readFloatAccelZ();
      i2cbuf[3] = GYRO.readFloatGyroX();
      i2cbuf[4] = GYRO.readFloatGyroY();
      i2cbuf[5] = GYRO.readFloatGyroZ();
    }
  }
  if (pin){
    for (uint8_t i(0); i < 8; i++){
      if ((pin >> i) & 1){
        DallasTemperature sensor;
        OneWire oneWire(PINMAP[i]);
        sensor.setOneWire(&oneWire);
        sensor.requestTemperatures();
        sensor.requestTemperatures();
        pinbuf[i] = sensor.getTempCByIndex(0);
        if (pinbuf[i] == -127.0){pinbuf[i] = analogRead(PINMAP[i]);}
      }
    }
  }
  return true;
}

bool IOTProtect::sendBackup() {
  if (!WiFi.isConnected() || mqtt.connected()){return false;}
  if (getLow(EEPROM_ADDR_BKCN) == 0){return true;}
  String main_topic = getString(EEPROM_ADDR_MTOP);
  for (uint16_t i(0);i<getLow(EEPROM_ADDR_BKCN);i++){
    _backup _local = getBackup(i);
    uint32_t value = _local.value << 16;
    String topic;
    switch (_local.trpin & 0x0f){
      case 0:case 1:case 2:case 3:case 4:case 5:case 6:case 7:
        topic = getString(EEPROM_ADDR_SP00+EEPROM_SIZE_CSTR*(_local.trpin&0x0f));
        break;
      case 8:case 9:case 10:case 11:case 12:case 13:
        topic = String((_local.trpin & 0x0f) & 4 ? "GYR":"ACC")+('X'+(_local.trpin & 0x0f)-8);
        break;
    }
    mqtt.publish(main_topic+"/"+topic,String(*((float *) &value)));
  }
  const bool aquired = takesystem();
  setLow(0,EEPROM_ADDR_BKCN);
  if (aquired){dropsystem();}
  return true;
}

bool IOTProtect::sendSensors() { 
  readSensors();
  uint16_t delayrecover = getLow(EEPROM_ADDR_STSD);
  
  String main_topic = getString(EEPROM_ADDR_MTOP);
  if (main_topic[main_topic.length()-1] != '/'){main_topic += '/';}
  
  const String type[2] = {"ACC", "GYR"};
  const char *id =  "XYZ";
  const uint8_t pin = getPin();
  const float i2cmem[6] = {0, 0, 0, 0, 0, 0};
  uint16_t addr = EEPROM_ADDR_SP00;
  String bssid;
  bool ret = true;
  
  readSensors();
 
  if (mqtt.connected()){
    for (uint8_t i(0); i < 6; i++){bssid += String(WiFi.BSSID()[i], HEX) + (i < 5 ? ':' : '\0');}
    mqtt.publish(main_topic + "BSSID/0",bssid);delay(25);
  }
  
  if (hasFlag(FLAG_STATUS_SNSET)){
    if (hasFlag(FLAG_STATUS_ICSET)) {
      if (memcmp(i2cbuf,i2cmem,6)){
        for (uint8_t i(0); i < 6; i++){
          String sup = "/"+String(abs(i2cbuf[i]) > 0.5 && (i < 3));
          if (mqtt.connected()){mqtt.publish(main_topic + type[i >= 3] + id[i-3*(i>=3)]+sup, String(i2cbuf[i]));delay(25);}
          else if (hasFlag(FLAG_STATUS_MMSET)){
            _backup _local;
            _local.value = (*(uint32_t *) &i2cbuf[i])>>16;
            _local.trpin |= 8+i;
            addBackup(_local);
          } else {ret = false;}
        }
      }
    }
  
    for (uint8_t i(0); i < 8; i++) {
      if ((pin >> i) & 1){
        String topic = getString(addr);
        bool triggered = false;
        uint8_t trigger_id = 255;
        for (byte x(0);x<getMax(TRIGGER) && !triggered;x++){
          _trigger _local = getTrigger(x);
          triggered = testTrigger(_local);
          if (triggered){
            char _topic[25];
            memcpy(_topic,_local.topic,25);
            topic = String(_topic);
            if (trigger_id > x){
              delayrecover = _local.alttime;
              trigger_id = x;
              while (issystem()){}
              takesystem();
              addEvent(FLAG_EVENT_EFTRIG|FLAG_EVENT_ECTST);
              addFlag(FLAG_STATUS_TRSET,false);
              dropsystem();
            }
          }
        }
        if (topic ==""){topic = "temperature/0";}
        if(mqtt.connected()){mqtt.publish(main_topic + topic, String(pinbuf[i]));delay(150);}
        else if (hasFlag(FLAG_STATUS_MMSET)){
          _backup _local;
          _local.value = (*(uint32_t *) &pinbuf[i])>>16;
          _local.trpin |= (trigger_id)<<4|i;
          addBackup(_local);
        } else {ret = false;}
      }
      addr += EEPROM_SIZE_CSTR;
    }
  }
  sleeptime = delayrecover;
  return ret;
}

void IOTProtect::sleep() {
  if (hasFlag(FLAG_STATUS_DPSET) && (!hasFlag(FLAG_STATUS_TRSET) ||
                                    ( hasFlag(FLAG_STATUS_TRSET) && !hasFlag(FLAG_STATUS_HDSET)))){
    mqtt.disconnect();
    println("SLEEPING FOR : "+String((sleeptime/60)/60)+":"+String((sleeptime%60)/60)+":"+String(sleeptime%60),ALL);
    ESP.deepSleep(sleeptime*1000000,RF_NO_CAL);
  }
  if(takesystem()){delFlag(FLAG_STATUS_TRSET,false);dropsystem();}
}

bool IOTProtect::update() {
  if (hasFlag(FLAG_STATUS_WAKEP)){
    if (getLow(EEPROM_ADDR_BKCN)){println("SENDING BACKUP : "+String(sendBackup() ? STR_MSG_SUCCESS:STR_MSG_FAILURE),ALL);} 
    println("SENDING SENSORS : "+String(sendSensors() ? STR_MSG_SUCCESS:STR_MSG_FAILURE),ALL);
    sleep();
  }

  if (hasFlag(FLAG_STATUS_WFSET)){
    if (WiFi.status() != WL_CONNECTED){
      String SSID = getString(EEPROM_ADDR_LSID);
      String PASS = getString(EEPROM_ADDR_LWPS);
      if (SSID != ""){
        println("CONNECTING TO : '"+SSID+"'",ALL);
        WiFi.begin(SSID,PASS);
        WiFi.setAutoConnect(true);
        for(uint8_t stamp(16);stamp>0 && !WiFi.isConnected();stamp--) {delay(150);}
      }
    }
  } else {if (WiFi.isConnected()){WiFi.disconnect();}}

  if (hasFlag(FLAG_STATUS_MQSET)){
    if (hasFlag(FLAG_STATUS_SMSET) && !isclientssl){
      //secureclient.setFingerprint(getFingerPrint().c_str());
      mqtt.disconnect();
      isclientssl = true;
    } else if (!hasFlag(FLAG_STATUS_SMSET) && isclientssl){
      mqtt.disconnect();
      isclientssl = false;
    }
    
    if (WiFi.isConnected() && !mqtt.connected()) {
      const String address = getString(EEPROM_ADDR_SADR);
      const String ident   = getString(EEPROM_ADDR_MIDN);
      const String login   = getString(EEPROM_ADDR_MLIN);
      const String pass    = getString(EEPROM_ADDR_MPSS);
      print("LINKING TO : "+address+":"+String(getLow(EEPROM_ADDR_SSPO))+" ",ALL);
      if (hasFlag(FLAG_STATUS_SMSET)){mqtt.begin(address.c_str(),getLow(EEPROM_ADDR_SSPO),secureclient);}
      else {mqtt.begin(address.c_str(),getLow(EEPROM_ADDR_SSPO),client);}
      
      if (login.length()) {
        if (pass.length()) {
          for(uint8_t stamp(5);stamp>0 && !mqtt.connected();stamp--)      {mqtt.connect(ident.c_str(),login.c_str(),pass.c_str(),false);delay(250);}
        } else {for(uint8_t stamp(5);stamp>0 && !mqtt.connected();stamp--){mqtt.connect(ident.c_str(),login.c_str(),false);delay(250);}}
      } else {for(uint8_t stamp(5);stamp>0 && !mqtt.connected();stamp--)  {mqtt.connect(ident.c_str(),false);delay(250);}}

      if (mqtt.connected()) {
        println(STR_MSG_SUCCESS,ALL);
        const uint16_t deepsleep = getLow(EEPROM_ADDR_STSD);
        const uint16_t timeout =    getLow(EEPROM_ADDR_MTOT);
        if (deepsleep && timeout) {mqtt.setOptions(deepsleep + timeout, true, timeout * 1000);}
      } else {println(STR_MSG_FAILURE,ALL);}
    }
  } else {if (mqtt.connected()){mqtt.disconnect();}}

  if (hasFlag(FLAG_STATUS_SRSET)){if (!Serial){Serial.begin(74880);println(STR_MSG_WELCOME);}}
  else {if (Serial){println("SERIAL END");Serial.end();}}
  
  if (hasFlag(FLAG_STATUS_TNSET)){
    if (hasFlag(FLAG_STATUS_SLSET)){
      if (!isserverssl && serverstart){
        telnetServer.stop();
        serverstart=false;
        for (uint8_t i(0);i<8;i++){externSession[i].client.stop();externSession[i].status = 0;}
        println("TELNET SERVER DOWN");
      }
      if (WiFi.isConnected() && !serverstart){
        secureServer = WiFiServerSecure(getLow(EEPROM_ADDR_SLPO));
        secureServer.setRSACert(new BearSSL::X509List(STR_SSL_CERT),new BearSSL::PrivateKey(STR_SSL_PKEY));
        secureServer.begin();
        serverstart = true;
        isserverssl = true;
        println("SSL   SERVER BEGIN");
      }
    } else {
      if ( isserverssl && serverstart){
        secureServer.stop();
        isserverssl=false;serverstart=false;
        for (uint8_t i(0);i<8;i++){externSession[i].client.stop();externSession[i].status = 0;}
        println("SECURE SERVER DOWN");
      }
      if (WiFi.isConnected() && !serverstart){
        telnetServer = WiFiServer(getLow(EEPROM_ADDR_SMPO));
        telnetServer.begin();
        serverstart = true;
        println("PLAIN SERVER BEGIN");
     }
   }
  } else {
    if (serverstart){
      if (isserverssl){secureServer.stop();println("SECURE SERVER DOWN");}
      else {telnetServer.stop();println("TELNET SERVER DOWN");}
      serverstart = false;
      isserverssl = false;
    }
  }
  return true;
}

int8_t IOTProtect::flacmp(uint8_t* buffer, uint16_t size, uint16_t addr) {
  uint8_t compare[size];
  getEEPROM(compare, size, addr);
  return memcmp(buffer, compare, size) == 0;
}

bool IOTProtect::setPin(uint8_t pin) {return setLow(pin,EEPROM_ADDR_PINS);}
uint8_t IOTProtect::getPin() {return getLow(EEPROM_ADDR_PINS);}

bool IOTProtect::checkPass(String key) {
  uint8_t hash[EEPROM_SIZE_CSTR];
  bool ret;
  const bool aquiresystem = takesystem();
  hashkey(key,hash);
  ret = flacmp(hash,EEPROM_SIZE_CSTR,EEPROM_ADDR_ISHA);
  if(aquiresystem){dropsystem();}
  addEvent(FLAG_EVENT_EFADMN | FLAG_EVENT_ECTST | (ret ? 0:FLAG_EVENT_ECFAL));
  return ret;
}

bool IOTProtect::access(uint16_t size, uint16_t addr,_session *session) {
  bool ret = false;
  if (hasFlag(FLAG_STATUS_SYSET,session)){ret = true;}
  else if ((addr >= EEPROM_AREA_CONF[0]) && (addr <= EEPROM_AREA_CONF[1])){
    if ((addr >= EEPROM_AREA_LOWS[0]) && (addr <= EEPROM_AREA_LOWS[1])){
      ret = isFlag(FLAG_STATUS_ADMIN|FLAG_STATUS_CONFG,session) && size == EEPROM_SIZE_CLOW;
    } else if ((addr >= EEPROM_AREA_STR[0]) && (addr <= EEPROM_AREA_STR[1])){
      ret = isFlag(FLAG_STATUS_ADMIN|FLAG_STATUS_CONFG,session) && size <= EEPROM_SIZE_CSTR;
      if ((addr >= EEPROM_AREA_HSEC[0]) && (addr <= EEPROM_AREA_HSEC[1])){ret = ret && !hasFlag(FLAG_STATUS_SAFEM);}
      else if (addr == EEPROM_ADDR_ISHA){ret = ret && size == EEPROM_SIZE_CSTR;}
    } else {ret = isFlag(FLAG_STATUS_CONFG|FLAG_STATUS_ADMIN,session) && !hasFlag(FLAG_STATUS_SAFEM);}
  } else if ((addr == EEPROM_AREA_FING[0]) && (addr+size == EEPROM_AREA_FING[1])){ret = isFlag(FLAG_STATUS_CONFG|FLAG_STATUS_ADMIN,session) && !hasFlag(FLAG_STATUS_SAFEM);}
  return ret;
}

bool IOTProtect::hashkey(String key, uint8_t *hash) {
  SHA3_256 Hash = SHA3_256();
  Hash.update(key.c_str(),key.length());
  Hash.finalize(hash, EEPROM_SIZE_CSTR);
  return true;
}

void IOTProtect::checkConf() {
  if (!WiFi.isConnected() && !mqtt.connected() && !isFlag(FLAG_STATUS_MMSET)){//Test WiFi then MQTT then if backup is active
    const bool aquiresystem = takesystem();
    addFlag(FLAG_STATUS_CONFG);//Switch to config mode
    if (!(hasFlag(FLAG_STATUS_TRSET|FLAG_STATUS_SRSET))){addFlag(FLAG_STATUS_TRSET|FLAG_STATUS_SRSET);}//Enable interfaces if needed
    if (aquiresystem){dropsystem();}
  }
  addEvent(FLAG_EVENT_EFCNFG|FLAG_EVENT_ECTST|(hasFlag(FLAG_STATUS_CONFG) ? FLAG_EVENT_ECFAL:0));//Register Event
}

String IOTProtect::getString(uint16_t addr) {
  uint8_t str[EEPROM_SIZE_CSTR+1];
  memset(str,0,EEPROM_SIZE_CSTR+1);
  getEEPROM(str,EEPROM_SIZE_CSTR,addr);
  return String((char *)str);
}

bool IOTProtect::setString(uint16_t addr,String str) {
  uint8_t data[EEPROM_SIZE_CSTR];
  memset(data,NULL,EEPROM_SIZE_CSTR);
  memcpy(data,str.c_str(),EEPROM_SIZE_CSTR);
  return setEEPROM(NULL, EEPROM_SIZE_CSTR, addr) && setEEPROM(data,EEPROM_SIZE_CSTR, addr);
}

uint16_t IOTProtect::getLow(uint16_t addr) {
  uint8_t value[2];
  getEEPROM(value, 2, addr);
  return value[0]<<8|value[1];
}

bool IOTProtect::setLow(uint16_t value, uint16_t addr) {
  uint8_t temp[2] = {value >> 8,value & 0xff};
  return setEEPROM(temp, 2, addr);
}

uint32_t IOTProtect::getMed(uint16_t addr) {
  uint8_t value[4];
  getEEPROM(value, 4, addr);
  return value[0]<<24|value[1]<<16|value[2]<<8|value[3];
}

bool IOTProtect::setMed(uint32_t value, uint16_t addr) {
  uint8_t temp[4] = {value>>24|value>>16|value>>8|value};
  return setEEPROM(temp, 4, addr);
}

void IOTProtect::checkCRC() {
  uint8_t crcmem[4];
  getEEPROM(crcmem, 4, EEPROM_ADDR_ICRC);//Recovering CRC
  uint32_t crcpool = crcmem[0]<<24|crcmem[1]<<16|crcmem[2]<<8|crcmem[3];
  if(!((getCRC() == crcpool) && crcpool)){//CRC is zero or differ from calculated
    const bool aquiresystem = takesystem();
    addFlag(FLAG_STATUS_RESET,false);//SET RESET MODE
    if (aquiresystem){dropsystem();}
  }
  addEvent(FLAG_EVENT_EFINTY|FLAG_EVENT_ECTST|(hasFlag(FLAG_STATUS_RESET) ? FLAG_EVENT_ECTST:0));
}

bool IOTProtect::setCRC() {
  uint8_t crcpool[4];
  bool ret = false;
  bool aquired = takesystem();
  getEEPROM(crcpool, 4, EEPROM_ADDR_ICRC);
  if (setEEPROM(crcpool, 4, EEPROM_ADDR_OCRC)) {
    uint32_t crc = getCRC();
    crcpool[0] = (crc >> 24) & 0xff;crcpool[1] = (crc >> 16 & 0xff);
    crcpool[2] = (crc >>  8) & 0xff;crcpool[3] = (crc & 0xff);
    ret = setEEPROM(crcpool, 4, EEPROM_ADDR_ICRC);
  }
  if (aquired){dropsystem();}
  return ret;
}

uint32_t IOTProtect::getCRC() {
  CRC32 crc;
  uint16_t count = EEPROM_AREA_CRC[1] - EEPROM_AREA_CRC[0];
  uint8_t crcpool[count];
  getEEPROM(crcpool, count, EEPROM_AREA_CRC[0]);
  for (uint16_t counter(0);counter < count; counter++) {crc.update(crcpool[counter]);}
  return crc.finalize();
}

bool IOTProtect::checkEEPROM() {
  bool ret = true;
  uint8_t fk_buffer,cp_buffer;
  for (uint16_t i(0);i<EEPROM_SIZE_FULL && ret;i++){
    getEEPROM(&fk_buffer,1,EEPROM_HEAD_CONF+i);
    setEEPROM(&fk_buffer,1,EEPROM_HEAD_CONF+i);
    getEEPROM(&cp_buffer,1,EEPROM_HEAD_CONF+i);
    ret = fk_buffer == cp_buffer;
  }
  return ret;
}

void IOTProtect::getEEPROM(uint8_t *buffer, uint16_t size, uint16_t addr) {
  for (uint16_t counter(0); counter < size; counter++) {buffer[counter] = EEPROM.read(addr++);}
}

bool IOTProtect::setEEPROM(uint8_t *buffer, uint16_t size, uint16_t addr) {
  if (hasFlag(FLAG_STATUS_RESET)){buffer = NULL;size = EEPROM_SIZE_FULL;addr = EEPROM_HEAD_CONF;}
  else if (!access(size, addr)){Serial.println("ACCESS DENIED");return false;}
  noInterrupts();
  for (uint16_t counter(0); counter < size; counter++){
    uint8_t value,write = buffer ? buffer[counter]:0;
    getEEPROM(&value,1,addr);
    if (value != write){
      EEPROM.write(addr++, write);
      if (!EEPROM.commit()){addEvent(FLAG_EVENT_EFEEPM|FLAG_EVENT_ECWRT|FLAG_EVENT_ECFAL);}
    } else {addr++;}
  }
  addr -= size+1;
  if ((addr >= (EEPROM_AREA_CRC[0]+EEPROM_SIZE_CMED)) && ((addr+size) <= EEPROM_AREA_CRC[1])){setCRC();}
  interrupts();
  return true;
}

bool IOTProtect::takesystem() {
  uint32_t flag = getFlag();
  if (flag & FLAG_STATUS_SYSET){return false;}
  flag |= FLAG_STATUS_SYSET;
  for (uint8_t i(0);i<4;i++){EEPROM.write(EEPROM_ADDR_STAT+i,flag>>(24-(8*i))&0xff);}
  return EEPROM.commit();
}

bool IOTProtect::issystem(){return hasFlag(FLAG_STATUS_SYSET);}

bool IOTProtect::dropsystem() {
  uint32_t flag = getFlag();
  if (!(flag & FLAG_STATUS_SYSET)){return false;}
  flag &= ~FLAG_STATUS_SYSET;
  for (uint8_t i(0);i<4;i++){EEPROM.write(EEPROM_ADDR_STAT+i,flag>>(24-(8*i))&0xff);}
  return EEPROM.commit();
}

uint32_t IOTProtect::getFlag(IOTProtect::_session *session) {
  if (session){return session->status;}
  uint8_t state[4];
  getEEPROM(state, 4, EEPROM_ADDR_STAT);
  return (state[0]<<24)|(state[1]<<16)|(state[2]<<8)|state[3];
}

bool IOTProtect::setFlag(uint32_t flag, bool update,IOTProtect::_session *session) {
  if (session){
    if (issystem() || session->status & FLAG_STATUS_ADMIN){session->status = flag;return true;}
    return false;
  }
  uint8_t state[4] = {flag >> 24,flag >> 16,flag >> 8, flag & 0xff};
  return setEEPROM(state, 4, EEPROM_ADDR_STAT) && ((update && this->update()) || true);
}

bool IOTProtect::setInternalKey(String &key) {
  uint8_t hash[EEPROM_SIZE_CSTR];
  hashkey(key,hash);
  return setInternalKey(hash);
}

bool IOTProtect::setInternalKey(uint8_t *hash) {return setEEPROM(hash, EEPROM_SIZE_CSTR, EEPROM_ADDR_ISHA);}
bool IOTProtect::addFlag(uint32_t flag, bool update,IOTProtect::_session *session) {return setFlag(getFlag(session) | flag,update,session);}
bool IOTProtect::delFlag(uint32_t flag, bool update,IOTProtect::_session *session) {return setFlag(getFlag(session) &~flag,update,session);}
bool IOTProtect::hasFlag(uint32_t flag,IOTProtect::_session *session) {return  getFlag(session) & flag;}
bool IOTProtect::isFlag (uint32_t flag,IOTProtect::_session *session) {return (getFlag(session) & flag) == flag;}

IOTProtect::~IOTProtect(){}
