#include <WiFi.h>
#include <WebSocketsClient.h>
#include <esp_wpa2.h> // Keep for Enterprise WiFi

#include "auth.h" // Fichier contenant les identifiants WiFi

// ====== CONFIG MQTT/WSS ======
const char* MQTT_HOST = "mqtt.edxo.ca";
const int   MQTT_WSS_PORT = 443;
const char* MQTT_PATH = "/";          // WebSocket path
char MQTT_CLIENT_ID[20]; // Will be generated from MAC
char MQTT_TOPIC_PUB[50]; // Will be generated
char MQTT_TOPIC_SUB[50]; // Will be generated

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

// encode Remaining Length (MQTT varint)
void mqtt_encode_remaining_length(uint32_t len, std::vector<uint8_t>& out) {
  do {
    uint8_t encodedByte = len % 128;
    len /= 128;
    if (len > 0) encodedByte |= 128;
    out.push_back(encodedByte);
  } while (len > 0);
}

// construit un paquet CONNECT très simple (MQTT 3.1.1, clean session, sans user/pass)
std::vector<uint8_t> mqtt_build_connect_packet(const char* clientId, uint16_t keepAliveSeconds = 60) {
  std::vector<uint8_t> pkt;

  // Variable header
  std::vector<uint8_t> vh;

  // "MQTT"
  vh.push_back(0x00);
  vh.push_back(0x04);
  vh.push_back('M');
  vh.push_back('Q');
  vh.push_back('T');
  vh.push_back('T');

  // protocol level 4 (MQTT 3.1.1)
  vh.push_back(0x04);

  // connect flags: clean session = 1, no will, no user/pass
  uint8_t connectFlags = 0;
  connectFlags |= 0x02; // Clean Session
  vh.push_back(connectFlags);

  // keep alive
  vh.push_back(keepAliveSeconds >> 8);
  vh.push_back(keepAliveSeconds & 0xFF);

  // Payload: Client ID
  std::vector<uint8_t> payload;
  uint16_t cidLen = strlen(clientId);
  payload.push_back(cidLen >> 8);
  payload.push_back(cidLen & 0xFF);
  for (uint16_t i = 0; i < cidLen; i++) {
    payload.push_back((uint8_t)clientId[i]);
  }

  // Fixed header
  pkt.push_back(0x10); // CONNECT

  // Remaining length = vh.size + payload.size (encodé varint)
  std::vector<uint8_t> rl;
  mqtt_encode_remaining_length(vh.size() + payload.size(), rl);
  pkt.insert(pkt.end(), rl.begin(), rl.end());

  // variable header + payload
  pkt.insert(pkt.end(), vh.begin(), vh.end());
  pkt.insert(pkt.end(), payload.begin(), payload.end());

  return pkt;
}

// PINGREQ
std::vector<uint8_t> mqtt_build_pingreq() {
  std::vector<uint8_t> pkt;
  pkt.push_back(0xC0); // PINGREQ
  pkt.push_back(0x00);
  return pkt;
}

// PUBLISH QoS0
std::vector<uint8_t> mqtt_build_publish(const char* topic, const char* message) {
  std::vector<uint8_t> pkt;

  // Topic
  std::vector<uint8_t> topicBuf;
  uint16_t tlen = strlen(topic);
  topicBuf.push_back(tlen >> 8);
  topicBuf.push_back(tlen & 0xFF);
  for (uint16_t i = 0; i < tlen; i++) {
    topicBuf.push_back((uint8_t)topic[i]);
  }

  // Payload
  std::vector<uint8_t> payload;
  uint16_t mlen = strlen(message);
  for (uint16_t i = 0; i < mlen; i++) {
    payload.push_back((uint8_t)message[i]);
  }

  // Fixed header
  pkt.push_back(0x30); // PUBLISH, QoS0

  // Remaining length
  std::vector<uint8_t> rl;
  mqtt_encode_remaining_length(topicBuf.size() + payload.size(), rl);
  pkt.insert(pkt.end(), rl.begin(), rl.end());

  // Topic + payload
  pkt.insert(pkt.end(), topicBuf.begin(), topicBuf.end());
  pkt.insert(pkt.end(), payload.begin(), payload.end());

  return pkt;
}

// SUBSCRIBE QoS0 sur un topic (packetId=1)
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
  payload.push_back(0x00); // requested QoS 0

  pkt.push_back(0x82); // SUBSCRIBE, QoS1
  std::vector<uint8_t> rl;
  mqtt_encode_remaining_length(vh.size() + payload.size(), rl);
  pkt.insert(pkt.end(), rl.begin(), rl.end());
  pkt.insert(pkt.end(), vh.begin(), vh.end());
  pkt.insert(pkt.end(), payload.begin(), payload.end());

  return pkt;
}

// parse PUBLISH très simple (UNSEULEMENT QoS0, pas de DUP, etc.)
void mqtt_parse_publish(const uint8_t* data, size_t len) {
  if (len < 4) return;

  uint8_t header = data[0];
  uint8_t msgType = header >> 4;
  if (msgType != 3) return; // pas un PUBLISH

  // Remaining length (on suppose petit, 1 octet pour le demo)
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

  // payload
  String payload;
  for (size_t i = idx; i < len; i++) {
    payload += (char)data[i];
  }

  Serial.print("[MQTT] PUBLISH reçu - topic='");
  Serial.print(topic);
  Serial.print("' payload='");
  Serial.print(payload);
  Serial.println("'");

  // Contrôler les LEDs
  if (topic == (String)LED1_SET_TOPIC) {
    if (payload == "ON") {
      digitalWrite(LED1_PIN, HIGH);
      Serial.println("LED 1 allumée");
    } else if (payload == "OFF") {
      digitalWrite(LED1_PIN, LOW);
      Serial.println("LED 1 éteinte");
    }
  } else if (topic == (String)LED2_SET_TOPIC) {
    if (payload == "ON") {
      digitalWrite(LED2_PIN, HIGH);
      Serial.println("LED 2 allumée");
    } else if (payload == "OFF") {
      digitalWrite(LED2_PIN, LOW);
      Serial.println("LED 2 éteinte");
    }
  }
}


// ===== WebSocket event =====

void webSocketEvent(WStype_t type, uint8_t * payload, size_t length) {
  switch (type) {
    case WStype_DISCONNECTED:
      Serial.println("[WS] Disconnected");
      mqttConnected = false;
      break;

    case WStype_CONNECTED:
      Serial.println("[WS] Connected to WSS, sending MQTT CONNECT");
      {
        auto pkt = mqtt_build_connect_packet(MQTT_CLIENT_ID);
        Serial.print("[MQTT] CONNECT packet: ");
        for (int i = 0; i < pkt.size(); i++) {
          if (pkt[i] < 0x10) Serial.print("0");
          Serial.print(pkt[i], HEX);
          Serial.print(" ");
        }
        Serial.println();
        webSocket.sendBIN(pkt.data(), pkt.size());
      }
      break;

    case WStype_BIN:
      Serial.printf("[WS] Binary frame len=%u\n", (unsigned)length);
      Serial.print("[MQTT] Received BIN packet: ");
      for (int i = 0; i < length; i++) {
        if (payload[i] < 0x10) Serial.print("0");
        Serial.print(payload[i], HEX);
        Serial.print(" ");
      }
      Serial.println();
      if (length >= 4 && payload[0] == 0x20) {
        // CONNACK
        // payload[0]=0x20, payload[1]=remaining length(2), payload[2]=flags, payload[3]=return code
        if (payload[3] == 0x00) {
          Serial.println("[MQTT] CONNACK OK, connecté");
          mqttConnected = true;

          // s'abonner à un topic
          auto subPkt = mqtt_build_subscribe(MQTT_TOPIC_SUB, 1);
          webSocket.sendBIN(subPkt.data(), subPkt.size());
        } else {
          Serial.print("[MQTT] CONNACK ERROR code=");
          Serial.println(payload[3]);
        }
      } else {
        // peut-être un PUBLISH
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

  // --- Configuration des broches (Pins) ---
  pinMode(LED1_PIN, OUTPUT);
  pinMode(LED2_PIN, OUTPUT);
  pinMode(BUTTON1_PIN, INPUT_PULLUP);
  pinMode(BUTTON2_PIN, INPUT_PULLUP);

  digitalWrite(LED1_PIN, LOW);
  digitalWrite(LED2_PIN, LOW);

  Serial.println();
  Serial.print("Connexion WiFi à ");
  Serial.println(WIFI_SSID); // From auth.h

  // Connexion WiFi (WPA2-Enterprise ou WPA2-Personal)
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
  sprintf(MQTT_CLIENT_ID, "esp32-%s", shortMac.c_str());
  sprintf(BUTTON1_STATE_TOPIC, "%s/button/1/state", MQTT_CLIENT_ID);
  sprintf(BUTTON2_STATE_TOPIC, "%s/button/2/state", MQTT_CLIENT_ID);
  sprintf(LED1_SET_TOPIC, "%s/led/1/set", MQTT_CLIENT_ID);
  sprintf(LED2_SET_TOPIC, "%s/led/2/set", MQTT_CLIENT_ID);

  // MQTT_TOPIC_PUB and MQTT_TOPIC_SUB will be directly mapped to button 1 state and led 1 set
  // This is a mapping from the original sketch structure
  strcpy(MQTT_TOPIC_PUB, BUTTON1_STATE_TOPIC);
  strcpy(MQTT_TOPIC_SUB, LED1_SET_TOPIC);
  
  Serial.printf("Device ID: %s\n", MQTT_CLIENT_ID);
  Serial.printf("Topic Bouton 1: %s\n", BUTTON1_STATE_TOPIC);
  Serial.printf("Topic Bouton 2: %s\n", BUTTON2_STATE_TOPIC);
  Serial.printf("Topic LED 1: %s\n", LED1_SET_TOPIC);
  Serial.printf("Topic LED 2: %s\n", LED2_SET_TOPIC);


  // WebSocket client
  webSocket.beginSSL(MQTT_HOST, MQTT_WSS_PORT, MQTT_PATH);
  webSocket.onEvent(webSocketEvent);

  // important : Cloudflare + Mosquitto n'ont généralement pas besoin de subprotocol,
  // mais certains brokers aiment "mqtt"
  webSocket.setExtraHeaders("Sec-WebSocket-Protocol: mqtt\r\n");

  // Pour le développement/test sans un CA valide, vous pourriez devoir désactiver la vérification (utiliser avec prudence!):
  // webSocket.setInsecure(); 
}

void loop() {
  webSocket.loop();

  // ping MQTT toutes les 30 s
  unsigned long now = millis();
  if (mqttConnected && now - lastPing > 30000) {
    lastPing = now;
    auto ping = mqtt_build_pingreq();
    webSocket.sendBIN(ping.data(), ping.size());
    Serial.println("[MQTT] PINGREQ envoyé");
  }

  // Lecture non-bloquante des boutons (toutes les 50ms)
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