#include "EmonLib.h"
#include <driver/adc.h>
#include <CredentialsManager.h>
#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include <SD.h>
#include <SD.h>
#include <Wire.h>
#include <LiquidCrystal.h>
#include <AsyncTCP.h>
#include <AsyncElegantOTA.h>
#include <WiFiAP.h>
#include "time.h"

TaskHandle_t Task1; //Cria uma tarefa
TaskHandle_t Task2; //Cria uma tarefa

const char* ntpServer = "pool.ntp.org";
const long  gmtOffset_sec = -3 * 3600;
const int   daylightOffset_sec = 0;

CredentialsManager cmg;                // Cria uma instância para gerenciar memoria SD
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
int releaseB = 0;
int buttonState;
int sd;

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
unsigned long count;
unsigned long pressTime;

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
#define ADC_BITS    12                 // Define a resolucao em bits
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

/*-------------------------------- ENVIA OS DADOS PARA O WEBSERVER ----------------------------------*/

String getVoltage() {
  float V1 = Vrms;
  return String(V1);
}

String getCurrent() {
  float current = Irms;
  if (current <= 0.26) {
    current = 0;
  }
  return String(current);
}

String getCurrent1() {
  float current1 = Irms1;
  if (current1 <= 0.26) {
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
  cmg.begin();                                                  // Inicia a biblioteca que salva as informações na memória flash
  lcd.begin(20, 4);                                             // Inicia o display LCD 20x4
  
  adc1_config_channel_atten(ADC1_CHANNEL_7, ADC_ATTEN_DB_11);   // Cofigura a atenuação de ruido do canal 6 e 5 em 11dB (canal 6 corresponde ao canal do terminal 35, 34 e 33)
  adc1_config_channel_atten(ADC1_CHANNEL_6, ADC_ATTEN_DB_11);
  adc1_config_channel_atten(ADC1_CHANNEL_5, ADC_ATTEN_DB_11);
  analogReadResolution(ADC_BITS);
  //analogReadResolution(10);                                   // Configura a resolução analógica para 10 bits (Necessário para a bibloteca EmonLib)
  
  pinMode(modeSelect, INPUT);                                   // Define o terminal do botão como entrada
  
  getOPmode();                                                  // Define o modo de operação
  configWebPages();                                             // Configura todas as páginas a serem exibidas no navegador
  getWifi();                                                    // Inicia o Wi-Fi
  
  genData();                                                    // Informações gerais armazenadas na memória flash
  if(sensorOP == 1){
    CT013.current(34, calibration);                             // Corrente: Terminal de entrada, calibração
    CT013_2.current(33, calibration);                           // Corrente: Terminal de entrada, calibração
    ZMPT.voltage(35, 350, 1.1);                                 // Tensão: Termnial de entrada, fator de calibração, phase shift (Defasagem em relação a medição e o observado pelo microcontrolador)
  }
  else if(sensorOP == 2){
    CT013.current(34, calibration);                             // Corrente: Terminal de entrada, calibração
    ZMPT.voltage(35, 350, 1.1);                                 // Tensão: Termnial de entrada, fator de calibração, phase shift (Defasagem em relação a medição e o observado pelo microcontrolador)
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
  
  server.begin();                                             // Inicia a comunicação de servidor via Wifi
  AsyncElegantOTA.begin(&server);                             // Inicia o WebServer assíncrono
  showIP();                                                   // Mostra o IP do microcontrolador no display LCD
  coreDefinition();
  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);   // Configura a hora e data
}

void loop() {
  // put your main code here, to run repeatedly:

}

void Task1code( void * pvParameters ){                  // TAREFA DO NÚCLEO 1
  Serial.print("Task1 running on core ");
  Serial.println(xPortGetCoreID());
  
  for(;;){
    if(sensorOP == 1){
    ZMPT.calcVI(12, 500);
    Irms = CT013.calcIrms(1200);
    Irms1 = CT013_2.calcIrms(1200);  // Calcula a corrente RMS (prv 1200)
    Vrms = ZMPT.Vrms;
    }
    else if(sensorOP == 2){
      ZMPT.calcVI(12, 1000);
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
  
    if (Irms <= 0.26) {
      Irms = 0;
    }
    if (Irms1 <= 0.26 && (sensorOP == 1 || sensorOP == 3)) {
      Irms1 = 0;
    }
    IrmsTotal = Irms + Irms1;
    Prms = (IrmsTotal) * (Vrms);
    String t = String(millis() - previousTime);               // Calcula o tempo que se passou entre o último cálculo e o atual
    previousTime = millis();                                  // Variável que armazena o momento do último cálculo
    lastConskw = conskw;
    conskw = lastConskw + ((Prms) / 1000) * (t.toFloat() / 1000 / 3600); // Soma o valor do último cálculo de consumo e o consumo atual em kWh (tempo gasto desde o último cálculo X potência
    
    relatorio(); // Gera o relatório
    //serialOut(); // Envia as informações para a porta serial
    delay(20);
  }
}

void Task2code( void * pvParameters ){                // TAREFA DO NÚCLEO 2
  Serial.print("Task2 running on core ");
  Serial.println(xPortGetCoreID());
  
  for(;;){
    if(opMode == 1){
      buttonState = digitalRead(modeSelect);
      if(buttonState == 1 && releaseB == 0){
        count = millis();
        releaseB = 1;
      }
      else if(buttonState == 0 && releaseB == 1){
        pressTime = millis() - count;
        releaseB = 0;
        sd = 0;
      }
      if(releaseB == 1){
        pressTime = millis() - count;
      }
      if(sd == 0 || pressTime >= 20000){
        if(pressTime <= 9000){
          sd = 1;
          millisNow = 0;
          page++;
        }
        else if(pressTime >= 20000){
          lcd.clear();
          lcd.setCursor(5,1);
          lcd.print("REINICIANDO");
          lcd.setCursor(7,2);
          lcd.print("SISTEMA");
          delay(1000);
          sd = 1;
          restart = 1;
          cmg.saveAPmode(0);
          cmg.saveOPmode(0);
        }
      }
      if(restart == 1){                       // Reinicia o microcontrolador
        if(millis() - restartClock >= 4000){
          ESP.restart();
        }
      }
      if(restart == 2){                         // Limpa o relatório simples
        cmg.writeFileSD("/cons.txt", "");
        cmg.writeFileSD("/consCSV.csv", "");
        cmg.writeFile("/writeCicle.txt", "0");
        restart = 0;
      }
      if(restart == 3){                         // Limpa o relatório CSV
        cmg.writeFileSD("/cons.txt", "");
        cmg.writeFileSD("/consCSV.csv", "");
        cmg.writeFile("/writeCicle.txt", "0");
        conskw = 0;
        restart = 0;
      }
      nextPage();  // Controla as "páginas" do display
    }else{delay(20);}
    //AsyncElegantOTA.loop();
    ws.cleanupClients();
    reconnect_wifi();
    delay(20);
  }
}


/*----------------------------------------------------------------------------------------------------------------------------------------------*/

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

void printCorrente(){
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
    request->send(SD, "/dataOut.html");
  });

  server.on("/instructions", HTTP_GET, [](AsyncWebServerRequest * request) { // Página de intruções de instalação
    request->send(SD, "/mainPage.html");
  });
  server.on("/a", HTTP_GET, [](AsyncWebServerRequest * request) { // Página de intruções de instalação
    request->send(SD, "/sensorR.html");
  });
  server.on("/v", HTTP_GET, [](AsyncWebServerRequest * request) { // Página de intruções de instalação
    request->send(SD, "/sensorR.html");
  });
  server.on("/p", HTTP_GET, [](AsyncWebServerRequest * request) { // Página de intruções de instalação
    request->send(SD, "/sensorR.html");
  });

  server.on("/", HTTP_GET, [](AsyncWebServerRequest * request) {            // Página inicial
    request->send(SD, "/mainPage.html");
  });

  server.on("/config", HTTP_GET, [](AsyncWebServerRequest * request) {      // Página de configurações do Wi-Fi convencional
    request->send(SD, "/configN.html");
  });

  server.on("/configAP", HTTP_GET, [](AsyncWebServerRequest * request) {    // Página de configurações do Wi-Fi AP (Access Point)
    request->send(SD, "/configAP.html");
  });

  server.on("/param", HTTP_GET, [](AsyncWebServerRequest * request) {       // Página de configurações de parâmetros
    request->send(SD, "/param.html");
  });

  server.on("/sensor", HTTP_GET, [](AsyncWebServerRequest * request) {      // Página de leitura dos sensores (todos)
    request->send(SD, "/sensorR.html");
  });

  server.on("/opMode", HTTP_GET, [](AsyncWebServerRequest * request) {      // Página de configuração do modo de operação
    request->send(SD, "/configOPmode.html");
  });

  server.on("/expSct", HTTP_GET, [](AsyncWebServerRequest * request) {       // Abre a página inicial e renicica o microcontrolador
    request->send(SD, "/expSct.html");
  });

  server.on("/expZmp", HTTP_GET, [](AsyncWebServerRequest * request) {       // Abre a página inicial e renicica o microcontrolador
    request->send(SD, "/expZmp.html");
  });

  server.on("/sensor.jpg", HTTP_GET, [](AsyncWebServerRequest * request) {       // Abre a página inicial e renicica o microcontrolador
    request->send(SD, "/sensor.jpg");
  });

  server.on("/lei-de-faraday.png", HTTP_GET, [](AsyncWebServerRequest * request) {       // Abre a página inicial e renicica o microcontrolador
    request->send(SD, "/lei-de-faraday.png");
  });

  server.on("/sensorTS.jpg", HTTP_GET, [](AsyncWebServerRequest * request) {       // Abre a página inicial e renicica o microcontrolador
    request->send(SD, "/sensorTS.jpg");
  });

  server.on("/disjuntor.jpg", HTTP_GET, [](AsyncWebServerRequest * request) {       // Abre a página inicial e renicica o microcontrolador
    request->send(SD, "/disjuntor.jpg");
  });

  server.on("/Article-Clean.css", HTTP_GET, [](AsyncWebServerRequest * request) {       // Abre a página inicial e renicica o microcontrolador
    request->send(SD, "/Article-Clean.css");
  });
  
  server.on("/reset", HTTP_GET, [](AsyncWebServerRequest * request) {       // Abre a página inicial e renicica o microcontrolador
    request->send(SD, "/mainPage.html");
    restart = 1;
    restartClock = millis();
  });

  server.on("/resetOptions", HTTP_GET, [](AsyncWebServerRequest * request) {       // Abre a página inicial e renicica o microcontrolador
    request->send(SD, "/resetOptions.html");
  });

  server.on("/resetData", HTTP_GET, [](AsyncWebServerRequest * request) {       // Abre a página inicial e renicica o microcontrolador
    request->send(SD, "/mainPage.html");
    restart = 2;
  });
  server.on("/resetCon", HTTP_GET, [](AsyncWebServerRequest * request) {       // Abre a página inicial e renicica o microcontrolador
    request->send(SD, "/mainPage.html");
    restart = 3;
  });

  server.on("/m1", HTTP_GET, [](AsyncWebServerRequest * request) {       // Abre a página inicial e renicica o microcontrolador
    request->send(SD, "/configAnswerPage.html");
    cmg.saveSensorOP(1);
    restart = 1;
    restartClock = millis();
  });

  server.on("/m2", HTTP_GET, [](AsyncWebServerRequest * request) {       // Abre a página inicial e renicica o microcontrolador
    request->send(SD, "/configAnswerPage.html");
    cmg.saveSensorOP(2);
    restart = 1;
    restartClock = millis();
  });

  server.on("/m3", HTTP_GET, [](AsyncWebServerRequest * request) {       // Abre a página inicial e renicica o microcontrolador
    request->send(SD, "/configAnswerPage.html");
    cmg.saveSensorOP(3);
    restart = 1;
    restartClock = millis();
  });

  server.on("/m4", HTTP_GET, [](AsyncWebServerRequest * request) {       // Abre a página inicial e renicica o microcontrolador
    request->send(SD, "/configAnswerPage.html");
    cmg.saveSensorOP(4);
    restart = 1;
    restartClock = millis();
  });


  /*-------------------------- RECURSOS HTML --------------------------*/
  server.on("/bootstrap.min.css", HTTP_GET, [](AsyncWebServerRequest * request) {
    request->send(SD, "/bootstrap.min.css");
  });

  server.on("/cons.txt", HTTP_GET, [](AsyncWebServerRequest * request) {
    request->send(SD, "/cons.txt");
  });

  server.on("/consCSV.csv", HTTP_GET, [](AsyncWebServerRequest * request) {
    request->send(SD, "/consCSV.csv");
  });

  server.on("/bootstrap.min.js", HTTP_GET, [](AsyncWebServerRequest * request) {
    request->send(SD, "/bootstrap.min.js");
  });

  server.on("/Footer-Basic.css", HTTP_GET, [](AsyncWebServerRequest * request) {
    request->send(SD, "/Footer-Basic.css");
  });

  server.on("/Header-Blue.css", HTTP_GET, [](AsyncWebServerRequest * request) {
    request->send(SD, "/Header-Blue.css");
  });

  server.on("/Login-Form-Clean.css", HTTP_GET, [](AsyncWebServerRequest * request) {
    request->send(SD, "/Login-Form-Clean.css");
  });

  server.on("/styles.css", HTTP_GET, [](AsyncWebServerRequest * request) {
    request->send(SD, "/styles.css");
  });

  server.on("/styles.min.css", HTTP_GET, [](AsyncWebServerRequest * request) {
    request->send(SD, "/styles.min.css");
  });

  server.on("/img1.webp", HTTP_GET, [](AsyncWebServerRequest * request) {
    request->send(SD, "/img1.webp");
  });

  server.on("/img2.jpeg", HTTP_GET, [](AsyncWebServerRequest * request) {
    request->send(SD, "/img2.jpeg");
  });

  server.on("/highcharts.js", HTTP_GET, [](AsyncWebServerRequest * request) {
    request->send(SD, "/highcharts.js");
  });

  server.on("/css?family=Source+Sans+Pro:300,400,700", HTTP_GET, [](AsyncWebServerRequest * request) {
    request->send(SD, "/css.css");
  });

  server.on("/Features-Boxed.css", HTTP_GET, [](AsyncWebServerRequest * request) {
    request->send(SD, "/Features-Boxed.css");
  });

  server.on("/Highlight-Blue.css", HTTP_GET, [](AsyncWebServerRequest * request) {
    request->send(SD, "/Highlight-Blue.css");
  });

  server.on("/Highlight-Clean.css", HTTP_GET, [](AsyncWebServerRequest * request) {
    request->send(SD, "/Highlight-Clean.css");
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
      request->send(SD, "/configAnswerPage.html");
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
    request->send(SD, "/configAnswerPage.html");
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
        cmg.saveOPmode(0);
        cmg.saveAPmode(0);
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
    lcd.print("IP do M.G.E:");
    lcd.setCursor(4, 2);
    lcd.print(WiFi.localIP());
  }
  if(opMode == 0){
    lcd.clear();
    lcd.setCursor(4, 1);
    lcd.print("IP do M.G.E:");
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
  if (millis() - millisNow >= 500) {
    if(page == 0){                        // Página 0 - Leituras dos sensores
      printDisplay();
      millisNow = millis();
    }else if(page == 1){
      printCorrente();
      millisNow = millis();
    }else if(page == 2){                  // Página 1 - Mostra o IP do microcontrolador
      showIP();
      millisNow = millis();
    }else if (page == 3){
      printLocalTime();
      millisNow = millis();
    }else if (page == 4){
      //parameters();
      millisNow = 0;
      page = 0;
    }
    else{
      millisNow = 0;
      page = 0;                           // Retorna a página 0
    }
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
  if(opMode == 1){
    struct tm timeinfo;
    if(!getLocalTime(&timeinfo)){
      Serial.println("Failed to obtain time");
      return("");
    }
    char timeStringBuff[50];
    strftime(timeStringBuff, sizeof(timeStringBuff), "%d/%m/%Y %H:%M:%S", &timeinfo);
    return(String(timeStringBuff));
  }
  else if(opMode == 0){
    return(String(millis()));
  }
}

void relatorio(){
  //int NT = ((60000/intervalo)*1440)*D; // NT = Número total de registros; D = dias de registro
  int NT = 10000;
  intervalo = 5000;

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
        relatorioCSV = "\ndata/hora,tensao(V),correnteF1(A),correnteF2(A),potencia(W),consumo(kWh)\n";
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

      cmg.appendFileSD("/cons.txt", String(relatorio).c_str());
      cmg.appendFileSD("/consCSV.csv", String(relatorioCSV).c_str());
      cmg.writeFile("/writeCicle.txt", String(writeCicle).c_str());
      writeCicle++;
    }
    storeCons = millis();
  }
}



/*-------------------------------- DEFINIÇÃO DOS NÚCLEOS --------------------------------*/
void coreDefinition(void){
   //Cria uma tarefa que vai ser executada na função Task1code(), com prioridade 1 e executada no núcleo 0
  xTaskCreatePinnedToCore(
                    Task1code,   /* Nome da função da tarefa. */
                    "Task1",     /* Nome da tarefa. */
                    10000,       /* tamanho do pacote da tarefa */
                    NULL,        /* Parâmetro da tarefa */
                    10,           /* Prioridade da tarefa */
                    &Task1,      /* Identificação para acompanhar a progresso da tarefa */
                    1);          /* Configura a tarefa para o núcleo 0 */                  
  delay(500); 

  //Cria uma tarefa que vai ser executada na função Task1code(), com prioridade 1 e executada no núcleo 1
  xTaskCreatePinnedToCore(
                    Task2code,   /* Nome da função da tarefa. */
                    "Task2",     /* Nome da tarefa. */
                    10000,       /* Tamanho do pacote da tarefa */
                    NULL,        /* Parâmetro da tarefa */
                    10,           /* Prioridade da tarefa */
                    &Task2,      /* Identificação para acompanhar a progresso da tarefa */
                    0);          /* Configura a tarefa para o núcleo 1 */
  delay(500); 
}
