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
const char* topsenOUT = "/valv/recuo";

/* DECLARATION OF A CLASS VARIABLE */
WiFiClient wifiClient;
PubSubClient MQTT(wifiClient);
TaskHandle_t Task1;

/* DEFINITION OF GPIO PINES VARIABLES */
#define s1 32
#define s2 33
#define s3 25
#define s4 26

/* DEFINITION OF A CONSTANT VARIABLES */
#define MSG_BUFFER_SIZE (17)

/* DECLARATION OF GLOBAL VARIABLES */
bool cles1 = 0;
bool cles2 = 0;
bool cles3 = 0;
bool cles4 = 0;
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

  pinMode(s2, INPUT_PULLUP);
  pinMode(s1, INPUT_PULLUP);
  pinMode(s3, INPUT_PULLUP);
  pinMode(s4, INPUT_PULLUP);

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

  if (!cles1) cles1 = digitalRead(s1);
  if (!cles2) cles2 = digitalRead(s2);
  if (!cles3) cles3 = digitalRead(s3);
  if (!cles4) cles4 = digitalRead(s4);

  //printf("Rampa 1: %d\tRampa 2: %d\tRampa 3: %d\tRampa 4: %d\n", cles1, cles2, cles3, cles4);

  if(cles1 || cles2 || cles3 || cles4) {
    printf("Rampa 1: %d\tRampa 2: %d\tRampa 3: %d\tRampa 4: %d\n", cles1, cles2, cles3, cles4);
    if (cles1) snprintf (msg, MSG_BUFFER_SIZE, "Peça na saida 1");
    else if (cles2) snprintf (msg, MSG_BUFFER_SIZE, "Peça na saida 2");
    else if (cles3) snprintf (msg, MSG_BUFFER_SIZE, "Peça na saida 3");
    else if (cles4) snprintf (msg, MSG_BUFFER_SIZE, "Peça na saida 4");
    
    printf("%s\n", msg);

    MQTT.publish(topsenOUT, msg, 2);
    cles1 = 0;
    cles2 = 0;
    cles3 = 0;
    cles4 = 0;
  }
}

/* CALLBACK FUNCTION FOR SUBSCRIBE TOPICS */
void callback(char* topic, byte* payload, unsigned int length) {
  String texto = "";
  for (int i = 0; i < length; i++) texto += (char)payload[i];

  if (String(msg) != texto && String(msg) != "") MQTT.publish(topsenOUT, msg, 2);
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
    if (MQTT.connect("ESP32-OUT")){
      MQTT.subscribe(topsenOUT);
    }
  }
}