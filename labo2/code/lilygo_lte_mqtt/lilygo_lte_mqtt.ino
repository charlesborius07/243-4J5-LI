// LilyGO T-SIM A7670G - Version LTE/Cellulaire avec MQTT via WebSocket
// Utilise le modem cellulaire A7670G au lieu du WiFi

#define TINY_GSM_MODEM_SIM7600  // Le A7670G est compatible avec SIM7600
#define TINY_GSM_RX_BUFFER 1024

#include <TinyGsmClient.h>
#include <WebSocketsClient.h>
#include <vector>

#include "auth.h" // Fichier contenant APN et identifiants MQTT

// ====== CONFIG MODEM A7670G ======
#define MODEM_TX 26
#define MODEM_RX 27
#define MODEM_PWRKEY 4
#define MODEM_DTR 32
#define MODEM_RI 33
#define MODEM_FLIGHT 25
#define MODEM_STATUS 34

#define SD_MISO 2
#define SD_MOSI 15
#define SD_SCLK 14
#define SD_CS 13

// ====== CONFIG MQTT/WSS ======
const char* MQTT_HOST = "mqtt.edxo.ca";      // Votre domaine Cloudflare
const int   MQTT_WSS_PORT = 443;             // Port sécurisé SSL
const char* MQTT_PATH = "/";                 // WebSocket path
char MQTT_CLIENT_ID[20];                     // Sera généré depuis l'IMEI

// Les identifiants MQTT (MQTT_USER, MQTT_PASS) sont définis dans auth.h
// Le broker MQTT (MQTT_BROKER, MQTT_PORT) est également dans auth.h

char BUTTON1_STATE_TOPIC[50];
char BUTTON2_STATE_TOPIC[50];
char LED1_SET_TOPIC[50];
char LED2_SET_TOPIC[50];

// --- Configuration des broches (Pins) ---
const int LED1_PIN = 12;   // LED rouge
const int LED2_PIN = 13;   // LED verte
const int BUTTON1_PIN = 0; // Bouton 1
const int BUTTON2_PIN = 35; // Bouton 2

// Serial pour le modem
HardwareSerial SerialAT(1);

// Clients
TinyGsm modem(SerialAT);
TinyGsmClient gsmClient(modem);

// WebSocket client utilisant le client GSM
WebSocketsClient webSocket;

// État MQTT
bool mqttConnected = false;
unsigned long lastPing = 0;

// Pour la lecture non-bloquante des boutons
long lastButtonCheck = 0;
int lastButton1State = HIGH;
int lastButton2State = HIGH;

// Reconnexion GPRS
unsigned long lastGprsCheck = 0;
const unsigned long GPRS_CHECK_INTERVAL = 30000; // 30 secondes

// ===== Helpers MQTT (identiques à la version WiFi) =====

void mqtt_encode_remaining_length(uint32_t len, std::vector<uint8_t>& out) {
  do {
    uint8_t encodedByte = len % 128;
    len /= 128;
    if (len > 0) encodedByte |= 128;
    out.push_back(encodedByte);
  } while (len > 0);
}

std::vector<uint8_t> mqtt_build_connect_packet(const char* clientId, const char* username, const char* password, uint16_t keepAliveSeconds = 60) {
  std::vector<uint8_t> pkt;
  std::vector<uint8_t> vh;

  // "MQTT"
  vh.push_back(0x00); vh.push_back(0x04);
  vh.push_back('M');  vh.push_back('Q');  vh.push_back('T');  vh.push_back('T');

  // Protocol Level 4 (3.1.1)
  vh.push_back(0x04);

  // FLAGS: User(1) + Pass(1) + CleanSession(1) = 11000010 = 0xC2
  uint8_t connectFlags = 0xC2;
  vh.push_back(connectFlags);

  // Keep Alive
  vh.push_back(keepAliveSeconds >> 8);
  vh.push_back(keepAliveSeconds & 0xFF);

  // --- PAYLOAD ---
  std::vector<uint8_t> payload;

  // 1. Client ID
  uint16_t cidLen = strlen(clientId);
  payload.push_back(cidLen >> 8); payload.push_back(cidLen & 0xFF);
  for (uint16_t i = 0; i < cidLen; i++) payload.push_back((uint8_t)clientId[i]);

  // 2. Username
  uint16_t userLen = strlen(username);
  payload.push_back(userLen >> 8); payload.push_back(userLen & 0xFF);
  for (uint16_t i = 0; i < userLen; i++) payload.push_back((uint8_t)username[i]);

  // 3. Password
  uint16_t passLen = strlen(password);
  payload.push_back(passLen >> 8); payload.push_back(passLen & 0xFF);
  for (uint16_t i = 0; i < passLen; i++) payload.push_back((uint8_t)password[i]);

  // --- ASSEMBLAGE FINAL ---
  // fixed header
  pkt.push_back(0x10); // CONNECT
  std::vector<uint8_t> rl;
  mqtt_encode_remaining_length(vh.size() + payload.size(), rl);
  pkt.insert(pkt.end(), rl.begin(), rl.end());
  pkt.insert(pkt.end(), vh.begin(), vh.end());
  pkt.insert(pkt.end(), payload.begin(), payload.end());

  return pkt;
}

std::vector<uint8_t> mqtt_build_subscribe_packet(const char* topic, uint16_t packetId, uint8_t qos = 0) {
  std::vector<uint8_t> pkt;
  std::vector<uint8_t> vh;

  // Packet ID
  vh.push_back(packetId >> 8);
  vh.push_back(packetId & 0xFF);

  // Topic
  std::vector<uint8_t> payload;
  uint16_t topicLen = strlen(topic);
  payload.push_back(topicLen >> 8); payload.push_back(topicLen & 0xFF);
  for (uint16_t i = 0; i < topicLen; i++) payload.push_back((uint8_t)topic[i]);
  payload.push_back(qos);

  // Assemblage
  pkt.push_back(0x82); // SUBSCRIBE
  std::vector<uint8_t> rl;
  mqtt_encode_remaining_length(vh.size() + payload.size(), rl);
  pkt.insert(pkt.end(), rl.begin(), rl.end());
  pkt.insert(pkt.end(), vh.begin(), vh.end());
  pkt.insert(pkt.end(), payload.begin(), payload.end());

  return pkt;
}

std::vector<uint8_t> mqtt_build_publish_packet(const char* topic, const char* payload_str, uint8_t qos = 0, bool retain = false) {
  std::vector<uint8_t> pkt;

  // Fixed Header
  uint8_t fixedHeader = 0x30; // PUBLISH
  if (retain) fixedHeader |= 0x01;
  if (qos == 1) fixedHeader |= 0x02;
  pkt.push_back(fixedHeader);

  // Variable Header
  std::vector<uint8_t> vh;
  uint16_t topicLen = strlen(topic);
  vh.push_back(topicLen >> 8); vh.push_back(topicLen & 0xFF);
  for (uint16_t i = 0; i < topicLen; i++) vh.push_back((uint8_t)topic[i]);

  // Payload
  std::vector<uint8_t> payload;
  uint16_t payloadLen = strlen(payload_str);
  for (uint16_t i = 0; i < payloadLen; i++) payload.push_back((uint8_t)payload_str[i]);

  // Remaining Length
  std::vector<uint8_t> rl;
  mqtt_encode_remaining_length(vh.size() + payload.size(), rl);
  pkt.insert(pkt.end(), rl.begin(), rl.end());
  pkt.insert(pkt.end(), vh.begin(), vh.end());
  pkt.insert(pkt.end(), payload.begin(), payload.end());

  return pkt;
}

std::vector<uint8_t> mqtt_build_pingreq_packet() {
  std::vector<uint8_t> pkt;
  pkt.push_back(0xC0); // PINGREQ
  pkt.push_back(0x00); // Remaining Length = 0
  return pkt;
}

// ============================================================================
// FONCTIONS MODEM
// ============================================================================

void modemPowerOn() {
  Serial.println("[MODEM] Allumage du modem...");
  pinMode(MODEM_PWRKEY, OUTPUT);
  digitalWrite(MODEM_PWRKEY, HIGH);
  delay(100);
  digitalWrite(MODEM_PWRKEY, LOW);
  delay(1000);
  digitalWrite(MODEM_PWRKEY, HIGH);
  delay(3000); // Attendre que le modem démarre
  Serial.println("[MODEM] ✓ Modem allumé");
}

bool initModem() {
  Serial.println("[MODEM] Initialisation...");

  SerialAT.begin(115200, SERIAL_8N1, MODEM_RX, MODEM_TX);
  delay(3000);

  if (!modem.restart()) {
    Serial.println("[MODEM] ✗ Échec du redémarrage");
    return false;
  }

  String modemInfo = modem.getModemInfo();
  Serial.print("[MODEM] Info: ");
  Serial.println(modemInfo);

  // Récupérer l'IMEI pour générer le Device ID
  String imei = modem.getIMEI();
  Serial.print("[MODEM] IMEI: ");
  Serial.println(imei);

  // Générer Device ID: lte-XXXXXX (6 derniers chiffres de l'IMEI)
  String shortIMEI = imei.substring(imei.length() - 6);
  snprintf(MQTT_CLIENT_ID, sizeof(MQTT_CLIENT_ID), "lte-%s", shortIMEI.c_str());
  Serial.print("[MQTT] Device ID: ");
  Serial.println(MQTT_CLIENT_ID);

  // Générer les topics MQTT
  snprintf(LED1_SET_TOPIC, sizeof(LED1_SET_TOPIC), "%s/led/1/set", MQTT_CLIENT_ID);
  snprintf(LED2_SET_TOPIC, sizeof(LED2_SET_TOPIC), "%s/led/2/set", MQTT_CLIENT_ID);
  snprintf(BUTTON1_STATE_TOPIC, sizeof(BUTTON1_STATE_TOPIC), "%s/button/1/state", MQTT_CLIENT_ID);
  snprintf(BUTTON2_STATE_TOPIC, sizeof(BUTTON2_STATE_TOPIC), "%s/button/2/state", MQTT_CLIENT_ID);

  Serial.println("[MODEM] ✓ Initialisé");
  return true;
}

bool connectToNetwork() {
  Serial.println("[NETWORK] Connexion au réseau cellulaire...");

  if (!modem.waitForNetwork(60000L)) {
    Serial.println("[NETWORK] ✗ Échec de connexion au réseau");
    return false;
  }

  String operator_name = modem.getOperator();
  Serial.print("[NETWORK] Opérateur: ");
  Serial.println(operator_name);

  int signalQuality = modem.getSignalQuality();
  Serial.print("[NETWORK] Signal: ");
  Serial.print(signalQuality);
  Serial.println(" dBm");

  Serial.println("[GPRS] Connexion GPRS...");
  if (!modem.gprsConnect(APN, APN_USER, APN_PASS)) {
    Serial.println("[GPRS] ✗ Échec de connexion GPRS");
    return false;
  }

  if (!modem.isGprsConnected()) {
    Serial.println("[GPRS] ✗ GPRS non connecté");
    return false;
  }

  IPAddress ip = modem.localIP();
  Serial.print("[GPRS] IP: ");
  Serial.println(ip);
  Serial.println("[GPRS] ✓ Connecté");

  return true;
}

// ============================================================================
// WEBSOCKET EVENT
// ============================================================================

void webSocketEvent(WStype_t type, uint8_t* payload, size_t length) {
  switch (type) {
    case WStype_DISCONNECTED:
      Serial.println("[WSS] Déconnecté");
      mqttConnected = false;
      break;

    case WStype_CONNECTED:
      Serial.println("[WSS] Connecté au broker");
      {
        // Envoyer MQTT CONNECT
        std::vector<uint8_t> connectPacket = mqtt_build_connect_packet(MQTT_CLIENT_ID, MQTT_USER, MQTT_PASS);
        webSocket.sendBIN(connectPacket.data(), connectPacket.size());
        Serial.println("[MQTT] CONNECT envoyé");
      }
      break;

    case WStype_BIN:
      // Message MQTT reçu
      if (length > 0) {
        uint8_t packetType = payload[0] >> 4;

        if (packetType == 2) { // CONNACK
          uint8_t returnCode = payload[3];
          if (returnCode == 0) {
            Serial.println("[MQTT] ✓ Connecté au broker");
            mqttConnected = true;

            // Souscrire aux topics de contrôle des LEDs
            std::vector<uint8_t> sub1 = mqtt_build_subscribe_packet(LED1_SET_TOPIC, 1);
            webSocket.sendBIN(sub1.data(), sub1.size());

            std::vector<uint8_t> sub2 = mqtt_build_subscribe_packet(LED2_SET_TOPIC, 2);
            webSocket.sendBIN(sub2.data(), sub2.size());

            Serial.println("[MQTT] Souscriptions envoyées");
          } else {
            Serial.print("[MQTT] ✗ Échec de connexion, code: ");
            Serial.println(returnCode);
          }
        }
        else if (packetType == 3) { // PUBLISH
          // Décoder le message PUBLISH
          int pos = 1;

          // Remaining length
          uint32_t remainingLength = 0;
          uint8_t multiplier = 1;
          uint8_t encodedByte;
          do {
            encodedByte = payload[pos++];
            remainingLength += (encodedByte & 127) * multiplier;
            multiplier *= 128;
          } while ((encodedByte & 128) != 0);

          // Topic length
          uint16_t topicLen = (payload[pos] << 8) | payload[pos + 1];
          pos += 2;

          // Topic
          char topic[100];
          memcpy(topic, &payload[pos], topicLen);
          topic[topicLen] = '\0';
          pos += topicLen;

          // Payload
          char msg[100];
          int msgLen = length - pos;
          memcpy(msg, &payload[pos], msgLen);
          msg[msgLen] = '\0';

          Serial.print("[MQTT] ← ");
          Serial.print(topic);
          Serial.print(" = ");
          Serial.println(msg);

          // Contrôle des LEDs
          if (strcmp(topic, LED1_SET_TOPIC) == 0) {
            if (strcmp(msg, "ON") == 0) {
              digitalWrite(LED1_PIN, HIGH);
              Serial.println("[LED1] Allumée (ROUGE)");
            } else if (strcmp(msg, "OFF") == 0) {
              digitalWrite(LED1_PIN, LOW);
              Serial.println("[LED1] Éteinte");
            }
          }
          else if (strcmp(topic, LED2_SET_TOPIC) == 0) {
            if (strcmp(msg, "ON") == 0) {
              digitalWrite(LED2_PIN, HIGH);
              Serial.println("[LED2] Allumée (VERTE)");
            } else if (strcmp(msg, "OFF") == 0) {
              digitalWrite(LED2_PIN, LOW);
              Serial.println("[LED2] Éteinte");
            }
          }
        }
        else if (packetType == 13) { // PINGRESP
          // Silent ping response
        }
      }
      break;

    default:
      break;
  }
}

// ============================================================================
// GESTION DES BOUTONS
// ============================================================================

void checkButtons() {
  long now = millis();

  if (now - lastButtonCheck < 100) {
    return;
  }
  lastButtonCheck = now;

  if (!mqttConnected) return;

  // Vérifier le bouton 1
  int button1State = digitalRead(BUTTON1_PIN);
  if (button1State != lastButton1State) {
    lastButton1State = button1State;

    const char* state = (button1State == LOW) ? "PRESSED" : "RELEASED";
    std::vector<uint8_t> pub = mqtt_build_publish_packet(BUTTON1_STATE_TOPIC, state);
    webSocket.sendBIN(pub.data(), pub.size());

    Serial.print("[BTN1] → ");
    Serial.println(state);
  }

  // Vérifier le bouton 2
  int button2State = digitalRead(BUTTON2_PIN);
  if (button2State != lastButton2State) {
    lastButton2State = button2State;

    const char* state = (button2State == LOW) ? "PRESSED" : "RELEASED";
    std::vector<uint8_t> pub = mqtt_build_publish_packet(BUTTON2_STATE_TOPIC, state);
    webSocket.sendBIN(pub.data(), pub.size());

    Serial.print("[BTN2] → ");
    Serial.println(state);
  }
}

// ============================================================================
// SETUP
// ============================================================================

void setup() {
  Serial.begin(115200);
  delay(2000);

  Serial.println();
  Serial.println("=== LilyGo T-SIM A7670G - MQTT via LTE ===");
  Serial.println();

  // Configuration des pins
  pinMode(LED1_PIN, OUTPUT);
  pinMode(LED2_PIN, OUTPUT);
  pinMode(BUTTON1_PIN, INPUT_PULLUP);
  pinMode(BUTTON2_PIN, INPUT_PULLUP);

  digitalWrite(LED1_PIN, LOW);
  digitalWrite(LED2_PIN, LOW);

  // Démarrage du modem
  modemPowerOn();

  if (!initModem()) {
    Serial.println("[ERREUR] Impossible d'initialiser le modem");
    Serial.println("Redémarrez l'appareil");
    while (true) {
      digitalWrite(LED1_PIN, !digitalRead(LED1_PIN));
      delay(200);
    }
  }

  if (!connectToNetwork()) {
    Serial.println("[ERREUR] Impossible de se connecter au réseau");
    Serial.println("Vérifiez votre carte SIM et l'APN");
    while (true) {
      digitalWrite(LED1_PIN, !digitalRead(LED1_PIN));
      delay(500);
    }
  }

  // Configuration WebSocket avec SSL
  webSocket.beginSSL(MQTT_HOST, MQTT_WSS_PORT, MQTT_PATH);
  webSocket.onEvent(webSocketEvent);
  webSocket.setReconnectInterval(5000);

  Serial.println("[WSS] En attente de connexion...");

  // Attendre la connexion MQTT
  int attempts = 0;
  while (!mqttConnected && attempts < 30) {
    webSocket.loop();
    delay(1000);
    attempts++;
  }

  if (!mqttConnected) {
    Serial.println("[ERREUR] Impossible de se connecter au broker MQTT");
    Serial.println("Vérifiez les identifiants dans auth.h");
    while (true) {
      digitalWrite(LED1_PIN, !digitalRead(LED1_PIN));
      delay(1000);
    }
  }

  Serial.println();
  Serial.println("=== Système prêt ===");
  Serial.println();

  // Clignoter les LEDs pour indiquer que tout est OK
  for (int i = 0; i < 3; i++) {
    digitalWrite(LED1_PIN, HIGH);
    digitalWrite(LED2_PIN, HIGH);
    delay(200);
    digitalWrite(LED1_PIN, LOW);
    digitalWrite(LED2_PIN, LOW);
    delay(200);
  }
}

// ============================================================================
// LOOP
// ============================================================================

void loop() {
  unsigned long now = millis();

  // Vérifier la connexion GPRS périodiquement
  if (now - lastGprsCheck > GPRS_CHECK_INTERVAL) {
    lastGprsCheck = now;

    if (!modem.isGprsConnected()) {
      Serial.println("[GPRS] Connexion perdue, reconnexion...");
      mqttConnected = false;
      if (connectToNetwork()) {
        webSocket.beginSSL(MQTT_HOST, MQTT_WSS_PORT, MQTT_PATH);
        Serial.println("[GPRS] ✓ Reconnecté");
      }
    }
  }

  // Traiter les événements WebSocket
  webSocket.loop();

  // Vérifier les boutons
  checkButtons();

  // Envoyer un PING MQTT toutes les 30 secondes
  if (mqttConnected && (now - lastPing > 30000)) {
    lastPing = now;
    std::vector<uint8_t> ping = mqtt_build_pingreq_packet();
    webSocket.sendBIN(ping.data(), ping.size());
  }

  // Petit délai pour ne pas surcharger
  delay(10);
}
