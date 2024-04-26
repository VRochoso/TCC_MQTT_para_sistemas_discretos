#include <WiFi.h>
#include "esp_wpa2.h"
#include <PubSubClient.h>

/** DEFINITION OF WIFI CONECT
 *  TIPO  | WiFi
 *  LOCAL | LOCAL
 *  UFU1  | UFU-INSTITUCIONAL
 *  UFU2  | eduroam
*/
//#define LOCAL
#define UFU1
//#define UFU2

/* DEFINITION OF WIFI ACCESS POINT VARIABLES */
#if defined(LOCAL)
  #define SSID "rocha"
  #define EAP_PASSWORD "12345678"
#elif defined(UFU1)
  #define SSID "UFU-Institucional"
  #define EAP_ANONYMOUS_IDENTITY "anonymous@ufu.br"  // anonymous@example.com, or you can use also nickname@example.com
  #define EAP_IDENTITY "rochoso@ufu.br"         // nickname@example.com, at some organizations should work nickname only without realm, but it is not recommended
  #define EAP_PASSWORD "Animes94"
#elif defined(UFU2)
  #define SSID "eduroam"
  #define EAP_ANONYMOUS_IDENTITY "anonymous@ufu.br"  // anonymous@example.com, or you can use also nickname@example.com
  #define EAP_IDENTITY "rochoso@ufu.br"         // nickname@example.com, at some organizations should work nickname only without realm, but it is not recommended
  #define EAP_PASSWORD "Animes94"
#endif

/* DECLARATION OF MQTT SERVER VARIABLES */
const char* mqttserver = "10.14.48.104";
const uint16_t mqttport = 1883;
const char* mqttUser = "rocha";
const char* mqttpsswrd = "rocha";

/* DECLARATION OF TOPICS VARIABLES */
const char* topsenIN = "/sensor/peça";

/* DECLARATION OF A CLASS VARIABLE */
WiFiClient wifiClient;
PubSubClient MQTT(wifiClient);
TaskHandle_t Task1;

/* DEFINITION OF GPIO PINES VARIABLES */
#define sP 33
#define sM 25
#define sG 12
#define sMET 26
#define sCONT 27

/* DEFINITION OF A CONSTANT VARIABLES */
#define MSG_BUFFER_SIZE (24)
#define WAIT_CAP 1

/* DECLARATION OF GLOBAL VARIABLES */
bool clesP = 0;
bool clesM = 0;
bool clesG = 0;
bool clesMET = 0;
bool clesCONT = 0;
bool flag = false;
unsigned long timercap = 0;
char msg[MSG_BUFFER_SIZE];

/* DECLARATION OF FUNCTIONS WITH DEAFULT INPUT VALUES */
void conectarWiFi(int timeout = 30);
void IP_print(IPAddress localip = WiFi.localIP());

/* SETUP FUNCTION FOR SETUP THE PROGRAM INITIALIZATION */
void setup() {
  Serial.begin(115200);
  conectarWiFi();

  // Setup of Server and Subscribe
  MQTT.setServer(mqttserver, mqttport);
  MQTT.setCallback(callback);

  pinMode(sM, INPUT_PULLUP);
  pinMode(sP, INPUT_PULLUP);
  pinMode(sG, INPUT_PULLUP);
  pinMode(sMET, INPUT_PULLUP);
  pinMode(sCONT, INPUT_PULLUP);

  // Setup a Task using the free core of ESP32
  int CORE_ID = int(!xPortGetCoreID());
  xTaskCreatePinnedToCore(
    Task1code, // Task function
    "Task1",   // Name of the Task to atach
    100000,     // Size to alocated to Task (Word or Byte)
    NULL,      // Task argument to be passed (void*)
    0,         // Priority of the Task [0 a 25]
    &Task1,    // Task ID
    CORE_ID);  // Task core ID (0 ou 1)
  delay(500);
}

/* TASK FUNCTION USING FREE CORE OF ESP32 */
void Task1code(void* pvParameters) {
  Serial.print("Conexões MQTT rodando no nucleo ");
  Serial.println(xPortGetCoreID());

  for (;;) {
    if (msg != "") {
      if (!MQTT.connected()) conectarMQTT();

      MQTT.loop();
    }
  }
}

/* LOOP FUNCTION TO RUNNING PROGRAM */
void loop() {
  delay(10);

  if (!clesM) clesM = digitalRead(sM);
  if (!clesP) clesP = digitalRead(sP);
  if (!clesG) clesG = digitalRead(sG);
  if (!clesMET && !flag) clesMET = digitalRead(sMET);
  if (!clesCONT && !flag) clesCONT = digitalRead(sCONT);

  if (!flag) {
    timercap = millis();
    if (clesCONT) flag = true;
  } else if (int(millis() - timercap) >= WAIT_CAP*1000) flag = false;

  //printf("sensor pequeno: %d\tsensor medio: %d\tsensor grande: %d\tsensor capacitivo: %d\tsensor metalico: %d\n", clesP, clesM, clesG, clesCONT, clesMET);

  if(clesP && clesCONT){
    printf("sensor pequeno: %d\tsensor medio: %d\tsensor grande: %d\tsensor capacitivo: %d\tsensor metalico: %d\n", clesP, clesM, clesG, clesCONT, clesMET);
    if (!clesM && !clesG && !clesMET) snprintf (msg, MSG_BUFFER_SIZE, "Peça Pequena");
    if (clesM && !clesG && !clesMET) snprintf (msg, MSG_BUFFER_SIZE, "Peça Média");
    if (clesM && clesG && !clesMET) snprintf (msg, MSG_BUFFER_SIZE, "Peça Grande");
    if (!clesM && !clesG && clesMET) snprintf (msg, MSG_BUFFER_SIZE, "Peça Pequena Metallica");
    if (clesM && !clesG && clesMET) snprintf (msg, MSG_BUFFER_SIZE, "Peça Média Metallica");
    if (clesM && clesG && clesMET) snprintf (msg, MSG_BUFFER_SIZE, "Peça Grande Metallica");

    printf("%s\n", msg);

    MQTT.publish(topsenIN, msg, 2);
    clesP = 0;
    clesM = 0;
    clesG = 0;
    clesMET = 0;
    clesCONT = 0;
  }
}

/* CALLBACK FUNCTION FOR SUBSCRIBE TOPICS */
void callback(char* topic, byte* payload, unsigned int length) {
  String texto = "";
  for (int i = 0; i < length; i++) texto += (char)payload[i];

  if (String(msg) != texto && String(msg) != "") MQTT.publish(topsenIN, msg, 2);
  else if (String(msg) != "") snprintf (msg, MSG_BUFFER_SIZE, "");
}

/* WIFI FUNCTION FOR CONECT TO SSID DEFINED */
void conectarWiFi(int timeout) {
  unsigned long inicio = millis();

  Serial.print(F("Conectando ao WiFi "));
  Serial.print(SSID);
  Serial.print(F(", aguarde..."));
  #ifdef EAP_ANONYMOUS_IDENTITY
    WiFi.disconnect(true);
    WiFi.mode(WIFI_STA);
    WiFi.begin(SSID, WPA2_AUTH_PEAP, EAP_ANONYMOUS_IDENTITY, EAP_IDENTITY, EAP_PASSWORD);
  #else
    WiFi.mode(WIFI_STA);
    WiFi.begin(SSID, EAP_PASSWORD);
  #endif

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(F("."));
    int timer = (millis() - inicio)/1000;
    if ((WiFi.status() == WL_CONNECT_FAILED || WiFi.status() == WL_CONNECTION_LOST) && timer <= timeout) {
      #ifdef EAP_ANONYMOUS_IDENTITY
        WiFi.disconnect(true);
        WiFi.mode(WIFI_STA);
        WiFi.begin(SSID, WPA2_AUTH_PEAP, EAP_ANONYMOUS_IDENTITY, EAP_IDENTITY, EAP_PASSWORD);
      #else
        WiFi.mode(WIFI_STA);
        WiFi.begin(SSID, EAP_PASSWORD);
      #endif
    } else if (timer > timeout) ESP.restart();
  }

  IP_print();
}

/* IP PRINT FUNCTION */
void IP_print(IPAddress localip) {
  byte mac[6];
  String IP = "";
  for (int i=0; i<4; i++)
    IP += i  ? "." + String(localip[i]) : String(localip[i]);
  int IP_len = IP.length();

  Serial.println(F("\n\n|************************************************|"));
  Serial.println(F("|\t\tWiFi is connected!\t\t |"));

  Serial.print(F("|"));
  for (int i = 0; i < int((37 - IP_len)/2); i++) Serial.print(F(" "));
  Serial.print(F("IP Address: "));
  Serial.print(WiFi.localIP()); //print LAN IP
  for (int i = 0; i < int((36 - IP_len)/2); i++) Serial.print(F(" "));
  Serial.print(F("|\n|\t MAC Address - "));

  WiFi.macAddress(mac);
  String texto =  String(mac[0], HEX) + ":" + String(mac[1], HEX) + ":" + String(mac[2], HEX) + ":" + String(mac[3], HEX) + ":" + String(mac[4], HEX) + ":" + String(mac[5], HEX);
  texto.toUpperCase();

  Serial.print(texto);
  Serial.println(F("\t |\n|************************************************|\n"));
}

/* MQTT FUNCTION FOR CONECT TO SERVER */
void conectarMQTT() {
  while (!MQTT.connected()) {
    if (MQTT.connect("ESP32-PEÇA")){
      MQTT.subscribe(topsenIN);
    }
  }
}