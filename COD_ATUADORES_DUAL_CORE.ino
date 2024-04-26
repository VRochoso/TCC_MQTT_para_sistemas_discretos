#include <WiFi.h>
#include "esp_wpa2.h"
#include <PubSubClient.h>

/* DEFINITION OF ACTION VARIABLES */
#define avanco 0
#define recuo 1

/** DEFINITION OF WIFI CONECT
 *  TIPO  | WiFi
 *  LOCAL | LOCAL
 *  UFU1  | UFU-INSTITUCIONAL
 *  UFU2  | eduroam
*/
// #define LOCAL
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
const char* topvalv = "/valv/cilindroacao";


/* DECLARATION OF A CLASS VARIABLE */
WiFiClient espClient;
PubSubClient MQTT(espClient);
TaskHandle_t Task1;

/* DEFINITION OF GPIO PINES VARIABLES */
#define valvS1 26  
#define valvS2 25
#define valvS3a 33
#define valvS3r 32

/* DECLARATION OF GLOBAL VARIABLES */
char statusacao = {0};
bool statusvalv1 = 0;
bool statusvalv2 = 0;
bool statusvalv3 = 0;


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
  
  pinMode(valvS1, OUTPUT);
  pinMode(valvS2, OUTPUT);
  pinMode(valvS3a, OUTPUT);
  pinMode(valvS3r, OUTPUT);

  Valv1(recuo);
  Valv2(recuo);
  Valv3(recuo);

  // Setup a Task using the free core of ESP32
  int CORE_ID = int(!xPortGetCoreID());
  xTaskCreatePinnedToCore(
    Task1code, // Task function
    "Task1",   // Name of the Task to atach
    10000,     // Size to alocated to Task (Word or Byte)
    NULL,      // Task argument to be passed (void*)
    0,         // Priority of the Task [0 a 25]
    &Task1,    // Task ID
    CORE_ID);  // Task core ID (0 ou 1)
  delay(500);
}

/* TASK FUNCTION USING FREE CORE OF ESP32 */
void Task1code(void* pvParameters) {
  Serial.print("Conexões (Wi-Fi e MQTT) rodando no nucleo ");
  Serial.println(xPortGetCoreID());

  for (;;) {
    if (!MQTT.connected()) conectarMQTT();

    MQTT.loop();
  }
}

/* LOOP FUNCTION TO RUNNING PROGRAM */
void loop() { }

/* CALLBACK FUNCTION FOR SUBSCRIBE TOPICS */
void callback(char* topic, byte* payload, unsigned int length) {
  if (statusacao != (char)payload[0]) {
    Serial.print("Mensagem recebida no tópico: ");
    Serial.println(topic);

    statusacao = (char)payload[0];
    Serial.println("Conteúdo da mensagem: " + String(statusacao));

    if ((char)payload[0] == '1') Valv1(recuo);
    else if ((char)payload[0] == '2') Valv1(avanco);
    else if ((char)payload[0] == '3') Valv2(recuo);
    else if ((char)payload[0] == '4') Valv2(avanco);
    else if ((char)payload[0] == '5') Valv3(recuo);
    else if ((char)payload[0] == '6') Valv3(avanco);
  }
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
    if (MQTT.connect("ESP32Client")){
      //publish();

      MQTT.subscribe(topvalv);
    }
  }
}

/* VALVE 1 FUNCTION FOR DRIVE THE VALVE */
void Valv1(bool status) {
  digitalWrite(valvS1, status);

  statusvalv1 = !status;
}

/* VALVE 2 FUNCTION FOR DRIVE THE VALVE */
void Valv2(bool status) {
  digitalWrite(valvS2, status);

  statusvalv2 = !status;
}

/* VALVE 3 FUNCTION FOR DRIVE THE VALVE */
void Valv3(bool status) {
  if (status) {
    digitalWrite(valvS3a, status);
    delay(10);
    digitalWrite(valvS3r, !status);
  } else {
    digitalWrite(valvS3r, !status);
    delay(10);
    digitalWrite(valvS3a, status);
  }

  statusvalv3 = !status;
}

/* PUBLISHER FUNCTION FOR RETURN VALVE STATUS */
void publish() {
  if (statusvalv1) MQTT.publish(topvalv1, "Avançado", 2);
  else if (!statusvalv1) MQTT.publish(topvalv1, "Recuado", 2);

  if (statusvalv2) MQTT.publish(topvalv2, "Avançado", 2);
  else if (!statusvalv2) MQTT.publish(topvalv2, "Recuado", 2);

  if (statusvalv3) MQTT.publish(topvalv3, "Avançado", 2);
  else if (!statusvalv3) MQTT.publish(topvalv3, "Recuado", 2);
}