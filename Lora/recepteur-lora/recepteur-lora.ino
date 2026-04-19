/*
 * Activite - Recepteur LoRa avec appel LLM de l'autre cote
 * Cegep de Limoilou - Objets connectes
 *
 * Materiel :
 *   - LilyGO T-Beam Supreme (ESP32-S3, SH1106 OLED, AXP2101 PMU)
 *
 * Dependances (Arduino Library Manager) :
 *   - ArduinoJson (Benoit Blanchon)
 *   - U8g2 (olikraus)
 *   - XPowersLib (Lewis He)
 *   - RadioLib (jgromes)
 *   - PubSubClient (Nick O'Leary)
 *
 * Board dans Arduino IDE :
 *   - ESP32S3 Dev Module (esp32:esp32:esp32s3)
 *   - USB CDC On Boot : Enabled
 *   - PSRAM : OPI PSRAM
 */

#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <esp_wpa2.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <U8g2lib.h>
#include <Wire.h>
#include <XPowersLib.h>
#include <RadioLib.h>
#include <SPI.h>
#include <PubSubClient.h>
#include <mbedtls/base64.h>

#include "config.h"

// =============================================
// CLASSE WRAPPER WEBSOCKET POUR PUBSUBCLIENT
// =============================================

class WebSocketClient : public Client {
private:
  WiFiClientSecure* _sslClient;
  bool _wsConnected;

  uint8_t _rxBuffer[512];
  size_t _rxBufferLen;
  size_t _rxBufferPos;

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

  bool readWebSocketFrame() {
    if (!_sslClient->available()) return false;

    uint8_t byte1 = _sslClient->read();
    if (!_sslClient->available()) return false;
    uint8_t byte2 = _sslClient->read();

    uint8_t opcode = byte1 & 0x0F;
    bool masked = (byte2 & 0x80) != 0;
    size_t payloadLen = byte2 & 0x7F;

    if (payloadLen == 126) {
      if (_sslClient->available() < 2) return false;
      payloadLen = (_sslClient->read() << 8) | _sslClient->read();
    } else if (payloadLen == 127) {
      if (_sslClient->available() < 8) return false;
      payloadLen = 0;
      for(int i = 0; i < 8; i++) {
        payloadLen = (payloadLen << 8) | _sslClient->read();
      }
    }

    uint8_t mask[4] = {0};
    if (masked) {
      if (_sslClient->available() < 4) return false;
      for(int i = 0; i < 4; i++) {
        mask[i] = _sslClient->read();
      }
    }

    if (opcode == 0x01 || opcode == 0x02) { // Text ou Binary
      if (_sslClient->available() < payloadLen) return false;

      _rxBufferLen = payloadLen < sizeof(_rxBuffer) ? payloadLen : sizeof(_rxBuffer);
      for(size_t i = 0; i < _rxBufferLen; i++) {
        _rxBuffer[i] = _sslClient->read();
        if (masked) _rxBuffer[i] ^= mask[i % 4];
      }
      _rxBufferPos = 0;
      return true;
    }
    else if (opcode == 0x08) { // Close
      Serial.println("[WSS] Serveur a ferme la connexion");
      _wsConnected = false;
      return false;
    }
    else if (opcode == 0x09) { // Ping
      uint8_t pong[2] = {0x8A, 0x00};
      _sslClient->write(pong, 2);
      return false;
    }

    return false;
  }

public:
  WebSocketClient(WiFiClientSecure* sslClient) {
    _sslClient = sslClient;
    _wsConnected = false;
    _rxBufferLen = 0;
    _rxBufferPos = 0;
  }

  int connect(IPAddress ip, uint16_t port) { return 0; }
  int connect(const char *host, uint16_t port) {
    Serial.println("[WSS] Connexion SSL...");

    if (!_sslClient->connect(host, port)) {
      Serial.println("[WSS] Echec connexion SSL");
      return 0;
    }

    Serial.println("[WSS] SSL connecte, envoi handshake WebSocket...");
    String wsKey = generateWebSocketKey();

    _sslClient->print("GET ");
    _sslClient->print(MQTT_PATH);
    _sslClient->print(" HTTP/1.1\r\nHost: ");
    _sslClient->print(host);
    _sslClient->print("\r\nUpgrade: websocket\r\n");
    _sslClient->print("Connection: Upgrade\r\n");
    _sslClient->print("Sec-WebSocket-Key: ");
    _sslClient->print(wsKey);
    _sslClient->print("\r\nSec-WebSocket-Protocol: mqtt\r\n");
    _sslClient->print("Sec-WebSocket-Version: 13\r\n\r\n");

    unsigned long timeout = millis();
    while (!_sslClient->available() && millis() - timeout < 5000) {
      delay(10);
    }

    if (!_sslClient->available()) {
      Serial.println("[WSS] Timeout handshake");
      return 0;
    }

    String response = "";
    while (_sslClient->available()) {
      char c = _sslClient->read();
      response += c;
      if (response.endsWith("\r\n\r\n")) break;
    }

    if (response.indexOf("101") > 0 && response.indexOf("Switching Protocols") > 0) {
      Serial.println("[WSS] Handshake WebSocket reussi!");
      _wsConnected = true;
      return 1;
    } else {
      Serial.println("[WSS] Handshake WebSocket echoue");
      return 0;
    }
  }

  size_t write(uint8_t b) {
    return write(&b, 1);
  }

  size_t write(const uint8_t *buf, size_t size) {
    if (!_wsConnected) return 0;

    uint8_t header[14];
    int headerLen = 2;
    header[0] = 0x82; // FIN + Binary frame

    if (size < 126) {
      header[1] = 0x80 | size;
    } else if (size < 65536) {
      header[1] = 0x80 | 126;
      header[2] = (size >> 8) & 0xFF;
      header[3] = size & 0xFF;
      headerLen = 4;
    } else {
      header[1] = 0x80 | 127;
      for(int i = 0; i < 8; i++) header[2 + i] = 0;
      header[6] = (size >> 24) & 0xFF;
      header[7] = (size >> 16) & 0xFF;
      header[8] = (size >> 8) & 0xFF;
      header[9] = size & 0xFF;
      headerLen = 10;
    }

    uint8_t mask[4];
    for(int i = 0; i < 4; i++) {
      mask[i] = random(0, 256);
      header[headerLen + i] = mask[i];
    }
    headerLen += 4;

    _sslClient->write(header, headerLen);

    for(size_t i = 0; i < size; i++) {
      uint8_t maskedByte = buf[i] ^ mask[i % 4];
      _sslClient->write(&maskedByte, 1);
    }

    return size;
  }

  int available() {
    if (_rxBufferPos < _rxBufferLen) {
      return _rxBufferLen - _rxBufferPos;
    }
    if (_sslClient->available()) {
      if (readWebSocketFrame()) {
        return _rxBufferLen - _rxBufferPos;
      }
    }
    return 0;
  }

  int read() {
    if (_rxBufferPos < _rxBufferLen) {
      return _rxBuffer[_rxBufferPos++];
    }
    if (_sslClient->available()) {
      if (readWebSocketFrame() && _rxBufferPos < _rxBufferLen) {
        return _rxBuffer[_rxBufferPos++];
      }
    }
    return -1;
  }

  int read(uint8_t *buf, size_t size) {
    size_t count = 0;
    while (count < size) {
      int c = read();
      if (c < 0) break;
      buf[count++] = (uint8_t)c;
    }
    return count;
  }

  int peek() {
    if (_rxBufferPos < _rxBufferLen) {
      return _rxBuffer[_rxBufferPos];
    }
    return -1;
  }

  void flush() { _sslClient->flush(); }
  void stop() { _wsConnected = false; _sslClient->stop(); }
  uint8_t connected() { return _wsConnected && _sslClient->connected(); }
  operator bool() { return _wsConnected; }
};

// =============================================
// PINS T-Beam Supreme
// =============================================

// I2C bus 0 : OLED + capteurs (SDA=17, SCL=18)
#define OLED_SDA        17
#define OLED_SCL        18

// I2C bus 1 : PMU AXP2101 (SDA=42, SCL=41)
#define PMU_SDA         42
#define PMU_SCL         41
#define PMU_IRQ_PIN     40

// =============================================
// LORA SX1262
// =============================================

#define LORA_SCK        12
#define LORA_MISO       13
#define LORA_MOSI       11
#define LORA_CS         10
#define LORA_DIO1       1
#define LORA_NRST       5
#define LORA_BUSY       4

// Pins SX1262 du T-Beam Supreme (NSS, DIO1, NRST, BUSY)
SX1262 radio = new Module(LORA_CS, LORA_DIO1, LORA_NRST, LORA_BUSY);

// =============================================
// OLED SH1106 128x64
// =============================================

U8G2_SH1106_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0, U8X8_PIN_NONE);

// =============================================
// PMU
// =============================================

XPowersAXP2101 pmu;

// =============================================
// CLIENTS MQTT & WIFI
// =============================================

WiFiClientSecure sslClient;
WebSocketClient wsClient(&sslClient);
PubSubClient mqttClient(wsClient);

// =============================================
// PROTOTYPES
// =============================================

void initPMU();
void oledPrint(String texte);
void connecterWiFi();
void connectMQTT();
String appelLLM(int valeurPot);

// =============================================
// SETUP
// =============================================

void setup() {
  Serial.begin(115200);
  delay(500);

  // SSL sans vérification du certificat
  sslClient.setInsecure();

  // I2C bus 0 : OLED
  Wire.begin(OLED_SDA, OLED_SCL);

  // I2C bus 1 : PMU
  Wire1.begin(PMU_SDA, PMU_SCL);

  // Initialiser le PMU (alimentation OLED, LoRa, GPS, etc.)
  initPMU();

  // Initialiser l'OLED avec support UTF-8 (accents francais)
  u8g2.begin();
  u8g2.enableUTF8Print();
  oledPrint("Demarrage...");

  // WiFi
  connecterWiFi();

  // MQTT
  mqttClient.setServer(MQTT_BROKER, MQTT_PORT);
  if (WiFi.status() == WL_CONNECTED) {
    connectMQTT();
  }

  // Initialiser LoRa
  oledPrint("Init LoRa...");
  // Passer -1 pour le CS afin que RadioLib puisse le contrôler
  SPI.begin(LORA_SCK, LORA_MISO, LORA_MOSI, -1);
  int state = radio.begin(915.0, 125.0, 9, 7, 0x12, 22, 8);
  if (state == RADIOLIB_ERR_NONE) {
    Serial.println("LoRa init success!");
    oledPrint("LoRa pret!\nAttente Rx...");
  } else {
    Serial.println("LoRa init failed, code " + String(state));
    oledPrint("Erreur LoRa:\n" + String(state));
    while (true);
  }
}

// =============================================
// LOOP
// =============================================

void loop() {
  // Maintenir la connexion MQTT
  if (WiFi.status() == WL_CONNECTED && !mqttClient.connected()) {
    connectMQTT();
  }
  if (mqttClient.connected()) {
    mqttClient.loop();
  }

  String receivedMsg;
  // Attente bloquante d'un message (avec timeout de 1 seconde pour garder la boucle active)
  int state = radio.receive(receivedMsg, 1000);
  
  if (state == RADIOLIB_ERR_NONE) {
    float rssi = radio.getRSSI();
    float snr = radio.getSNR();
    Serial.println("RX: " + receivedMsg + " RSSI: " + String(rssi) + " SNR: " + String(snr));
    
    oledPrint("RX: " + receivedMsg + "\nRSSI:" + String(rssi, 1) + " SNR:" + String(snr, 1) + "\nTraitement...");
    
    // Parser le JSON pour trouver la valeur du potentiometre
    JsonDocument doc;
    DeserializationError err = deserializeJson(doc, receivedMsg);
    int pot = 0;
    if (!err) {
      pot = doc["pot"] | 0;
    }
    
    // Appel du LLM
    String llmReply = appelLLM(pot);
    Serial.println("LLM: " + llmReply);
    
    // Renvoi de la reponse a l'emetteur via LoRa
    int txState = radio.transmit(llmReply);
    
    if (txState == RADIOLIB_ERR_NONE) {
      oledPrint("TX: " + llmReply + "\nSucces!");
    } else {
      oledPrint("TX Err: " + String(txState));
    }

    // Publication MQTT du résultat LLM
    if (mqttClient.connected()) {
      if (mqttClient.publish(TOPIC_PUB_DECISION, llmReply.c_str())) {
        Serial.println("Publié sur MQTT: " + String(TOPIC_PUB_DECISION) + " = " + llmReply);
      } else {
        Serial.println("Échec de la publication MQTT");
      }
    }
    
    // Petite pause pour bien lire l'ecran
    delay(2000);
    oledPrint("Attente Rx...");
  }
}

// =============================================
// INITIALISATION PMU AXP2101
// =============================================

void initPMU() {
  if (!pmu.init(Wire1, AXP2101_SLAVE_ADDRESS, PMU_SDA, PMU_SCL)) {
    Serial.println("Avertissement: PMU AXP2101 non detecte");
    return;
  }
  Serial.println("PMU AXP2101 initialise");

  // Alimenter les peripheriques du T-Beam Supreme
  pmu.setALDO1Voltage(3300);  pmu.enableALDO1();  // capteurs
  pmu.setALDO2Voltage(3300);  pmu.enableALDO2();  // capteurs
  pmu.setALDO3Voltage(3300);  pmu.enableALDO3();  // LoRa
  pmu.setALDO4Voltage(3300);  pmu.enableALDO4();  // GPS
  pmu.setBLDO1Voltage(3300);  pmu.enableBLDO1();  // SD card
  pmu.setBLDO2Voltage(3300);  pmu.enableBLDO2();
  pmu.setDC3Voltage(3300);    pmu.enableDC3();     // M.2
  pmu.setDC5Voltage(3300);    pmu.enableDC5();

  // LED de charge
  pmu.setChargingLedMode(XPOWERS_CHG_LED_CTRL_CHG);
}

// =============================================
// CONNEXION WIFI
// =============================================

void connecterWiFi() {
  Serial.println("Connexion WiFi...");
  oledPrint("WiFi...\n" + String(WIFI_SSID));

  WiFi.disconnect(true);
  WiFi.mode(WIFI_STA);
  delay(100);

  if (USE_WPA2_ENTERPRISE) {
    esp_wifi_sta_wpa2_ent_set_identity((uint8_t*)EAP_IDENTITY, strlen(EAP_IDENTITY));
    esp_wifi_sta_wpa2_ent_set_username((uint8_t*)EAP_USERNAME, strlen(EAP_USERNAME));
    esp_wifi_sta_wpa2_ent_set_password((uint8_t*)EAP_PASSWORD, strlen(EAP_PASSWORD));
    esp_wifi_sta_wpa2_ent_enable();
    WiFi.begin(WIFI_SSID);
  } else {
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  }

  int tentatives = 0;
  while (WiFi.status() != WL_CONNECTED && tentatives < 40) {
    delay(500);
    Serial.print(".");
    tentatives++;
  }

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\nWiFi connecte : " + WiFi.localIP().toString());
    oledPrint("WiFi OK\n" + WiFi.localIP().toString());
    delay(1000);
  } else {
    Serial.println("\nErreur WiFi !");
    oledPrint("Erreur WiFi\nVerifie config");
    // On ne bloque pas forcement pour que LoRa puisse marcher sans WiFi meme si le LLM va echouer
  }
}

// =============================================
// CONNEXION MQTT
// =============================================

void connectMQTT() {
  if (wsClient.connected() || mqttClient.connected()) return;
  
  Serial.println("Connexion au broker MQTT WSS...");
  if (mqttClient.connect(MQTT_CLIENT_ID, MQTT_USER, MQTT_PASS)) {
    Serial.println("MQTT Connecté !");
  } else {
    Serial.print("Echec connexion MQTT, code ");
    Serial.println(mqttClient.state());
  }
}

// =============================================
// APPEL API OPENWEBUI / GROQ
// =============================================

String appelLLM(int valeurPot) {
  if (WiFi.status() != WL_CONNECTED) {
    return "{\"action\":\"none\",\"msg\":\"No WiFi\"}";
  }

  HTTPClient http;
  http.begin(OPENWEBUI_URL);
  http.addHeader("Content-Type", "application/json");
  http.addHeader("Authorization", String("Bearer ") + API_KEY);
  http.setTimeout(30000);

  // Construction du JSON
  JsonDocument doc;
  doc["model"] = MODEL_NAME;

  JsonArray messages = doc["messages"].to<JsonArray>();

  JsonObject systemMsg = messages.add<JsonObject>();
  systemMsg["role"]    = "system";
  systemMsg["content"] = SYSTEM_PROMPT;

  JsonObject userMsg = messages.add<JsonObject>();
  userMsg["role"]    = "user";
  userMsg["content"] = "potentiometre: " + String(valeurPot);

  String payload;
  serializeJson(doc, payload);

  Serial.println("Payload LLM: " + payload);

  int httpCode = http.POST(payload);
  String reponse = "";

  if (httpCode == 200) {
    String body = http.getString();

    JsonDocument rep;
    DeserializationError err = deserializeJson(rep, body);

    if (!err) {
      reponse = rep["choices"][0]["message"]["content"].as<String>();
      // Nettoyer d'eventuelles balises markdown (Groq a tendance a encadrer les JSON)
      reponse.replace("```json", "");
      reponse.replace("```", "");
      reponse.trim();
    } else {
      reponse = "{\"action\":\"none\",\"msg\":\"JSON Err\"}";
    }
  } else {
    reponse = "{\"action\":\"none\",\"msg\":\"HTTP " + String(httpCode) + "\"}";
    Serial.println(http.getString());
  }

  http.end();
  return reponse;
}

// =============================================
// AFFICHAGE OLED MULTILIGNES
// =============================================

void oledPrint(String texte) {
  u8g2.clearBuffer();
  u8g2.setFont(u8g2_font_helvB08_tf);
  
  int y = 10;
  int maxWidth = 128;
  const char* p = texte.c_str();

  while (*p && y <= 64) {
    const char* lineStart = p;
    const char* lastSpace = NULL;
    const char* scan = p;

    while (*scan && *scan != '\n') {
      const char* next = scan;
      if ((*next & 0x80) == 0) next += 1;
      else if ((*next & 0xE0) == 0xC0) next += 2;
      else if ((*next & 0xF0) == 0xE0) next += 3;
      else next += 4;

      int len = next - lineStart;
      char buf[128];
      if (len < (int)sizeof(buf)) {
        memcpy(buf, lineStart, len);
        buf[len] = '\0';
        if (u8g2.getUTF8Width(buf) > maxWidth) break;
      }

      if (*scan == ' ') lastSpace = scan;
      scan = next;
    }

    const char* lineEnd;
    if (*scan == '\0' || *scan == '\n') {
      lineEnd = scan;
      p = (*scan == '\n') ? scan + 1 : scan;
    } else if (lastSpace && lastSpace > lineStart) {
      lineEnd = lastSpace;
      p = lastSpace + 1;
    } else {
      lineEnd = scan;
      p = scan;
    }

    int len = lineEnd - lineStart;
    char lineBuf[128];
    if (len >= (int)sizeof(lineBuf)) len = sizeof(lineBuf) - 1;
    memcpy(lineBuf, lineStart, len);
    lineBuf[len] = '\0';
    u8g2.drawUTF8(0, y, lineBuf);
    y += 11;
  }

  u8g2.sendBuffer();
}