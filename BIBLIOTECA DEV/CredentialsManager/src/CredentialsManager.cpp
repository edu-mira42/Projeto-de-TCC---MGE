#define FS_NO_GLOBALS
#include <Arduino.h>
#include "CredentialsManager.h"
#include <SPIFFS.h>
#include <SD.h>
#include <SPI.h>
#include <Wire.h>


CredentialsManager::CredentialsManager(){}


void CredentialsManager::begin(int baudRate){
  Serial.begin(baudRate);
  if(!SPIFFS.begin(true)){
    Serial.println("An Error has occurred while mounting SPIFFS...");
    delay(100);
    Serial.print("Restarting microcontroller");
    ESP.restart();
    return;
  }else{
    Serial.println("SPIFFS mounted successfully!");
  }

  SD.begin(5);  
  if(!SD.begin(5)) {
    Serial.println("Card Mount Failed");
    return;
  }
  uint8_t cardType = SD.cardType();
  if(cardType == CARD_NONE) {
    Serial.println("No SD card attached");
    return;
  }
  Serial.println("Initializing SD card...");
  if (!SD.begin(5)) {
    Serial.println("ERROR - SD card initialization failed!");
    return;
  }
}

///////////////////////////////////////////////Read functions//////////////////////////////////////////////////


String CredentialsManager::readFile(const char * fileName){
  fs::File file = SPIFFS.open(fileName, "r");
  if(!file){
    return String();
  }
  String fileContent;
  while(file.available()){
    fileContent+=String((char)file.read());
  }
  file.close();
  return fileContent;
}

int CredentialsManager::readIntValue(const char* fileName, String content){
  fs::File file = SPIFFS.open(fileName);
  while(file.available()){
    content = file.readString();
  }
  file.close();
  return(content.toInt());
}

int CredentialsManager::getSensorOP(String content){
  fs::File file = SPIFFS.open("/sensorOP.txt");
  while(file.available()){
    content = file.readString();
  }
  file.close();
  return(content.toInt());
}


const char* CredentialsManager::getSSID(String ssid){
  fs::File file = SPIFFS.open("/ssid.txt");
    while(file.available()){
      ssid = file.readString();
    }
    file.close();
    return(ssid.c_str());
}


const char* CredentialsManager::getPASS(String pass){
  fs::File file1 = SPIFFS.open("/pass.txt");
    while(file1.available()){
      pass = file1.readString();
    }
    file1.close();
    return(pass.c_str());
}

const char* CredentialsManager::getSSIDAP(String ssidAP){
  fs::File file2 = SPIFFS.open("/ssidAP.txt");
    while(file2.available()){
      ssidAP = file2.readString();
    }
    file2.close();
    return(ssidAP.c_str());
}

const char* CredentialsManager::getPASSAP(String passAP){
  fs::File file3 = SPIFFS.open("/passAP.txt");
    while(file3.available()){
      passAP = file3.readString();
    }
    file3.close();
    return(passAP.c_str());
}

const char* CredentialsManager::getUserHTTP(String userHTTP){
  fs::File file4 = SPIFFS.open("/userHTTP.txt");
    while(file4.available()){
      userHTTP = file4.readString();
    }
    file4.close();
    return(userHTTP.c_str());
}

const char* CredentialsManager::getPassHTTP(String passHTTP){
  fs::File file5 = SPIFFS.open("/passHTTP.txt");
    while(file5.available()){
      passHTTP = file5.readString();
    }
    file5.close();
    return(passHTTP.c_str());
}

int CredentialsManager::getOPmode(){
  int opMode;
  fs::File file6 = SPIFFS.open("/opMode.txt");
    while(file6.available()){
      opMode = file6.readString().toInt();
    }
    file6.close();
    return(opMode);
}

int CredentialsManager::getAPmode(){
  int apMode;
  fs::File file7 = SPIFFS.open("/apMode.txt");
    while(file7.available()){
      apMode = (file7.readString()).toInt();
    }
    file7.close();
    return(apMode);
}

/////////////////////////////////////////////Write functions////////////////////////////////////////////////////

void CredentialsManager::format(){
  bool format = SPIFFS.format();
  if(format == true){
    Serial.println("Memory formatted successifully!");
  }
}

void CredentialsManager::writeFile(const char * fileName, const char * message){
  fs::File file = SPIFFS.open(fileName, "w");
  if(!file){
    return;
  }
  file.print(message);
  file.close();
}

void CredentialsManager::appendFile(const char * fileName, const char * message){
  fs::File file = SPIFFS.open(fileName, "a");
  if(!file){
    return;
  }
  file.println(message);
  file.close();
}

void CredentialsManager::writeIntValue(const char* fileName, int value){
  fs::File file = SPIFFS.open(fileName, FILE_WRITE);
    file.print(value);
    file.close();
    Serial.print("Written ");
    Serial.print(value);
    Serial.print(" into file ");
    Serial.println(fileName);
}

void CredentialsManager::saveSensorOP(int value){
  fs::File file = SPIFFS.open("/sensorOP.txt", FILE_WRITE);
    file.print(value);
    file.close();
}

void CredentialsManager::saveSSID(const char* SSID){
  fs::File file8 = SPIFFS.open("/ssid.txt", FILE_WRITE);
    file8.print(SSID);
    file8.close();
}

void CredentialsManager::savePASS(const char* PASS){
  fs::File file9 = SPIFFS.open("/pass.txt", FILE_WRITE);
    file9.print(PASS);
    file9.close();
}

void CredentialsManager::saveSSIDAP(const char* SSIDAP){
  fs::File file10 = SPIFFS.open("/ssidAP.txt", FILE_WRITE);
    file10.print(SSIDAP);
    file10.close();
}

void CredentialsManager::savePASSAP(const char* PASSAP){
  fs::File file11 = SPIFFS.open("/passAP.txt", FILE_WRITE);
    file11.print(PASSAP);
    file11.close();
}

void CredentialsManager::saveUserHTTP(const char* USERHTTP){
  fs::File file12 = SPIFFS.open("/userHTTP.txt", FILE_WRITE);
    file12.print(USERHTTP);
    file12.close();
}

void CredentialsManager::savePassHTTP(const char* PASSHTTP){
  fs::File file13 = SPIFFS.open("/passHTTP.txt", FILE_WRITE);
    file13.print(PASSHTTP);
    file13.close();
}

void CredentialsManager::saveOPmode(int opM){
  fs::File file14 = SPIFFS.open("/opMode.txt", FILE_WRITE);
    file14.print(opM);
    file14.close();
}

void CredentialsManager::saveAPmode(int apM){
  fs::File file15 = SPIFFS.open("/apMode.txt", FILE_WRITE);
    file15.print(apM);
    file15.close();
}

/////////////////////////////////////////////SD CARD write functions////////////////////////////////////////////////////

void CredentialsManager::writeFileSD(const char * fileName, const char * message){
  File file = SD.open(fileName, "w");
  if(!file){
    return;
  }
  file.print(message);
  file.close();
}

void CredentialsManager::appendFileSD(const char * fileName, const char * message){
  File file = SD.open(fileName, "a");
  if(!file){
    return;
  }
  file.println(message);
  file.close();
}

void CredentialsManager::writeIntValueSD(const char* fileName, int value){
  File file = SD.open(fileName, FILE_WRITE);
    file.print(value);
    file.close();
    Serial.print("Written ");
    Serial.print(value);
    Serial.print(" into file ");
    Serial.println(fileName);
}

/////////////////////////////////////////////SD CARD read functions////////////////////////////////////////////////////

String CredentialsManager::readFileSD(const char * fileName){
  File file = SD.open(fileName, "r");
  if(!file){
    return String();
  }
  String fileContent;
  while(file.available()){
    fileContent+=String((char)file.read());
  }
  file.close();
  return fileContent;
}

int CredentialsManager::readIntValueSD(const char* fileName, String content){
  File file = SD.open(fileName);
  while(file.available()){
    content = file.readString();
  }
  file.close();
  return(content.toInt());
}