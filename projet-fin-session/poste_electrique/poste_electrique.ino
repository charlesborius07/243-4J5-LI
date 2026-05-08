#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <PubSubClient.h>
#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BME280.h>
#include <esp_wpa2.h>
#include <mbedtls/base64.h>
<<<<<<< HEAD
=======
#include <HTTPClient.h>
#include <ArduinoJson.h>
>>>>>>> aa343ff121737cf536f2414c120e7c0f7d365f0b
#include "auth.h"

#define PIN_PIR 13
#define PIN_POT1 34
#define PIN_POT2 35
#define PIN_LED1 14
<<<<<<< HEAD
#define PIN_LED2 15
#define I2C_SCK 22
#define I2C_SDI 21

Adafruit_BME280 bme;
const char* MQTT_HOST = MQTT_BROKER;
const int   MQTT_WSS_PORT = 443;
const char* MQTT_PATH = "/";
=======
#define PIN_LED2 25
#define I2C_SCK 22
#define I2C_SDI 21

// ====== LLM CONFIG ======
const char* OPENWEBUI_URL = "https://api.groq.com/openai/v1/chat/completions";
const char* API_KEY       = "REMOVED";
const char* MODEL_NAME    = "openai/gpt-oss-20b";
const char* SYSTEM_PROMPT = "Tu es un contrôleur IoT. Tu reçois les données d'un poste électrique. Si la tension > 250V ou courant > 80A, active l'alarme (led2: 'on'), sinon éteins-la. Renvoie ABSOLUMENT UNIQUEMENT un JSON: {'summary': '...', 'led1': 'on'/'off', 'led2': 'on'/'off'}.";

Adafruit_BME280 bme;
const char* MQTT_HOST = MQTT_BROKER;
const int   MQTT_WSS_PORT = MQTT_PORT;
const char* MQTT_PATH = MQTT_PATH_STR;
>>>>>>> aa343ff121737cf536f2414c120e7c0f7d365f0b
const char* TOPIC_BASE = "hydro-limoilou/poste-05";

char TOPIC_TEMP[60];
char TOPIC_HUM[60];
<<<<<<< HEAD
char TOPIC_STATUS[60];
char TOPIC_INTRUSION[60];
char TOPIC_TENSION[60];
char TOPIC_COURANT[60];
char TOPIC_LED1[60];
char TOPIC_LED2[60];

=======
char TOPIC_PRESSURE[60];
char TOPIC_VIBRATION[60];
char TOPIC_STATUS[60];
char TOPIC_STATUS_LLM[60];
char TOPIC_INTRUSION[60];
char TOPIC_TILT[60];
char TOPIC_TENSION_LINE[60];
char TOPIC_CURRENT_LINE[60];
char TOPIC_LED1[60];
char TOPIC_LED2[60];

// ====== WEBSOCKET CLIENT ======
>>>>>>> aa343ff121737cf536f2414c120e7c0f7d365f0b
class WebSocketClient : public Client {
private:
  WiFiClientSecure* _sslClient;
  bool _wsConnected;
  uint8_t _rxBuffer[512];
  size_t _rxBufferLen;
  size_t _rxBufferPos;

  String generateWebSocketKey() {
    uint8_t key[16];
    for(int i = 0; i < 16; i++) key[i] = random(0, 256);
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
      for(int i = 0; i < 8; i++) payloadLen = (payloadLen << 8) | _sslClient->read();
    }
    uint8_t mask[4] = {0};
    if (masked) {
      if (_sslClient->available() < 4) return false;
      for(int i = 0; i < 4; i++) mask[i] = _sslClient->read();
    }
    if (opcode == 0x01 || opcode == 0x02) {
      if (_sslClient->available() < payloadLen) return false;
      _rxBufferLen = payloadLen < sizeof(_rxBuffer) ? payloadLen : sizeof(_rxBuffer);
      for(size_t i = 0; i < _rxBufferLen; i++) {
        _rxBuffer[i] = _sslClient->read();
        if (masked) _rxBuffer[i] ^= mask[i % 4];
      }
      _rxBufferPos = 0;
      return true;
    } else if (opcode == 0x08) {
<<<<<<< HEAD
      Serial.println("[WSS] Serveur a ferme la connexion");
=======
>>>>>>> aa343ff121737cf536f2414c120e7c0f7d365f0b
      _wsConnected = false;
      return false;
    } else if (opcode == 0x09) {
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
<<<<<<< HEAD
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
    while (!_sslClient->available() && millis() - timeout < 5000) delay(10);
    if (!_sslClient->available()) {
      Serial.println("[WSS] Timeout handshake");
      return 0;
    }
=======
    if (!_sslClient->connect(host, port)) return 0;
    String wsKey = generateWebSocketKey();
    _sslClient->print("GET "); _sslClient->print(MQTT_PATH);
    _sslClient->print(" HTTP/1.1\r\nHost: "); _sslClient->print(host);
    _sslClient->print("\r\nUpgrade: websocket\r\nConnection: Upgrade\r\n");
    _sslClient->print("Sec-WebSocket-Key: "); _sslClient->print(wsKey);
    _sslClient->print("\r\nSec-WebSocket-Protocol: mqtt\r\nSec-WebSocket-Version: 13\r\n\r\n");
    unsigned long timeout = millis();
    while (!_sslClient->available() && millis() - timeout < 5000) delay(10);
    if (!_sslClient->available()) return 0;
>>>>>>> aa343ff121737cf536f2414c120e7c0f7d365f0b
    String response = "";
    while (_sslClient->available()) {
      char c = _sslClient->read();
      response += c;
      if (response.endsWith("\r\n\r\n")) break;
    }
<<<<<<< HEAD
    if (response.indexOf("101") > 0 && response.indexOf("Switching Protocols") > 0) {
      Serial.println("[WSS] Handshake WebSocket reussi!");
      _wsConnected = true;
      return 1;
    } else {
      Serial.println("[WSS] Handshake WebSocket echoue");
      return 0;
    }
=======
    if (response.indexOf("101") > 0) {
      _wsConnected = true;
      return 1;
    }
    return 0;
>>>>>>> aa343ff121737cf536f2414c120e7c0f7d365f0b
  }

  size_t write(uint8_t b) { return write(&b, 1); }
  size_t write(const uint8_t *buf, size_t size) {
    if (!_wsConnected) return 0;
<<<<<<< HEAD
    uint8_t header[14];
    int headerLen = 2;
    header[0] = 0x82;
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
    if (_rxBufferPos < _rxBufferLen) return _rxBufferLen - _rxBufferPos;
    if (_sslClient->available()) {
      if (readWebSocketFrame()) return _rxBufferLen - _rxBufferPos;
    }
    return 0;
  }

  int read() {
    if (_rxBufferPos < _rxBufferLen) return _rxBuffer[_rxBufferPos++];
    if (_sslClient->available()) {
      if (readWebSocketFrame() && _rxBufferPos < _rxBufferLen) return _rxBuffer[_rxBufferPos++];
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
    if (_rxBufferPos < _rxBufferLen) return _rxBuffer[_rxBufferPos];
    return -1;
  }

=======
    uint8_t header[14]; int headerLen = 2; header[0] = 0x82;
    if (size < 126) header[1] = 0x80 | size;
    else if (size < 65536) { header[1] = 0x80 | 126; header[2] = (size >> 8) & 0xFF; header[3] = size & 0xFF; headerLen = 4; }
    else { header[1] = 0x80 | 127; headerLen = 10; }
    uint8_t mask[4];
    for(int i = 0; i < 4; i++) { mask[i] = random(0, 256); header[headerLen + i] = mask[i]; }
    headerLen += 4;
    _sslClient->write(header, headerLen);
    for(size_t i = 0; i < size; i++) { uint8_t maskedByte = buf[i] ^ mask[i % 4]; _sslClient->write(&maskedByte, 1); }
    return size;
  }
  int available() { if (_rxBufferPos < _rxBufferLen) return _rxBufferLen - _rxBufferPos; if (_sslClient->available()) if (readWebSocketFrame()) return _rxBufferLen - _rxBufferPos; return 0; }
  int read() { if (_rxBufferPos < _rxBufferLen) return _rxBuffer[_rxBufferPos++]; if (_sslClient->available()) if (readWebSocketFrame() && _rxBufferPos < _rxBufferLen) return _rxBuffer[_rxBufferPos++]; return -1; }
  int read(uint8_t *buf, size_t size) { size_t count = 0; while (count < size) { int c = read(); if (c < 0) break; buf[count++] = (uint8_t)c; } return count; }
  int peek() { if (_rxBufferPos < _rxBufferLen) return _rxBuffer[_rxBufferPos]; return -1; }
>>>>>>> aa343ff121737cf536f2414c120e7c0f7d365f0b
  void flush() { _sslClient->flush(); }
  void stop() { _wsConnected = false; _sslClient->stop(); }
  uint8_t connected() { return _wsConnected && _sslClient->connected(); }
  operator bool() { return _wsConnected; }
};

WiFiClientSecure wifiClient;
WebSocketClient wsClient(&wifiClient);
PubSubClient mqttClient(wsClient);

unsigned long lastTelemetry = 0;
<<<<<<< HEAD
const unsigned long TELEMETRY_INTERVAL = 5000;
int lastPIRState = LOW;
bool intrusionActive = false;

void mqttCallback(char* topic, byte* payload, unsigned int length) {
  String msg = "";
  for (unsigned int i = 0; i < length; i++) msg += (char)payload[i];
  Serial.print("[MQTT] <- ");
  Serial.print(topic);
  Serial.print(" = ");
  Serial.println(msg);
  if (strcmp(topic, TOPIC_LED1) == 0) {
    if (msg.indexOf("\"state\":\"on\"") >= 0 || msg == "on") {
      digitalWrite(PIN_LED1, HIGH);
      Serial.println("[LED1] Allumee (RESEAU ACTIF)");
    } else {
      digitalWrite(PIN_LED1, LOW);
      Serial.println("[LED1] Eteinte");
    }
  } else if (strcmp(topic, TOPIC_LED2) == 0) {
    if (msg.indexOf("\"state\":\"on\"") >= 0 || msg == "on") {
      digitalWrite(PIN_LED2, HIGH);
      Serial.println("[LED2] Allumee (ALARME)");
    } else {
      digitalWrite(PIN_LED2, LOW);
      Serial.println("[LED2] Eteinte");
    }
=======
const unsigned long TELEMETRY_INTERVAL = 10000;
unsigned long lastLLMCall = 0;
const unsigned long LLM_INTERVAL = 60000;
int lastPIRState = LOW;
bool intrusionActive = false;

// ====== LLM FUNCTION ======
String appelLLM(String prompt) {
  HTTPClient http;
  http.begin(OPENWEBUI_URL);
  http.addHeader("Content-Type", "application/json");
  http.addHeader("Authorization", String("Bearer ") + API_KEY);
  JsonDocument doc;
  doc["model"] = MODEL_NAME;
  JsonArray messages = doc["messages"].to<JsonArray>();
  JsonObject systemMsg = messages.add<JsonObject>();
  systemMsg["role"] = "system"; systemMsg["content"] = SYSTEM_PROMPT;
  JsonObject userMsg = messages.add<JsonObject>();
  userMsg["role"] = "user"; userMsg["content"] = prompt;
  String payload; serializeJson(doc, payload);
  int httpCode = http.POST(payload);
  String reponse = "Erreur appel LLM";
  if (httpCode == 200) {
    JsonDocument rep;
    deserializeJson(rep, http.getString());
    reponse = rep["choices"][0]["message"]["content"].as<String>();
  }
  http.end();
  return reponse;
}

void mqttCallback(char* topic, byte* payload, unsigned int length) {
  String msg = ""; for (unsigned int i = 0; i < length; i++) msg += (char)payload[i];
  Serial.print("[MQTT] <- "); Serial.print(topic); Serial.print(" = "); Serial.println(msg);
  JsonDocument doc; deserializeJson(doc, msg);
  String state = doc["state"] | "";
  if (strcmp(topic, TOPIC_LED1) == 0) {
    if (state == "on") digitalWrite(PIN_LED1, HIGH);
    else if (state == "off") digitalWrite(PIN_LED1, LOW);
  } else if (strcmp(topic, TOPIC_LED2) == 0) {
    if (state == "on") digitalWrite(PIN_LED2, HIGH);
    else if (state == "off") digitalWrite(PIN_LED2, LOW);
>>>>>>> aa343ff121737cf536f2414c120e7c0f7d365f0b
  }
}

void publishTelemetry() {
<<<<<<< HEAD
  if (!mqttClient.connected()) return;
  unsigned long ts = millis() / 1000;
  float temp = bme.readTemperature();
  float hum = bme.readHumidity();
  if (!isnan(temp)) {
    char payload[100];
    snprintf(payload, sizeof(payload), "{\"value\":%.1f,\"unit\":\"C\",\"ts\":%lu}", temp, ts);
    mqttClient.publish(TOPIC_TEMP, payload);
    Serial.print("[MQTT] -> ");
    Serial.println(payload);
  }
  if (!isnan(hum)) {
    char payload[100];
    snprintf(payload, sizeof(payload), "{\"value\":%.1f,\"unit\":\"%%\",\"ts\":%lu}", hum, ts);
    mqttClient.publish(TOPIC_HUM, payload);
  }
  int pot1 = analogRead(PIN_POT1);
  float tension = pot1 * 3.3 / 4095 * 100;
  char payloadTension[100];
  snprintf(payloadTension, sizeof(payloadTension), "{\"value\":%.1f,\"unit\":\"V\",\"ts\":%lu}", tension, ts);
  mqttClient.publish(TOPIC_TENSION, payloadTension);
  int pot2 = analogRead(PIN_POT2);
  float courant = pot2 * 3.3 / 4095 * 50;
  char payloadCourant[100];
  snprintf(payloadCourant, sizeof(payloadCourant), "{\"value\":%.2f,\"unit\":\"A\",\"ts\":%lu}", courant, ts);
  mqttClient.publish(TOPIC_COURANT, payloadCourant);
  long uptime = millis() / 1000;
  int rssi = WiFi.RSSI();
  char statusPayload[150];
  snprintf(statusPayload, sizeof(statusPayload), "{\"uptime\":%lu,\"rssi\":%d,\"link\":\"wifi\",\"ip\":\"%s\"}", uptime, rssi, WiFi.localIP().toString().c_str());
  mqttClient.publish(TOPIC_STATUS, statusPayload);
=======
  Serial.println("[Telemetry] Entrée dans publishTelemetry...");
  if (!mqttClient.connected()) {
    Serial.println("[MQTT] Client non connecté, impossible de publier.");
    return;
  }
  unsigned long ts = millis() / 1000;
  float temp = bme.readTemperature();
  float hum = bme.readHumidity();
  float pres = bme.readPressure() / 100.0F;

  if (!isnan(temp)) { 
    char p[100]; snprintf(p, sizeof(p), "{\"value\":%.1f,\"unit\":\"C\",\"ts\":%lu}", temp, ts); 
    mqttClient.publish(TOPIC_TEMP, p); 
    Serial.print("[MQTT] Publié sur "); Serial.print(TOPIC_TEMP); Serial.print(": "); Serial.println(p);
  } else {
    Serial.println("[Telemetry] Température NaN");
  }
  if (!isnan(hum)) { 
    char p[100]; snprintf(p, sizeof(p), "{\"value\":%.1f,\"unit\":\"%%\",\"ts\":%lu}", hum, ts); 
    mqttClient.publish(TOPIC_HUM, p); 
    Serial.print("[MQTT] Publié sur "); Serial.print(TOPIC_HUM); Serial.print(": "); Serial.println(p);
  } else {
    Serial.println("[Telemetry] Humidité NaN");
  }
  if (!isnan(pres)) { 
    char p[100]; snprintf(p, sizeof(p), "{\"value\":%.2f,\"unit\":\"hPa\",\"ts\":%lu}", pres, ts); 
    mqttClient.publish(TOPIC_PRESSURE, p); 
    Serial.print("[MQTT] Publié sur "); Serial.print(TOPIC_PRESSURE); Serial.print(": "); Serial.println(p);
  } else {
    Serial.println("[Telemetry] Pression NaN");
  }
  
  char payloadVib[100]; snprintf(payloadVib, sizeof(payloadVib), "{\"x\":%.2f,\"y\":%.2f,\"z\":%.2f,\"ts\":%lu}", 0.02, -0.01, 9.81, ts); 
  mqttClient.publish(TOPIC_VIBRATION, payloadVib);
  Serial.print("[MQTT] Publié sur "); Serial.print(TOPIC_VIBRATION); Serial.print(": "); Serial.println(payloadVib);
  
  int pot1 = analogRead(PIN_POT1); float voltage = map(pot1, 0, 4095, 200, 260);
  char pV[100]; snprintf(pV, sizeof(pV), "{\"value\":%.1f,\"unit\":\"V\",\"ts\":%lu}", voltage, ts); 
  mqttClient.publish(TOPIC_TENSION_LINE, pV);
  Serial.print("[MQTT] Publié sur "); Serial.print(TOPIC_TENSION_LINE); Serial.print(": "); Serial.println(pV);
  
  int pot2 = analogRead(PIN_POT2); float current = map(pot2, 0, 4095, 0, 100);
  char pC[100]; snprintf(pC, sizeof(pC), "{\"value\":%.1f,\"unit\":\"A\",\"ts\":%lu}", current, ts); 
  mqttClient.publish(TOPIC_CURRENT_LINE, pC);
  Serial.print("[MQTT] Publié sur "); Serial.print(TOPIC_CURRENT_LINE); Serial.print(": "); Serial.println(pC);

  long uptime = millis() / 1000; int rssi = WiFi.RSSI();
  char statusPayload[200]; snprintf(statusPayload, sizeof(statusPayload), "{\"uptime\":%lu,\"rssi\":%d,\"link\":\"lora\",\"battery_v\":%.2f,\"ts\":%lu}", uptime, rssi, 3.92, ts);
  mqttClient.publish(TOPIC_STATUS, statusPayload, true);
  Serial.print("[MQTT] Publié sur "); Serial.print(TOPIC_STATUS); Serial.print(": "); Serial.println(statusPayload);
}

void publishAlarm(const char* type, const char* level, float value, const char* unit) {
  unsigned long ts = millis() / 1000;
  char topic[60];
  snprintf(topic, sizeof(topic), "%s/alarm/%s", TOPIC_BASE, type);
  char payload[150];
  snprintf(payload, sizeof(payload), "{\"level\":\"%s\", \"value\":%.2f, \"unit\":\"%s\", \"ts\":%lu}", level, value, unit, ts);
  mqttClient.publish(topic, payload);
>>>>>>> aa343ff121737cf536f2414c120e7c0f7d365f0b
}

void checkIntrusion() {
  int pirState = digitalRead(PIN_PIR);
  if (pirState == HIGH && lastPIRState == LOW) {
    intrusionActive = true;
<<<<<<< HEAD
    digitalWrite(PIN_LED2, HIGH);
    Serial.println("[PIR] INTRUSION DETECTEE!");
    unsigned long ts = millis() / 1000;
    char payload[100];
    snprintf(payload, sizeof(payload), "{\"level\":\"alert\",\"detected\":true,\"ts\":%lu}", ts);
    mqttClient.publish(TOPIC_INTRUSION, payload);
  } else if (pirState == LOW && lastPIRState == HIGH) {
    intrusionActive = false;
    digitalWrite(PIN_LED2, LOW);
    Serial.println("[PIR] Intrusion terminee");
    unsigned long ts = millis() / 1000;
    char payload[100];
    snprintf(payload, sizeof(payload), "{\"level\":\"normal\",\"detected\":false,\"ts\":%lu}", ts);
    mqttClient.publish(TOPIC_INTRUSION, payload);
=======
    publishAlarm("motion", "warning", 1.0, "bool");
  } else if (pirState == LOW && lastPIRState == HIGH) {
    intrusionActive = false;
>>>>>>> aa343ff121737cf536f2414c120e7c0f7d365f0b
  }
  lastPIRState = pirState;
}

bool reconnectMQTT() {
<<<<<<< HEAD
  Serial.println("[MQTT] Connexion au broker...");
  if (mqttClient.connect(MQTT_CLIENT_ID, MQTT_USER, MQTT_PASS)) {
    Serial.println("[MQTT] Connecte!");
    mqttClient.subscribe(TOPIC_LED1);
    mqttClient.subscribe(TOPIC_LED2);
    return true;
  }
  Serial.print("[MQTT] Echec, code: ");
  Serial.println(mqttClient.state());
=======
  if (mqttClient.connect(MQTT_CLIENT_ID, MQTT_USER, MQTT_PASS)) {
    mqttClient.subscribe(TOPIC_LED1); mqttClient.subscribe(TOPIC_LED2);
    return true;
  }
>>>>>>> aa343ff121737cf536f2414c120e7c0f7d365f0b
  return false;
}

void setup() {
<<<<<<< HEAD
  Serial.begin(115200);
  delay(2000);
  Serial.println();
  Serial.println("=== Poste Electrique de Transformation Urbain ===");
  Serial.println();
  pinMode(PIN_PIR, INPUT);
  pinMode(PIN_POT1, INPUT);
  pinMode(PIN_POT2, INPUT);
  pinMode(PIN_LED1, OUTPUT);
  pinMode(PIN_LED2, OUTPUT);
  digitalWrite(PIN_LED1, LOW);
  digitalWrite(PIN_LED2, LOW);
=======
  Serial.begin(115200); delay(2000);
  pinMode(PIN_PIR, INPUT); pinMode(PIN_POT1, INPUT); pinMode(PIN_POT2, INPUT);
  pinMode(PIN_LED1, OUTPUT);   pinMode(PIN_LED2, OUTPUT);
>>>>>>> aa343ff121737cf536f2414c120e7c0f7d365f0b
  Wire.begin(I2C_SDI, I2C_SCK);
  if (!bme.begin(0x76) && !bme.begin(0x77)) {
    Serial.println("ERREUR: BME280 non detecte!");
  } else {
    Serial.println("BME280 detecte avec succes");
  }
<<<<<<< HEAD
  Serial.print("Connexion WiFi a ");
  Serial.println(WIFI_SSID);
  WiFi.disconnect(true);
  WiFi.mode(WIFI_STA);
  #ifdef WIFI_SECURITY_WPA2_ENTERPRISE
    Serial.println("Using WPA2-Enterprise connection.");
=======
  WiFi.mode(WIFI_STA);

  #ifdef WIFI_SECURITY_WPA2_ENTERPRISE
>>>>>>> aa343ff121737cf536f2414c120e7c0f7d365f0b
    esp_wifi_sta_wpa2_ent_set_identity((uint8_t *)EAP_IDENTITY, strlen(EAP_IDENTITY));
    esp_wifi_sta_wpa2_ent_set_username((uint8_t *)EAP_USERNAME, strlen(EAP_USERNAME));
    esp_wifi_sta_wpa2_ent_set_password((uint8_t *)EAP_PASSWORD, strlen(EAP_PASSWORD));
    esp_wifi_sta_wpa2_ent_enable();
    WiFi.begin(WIFI_SSID);
<<<<<<< HEAD
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
  }
  Serial.println("\nWiFi connecte!");
  Serial.print("Adresse IP: ");
  Serial.println(WiFi.localIP());
  snprintf(TOPIC_TEMP, sizeof(TOPIC_TEMP), "%s/telemetry/temperature", TOPIC_BASE);
  snprintf(TOPIC_HUM, sizeof(TOPIC_HUM), "%s/telemetry/humidity", TOPIC_BASE);
  snprintf(TOPIC_TENSION, sizeof(TOPIC_TENSION), "%s/telemetry/voltage", TOPIC_BASE);
  snprintf(TOPIC_COURANT, sizeof(TOPIC_COURANT), "%s/telemetry/current", TOPIC_BASE);
  snprintf(TOPIC_STATUS, sizeof(TOPIC_STATUS), "%s/status", TOPIC_BASE);
  snprintf(TOPIC_INTRUSION, sizeof(TOPIC_INTRUSION), "%s/alarm/intrusion", TOPIC_BASE);
  snprintf(TOPIC_LED1, sizeof(TOPIC_LED1), "%s/actuators/led_1", TOPIC_BASE);
  snprintf(TOPIC_LED2, sizeof(TOPIC_LED2), "%s/actuators/led_2", TOPIC_BASE);
  Serial.print("[MQTT] Device ID: ");
  Serial.println(MQTT_CLIENT_ID);
  wifiClient.setInsecure();
  mqttClient.setServer(MQTT_HOST, MQTT_WSS_PORT);
  mqttClient.setCallback(mqttCallback);
  mqttClient.setKeepAlive(60);
  if (!wsClient.connect(MQTT_HOST, MQTT_WSS_PORT)) {
    Serial.println("[ERREUR] Impossible de se connecter via WebSocket");
    while (true) {
      digitalWrite(PIN_LED1, !digitalRead(PIN_LED1));
      delay(1000);
    }
  }
  if (!reconnectMQTT()) {
    Serial.println("[ERREUR] Impossible de se connecter au broker MQTT");
    while (true) {
      digitalWrite(PIN_LED1, !digitalRead(PIN_LED1));
      delay(1000);
    }
  }
  digitalWrite(PIN_LED1, HIGH);
  Serial.println();
  Serial.println("=== Systeme pret ===");
=======
  #else
    WiFi.begin(WIFI_SSID, "votre_mot_de_passe");
  #endif
  while (WiFi.status() != WL_CONNECTED) delay(500);
  snprintf(TOPIC_TEMP, 60, "%s/telemetry/temperature", TOPIC_BASE);
  snprintf(TOPIC_HUM, 60, "%s/telemetry/humidity", TOPIC_BASE);
  snprintf(TOPIC_PRESSURE, 60, "%s/telemetry/pressure", TOPIC_BASE);
  snprintf(TOPIC_VIBRATION, 60, "%s/telemetry/vibration", TOPIC_BASE);
  snprintf(TOPIC_TENSION_LINE, 60, "%s/telemetry/voltage_line", TOPIC_BASE);
  snprintf(TOPIC_CURRENT_LINE, 60, "%s/telemetry/current_line", TOPIC_BASE);
  snprintf(TOPIC_STATUS, 60, "%s/status", TOPIC_BASE);
  snprintf(TOPIC_STATUS_LLM, 60, "%s/status/llm", TOPIC_BASE);
  snprintf(TOPIC_INTRUSION, 60, "%s/alarm/intrusion", TOPIC_BASE);
  snprintf(TOPIC_TILT, 60, "%s/alarm/tilt", TOPIC_BASE);
  snprintf(TOPIC_LED1, 60, "%s/actuators/led_1", TOPIC_BASE);
  snprintf(TOPIC_LED2, 60, "%s/actuators/led_2", TOPIC_BASE);
  wifiClient.setInsecure(); mqttClient.setServer(MQTT_HOST, MQTT_WSS_PORT);
  mqttClient.setCallback(mqttCallback); reconnectMQTT();
>>>>>>> aa343ff121737cf536f2414c120e7c0f7d365f0b
}

void loop() {
  if (WiFi.status() != WL_CONNECTED) {
<<<<<<< HEAD
    Serial.println("[WiFi] Connexion perdue, reconnexion...");
    while (WiFi.status() != WL_CONNECTED) {
      delay(500);
      Serial.print(".");
    }
    Serial.println("\nWiFi reconnecte!");
    if (wsClient.connect(MQTT_HOST, MQTT_WSS_PORT)) reconnectMQTT();
  }
  if (!mqttClient.connected()) reconnectMQTT();
  mqttClient.loop();
  checkIntrusion();
  unsigned long now = millis();
  if (now - lastTelemetry >= TELEMETRY_INTERVAL) {
    lastTelemetry = now;
    publishTelemetry();
=======
    #ifdef WIFI_SECURITY_WPA2_ENTERPRISE
      WiFi.begin(WIFI_SSID);
    #else
      WiFi.begin(WIFI_SSID, "votre_mot_de_passe");
    #endif
    while(WiFi.status() != WL_CONNECTED) delay(500);
  }
  if (!mqttClient.connected()) reconnectMQTT();
  mqttClient.loop(); checkIntrusion();
  
  // Contrôle local direct des LEDs (priorité locale)
  int pot1 = analogRead(PIN_POT1); float voltage = map(pot1, 0, 4095, 200, 260);
  int pot2 = analogRead(PIN_POT2); float current = map(pot2, 0, 4095, 0, 100);
  
  static bool lastLed1State = -1;
  static bool lastLed2State = -1;

  bool newLed1State = (voltage > 250.0);
  bool newLed2State = (current > 80.0);

  if (newLed1State != lastLed1State) {
    digitalWrite(PIN_LED1, newLed1State ? HIGH : LOW);
    publishAlarm("voltage", newLed1State ? "warning" : "info", voltage, "V");
    // Publication actuator
    mqttClient.publish(TOPIC_LED1, newLed1State ? "{\"state\": \"on\"}" : "{\"state\": \"off\"}");
    Serial.print("[MQTT] Publié sur "); Serial.print(TOPIC_LED1); Serial.print(": "); Serial.println(newLed1State ? "{\"state\": \"on\"}" : "{\"state\": \"off\"}");
    Serial.print("[Actionneur] LED1 (Tension) mise à: "); Serial.println(newLed1State ? "on" : "off");
    lastLed1State = newLed1State;
  }
  
  if (newLed2State != lastLed2State) {
    digitalWrite(PIN_LED2, newLed2State ? HIGH : LOW);
    publishAlarm("current", newLed2State ? "warning" : "info", current, "A");
    // Publication actuator
    mqttClient.publish(TOPIC_LED2, newLed2State ? "{\"state\": \"on\"}" : "{\"state\": \"off\"}");
    Serial.print("[MQTT] Publié sur "); Serial.print(TOPIC_LED2); Serial.print(": "); Serial.println(newLed2State ? "{\"state\": \"on\"}" : "{\"state\": \"off\"}");
    Serial.print("[Actionneur] LED2 (Courant) mise à: "); Serial.println(newLed2State ? "on" : "off");
    lastLed2State = newLed2State;
  }

  unsigned long now = millis();
  if (now - lastTelemetry >= TELEMETRY_INTERVAL) { lastTelemetry = now; publishTelemetry(); }
  if (now - lastLLMCall >= LLM_INTERVAL) {
    lastLLMCall = now;
    int pot1 = analogRead(PIN_POT1); float voltage = map(pot1, 0, 4095, 200, 260);
    int pot2 = analogRead(PIN_POT2); float current = map(pot2, 0, 4095, 0, 100);
    
    String prompt = "Données: Temp=" + String(bme.readTemperature()) + "C, Tension=" + String(voltage) + "V, Courant=" + String(current) + "A, Intrusion=" + (intrusionActive ? "Oui" : "Non");
    String responseJson = appelLLM(prompt);
    
    // Parse LLM JSON
    JsonDocument doc;
    deserializeJson(doc, responseJson);
    
    // Update LEDs
    if (doc.containsKey("led1")) {
      String l1 = doc["led1"];
      digitalWrite(PIN_LED1, (l1 == "on") ? HIGH : LOW);
      Serial.print("[Actionneur] LED1 mise à: "); Serial.println(l1);
    }
    if (doc.containsKey("led2")) {
      String l2 = doc["led2"];
      digitalWrite(PIN_LED2, (l2 == "on") ? HIGH : LOW);
      Serial.print("[Actionneur] LED2 mise à: "); Serial.println(l2);
    }
    
    // Publish
    mqttClient.publish(TOPIC_STATUS_LLM, responseJson.c_str(), true);
    Serial.println("[LLM] Resumé JSON publié: " + responseJson);
>>>>>>> aa343ff121737cf536f2414c120e7c0f7d365f0b
  }
  delay(10);
}
