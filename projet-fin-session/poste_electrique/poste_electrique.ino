#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <PubSubClient.h>
#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BME280.h>
#include <esp_wpa2.h>
#include <mbedtls/base64.h>
#include "auth.h"

#define PIN_PIR 13
#define PIN_POT1 34
#define PIN_POT2 35
#define PIN_LED1 14
#define PIN_LED2 15
#define I2C_SCK 22
#define I2C_SDI 21

Adafruit_BME280 bme;
const char* MQTT_HOST = MQTT_BROKER;
const int   MQTT_WSS_PORT = 443;
const char* MQTT_PATH = "/";
const char* TOPIC_BASE = "hydro-limoilou/poste-05";

char TOPIC_TEMP[60];
char TOPIC_HUM[60];
char TOPIC_STATUS[60];
char TOPIC_INTRUSION[60];
char TOPIC_TENSION[60];
char TOPIC_COURANT[60];
char TOPIC_LED1[60];
char TOPIC_LED2[60];

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
      Serial.println("[WSS] Serveur a ferme la connexion");
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

  size_t write(uint8_t b) { return write(&b, 1); }
  size_t write(const uint8_t *buf, size_t size) {
    if (!_wsConnected) return 0;
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

  void flush() { _sslClient->flush(); }
  void stop() { _wsConnected = false; _sslClient->stop(); }
  uint8_t connected() { return _wsConnected && _sslClient->connected(); }
  operator bool() { return _wsConnected; }
};

WiFiClientSecure wifiClient;
WebSocketClient wsClient(&wifiClient);
PubSubClient mqttClient(wsClient);

unsigned long lastTelemetry = 0;
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
  }
}

void publishTelemetry() {
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
}

void checkIntrusion() {
  int pirState = digitalRead(PIN_PIR);
  if (pirState == HIGH && lastPIRState == LOW) {
    intrusionActive = true;
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
  }
  lastPIRState = pirState;
}

bool reconnectMQTT() {
  Serial.println("[MQTT] Connexion au broker...");
  if (mqttClient.connect(MQTT_CLIENT_ID, MQTT_USER, MQTT_PASS)) {
    Serial.println("[MQTT] Connecte!");
    mqttClient.subscribe(TOPIC_LED1);
    mqttClient.subscribe(TOPIC_LED2);
    return true;
  }
  Serial.print("[MQTT] Echec, code: ");
  Serial.println(mqttClient.state());
  return false;
}

void setup() {
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
  Wire.begin(I2C_SDI, I2C_SCK);
  if (!bme.begin(0x76) && !bme.begin(0x77)) {
    Serial.println("ERREUR: BME280 non detecte!");
  } else {
    Serial.println("BME280 detecte avec succes");
  }
  Serial.print("Connexion WiFi a ");
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
}

void loop() {
  if (WiFi.status() != WL_CONNECTED) {
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
  }
  delay(10);
}
