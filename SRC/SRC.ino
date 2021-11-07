#include "EmonLib.h"
#include <driver/adc.h>
#include <CredentialsManager.h>
#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include <SPIFFS.h>
#include <Wire.h>
#include <LiquidCrystal.h>
#include <AsyncTCP.h>
#include <AsyncElegantOTA.h>
#include <WiFiAP.h>
#include "time.h"

const char* ntpServer = "pool.ntp.org";
const long  gmtOffset_sec = -3 * 3600;
const int   daylightOffset_sec = 0;

CredentialsManager cmg;                // Cria uma instância para gerenciar memoria SPIFFS
EnergyMonitor CT013;                   // Cria uma instância para o sensor de corrente
EnergyMonitor CT013_2;                 // Cria uma instância para o sensor de corrente
EnergyMonitor ZMPT;                    // Cria uma instância para o sensor de tensão

LiquidCrystal lcd(13, 12, 14, 27, 26, 25); // Define os terminais do display LCD 20x4 (RS, E, D4, D5, D6, D7)

AsyncWebServer server(80);                 // Define a porta 80 para o WebServer
AsyncWebSocket ws("/ws");                  // Cria uma instância para o WebSocket

int modeSelect = 17;                       // Terminal do botão

int i = 0;

int restart = 0;

float conskw;
float lastConskw;
float calibration;

double Vrms = 0;
double Irms = 0;
double Irms1 = 0;
double IrmsTotal = 0;
double Prms = 0;

int millisNow = 0;

int opMode = 0;
int apMode = 0;
int sensorOP = 1;

int intervalo = 0;
int NT = 0;
int D = 0;

int page = 0;

unsigned long previousTime;

String relatorioCSV;

float TENS;
float previousTENS;
float writeCicle = 0;

unsigned long tempo = 0;
unsigned long WifiAttempt = 30000;
unsigned long pageDelay = 0;
unsigned long now = 0;
unsigned long restartClock = 0;
unsigned long storeCons = 0;

String qsid = "";
String qpass = "";
String APsid = "";
String APpass = "";
String USER = "";
String PASSW = "";
String calibrationValue = "";
String Voltage = "";

String acesso = "";
String senha = "";
String acessoAP = "";
String senhaAP = "";
String userHTTP = "";
String passHTTP = "";
String cValue = "";
String Cvoltage = "";

const char* PARAM_INPUT_1 = "input1";
const char* PARAM_INPUT_2 = "input2";
const char* PARAM_INPUT_3 = "input3";
const char* PARAM_INPUT_4 = "input4";
const char* PARAM_INPUT_5 = "input5";
const char* PARAM_INPUT_6 = "input6";
const char* PARAM_INPUT_7 = "input7";
const char* PARAM_INPUT_8 = "input8";

#define ADC_INPUT 34                   // Define o terminal em que o sensor esta conectado
#define ADC_BITS    10                 // Define a resolucao em bits
#define ADC_COUNTS  (1<<ADC_BITS)


/*------------------------------------------ CARREGA FUNÇÕES PARA O SETUP ------------------------------------------*/
void handleWebSocketMessage(void *arg, uint8_t *data, size_t len);
void onEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type,
             void *arg, uint8_t *data, size_t len);
void initWebSocket(void);
String processor(const String& var);
void notFound(AsyncWebServerRequest *request);
void configWebPages(void);
void getWifi(void);
void genData(void);
//void sensorOP();

/*-------------------------------- ENVIA OS DADOS PARA O WEBSERVER ----------------------------------*/

String getVoltage() {
  float V1 = Vrms;
  return String(V1);
}

String getCurrent() {
  float current = Irms;
  if (current <= 0.2) {
    current = 0;
  }
  return String(current);
}

String getCurrent1() {
  float current1 = Irms1;
  if (current1 <= 0.2) {
    current1 = 0;
  }
  return String(current1);
}

String getPower() {
  float power = Prms;
  if (power < 15.34) {
    power = 0;
  }
  return String(power);
}

String getConsumption() {
  return String(conskw);
}


/*------------------------------------------------ CONFIGURA O FUNCIONAMENTO DO MICROCONTROLADOR ------------------------------------------------*/

void setup() {
  Serial.begin(115200);
  cmg.begin();                                                // Inicia a biblioteca que salva as informações na memória flash
  lcd.begin(20, 4);                                           // Inicia o display LCD 20x4
  
  adc1_config_channel_atten(ADC1_CHANNEL_7, ADC_ATTEN_DB_11); // Cofigura a atenuação de ruido do canal 6 e 5 em 11dB (canal 6 corresponde ao canal do terminal 35, 34 e 33)
  adc1_config_channel_atten(ADC1_CHANNEL_6, ADC_ATTEN_DB_11);
  adc1_config_channel_atten(ADC1_CHANNEL_5, ADC_ATTEN_DB_11);
  analogReadResolution(10);                                   // Configura a resolução analógica para 10 bits (Necessário para a bibloteca EmonLib)
  
  pinMode(modeSelect, INPUT);                                 // Define o terminal do botão como entrada
  
  getOPmode();                                                // Define o modo de operação
  configWebPages();                                           // Configura todas as páginas a serem exibidas no navegador
  getWifi();                                                  // Inicia o Wi-Fi
  
  genData();                                                  // Informações gerais armazenadas na memória flash
  sensorOP = 3; //remover depois
  if(sensorOP == 1){
    CT013.current(34, calibration);                             // Corrente: Terminal de entrada, calibração
    CT013_2.current(33, calibration);                           // Corrente: Terminal de entrada, calibração
    ZMPT.voltage(35, 106.8, 1.7);                                // Tensão: Termnial de entrada, fator de calibração, phase shift (Defasagem em relação a medição e o observado pelo microcontrolador)
  }
  else if(sensorOP == 2){
    CT013.current(34, calibration);                             // Corrente: Terminal de entrada, calibração
    ZMPT.voltage(35, 106.8, 1.7);                                // Tensão: Termnial de entrada, fator de calibração, phase shift (Defasagem em relação a medição e o observado pelo microcontrolador)
  }
  else if(sensorOP == 3){
    CT013.current(34, calibration);                             // Corrente: Terminal de entrada, calibração
    CT013_2.current(33, calibration);                           // Corrente: Terminal de entrada, calibração
    Vrms = TENS;
  }
  else if(sensorOP == 4){
    CT013.current(34, calibration);                             // Corrente: Terminal de entrada, calibração
    Vrms = TENS;
  }
  else{
    sensorOP = 1;
  }

  //sensorOP();                                                 // Configura os sensores de acordo com o modo de operação selecionado
  
  server.begin();                                             // Inicia a comunicação de servidor via Wifi
  AsyncElegantOTA.begin(&server);                             // Inicia o WebServer assíncrono
  showIP();                                                   // Mostra o IP do microcontrolador no display LCD
  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);   // Configura a hora e data
}

///////////////////////////////////////////// void Loop ///////////////////////////////////////////////////////////////

void loop() {
  if(sensorOP == 1){
    ZMPT.calcVI(20, 1200);
    Irms = CT013.calcIrms(1200);
    Irms1 = CT013_2.calcIrms(1200);  // Calcula a corrente RMS (prv 1200)
    Vrms = ZMPT.Vrms;
  }
  else if(sensorOP == 2){
    ZMPT.calcVI(20, 1200);
    Irms = CT013.calcIrms(1200);
    Vrms = ZMPT.Vrms;
  }
  else if(sensorOP == 3){
    Irms = CT013.calcIrms(1200);
    Irms1 = CT013_2.calcIrms(1200);  // Calcula a corrente RMS (prv 1200)
  }
  else if(sensorOP == 4){
    Irms = CT013.calcIrms(1200);
  }

  if (Irms <= 0.2) {
    Irms = 0;
  }
  if (Irms1 <= 0.2 && (sensorOP == 1 || sensorOP == 3)) {
    Irms1 = 0;
  }
  
  // Calcula o consumo durante entre a última vez que foi calculado e agora
  IrmsTotal = Irms + Irms1;
  Prms = (IrmsTotal) * (Vrms);
  String t = String(millis() - previousTime);               // Calcula o tempo que se passou entre o último cálculo e o atual
  previousTime = millis();                                  // Variável que armazena o momento do último cálculo
  lastConskw = conskw;
  conskw = lastConskw + ((Prms) / 1000) * (t.toFloat() / 1000 / 3600); // Soma o valor do último cálculo de consumo e o consumo atual em kWh (tempo gasto desde o último cálculo X potência
  
  relatorio(); // Gera o relatório
  nextPage();  // Controla as "páginas" do display
  serialOut(); // Envia as informações para a porta serial

  if(restart == 1){                       // Reinicia o microcontrolador
    if(millis() - restartClock >= 4000){
      ESP.restart();
    }
  }
  if(restart == 2){                         // Limpa o relatório simples
    cmg.writeFile("/cons.txt", "");
    cmg.writeFile("/consCSV.txt", "");
    cmg.writeFile("/writeCicle.txt", "0");
    restart = 0;
  }
  if(restart == 3){                         // Limpa o relatório CSV
    cmg.writeFile("/cons.txt", "");
    cmg.writeFile("/consCSV.txt", "");
    conskw = 0;
    restart = 0;
  }
    
  AsyncElegantOTA.loop();
  ws.cleanupClients();
  reconnect_wifi();
}

void serialOut() {
  if (Irms <= 0.2) {
    Irms = 0;
    Serial.print("A: "); Serial.print(IrmsTotal); Serial.print("  ");
    Serial.print("W: "); Serial.print(Prms); Serial.print("  ");
    Serial.print("KWh: "); Serial.print(conskw); Serial.print("  ");
    Serial.println("");
  }

  else if (Irms > 0.2) {
    Serial.print("A: "); Serial.print(IrmsTotal); Serial.print("  ");
    Serial.print("W: "); Serial.print(Prms); Serial.print("  ");
    Serial.print("KWh: "); Serial.print(conskw); Serial.print("  ");
    Serial.println("");
  }
}

void printDisplay() {
  if (millis() - millisNow >= 250) {
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Tensao: ");
    lcd.print(Vrms);
    lcd.print(" V");
    lcd.setCursor(0, 1);
    lcd.print("Corrente: ");
    lcd.print(IrmsTotal);
    lcd.print(" A");
    lcd.setCursor(0, 2);
    lcd.print("Potencia: ");
    lcd.print(IrmsTotal * Vrms);
    lcd.print(" W");
    lcd.setCursor(0, 3);
    lcd.print("Consumo: ");
    lcd.print(conskw);
    lcd.print(" kWh");
    millisNow = millis();
  }
}

void printCorrente(){
  if (millis() - millisNow >= 250) {
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Corrente F1:");
    lcd.print(Irms);
    lcd.print(" A");
    lcd.setCursor(0, 1);
    lcd.print("Corrente F2:");
    lcd.print(Irms1);
    lcd.print(" A");
    lcd.setCursor(0, 2);
    lcd.print("Potencia: ");
    lcd.print(IrmsTotal * Vrms);
    lcd.print(" W");
    lcd.setCursor(0, 3);
    lcd.print("Consumo: ");
    lcd.print(conskw);
    lcd.print(" kWh");
    millisNow = millis();
  }
}

void genData(void){
  // Recupera as informações (consumo, calibração, tensão, ciclo do registro - respecitivamente) da memória
  conskw = cmg.readFile("/consumo.txt").toFloat();
  calibration = cmg.readFile("/calibration.txt").toFloat();
  TENS = cmg.readFile("/voltage.txt").toFloat();
  previousTENS = cmg.readFile("/voltage.txt").toFloat();
  writeCicle = cmg.readFile("/writeCicle.txt").toFloat();
  D = cmg.readIntValue("/perido.txt");
  intervalo = cmg.readIntValue("/intervalo.txt");
}

void handleWebSocketMessage(void *arg, uint8_t *data, size_t len) {
  AwsFrameInfo *info = (AwsFrameInfo*)arg;
  if (info->final && info->index == 0 && info->len == len && info->opcode == WS_TEXT) {
    data[len] = 0;
    if (strcmp((char*)data, "toggle") == 0) {

    }
    if (strcmp((char*)data, "toggle1") == 0) {
      ESP.restart();
    }
    if (strcmp((char*)data, "toggle2") == 0) {
      cmg.saveSSID("");
      cmg.savePASS("");
      cmg.saveSSIDAP("");
      cmg.savePASSAP("");
      cmg.saveUserHTTP("");
      cmg.savePassHTTP("");
      cmg.saveOPmode(0);
      cmg.saveAPmode(0);
      ESP.restart();
    }
  }
}

void onEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type,
             void *arg, uint8_t *data, size_t len) {
  switch (type) {
    case WS_EVT_CONNECT:
      Serial.printf("WebSocket client #%u connected from %s\n", client->id(), client->remoteIP().toString().c_str());
      break;
    case WS_EVT_DISCONNECT:
      Serial.printf("WebSocket client #%u disconnected\n", client->id());
      break;
    case WS_EVT_DATA:
      handleWebSocketMessage(arg, data, len);
      break;
    case WS_EVT_PONG:
    case WS_EVT_ERROR:
      break;
  }
}

void initWebSocket(void) {
  ws.onEvent(onEvent);
  server.addHandler(&ws);
}

String processor(const String& var) {
  //Serial.println(var);
  if (var == "VOLTAGE") {
    return getVoltage();
  }
  else if (var == "CURRENT") {
    return getCurrent();
  }
  else if (var == "POWER") {
    return getPower();
  }
  else if (var == "CONS") {
    return getConsumption();
  }
  return String();
}

void notFound(AsyncWebServerRequest *request) {
  request->send(404, "text/plain", "Not found");
}

void configWebPages(void) {
  initWebSocket();

  /*-------------------------- PÁGINAS --------------------------*/

  server.on("/dataOut", HTTP_GET, [](AsyncWebServerRequest * request) {     // Página de exportação de dados
    request->send(SPIFFS, "/dataOut.html");
  });

  server.on("/instructions", HTTP_GET, [](AsyncWebServerRequest * request) { // Página de intruções de instalação
    request->send(SPIFFS, "/mainPage.html");
  });
  server.on("/a", HTTP_GET, [](AsyncWebServerRequest * request) { // Página de intruções de instalação
    request->send(SPIFFS, "/sensorR.html");
  });
  server.on("/v", HTTP_GET, [](AsyncWebServerRequest * request) { // Página de intruções de instalação
    request->send(SPIFFS, "/sensorR.html");
  });
  server.on("/p", HTTP_GET, [](AsyncWebServerRequest * request) { // Página de intruções de instalação
    request->send(SPIFFS, "/sensorR.html");
  });

  server.on("/", HTTP_GET, [](AsyncWebServerRequest * request) {            // Página inicial
    request->send(SPIFFS, "/mainPage.html");
  });

  server.on("/config", HTTP_GET, [](AsyncWebServerRequest * request) {      // Página de configurações do Wi-Fi convencional
    request->send(SPIFFS, "/configN.html");
  });

  server.on("/configAP", HTTP_GET, [](AsyncWebServerRequest * request) {    // Página de configurações do Wi-Fi AP (Access Point)
    request->send(SPIFFS, "/configAP.html");
  });

  server.on("/param", HTTP_GET, [](AsyncWebServerRequest * request) {       // Página de configurações de parâmetros
    request->send(SPIFFS, "/param.html");
  });

  server.on("/sensor", HTTP_GET, [](AsyncWebServerRequest * request) {      // Página de leitura dos sensores (todos)
    request->send(SPIFFS, "/sensorR.html");
  });

  server.on("/opMode", HTTP_GET, [](AsyncWebServerRequest * request) {      // Página de configuração do modo de operação
    request->send(SPIFFS, "/configOPmode.html");
  });

  server.on("/expSct", HTTP_GET, [](AsyncWebServerRequest * request) {       // Abre a página inicial e renicica o microcontrolador
    request->send(SPIFFS, "/expSct.html");
  });

  server.on("/sensor.jpg", HTTP_GET, [](AsyncWebServerRequest * request) {       // Abre a página inicial e renicica o microcontrolador
    request->send(SPIFFS, "/sensor.jpg");
  });

  server.on("/lei-de-faraday.png", HTTP_GET, [](AsyncWebServerRequest * request) {       // Abre a página inicial e renicica o microcontrolador
    request->send(SPIFFS, "/lei-de-faraday.png");
  });

  server.on("/Article-Clean.css", HTTP_GET, [](AsyncWebServerRequest * request) {       // Abre a página inicial e renicica o microcontrolador
    request->send(SPIFFS, "/Article-Clean.css");
  });
  
  server.on("/reset", HTTP_GET, [](AsyncWebServerRequest * request) {       // Abre a página inicial e renicica o microcontrolador
    request->send(SPIFFS, "/mainPage.html");
    restart = 1;
    restartClock = millis();
  });

  server.on("/resetOptions", HTTP_GET, [](AsyncWebServerRequest * request) {       // Abre a página inicial e renicica o microcontrolador
    request->send(SPIFFS, "/resetOptions.html");
  });

  server.on("/resetData", HTTP_GET, [](AsyncWebServerRequest * request) {       // Abre a página inicial e renicica o microcontrolador
    request->send(SPIFFS, "/mainPage.html");
    restart = 2;
  });
  server.on("/resetCon", HTTP_GET, [](AsyncWebServerRequest * request) {       // Abre a página inicial e renicica o microcontrolador
    request->send(SPIFFS, "/mainPage.html");
    restart = 3;
  });

  server.on("/m1", HTTP_GET, [](AsyncWebServerRequest * request) {       // Abre a página inicial e renicica o microcontrolador
    request->send(SPIFFS, "/configAnswerPage.html");
    cmg.saveSensorOP(1);
    restart = 1;
    restartClock = millis();
  });

  server.on("/m2", HTTP_GET, [](AsyncWebServerRequest * request) {       // Abre a página inicial e renicica o microcontrolador
    request->send(SPIFFS, "/configAnswerPage.html");
    cmg.saveSensorOP(2);
    restart = 1;
    restartClock = millis();
  });

  server.on("/m3", HTTP_GET, [](AsyncWebServerRequest * request) {       // Abre a página inicial e renicica o microcontrolador
    request->send(SPIFFS, "/configAnswerPage.html");
    cmg.saveSensorOP(3);
    restart = 1;
    restartClock = millis();
  });

  server.on("/m4", HTTP_GET, [](AsyncWebServerRequest * request) {       // Abre a página inicial e renicica o microcontrolador
    request->send(SPIFFS, "/configAnswerPage.html");
    cmg.saveSensorOP(4);
    restart = 1;
    restartClock = millis();
  });


  /*-------------------------- RECURSOS HTML --------------------------*/
  server.on("/bootstrap.min.css", HTTP_GET, [](AsyncWebServerRequest * request) {
    request->send(SPIFFS, "/bootstrap.min.css");
  });

  server.on("/cons.txt", HTTP_GET, [](AsyncWebServerRequest * request) {
    request->send(SPIFFS, "/cons.txt");
  });

  server.on("/consCSV.txt", HTTP_GET, [](AsyncWebServerRequest * request) {
    request->send(SPIFFS, "/consCSV.txt");
  });

  server.on("/bootstrap.min.js", HTTP_GET, [](AsyncWebServerRequest * request) {
    request->send(SPIFFS, "/bootstrap.min.js");
  });

  server.on("/Footer-Basic.css", HTTP_GET, [](AsyncWebServerRequest * request) {
    request->send(SPIFFS, "/Footer-Basic.css");
  });

  server.on("/Header-Blue.css", HTTP_GET, [](AsyncWebServerRequest * request) {
    request->send(SPIFFS, "/Header-Blue.css");
  });

  server.on("/Login-Form-Clean.css", HTTP_GET, [](AsyncWebServerRequest * request) {
    request->send(SPIFFS, "/Login-Form-Clean.css");
  });

  server.on("/styles.css", HTTP_GET, [](AsyncWebServerRequest * request) {
    request->send(SPIFFS, "/styles.css");
  });

  server.on("/styles.min.css", HTTP_GET, [](AsyncWebServerRequest * request) {
    request->send(SPIFFS, "/styles.min.css");
  });

  server.on("/img1.webp", HTTP_GET, [](AsyncWebServerRequest * request) {
    request->send(SPIFFS, "/img1.webp");
  });

  server.on("/img2.jpeg", HTTP_GET, [](AsyncWebServerRequest * request) {
    request->send(SPIFFS, "/img2.jpeg");
  });

  server.on("/highcharts.js", HTTP_GET, [](AsyncWebServerRequest * request) {
    request->send(SPIFFS, "/highcharts.js");
  });

  server.on("/css?family=Source+Sans+Pro:300,400,700", HTTP_GET, [](AsyncWebServerRequest * request) {
    request->send(SPIFFS, "/css.css");
  });

  server.on("/Features-Boxed.css", HTTP_GET, [](AsyncWebServerRequest * request) {
    request->send(SPIFFS, "/Features-Boxed.css");
  });

  server.on("/Highlight-Blue.css", HTTP_GET, [](AsyncWebServerRequest * request) {
    request->send(SPIFFS, "/Highlight-Blue.css");
  });

  server.on("/Highlight-Clean.css", HTTP_GET, [](AsyncWebServerRequest * request) {
    request->send(SPIFFS, "/Highlight-Clean.css");
  });

  server.on("/voltage", HTTP_GET, [](AsyncWebServerRequest * request) {
    request->send_P(200, "text/plain", getVoltage().c_str());
  });
  server.on("/current", HTTP_GET, [](AsyncWebServerRequest * request) {
    request->send_P(200, "text/plain", getCurrent().c_str());
  });
  server.on("/current1", HTTP_GET, [](AsyncWebServerRequest * request) {
    request->send_P(200, "text/plain", getCurrent1().c_str());
  });
  server.on("/power", HTTP_GET, [](AsyncWebServerRequest * request) {
    request->send_P(200, "text/plain", getPower().c_str());
  });
  server.on("/consumption", HTTP_GET, [](AsyncWebServerRequest * request) {
    request->send_P(200, "text/plain", getConsumption().c_str());
  });

  // Send a GET request to <ESP_IP>/get?input1=<inputMessage>

  server.on("/get", HTTP_GET, [] (AsyncWebServerRequest * request) {
    String inputMessage;
    String inputMessage1;
    String inputMessage2;
    String inputMessage3;
    String inputMessage4;
    String inputMessage5;
    String inputMessage6;
    String inputMessage7;

    String inputParam;
    String inputParam1;
    String inputParam2;
    String inputParam3;
    String inputParam4;
    String inputParam5;
    String inputParam6;
    String inputParam7;

    // GET input1 value on <ESP_IP>/get?input1=<inputMessage>
    if ((request->hasParam(PARAM_INPUT_1)) && (request->hasParam(PARAM_INPUT_2))) {
      inputMessage = request->getParam(PARAM_INPUT_1)->value();
      inputParam = PARAM_INPUT_1;
      acesso = inputMessage;

      inputMessage1 = request->getParam(PARAM_INPUT_2)->value();
      inputParam1 = PARAM_INPUT_2;
      senha = inputMessage1;
    }

    if ((request->hasParam(PARAM_INPUT_3)) && (request->hasParam(PARAM_INPUT_4))) {
      inputMessage2 = request->getParam(PARAM_INPUT_3)->value();
      inputParam2 = PARAM_INPUT_3;
      acessoAP = inputMessage2;

      inputMessage3 = request->getParam(PARAM_INPUT_4)->value();
      inputParam3 = PARAM_INPUT_3;
      senhaAP = inputMessage3;
    }

    if ((request->hasParam(PARAM_INPUT_5)) && (request->hasParam(PARAM_INPUT_6))) {
      inputMessage4 = request->getParam(PARAM_INPUT_5)->value();
      inputParam4 = PARAM_INPUT_5;
      userHTTP = inputMessage4;

      inputMessage5 = request->getParam(PARAM_INPUT_6)->value();
      inputParam5 = PARAM_INPUT_6;
      passHTTP = inputMessage5;
    }

    if ((request->hasParam(PARAM_INPUT_7))) {
      inputMessage6 = request->getParam(PARAM_INPUT_7)->value();
      inputParam6 = PARAM_INPUT_7;
      calibrationValue = inputMessage6;
    }

    if ((request->hasParam(PARAM_INPUT_8))) {
      inputMessage7 = request->getParam(PARAM_INPUT_8)->value();
      inputParam7 = PARAM_INPUT_8;
      Voltage = inputMessage7;
    }

    // ARMAZENA PROVISORIAMENTE OS DADOS OBTIDOS
    String qsid = inputMessage;
    String qpass = inputMessage1;

    String APsid = inputMessage2;
    String APpass = inputMessage3;

    String USER = inputMessage4;
    String PASSW = inputMessage5;

    String cValue = inputMessage6;

    String Cvoltage = inputMessage7;


    //GUARDA AS CREDENCIAIS PARA CLIENTE WIFI
    if (qsid.length() > 0 && qpass.length() > 0) {
      cmg.saveSSID(qsid.c_str());
      cmg.savePASS(qpass.c_str());
      cmg.saveOPmode(1);
      cmg.saveAPmode(0);
    }

    //GUARDA AS CREDENCIAIS PARA WIFI AP
    if (APsid.length() > 0 && APpass.length() > 0) {
      cmg.saveSSIDAP(APsid.c_str());
      cmg.savePASSAP(APpass.c_str());
      cmg.saveOPmode(0);
      cmg.saveAPmode(1);
    }

    //GUARDA AS CREDENCIAIS PARA LOGIN HTTP
    if (USER.length() > 0 && PASSW.length() > 0) {
      cmg.saveUserHTTP(USER.c_str());
      cmg.savePassHTTP(PASSW.c_str());
      request->send(SPIFFS, "/configAnswerPage.html");
    }

    if (cValue.length() > 0) {
      cmg.writeFile("/calibration.txt", cValue.c_str());
    }

    if (Cvoltage.length() > 0) {
      cmg.writeFile("/voltage.txt", Cvoltage.c_str());
    }

    else {
      inputMessage = "No message sent";
      inputParam = "none";
    }
    request->send(SPIFFS, "/configAnswerPage.html");
  });

  server.onNotFound(notFound);
}

void getWifi(void) {
  if (cmg.getOPmode() == 1) {
    lcd.clear();
    lcd.setCursor(4, 1);
    lcd.print("Conectando a");
    lcd.setCursor(2, 2);
    lcd.print(cmg.getSSID());

    Serial.println();
    Serial.println();
    Serial.print("Conectando a ");
    Serial.println(cmg.getSSID());
    WiFi.begin(cmg.getSSID(), cmg.getPASS());

    now = millis();

    while (WiFi.status() != WL_CONNECTED) {
      delay(500);
      Serial.print(".");
      if (millis() - now > WifiAttempt) {
        ESP.restart();
      }
    }

    Serial.println("");
    Serial.println("WiFi connected.");
    Serial.println("IP address: ");
    Serial.println(WiFi.localIP());
    lcd.clear();
    lcd.setCursor(5, 1);
    lcd.print("Conectado!");
    lcd.setCursor(2, 2);
    lcd.print("IP: ");
    lcd.print(WiFi.localIP());
  }

  else if (cmg.getOPmode() == 0) {
    if (cmg.getAPmode() == 0) {
      WiFi.softAP("ESP32", "");
    }
    else if (cmg.getAPmode() == 1) {
      WiFi.softAP(cmg.getSSIDAP(), cmg.getPASSAP());
    }
    IPAddress myIP = WiFi.softAPIP();
    Serial.print("AP IP address: ");
    Serial.println(myIP);
    Serial.println("Server started");
  }
  delay(2500);
}

void reconnect_wifi() {
  /* se já está conectado a rede WI-FI, nada é feito. 
  Caso contrário, são efetuadas tentativas de conexão */
  if (WiFi.status() == WL_CONNECTED)
    return;
         
  if (cmg.getOPmode() == 1) {
    WiFi.begin(cmg.getSSID(), cmg.getPASS());

    now = millis();

    while (WiFi.status() != WL_CONNECTED) {
      if (millis() - now > WifiAttempt) {
        ESP.restart();
      }
    }
  }
  else if (cmg.getOPmode() == 0) {
    if (cmg.getAPmode() == 0) {
      WiFi.softAP("ESP32", "");
    }
    else if (cmg.getAPmode() == 1) {
      WiFi.softAP(cmg.getSSIDAP(), cmg.getPASSAP());
    }
    IPAddress myIP = WiFi.softAPIP();
  }
}

void showIP() {
  if(opMode == 1){
    lcd.clear();
    lcd.setCursor(4, 1);
    lcd.print("IP do ESP32:");
    lcd.setCursor(4, 2);
    lcd.print(WiFi.localIP());
  }
  if(opMode == 0){
    lcd.clear();
    lcd.setCursor(4, 1);
    lcd.print("IP do ESP32:");
    lcd.setCursor(4, 2);
    lcd.print(WiFi.softAPIP());
  }
}

void parameters(){
  lcd.clear();
  lcd.setCursor(0,1);
  lcd.print("Fator de calibracao:");
  lcd.setCursor(8,2);
  lcd.print(calibration);
}

void getOPmode(){
  opMode = cmg.getOPmode();
  apMode = cmg.getAPmode();
  sensorOP = cmg.getSensorOP();  
}

void nextPage(){
  if(millis() - pageDelay >= 250){      // Debounce - Espera 3/4 de segundo para que o botão possa ser pressionado novamente (evita erros na passagem de páginas)
    if(digitalRead(modeSelect) == 1){   // Detecta o botão que comanda a passagem de páginas
      page++;
      pageDelay = millis();
    }
  }
  if(page == 0){                        // Página 0 - Leituras dos sensores
    printDisplay();
  }else if(page == 1){
    printCorrente();
  }else if(page == 2){                  // Página 1 - Mostra o IP do microcontrolador
    showIP();
  }else if (page == 3){
    printLocalTime();
  }else if (page == 4){
    parameters();
  }
  else{
    page = 0;                           // Retorna a página 0
  }
}

void printLocalTime(){
  struct tm timeinfo;
  if(!getLocalTime(&timeinfo)){
    Serial.println("Failed to obtain time");
    page++;
    return;
  }
  /*Serial.println(&timeinfo, "%A, %B %d %Y %H:%M:%S");*/
  lcd.clear();
  lcd.setCursor(5,1);
  lcd.print(&timeinfo, "%d/%m/%Y"); // dia, mês e ano
  lcd.setCursor(6,2);
  lcd.print(&timeinfo, "%H:%M:%S"); // Hora, minuto e segundo
}

String timeStamp(){
  struct tm timeinfo;
  if(!getLocalTime(&timeinfo)){
    Serial.println("Failed to obtain time");
    return("");
  }
  char timeStringBuff[50];
  strftime(timeStringBuff, sizeof(timeStringBuff), "%d/%m/%Y %H:%M:%S", &timeinfo);
  Serial.println(timeStringBuff);
  return(String(timeStringBuff));
}

void relatorio(){
  //int NT = ((60000/intervalo)*1440)*D; // NT = Número total de registros; D = dias de registro
  int NT = 1000;
  intervalo = 30000;

  if(millis() - storeCons >= intervalo){
    cmg.writeFile("/consumo.txt", String(conskw).c_str());
    if(writeCicle <= NT){
      //Relatório simples
      String relatorio = String(timeStamp());
      relatorio += ",";
      relatorio += "Tensao: ";
      relatorio += String(Vrms);
      relatorio += "V, ";
      relatorio += "Corrente (S1): ";
      relatorio += String(Irms);
      relatorio += "A, ";
      relatorio += "Corrente (S2): ";
      relatorio += String(Irms1);
      relatorio += "A, ";
      relatorio += "Potencia: ";
      relatorio += String(Prms);
      relatorio += "W, ";
      relatorio += "Consumo (acumulado): ";
      relatorio += String(conskw);
      relatorio += "kWh";

      //Relatório CSV
      if(writeCicle == 0){
        relatorioCSV = "data/hora,tensao(V),correnteF1(A),correnteF2(A),potencia(W),consumo(kWh)\n";
        relatorioCSV += String(timeStamp());
      }else{
        relatorioCSV = String(timeStamp());
      }
      relatorioCSV += ",";
      relatorioCSV += String(Vrms);
      relatorioCSV += ",";
      relatorioCSV += String(Irms);
      relatorioCSV += ",";
      relatorioCSV += String(Irms1);
      relatorioCSV += ",";
      relatorioCSV += String(Prms);
      relatorioCSV += ",";
      relatorioCSV += String(conskw);

      cmg.appendFile("/cons.txt", String(relatorio).c_str());
      cmg.appendFile("/consCSV.txt", String(relatorioCSV).c_str());
      cmg.writeFile("/writeCicle.txt", String(writeCicle).c_str());
      writeCicle++;
    }
    storeCons = millis();
  }
}
