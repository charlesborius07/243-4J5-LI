/*
 * Activite - Recepteur LoRa avec appel LLM de l'autre cote
 * Cegep de Limoilou - Objets connectes
 *
 * Materiel :
 *   - LilyGO T-Beam Supreme (ESP32-S3, SH1106 OLED, AXP2101 PMU)
 *   - LED sur GPIO 46
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

#define LED_ACTION      46    

#define OLED_SDA        17
#define OLED_SCL        18
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
void clignoterLED(int fois);

// =============================================
// SETUP
// =============================================

void setup() {
  Serial.begin(115200);
  delay(500);

  pinMode(LED_ACTION, OUTPUT);
  digitalWrite(LED_ACTION, LOW);

  sslClient.setInsecure();

  Wire.begin(OLED_SDA, OLED_SCL);
  Wire1.begin(PMU_SDA, PMU_SCL);

  initPMU();

  u8g2.begin();
  u8g2.enableUTF8Print();
  oledPrint("Demarrage...");

  connecterWiFi();

  mqttClient.setServer(MQTT_BROKER, MQTT_PORT);
  if (WiFi.status() == WL_CONNECTED) {
    connectMQTT();
  }

  oledPrint("Init LoRa...");
  SPI.begin(LORA_SCK, LORA_MISO, LORA_MOSI, -1);
  int state = radio.begin(915.0, 125.0, 9, 7, 0x12, 22, 8);
  if (state == RADIOLIB_ERR_NONE) {
    Serial.println("LoRa init success!");
    oledPrint("LoRA RX Pret!\nEn attente...");
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
  if (WiFi.status() == WL_CONNECTED && !mqttClient.connected()) {
    connectMQTT();
  }
  if (mqttClient.connected()) {
    mqttClient.loop();
  }

  String receivedMsg;
  int state = radio.receive(receivedMsg, 1000);
  
  if (state == RADIOLIB_ERR_NONE) {
    clignoterLED(2); 
    
    float rssi = radio.getRSSI();
    float snr = radio.getSNR();
    Serial.println("RX: " + receivedMsg + " RSSI: " + String(rssi) + " SNR: " + String(snr));
    
    JsonDocument doc;
    DeserializationError err = deserializeJson(doc, receivedMsg);
    int pot = 0;
    if (!err) {
      pot = doc["pot"] | 0;
    }
    
    oledPrint("RECU:\n" + receivedMsg + "\nRSSI:" + String(rssi, 1) + " SNR:" + String(snr, 1));
    
    clignoterLED(1); 
    String llmReply = appelLLM(pot);
    Serial.println("LLM: " + llmReply);
    
    oledPrint("RECU:\n" + receivedMsg + "\nRSSI:" + String(rssi,1) + " SNR:" + String(snr,1) + "\nLLM:\n" + llmReply);
    
    radio.transmit(llmReply);
    clignoterLED(2); 

    if (mqttClient.connected()) {
      mqttClient.publish(TOPIC_PUB_DECISION, llmReply.c_str());
    }
    
    delay(3000); 
    oledPrint("LoRA RX Pret!\nEn attente...");
  }
}

// =============================================
// FONCTIONS
// =============================================

void clignoterLED(int fois) {
  for (int i = 0; i < fois; i++) {
    digitalWrite(LED_ACTION, HIGH);
    delay(100);
    digitalWrite(LED_ACTION, LOW);
    if (i < fois - 1) delay(100);
  }
}

void initPMU() {
  if (!pmu.init(Wire1, AXP2101_SLAVE_ADDRESS, PMU_SDA, PMU_SCL)) return;
  pmu.setALDO1Voltage(3300); pmu.enableALDO1();
  pmu.setALDO2Voltage(3300); pmu.enableALDO2();
  pmu.setALDO3Voltage(3300); pmu.enableALDO3();
  pmu.setALDO4Voltage(3300); pmu.enableALDO4();
  pmu.setBLDO1Voltage(3300); pmu.enableBLDO1();
  pmu.setBLDO2Voltage(3300); pmu.enableBLDO2();
  pmu.setDC3Voltage(3300);   pmu.enableDC3();
  pmu.setDC5Voltage(3300);   pmu.enableDC5();
  pmu.setChargingLedMode(XPOWERS_CHG_LED_CTRL_CHG);
}

void connecterWiFi() {
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
    tentatives++;
  }
}

void connectMQTT() {
  if (mqttClient.connect(MQTT_CLIENT_ID, MQTT_USER, MQTT_PASS)) {
    Serial.println("MQTT Connecté !");
  }
}

String appelLLM(int valeurPot) {
  if (WiFi.status() != WL_CONNECTED) return "{\"action\":\"none\",\"msg\":\"No WiFi\"}";
  HTTPClient http;
  http.begin(OPENWEBUI_URL);
  http.addHeader("Content-Type", "application/json");
  http.addHeader("Authorization", String("Bearer ") + API_KEY);
  http.setTimeout(30000);
  JsonDocument doc;
  doc["model"] = MODEL_NAME;
  JsonArray messages = doc["messages"].to<JsonArray>();
  JsonObject systemMsg = messages.add<JsonObject>();
  systemMsg["role"] = "system";
  systemMsg["content"] = SYSTEM_PROMPT;
  JsonObject userMsg = messages.add<JsonObject>();
  userMsg["role"] = "user";
  userMsg["content"] = "potentiometre: " + String(valeurPot);
  String payload;
  serializeJson(doc, payload);
  int httpCode = http.POST(payload);
  String reponse = "";
  if (httpCode == 200) {
    JsonDocument rep;
    deserializeJson(rep, http.getString());
    reponse = rep["choices"][0]["message"]["content"].as<String>();
    reponse.replace("```json", "");
    reponse.replace("```", "");
    reponse.trim();
  } else {
    reponse = "{\"action\":\"none\",\"msg\":\"HTTP Err\"}";
  }
  http.end();
  return reponse;
}

void oledPrint(String texte) {
  u8g2.clearBuffer();
  u8g2.setFont(u8g2_font_helvB08_tf);
  int y = 10;
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
        if (u8g2.getUTF8Width(buf) > 128) break;
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
