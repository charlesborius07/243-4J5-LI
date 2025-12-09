/*
  Projet: Client MQTT avec connexion WiFi WPA2-Enterprise
  Auteur: Gemini
  Date: 2025-12-08

  Description:
  Ce code transforme l'ESP32 en client MQTT. Il se connecte à un réseau WiFi
  WPA2-Enterprise, puis au broker MQTT à mqtt.edxo.ca.

  Fonctionnalités:
  - Lit l'état de deux boutons et publie les changements sur des topics MQTT.
  - S'abonne à des topics MQTT pour contrôler deux LEDs.
  - Utilise l'adresse MAC pour générer un Client ID et des topics uniques.
  - Gère la reconnexion automatique au broker MQTT.
*/

#include <WiFi.h>
#include <esp_wpa2.h>
#include <PubSubClient.h>
#include "auth.h" // Fichier contenant les identifiants WiFi

// --- Configuration des broches (Pins) ---
const int LED1_PIN = 22;
const int LED2_PIN = 23;
const int BUTTON1_PIN = 18;
const int BUTTON2_PIN = 19;

// --- Configuration MQTT ---
const char* MQTT_BROKER = "mqtt.edxo.ca";
const int MQTT_PORT = 1883; // Changed back to 1883 for unencrypted MQTT
char deviceId[20];

// Topics MQTT (le deviceId sera préfixé)
char BUTTON1_STATE_TOPIC[50];
char BUTTON2_STATE_TOPIC[50];
char LED1_SET_TOPIC[50];
char LED2_SET_TOPIC[50];

// --- Variables Globales ---
WiFiClient wifiClient; // Use standard WiFiClient
PubSubClient mqttClient(wifiClient);

// Pour la lecture non-bloquante des boutons
long lastButtonCheck = 0;
int lastButton1State = HIGH;
long lastButton2State = HIGH;

// --- Functions ---

// Appelé à la réception d'un message MQTT
void callback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Message reçu sur le topic: ");
  Serial.println(topic);

  // Convertir le payload en String
  char message[length + 1];
  for (int i = 0; i < length; i++) {
    message[i] = (char)payload[i];
  }
  message[length] = '\0';
  String messageStr = String(message);

  // Contrôler les LEDs
  if (strcmp(topic, LED1_SET_TOPIC) == 0) {
    if (messageStr == "ON") {
      digitalWrite(LED1_PIN, HIGH);
      Serial.println("LED 1 allumée");
    } else if (messageStr == "OFF") {
      digitalWrite(LED1_PIN, LOW);
      Serial.println("LED 1 éteinte");
    }
  } else if (strcmp(topic, LED2_SET_TOPIC) == 0) {
    if (messageStr == "ON") {
      digitalWrite(LED2_PIN, HIGH);
      Serial.println("LED 2 allumée");
    } else if (messageStr == "OFF") {
      digitalWrite(LED2_PIN, LOW);
      Serial.println("LED 2 éteinte");
    }
  }
}

// Pour (re)connecter au broker MQTT
void reconnect() {
  while (!mqttClient.connected()) {
    Serial.print("Tentative de connexion MQTT...");
    Serial.print("Tentative de connexion MQTT avec Client ID: ");
    Serial.println(deviceId);
    if (mqttClient.connect(deviceId)) {
      Serial.println("connecté!");
      // S'abonner aux topics de contrôle des LEDs
      mqttClient.subscribe(LED1_SET_TOPIC);
      mqttClient.subscribe(LED2_SET_TOPIC);
      Serial.print("Abonné à: "); Serial.println(LED1_SET_TOPIC);
      Serial.print("Abonné à: "); Serial.println(LED2_SET_TOPIC);
    } else {
      Serial.print("échec, rc=");
      Serial.print(mqttClient.state());
      Serial.print(" (");
      switch (mqttClient.state()) {
        case MQTT_CONNECTION_TIMEOUT: Serial.print("MQTT_CONNECTION_TIMEOUT"); break;
        case MQTT_CONNECTION_LOST: Serial.print("MQTT_CONNECTION_LOST"); break;
        case MQTT_CONNECT_FAILED: Serial.print("MQTT_CONNECT_FAILED"); break;
        case MQTT_DISCONNECTED: Serial.print("MQTT_DISCONNECTED"); break;
        case MQTT_CONNECTED: Serial.print("MQTT_CONNECTED"); break;
        case MQTT_CONNECT_BAD_PROTOCOL: Serial.print("MQTT_CONNECT_BAD_PROTOCOL"); break;
        case MQTT_CONNECT_BAD_CLIENT_ID: Serial.print("MQTT_CONNECT_BAD_CLIENT_ID"); break;
        case MQTT_CONNECT_UNAVAILABLE: Serial.print("MQTT_CONNECT_UNAVAILABLE"); break;
        case MQTT_CONNECT_BAD_CREDENTIALS: Serial.print("MQTT_CONNECT_BAD_CREDENTIALS"); break;
        case MQTT_CONNECT_UNAUTHORIZED: Serial.print("MQTT_CONNECT_UNAUTHORIZED"); break;
        default: Serial.print("UNKNOWN"); break;
      }
      Serial.println(") nouvelle tentative dans 5 secondes");
      delay(5000);
    }
  }
}

void setup() {
  Serial.begin(115200);

  // Configurer les broches
  pinMode(LED1_PIN, OUTPUT);
  pinMode(LED2_PIN, OUTPUT);
  pinMode(BUTTON1_PIN, INPUT_PULLUP);
  pinMode(BUTTON2_PIN, INPUT_PULLUP);

  digitalWrite(LED1_PIN, LOW);
  digitalWrite(LED2_PIN, LOW);
  
  delay(10);
  Serial.println();
  Serial.print("Connexion WiFi à ");
  Serial.println(WIFI_SSID);

  WiFi.disconnect(true);
  WiFi.mode(WIFI_STA);

#ifdef WIFI_SECURITY_WPA2_ENTERPRISE
  Serial.println("Using WPA2-Enterprise connection.");
  esp_wifi_sta_wpa2_ent_set_identity((uint8_t *)EAP_IDENTITY, strlen(EAP_IDENTITY));
  esp_wifi_sta_wpa2_ent_set_username((uint8_t *)EAP_USERNAME, strlen(EAP_USERNAME));
  esp_wifi_sta_wpa2_ent_set_password((uint8_t *)EAP_PASSWORD, strlen(EAP_PASSWORD));
  esp_wifi_sta_wpa2_ent_enable();
  WiFi.begin(WIFI_SSID);
#elif defined(WIFI_SECURITY_WPA2_PERSONAL)
  Serial.println("Using WPA2-Personal connection.");
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
#else
  // Fallback for open networks or if no security is defined
  Serial.println("Using Open/Undefined WiFi connection.");
  WiFi.begin(WIFI_SSID);
#endif

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
    Serial.print(" WiFi Status: ");
    Serial.println(WiFi.status());
  }
  Serial.println("\nWiFi connecté!");
  Serial.print("Adresse IP: ");
  Serial.println(WiFi.localIP());

  // Générer l'ID et les topics MQTT à partir de l'adresse MAC
  String mac = WiFi.macAddress();
  mac.replace(":", "");
  String shortMac = mac.substring(6);
  sprintf(deviceId, "esp32-%s", shortMac.c_str());
  
  sprintf(BUTTON1_STATE_TOPIC, "%s/button/1/state", deviceId);
  sprintf(BUTTON2_STATE_TOPIC, "%s/button/2/state", deviceId);
  sprintf(LED1_SET_TOPIC, "%s/led/1/set", deviceId);
  sprintf(LED2_SET_TOPIC, "%s/led/2/set", deviceId);

  Serial.printf("Device ID: %s\n", deviceId);
  Serial.printf("Topic Bouton 1: %s\n", BUTTON1_STATE_TOPIC);
  Serial.printf("Topic LED 1: %s\n", LED1_SET_TOPIC);

  // Configuration du client MQTT
  mqttClient.setServer(MQTT_BROKER, MQTT_PORT);
  mqttClient.setCallback(callback);
}

void loop() {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("WiFi déconnecté.");
    delay(1000);
    return;
  }

  if (!mqttClient.connected()) {
    reconnect();
  }
  mqttClient.loop();

  // Lecture non-bloquante des boutons (toutes les 50ms)
  if (millis() - lastButtonCheck > 50) {
    lastButtonCheck = millis();

    int button1State = digitalRead(BUTTON1_PIN);
    if (button1State != lastButton1State) {
      lastButton1State = button1State;
      const char* stateMsg = (button1State == LOW) ? "PRESSED" : "RELEASED";
      mqttClient.publish(BUTTON1_STATE_TOPIC, stateMsg);
      Serial.printf("Bouton 1: %s\n", stateMsg);
    }

    int button2State = digitalRead(BUTTON2_PIN);
    if (button2State != lastButton2State) {
      lastButton2State = button2State;
      const char* stateMsg = (button2State == LOW) ? "PRESSED" : "RELEASED";
      mqttClient.publish(BUTTON2_STATE_TOPIC, stateMsg);
       Serial.printf("Bouton 2: %s\n", stateMsg);
    }
  }
}
