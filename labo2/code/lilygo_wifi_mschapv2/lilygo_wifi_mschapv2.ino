#include <WiFi.h>
#include <WebSocketsClient.h>
#include <esp_wpa2.h> // Keep for Enterprise WiFi
#include <vector>

#include "auth.h" // Fichier contenant les identifiants WiFi (WIFI_SSID, WIFI_PASSWORD, etc.)

// ====== CONFIG MQTT/WSS ======
const char* MQTT_HOST = "mqtt.edxo.ca";      // Votre domaine Cloudflare
const int   MQTT_WSS_PORT = 443;             // Port sécurisé SSL
const char* MQTT_PATH = "/";                 // WebSocket path
char MQTT_CLIENT_ID[20];                     // Will be generated from MAC

// ====== IDENTIFIANTS MOSQUITTO ======
const char* MQTT_USER = "esp_user";          // Votre utilisateur Mosquitto
const char* MQTT_PASS = "1234";              // Votre mot de passe (celui défini sur le Pi)

char BUTTON1_STATE_TOPIC[50];
char BUTTON2_STATE_TOPIC[50];
char LED1_SET_TOPIC[50];
char LED2_SET_TOPIC[50];

// --- Configuration des broches (Pins) ---
const int LED1_PIN = 22;
const int LED2_PIN = 23;
const int BUTTON1_PIN = 18;
const int BUTTON2_PIN = 19;

WebSocketsClient webSocket;

// état MQTT
bool mqttConnected = false;
unsigned long lastPing = 0;

// Pour la lecture non-bloquante des boutons
long lastButtonCheck = 0;
int lastButton1State = HIGH;
long lastButton2State = HIGH;

// ===== helpers MQTT =====

void mqtt_encode_remaining_length(uint32_t len, std::vector<uint8_t>& out) {
  do {
    uint8_t encodedByte = len % 128;
    len /= 128;
    if (len > 0) encodedByte |= 128;
    out.push_back(encodedByte);
  } while (len > 0);
}

// VERSION MISE À JOUR POUR USERNAME / PASSWORD
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

std::vector<uint8_t> mqtt_build_pingreq() {
  std::vector<uint8_t> pkt;
  pkt.push_back(0xC0);
  pkt.push_back(0x00);
  return pkt;
}

std::vector<uint8_t> mqtt_build_publish(const char* topic, const char* message) {
  std::vector<uint8_t> pkt;
  std::vector<uint8_t> topicBuf;

  uint16_t tlen = strlen(topic);
  topicBuf.push_back(tlen >> 8);
  topicBuf.push_back(tlen & 0xFF);
  for (uint16_t i = 0; i < tlen; i++) {
    topicBuf.push_back((uint8_t)topic[i]);
  }

  std::vector<uint8_t> payload;
  uint16_t mlen = strlen(message);
  for (uint16_t i = 0; i < mlen; i++) {
    payload.push_back((uint8_t)message[i]);
  }

  pkt.push_back(0x30); // PUBLISH QoS0

  std::vector<uint8_t> rl;
  mqtt_encode_remaining_length(topicBuf.size() + payload.size(), rl);
  pkt.insert(pkt.end(), rl.begin(), rl.end());

  pkt.insert(pkt.end(), topicBuf.begin(), topicBuf.end());
  pkt.insert(pkt.end(), payload.begin(), payload.end());

  return pkt;
}

std::vector<uint8_t> mqtt_build_subscribe(const char* topic, uint16_t packetId = 1) {
  std::vector<uint8_t> pkt;
  std::vector<uint8_t> vh;
  vh.push_back(packetId >> 8);
  vh.push_back(packetId & 0xFF);

  std::vector<uint8_t> payload;
  uint16_t tlen = strlen(topic);
  payload.push_back(tlen >> 8);
  payload.push_back(tlen & 0xFF);
  for (uint16_t i = 0; i < tlen; i++) payload.push_back((uint8_t)topic[i]);
  payload.push_back(0x00); // QoS 0

  pkt.push_back(0x82); // SUBSCRIBE QoS1
  std::vector<uint8_t> rl;
  mqtt_encode_remaining_length(vh.size() + payload.size(), rl);
  pkt.insert(pkt.end(), rl.begin(), rl.end());
  pkt.insert(pkt.end(), vh.begin(), vh.end());
  pkt.insert(pkt.end(), payload.begin(), payload.end());

  return pkt;
}

void mqtt_parse_publish(const uint8_t* data, size_t len) {
  if (len < 4) return;

  uint8_t header = data[0];
  uint8_t msgType = header >> 4;
  if (msgType != 3) return; // pas un PUBLISH

  // on suppose Remaining Length tient sur 1 octet (demo only)
  uint8_t rl = data[1];
  size_t idx = 2;

  if (idx + 2 > len) return;
  uint16_t topicLen = (data[idx] << 8) | data[idx+1];
  idx += 2;
  if (idx + topicLen > len) return;

  String topic;
  for (uint16_t i = 0; i < topicLen; i++) {
    topic += (char)data[idx + i];
  }
  idx += topicLen;

  String payload;
  for (size_t i = idx; i < len; i++) {
    payload += (char)data[i];
  }

  Serial.print("[MQTT] PUBLISH reçu - topic='");
  Serial.print(topic);
  Serial.print("' payload='");
  Serial.print(payload);
  Serial.println("'");

  if (topic == String(LED1_SET_TOPIC)) {
    digitalWrite(LED1_PIN, (payload == "ON") ? HIGH : LOW);
  } else if (topic == String(LED2_SET_TOPIC)) {
    digitalWrite(LED2_PIN, (payload == "ON") ? HIGH : LOW);
  }
}

// ===== WebSocket event =====

void webSocketEvent(WStype_t type, uint8_t * payload, size_t length) {
  Serial.printf("[WS] Event type=%d, len=%u\n", type, (unsigned)length);

  switch (type) {
    case WStype_ERROR:
      Serial.println("[WS] ERROR");
      mqttConnected = false;
      break;

    case WStype_DISCONNECTED:
      Serial.println("[WS] DISCONNECTED");
      mqttConnected = false;
      break;

    case WStype_CONNECTED:
      Serial.printf("[WS] CONNECTED to: %s\n", payload);
      {
        // Envoi du paquet CONNECT avec USER et PASSWORD
        auto pkt = mqtt_build_connect_packet(MQTT_CLIENT_ID, MQTT_USER, MQTT_PASS);
        
        Serial.print("[MQTT] CONNECT packet:");
        for (auto b : pkt) Serial.printf(" %02X", b);
        Serial.println();
        webSocket.sendBIN(pkt.data(), pkt.size());
      }
      break;

    case WStype_TEXT:
      Serial.print("[WS] TEXT: ");
      Serial.write(payload, length);
      Serial.println();
      break;

    case WStype_BIN:
      Serial.printf("[WS] BIN (%u bytes):", (unsigned)length);
      for (size_t i = 0; i < length; i++) Serial.printf(" %02X", payload[i]);
      Serial.println();

      if (length >= 4 && payload[0] == 0x20) {
        // CONNACK
        uint8_t rc = payload[3];
        Serial.printf("[MQTT] CONNACK rc=%u\n", rc);
        if (rc == 0) {
          mqttConnected = true;
          // s'abonner aux topics LED
          auto sub1 = mqtt_build_subscribe(LED1_SET_TOPIC, 1);
          auto sub2 = mqtt_build_subscribe(LED2_SET_TOPIC, 2);
          webSocket.sendBIN(sub1.data(), sub1.size());
          webSocket.sendBIN(sub2.data(), sub2.size());
        } else {
          Serial.println("ERREUR D'AUTHENTIFICATION ! Vérifiez user/pass.");
          mqttConnected = false;
        }
      } else {
        mqtt_parse_publish(payload, length);
      }
      break;

    default:
      break;
  }
}

// ===== setup/loop =====

void setup() {
  Serial.begin(115200);
  delay(1000);

  pinMode(LED1_PIN, OUTPUT);
  pinMode(LED2_PIN, OUTPUT);
  pinMode(BUTTON1_PIN, INPUT_PULLUP);
  pinMode(BUTTON2_PIN, INPUT_PULLUP);
  digitalWrite(LED1_PIN, LOW);
  digitalWrite(LED2_PIN, LOW);

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

  // ID & topics MQTT
  String mac = WiFi.macAddress();
  mac.replace(":", "");
  String shortMac = mac.substring(6);
  sprintf(MQTT_CLIENT_ID, "esp32-%s", shortMac.c_str());
  sprintf(BUTTON1_STATE_TOPIC, "%s/button/1/state", MQTT_CLIENT_ID);
  sprintf(BUTTON2_STATE_TOPIC, "%s/button/2/state", MQTT_CLIENT_ID);
  sprintf(LED1_SET_TOPIC, "%s/led/1/set", MQTT_CLIENT_ID);
  sprintf(LED2_SET_TOPIC, "%s/led/2/set", MQTT_CLIENT_ID);

  Serial.printf("Device ID: %s\n", MQTT_CLIENT_ID);
  Serial.printf("Topic Bouton 1: %s\n", BUTTON1_STATE_TOPIC);
  Serial.printf("Topic Bouton 2: %s\n", BUTTON2_STATE_TOPIC);
  Serial.printf("Topic LED 1: %s\n", LED1_SET_TOPIC);
  Serial.printf("Topic LED 2: %s\n", LED2_SET_TOPIC);

  // WebSocket client avec SSL
  // Note: le dernier argument "mqtt" est CRUCIAL pour Mosquitto
  webSocket.beginSSL(MQTT_HOST, MQTT_WSS_PORT, MQTT_PATH, "", "mqtt");
  
  webSocket.onEvent(webSocketEvent);
  webSocket.setReconnectInterval(5000);
}

void loop() {
  webSocket.loop();

  unsigned long now = millis();

  if (mqttConnected && now - lastPing > 30000) {
    lastPing = now;
    auto ping = mqtt_build_pingreq();
    webSocket.sendBIN(ping.data(), ping.size());
    Serial.println("[MQTT] PINGREQ envoyé");
  }

  if (mqttConnected && millis() - lastButtonCheck > 50) {
    lastButtonCheck = millis();

    int button1State = digitalRead(BUTTON1_PIN);
    if (button1State != lastButton1State) {
      lastButton1State = button1State;
      const char* stateMsg = (button1State == LOW) ? "PRESSED" : "RELEASED";
      auto pub = mqtt_build_publish(BUTTON1_STATE_TOPIC, stateMsg);
      webSocket.sendBIN(pub.data(), pub.size());
      Serial.printf("Bouton 1: %s\n", stateMsg);
    }

    int button2State = digitalRead(BUTTON2_PIN);
    if (button2State != lastButton2State) {
      lastButton2State = button2State;
      const char* stateMsg = (button2State == LOW) ? "PRESSED" : "RELEASED";
      auto pub = mqtt_build_publish(BUTTON2_STATE_TOPIC, stateMsg);
      webSocket.sendBIN(pub.data(), pub.size());
      Serial.printf("Bouton 2: %s\n", stateMsg);
    }
  }
}