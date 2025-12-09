// LilyGO T-SIM A7670G - Version LTE/Cellulaire avec MQTT via WebSocket SSL
// Utilise le modem cellulaire A7670G avec WebSocket manuel sur ESP_SSLClient

#define TINY_GSM_MODEM_SIM7600  // Le A7670G est compatible avec SIM7600
#define TINY_GSM_RX_BUFFER 1024

#include <TinyGsmClient.h>

// ESP_SSLClient configuration
#define ENABLE_DEBUG
#define ENABLE_ERROR_STRING
#define DEBUG_PORT Serial
#define SSLCLIENT_INSECURE_ONLY

#include <ESP_SSLClient.h>
#include <vector>
#include <mbedtls/base64.h>

#include "auth.h"

// ====== CONFIG MODEM A7670G ======
#define MODEM_TX 26
#define MODEM_RX 27
#define MODEM_PWRKEY 4
#define MODEM_DTR 12
#define MODEM_RI 13
#define MODEM_FLIGHT 25
#define MODEM_STATUS 0

#define SD_MISO 2
#define SD_MOSI 15
#define SD_SCLK 14
#define SD_CS 13

// ====== CONFIG MQTT/WSS ======
const char* MQTT_HOST = MQTT_BROKER;
const int   MQTT_WSS_PORT = 443;
const char* MQTT_PATH = "/";

char BUTTON1_STATE_TOPIC[50];
char BUTTON2_STATE_TOPIC[50];
char LED1_SET_TOPIC[50];
char LED2_SET_TOPIC[50];

// --- Configuration des broches (Pins) ---
const int LED1_PIN = 32;
const int LED2_PIN = 33;
const int BUTTON1_PIN = 34;
const int BUTTON2_PIN = 35;

// Serial pour le modem
HardwareSerial SerialAT(1);

// Clients
TinyGsm modem(SerialAT);
TinyGsmClient gsmClient(modem, 0);
ESP_SSLClient sslClient;

// État WebSocket/MQTT
bool wsConnected = false;
bool mqttConnected = false;
unsigned long lastPing = 0;
long lastButtonCheck = 0;
int lastButton1State = HIGH;
int lastButton2State = HIGH;
unsigned long lastGprsCheck = 0;
const unsigned long GPRS_CHECK_INTERVAL = 30000;

// ===== WebSocket Manuel =====

String generateWebSocketKey() {
  uint8_t key[16];
  for(int i = 0; i < 16; i++) {
    key[i] = random(0, 256);
  }

  size_t olen;
  unsigned char output[64];
  mbedtls_base64_encode(output, sizeof(output), &olen, key, 16);
  return String((char*)output);
}

bool connectWebSocket() {
  Serial.println("[WSS] Connexion SSL...");

  if (!sslClient.connect(MQTT_HOST, MQTT_WSS_PORT)) {
    Serial.println("[WSS] Echec connexion SSL");
    return false;
  }

  Serial.println("[WSS] SSL connecte, envoi handshake WebSocket...");

  String wsKey = generateWebSocketKey();

  // Envoyer le handshake WebSocket
  sslClient.print("GET ");
  sslClient.print(MQTT_PATH);
  sslClient.print(" HTTP/1.1\r\n");
  sslClient.print("Host: ");
  sslClient.print(MQTT_HOST);
  sslClient.print("\r\n");
  sslClient.print("Upgrade: websocket\r\n");
  sslClient.print("Connection: Upgrade\r\n");
  sslClient.print("Sec-WebSocket-Key: ");
  sslClient.print(wsKey);
  sslClient.print("\r\n");
  sslClient.print("Sec-WebSocket-Protocol: mqtt\r\n");
  sslClient.print("Sec-WebSocket-Version: 13\r\n");
  sslClient.print("\r\n");

  // Attendre la réponse
  unsigned long timeout = millis();
  while (!sslClient.available() && millis() - timeout < 5000) {
    delay(10);
  }

  if (!sslClient.available()) {
    Serial.println("[WSS] Timeout handshake");
    return false;
  }

  // Lire la réponse
  String response = "";
  while (sslClient.available()) {
    char c = sslClient.read();
    response += c;
    if (response.endsWith("\r\n\r\n")) break;
  }

  if (response.indexOf("101") > 0 && response.indexOf("Switching Protocols") > 0) {
    Serial.println("[WSS] Handshake WebSocket reussi!");
    wsConnected = true;
    return true;
  } else {
    Serial.println("[WSS] Handshake WebSocket echoue");
    Serial.println(response);
    return false;
  }
}

void sendWebSocketFrame(uint8_t* data, size_t length) {
  if (!wsConnected) return;

  // Frame WebSocket binaire avec masque
  uint8_t header[14];
  int headerLen = 2;

  header[0] = 0x82; // FIN + Binary frame

  if (length < 126) {
    header[1] = 0x80 | length; // Masked + length
  } else if (length < 65536) {
    header[1] = 0x80 | 126;
    header[2] = (length >> 8) & 0xFF;
    header[3] = length & 0xFF;
    headerLen = 4;
  } else {
    header[1] = 0x80 | 127;
    for(int i = 0; i < 8; i++) {
      header[2 + i] = 0; // On ne supporte pas les très gros messages
    }
    header[6] = (length >> 24) & 0xFF;
    header[7] = (length >> 16) & 0xFF;
    header[8] = (length >> 8) & 0xFF;
    header[9] = length & 0xFF;
    headerLen = 10;
  }

  // Masque aléatoire
  uint8_t mask[4];
  for(int i = 0; i < 4; i++) {
    mask[i] = random(0, 256);
    header[headerLen + i] = mask[i];
  }
  headerLen += 4;

  // Envoyer le header
  sslClient.write(header, headerLen);

  // Envoyer les données masquées
  for(size_t i = 0; i < length; i++) {
    uint8_t maskedByte = data[i] ^ mask[i % 4];
    sslClient.write(&maskedByte, 1);
  }
}

bool readWebSocketFrame(uint8_t* buffer, size_t* length) {
  if (!sslClient.available()) return false;

  uint8_t byte1 = sslClient.read();
  if (!sslClient.available()) return false;
  uint8_t byte2 = sslClient.read();

  uint8_t opcode = byte1 & 0x0F;
  bool masked = (byte2 & 0x80) != 0;
  size_t payloadLen = byte2 & 0x7F;

  // Lire la longueur étendue si nécessaire
  if (payloadLen == 126) {
    if (sslClient.available() < 2) return false;
    payloadLen = (sslClient.read() << 8) | sslClient.read();
  } else if (payloadLen == 127) {
    if (sslClient.available() < 8) return false;
    payloadLen = 0;
    for(int i = 0; i < 8; i++) {
      payloadLen = (payloadLen << 8) | sslClient.read();
    }
  }

  // Lire le masque si présent (normalement pas de masque du serveur)
  uint8_t mask[4] = {0};
  if (masked) {
    if (sslClient.available() < 4) return false;
    for(int i = 0; i < 4; i++) {
      mask[i] = sslClient.read();
    }
  }

  // Lire le payload
  if (opcode == 0x01 || opcode == 0x02) { // Text ou Binary
    if (sslClient.available() < payloadLen) return false;

    for(size_t i = 0; i < payloadLen && i < *length; i++) {
      buffer[i] = sslClient.read();
      if (masked) buffer[i] ^= mask[i % 4];
    }
    *length = payloadLen;
    return true;
  }
  else if (opcode == 0x08) { // Close
    Serial.println("[WSS] Serveur a ferme la connexion");
    wsConnected = false;
    return false;
  }
  else if (opcode == 0x09) { // Ping
    // Répondre avec Pong
    uint8_t pong[2] = {0x8A, 0x00};
    sslClient.write(pong, 2);
    return false;
  }

  return false;
}

// ===== Helpers MQTT =====

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

  vh.push_back(0x00); vh.push_back(0x04);
  vh.push_back('M');  vh.push_back('Q');  vh.push_back('T');  vh.push_back('T');
  vh.push_back(0x04);
  uint8_t connectFlags = 0xC2;
  vh.push_back(connectFlags);
  vh.push_back(keepAliveSeconds >> 8);
  vh.push_back(keepAliveSeconds & 0xFF);

  std::vector<uint8_t> payload;
  uint16_t cidLen = strlen(clientId);
  payload.push_back(cidLen >> 8); payload.push_back(cidLen & 0xFF);
  for (uint16_t i = 0; i < cidLen; i++) payload.push_back((uint8_t)clientId[i]);

  uint16_t userLen = strlen(username);
  payload.push_back(userLen >> 8); payload.push_back(userLen & 0xFF);
  for (uint16_t i = 0; i < userLen; i++) payload.push_back((uint8_t)username[i]);

  uint16_t passLen = strlen(password);
  payload.push_back(passLen >> 8); payload.push_back(passLen & 0xFF);
  for (uint16_t i = 0; i < passLen; i++) payload.push_back((uint8_t)password[i]);

  pkt.push_back(0x10);
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

  vh.push_back(packetId >> 8);
  vh.push_back(packetId & 0xFF);

  std::vector<uint8_t> payload;
  uint16_t topicLen = strlen(topic);
  payload.push_back(topicLen >> 8); payload.push_back(topicLen & 0xFF);
  for (uint16_t i = 0; i < topicLen; i++) payload.push_back((uint8_t)topic[i]);
  payload.push_back(qos);

  pkt.push_back(0x82);
  std::vector<uint8_t> rl;
  mqtt_encode_remaining_length(vh.size() + payload.size(), rl);
  pkt.insert(pkt.end(), rl.begin(), rl.end());
  pkt.insert(pkt.end(), vh.begin(), vh.end());
  pkt.insert(pkt.end(), payload.begin(), payload.end());

  return pkt;
}

std::vector<uint8_t> mqtt_build_publish_packet(const char* topic, const char* payload_str, uint8_t qos = 0, bool retain = false) {
  std::vector<uint8_t> pkt;

  uint8_t fixedHeader = 0x30;
  if (retain) fixedHeader |= 0x01;
  if (qos == 1) fixedHeader |= 0x02;
  pkt.push_back(fixedHeader);

  std::vector<uint8_t> vh;
  uint16_t topicLen = strlen(topic);
  vh.push_back(topicLen >> 8); vh.push_back(topicLen & 0xFF);
  for (uint16_t i = 0; i < topicLen; i++) vh.push_back((uint8_t)topic[i]);

  std::vector<uint8_t> payload;
  uint16_t payloadLen = strlen(payload_str);
  for (uint16_t i = 0; i < payloadLen; i++) payload.push_back((uint8_t)payload_str[i]);

  std::vector<uint8_t> rl;
  mqtt_encode_remaining_length(vh.size() + payload.size(), rl);
  pkt.insert(pkt.end(), rl.begin(), rl.end());
  pkt.insert(pkt.end(), vh.begin(), vh.end());
  pkt.insert(pkt.end(), payload.begin(), payload.end());

  return pkt;
}

std::vector<uint8_t> mqtt_build_pingreq_packet() {
  std::vector<uint8_t> pkt;
  pkt.push_back(0xC0);
  pkt.push_back(0x00);
  return pkt;
}

void processMqttMessage(uint8_t* payload, size_t length) {
  if (length == 0) return;

  uint8_t packetType = payload[0] >> 4;

  if (packetType == 2) { // CONNACK
    uint8_t returnCode = payload[3];
    if (returnCode == 0) {
      Serial.println("[MQTT] Connecte au broker");
      mqttConnected = true;

      std::vector<uint8_t> sub1 = mqtt_build_subscribe_packet(LED1_SET_TOPIC, 1);
      sendWebSocketFrame(sub1.data(), sub1.size());

      std::vector<uint8_t> sub2 = mqtt_build_subscribe_packet(LED2_SET_TOPIC, 2);
      sendWebSocketFrame(sub2.data(), sub2.size());

      Serial.println("[MQTT] Souscriptions envoyees");
    } else {
      Serial.print("[MQTT] Echec de connexion, code: ");
      Serial.println(returnCode);
    }
  }
  else if (packetType == 3) { // PUBLISH
    int pos = 1;

    uint32_t remainingLength = 0;
    uint8_t multiplier = 1;
    uint8_t encodedByte;
    do {
      encodedByte = payload[pos++];
      remainingLength += (encodedByte & 127) * multiplier;
      multiplier *= 128;
    } while ((encodedByte & 128) != 0);

    uint16_t topicLen = (payload[pos] << 8) | payload[pos + 1];
    pos += 2;

    char topic[100];
    memcpy(topic, &payload[pos], topicLen);
    topic[topicLen] = '\0';
    pos += topicLen;

    char msg[100];
    int msgLen = length - pos;
    memcpy(msg, &payload[pos], msgLen);
    msg[msgLen] = '\0';

    Serial.print("[MQTT] <- ");
    Serial.print(topic);
    Serial.print(" = ");
    Serial.println(msg);

    if (strcmp(topic, LED1_SET_TOPIC) == 0) {
      if (strcmp(msg, "ON") == 0) {
        digitalWrite(LED1_PIN, HIGH);
        Serial.println("[LED1] Allumee (ROUGE)");
      } else if (strcmp(msg, "OFF") == 0) {
        digitalWrite(LED1_PIN, LOW);
        Serial.println("[LED1] Eteinte");
      }
    }
    else if (strcmp(topic, LED2_SET_TOPIC) == 0) {
      if (strcmp(msg, "ON") == 0) {
        digitalWrite(LED2_PIN, HIGH);
        Serial.println("[LED2] Allumee (VERTE)");
      } else if (strcmp(msg, "OFF") == 0) {
        digitalWrite(LED2_PIN, LOW);
        Serial.println("[LED2] Eteinte");
      }
    }
  }
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
  delay(3000);
  Serial.println("[MODEM] Modem allume");
}

bool initModem() {
  Serial.println("[MODEM] Initialisation...");

  SerialAT.begin(115200, SERIAL_8N1, MODEM_RX, MODEM_TX);
  delay(3000);

  if (!modem.restart()) {
    Serial.println("[MODEM] Echec du redemarrage");
    return false;
  }

  String modemInfo = modem.getModemInfo();
  Serial.print("[MODEM] Info: ");
  Serial.println(modemInfo);

  String imei = modem.getIMEI();
  Serial.print("[MODEM] IMEI: ");
  Serial.println(imei);

  Serial.print("[MQTT] Device ID: ");
  Serial.println(MQTT_CLIENT_ID);

  snprintf(LED1_SET_TOPIC, sizeof(LED1_SET_TOPIC), "%s/led/1/set", MQTT_CLIENT_ID);
  snprintf(LED2_SET_TOPIC, sizeof(LED2_SET_TOPIC), "%s/led/2/set", MQTT_CLIENT_ID);
  snprintf(BUTTON1_STATE_TOPIC, sizeof(BUTTON1_STATE_TOPIC), "%s/button/1/state", MQTT_CLIENT_ID);
  snprintf(BUTTON2_STATE_TOPIC, sizeof(BUTTON2_STATE_TOPIC), "%s/button/2/state", MQTT_CLIENT_ID);

  Serial.println("[MODEM] Initialise");
  return true;
}

bool connectToNetwork() {
  Serial.println("[NETWORK] Configuration de l'APN...");

  modem.sendAT("+CGDCONT=1,\"IP\",\"", APN, "\"");
  if (modem.waitResponse() != 1) {
    Serial.println("[NETWORK] Echec de configuration APN");
  } else {
    Serial.println("[NETWORK] APN configure");
  }

  Serial.println("[NETWORK] Connexion au reseau cellulaire...");

  if (!modem.waitForNetwork(60000L)) {
    Serial.println("[NETWORK] Echec de connexion au reseau");
    return false;
  }

  String operator_name = modem.getOperator();
  Serial.print("[NETWORK] Operateur: ");
  Serial.println(operator_name);

  int signalQuality = modem.getSignalQuality();
  Serial.print("[NETWORK] Signal: ");
  Serial.print(signalQuality);
  Serial.println(" dBm");

  Serial.println("[GPRS] Connexion GPRS...");
  if (!modem.gprsConnect(APN, APN_USER, APN_PASS)) {
    Serial.println("[GPRS] Echec de connexion GPRS");
    return false;
  }

  if (!modem.isGprsConnected()) {
    Serial.println("[GPRS] GPRS non connecte");
    return false;
  }

  IPAddress ip = modem.localIP();
  Serial.print("[GPRS] IP: ");
  Serial.println(ip);
  Serial.println("[GPRS] Connecte");

  return true;
}

void checkButtons() {
  long now = millis();

  if (now - lastButtonCheck < 100) {
    return;
  }
  lastButtonCheck = now;

  if (!mqttConnected) return;

  int button1State = digitalRead(BUTTON1_PIN);
  if (button1State != lastButton1State) {
    lastButton1State = button1State;

    const char* state = (button1State == LOW) ? "PRESSED" : "RELEASED";
    std::vector<uint8_t> pub = mqtt_build_publish_packet(BUTTON1_STATE_TOPIC, state);
    sendWebSocketFrame(pub.data(), pub.size());

    Serial.print("[BTN1] -> ");
    Serial.println(state);
  }

  int button2State = digitalRead(BUTTON2_PIN);
  if (button2State != lastButton2State) {
    lastButton2State = button2State;

    const char* state = (button2State == LOW) ? "PRESSED" : "RELEASED";
    std::vector<uint8_t> pub = mqtt_build_publish_packet(BUTTON2_STATE_TOPIC, state);
    sendWebSocketFrame(pub.data(), pub.size());

    Serial.print("[BTN2] -> ");
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
  Serial.println("=== LilyGo T-SIM A7670G - MQTT via LTE + WebSocket SSL ===");
  Serial.println();

  pinMode(LED1_PIN, OUTPUT);
  pinMode(LED2_PIN, OUTPUT);
  pinMode(BUTTON1_PIN, INPUT_PULLUP);
  pinMode(BUTTON2_PIN, INPUT_PULLUP);

  digitalWrite(LED1_PIN, LOW);
  digitalWrite(LED2_PIN, LOW);

  modemPowerOn();

  if (!initModem()) {
    Serial.println("[ERREUR] Impossible d'initialiser le modem");
    while (true) {
      digitalWrite(LED1_PIN, !digitalRead(LED1_PIN));
      delay(200);
    }
  }

  if (!connectToNetwork()) {
    Serial.println("[ERREUR] Impossible de se connecter au reseau");
    while (true) {
      digitalWrite(LED1_PIN, !digitalRead(LED1_PIN));
      delay(500);
    }
  }

  // Configurer ESP_SSLClient
  Serial.println("[SSL] Configuration du client SSL...");
  sslClient.setClient(&gsmClient);
  sslClient.setInsecure();
  sslClient.setBufferSizes(2048, 1024);
  sslClient.setDebugLevel(1);

  // Connexion WebSocket
  if (!connectWebSocket()) {
    Serial.println("[ERREUR] Impossible de se connecter via WebSocket");
    while (true) {
      digitalWrite(LED1_PIN, !digitalRead(LED1_PIN));
      delay(1000);
    }
  }

  // Envoyer MQTT CONNECT
  Serial.println("[MQTT] Envoi du paquet CONNECT...");
  std::vector<uint8_t> connectPacket = mqtt_build_connect_packet(MQTT_CLIENT_ID, MQTT_USER, MQTT_PASS);
  sendWebSocketFrame(connectPacket.data(), connectPacket.size());

  // Attendre CONNACK
  unsigned long timeout = millis();
  while (!mqttConnected && millis() - timeout < 10000) {
    if (sslClient.available()) {
      uint8_t buffer[512];
      size_t len = sizeof(buffer);
      if (readWebSocketFrame(buffer, &len)) {
        processMqttMessage(buffer, len);
      }
    }
    delay(10);
  }

  if (!mqttConnected) {
    Serial.println("[ERREUR] Pas de reponse MQTT");
    while (true) {
      digitalWrite(LED1_PIN, !digitalRead(LED1_PIN));
      delay(1000);
    }
  }

  Serial.println();
  Serial.println("=== Systeme pret ===");
  Serial.println();

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

  // Vérifier la connexion GPRS
  if (now - lastGprsCheck > GPRS_CHECK_INTERVAL) {
    lastGprsCheck = now;

    if (!modem.isGprsConnected()) {
      Serial.println("[GPRS] Connexion perdue");
      wsConnected = false;
      mqttConnected = false;
      sslClient.stop();

      if (connectToNetwork() && connectWebSocket()) {
        std::vector<uint8_t> connectPacket = mqtt_build_connect_packet(MQTT_CLIENT_ID, MQTT_USER, MQTT_PASS);
        sendWebSocketFrame(connectPacket.data(), connectPacket.size());
      }
    }
  }

  // Lire les messages WebSocket/MQTT
  if (wsConnected && sslClient.available()) {
    uint8_t buffer[512];
    size_t len = sizeof(buffer);
    if (readWebSocketFrame(buffer, &len)) {
      processMqttMessage(buffer, len);
    }
  }

  // Vérifier les boutons
  checkButtons();

  // Envoyer un PING MQTT toutes les 30 secondes
  if (mqttConnected && (now - lastPing > 30000)) {
    lastPing = now;
    std::vector<uint8_t> ping = mqtt_build_pingreq_packet();
    sendWebSocketFrame(ping.data(), ping.size());
  }

  delay(10);
}
