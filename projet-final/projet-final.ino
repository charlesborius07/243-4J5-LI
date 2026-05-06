// LilyGO T-SIM A7670G - Application finale: Capteurs BME280 + PIR et MQTT via LTE
#define TINY_GSM_MODEM_SIM7600
#define TINY_GSM_RX_BUFFER 1024

#include <TinyGsmClient.h>
#include <PubSubClient.h>
#include <ESP_SSLClient.h>
#include <mbedtls/base64.h>
#include "auth.h"
#include <Wire.h>
#include <Adafruit_BME280.h>

// ====== CONFIG MODEM A7670G ======
#define MODEM_TX 26
#define MODEM_RX 27
#define MODEM_PWRKEY 4
#define MODEM_DTR 12
#define MODEM_RI 13
#define MODEM_FLIGHT 25
#define MODEM_STATUS 0

// ====== CONFIG CAPTEURS ======
#define PIN_PIR 13
#define PIN_POT1 34
#define PIN_POT2 35
#define PIN_LED1 14
#define PIN_LED2 15
#define I2C_SCK 22
#define I2C_SDI 21

Adafruit_BME280 bme;

// ====== CONFIG MQTT/WSS ======
const char* MQTT_HOST = MQTT_BROKER;
const int   MQTT_WSS_PORT = 443;
const char* MQTT_PATH = "/";

// Topics
#define TOPIC_ROOT "hydro-limoilou/poste-05/"
const char* TOPIC_STATUS = TOPIC_ROOT "status";

// ====== CLASSE WRAPPER WEBSOCKET (Source: firmware.ino) ======
class WebSocketClient : public Client {
private:
  ESP_SSLClient* _sslClient;
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
    } else if (opcode == 0x08) { _wsConnected = false; return false; }
    else if (opcode == 0x09) { uint8_t pong[2] = {0x8A, 0x00}; _sslClient->write(pong, 2); return false; }
    return false;
  }
public:
  WebSocketClient(ESP_SSLClient* sslClient) : _sslClient(sslClient), _wsConnected(false), _rxBufferLen(0), _rxBufferPos(0) {}
  int connect(IPAddress ip, uint16_t port) { return 0; }
  int connect(const char *host, uint16_t port) {
    if (!_sslClient->connect(host, port)) return 0;
    String wsKey = generateWebSocketKey();
    _sslClient->print("GET " + String(MQTT_PATH) + " HTTP/1.1\r\nHost: " + String(host) + "\r\nUpgrade: websocket\r\nConnection: Upgrade\r\nSec-WebSocket-Key: " + wsKey + "\r\nSec-WebSocket-Protocol: mqtt\r\nSec-WebSocket-Version: 13\r\n\r\n");
    unsigned long timeout = millis();
    while (!_sslClient->available() && millis() - timeout < 5000) delay(10);
    if (!_sslClient->available()) return 0;
    String response = "";
    while (_sslClient->available()) { response += (char)_sslClient->read(); if (response.endsWith("\r\n\r\n")) break; }
    if (response.indexOf("101") > 0) { _wsConnected = true; return 1; }
    return 0;
  }
  size_t write(uint8_t b) { return write(&b, 1); }
  size_t write(const uint8_t *buf, size_t size) {
    if (!_wsConnected) return 0;
    uint8_t header[14]; int headerLen = 2; header[0] = 0x82;
    if (size < 126) header[1] = 0x80 | size;
    else if (size < 65536) { header[1] = 0x80 | 126; header[2] = (size >> 8) & 0xFF; header[3] = size & 0xFF; headerLen = 4; }
    else { header[1] = 0x80 | 127; for(int i=0; i<8; i++) header[2+i] = 0; header[6] = (size >> 24) & 0xFF; header[7] = (size >> 16) & 0xFF; header[8] = (size >> 8) & 0xFF; header[9] = size & 0xFF; headerLen = 10; }
    uint8_t mask[4]; for(int i=0; i<4; i++) { mask[i] = random(0, 256); header[headerLen + i] = mask[i]; }
    headerLen += 4;
    _sslClient->write(header, headerLen);
    for(size_t i=0; i<size; i++) { uint8_t mb = buf[i] ^ mask[i % 4]; _sslClient->write(&mb, 1); }
    return size;
  }
  int available() { if (_rxBufferPos < _rxBufferLen) return _rxBufferLen - _rxBufferPos; if (_sslClient->available() && readWebSocketFrame()) return _rxBufferLen - _rxBufferPos; return 0; }
  int read() { if (_rxBufferPos < _rxBufferLen) return _rxBuffer[_rxBufferPos++]; if (_sslClient->available() && readWebSocketFrame() && _rxBufferPos < _rxBufferLen) return _rxBuffer[_rxBufferPos++]; return -1; }
  int read(uint8_t *buf, size_t size) { size_t count = 0; while(count < size) { int c = read(); if(c < 0) break; buf[count++] = (uint8_t)c; } return count; }
  int peek() { if (_rxBufferPos < _rxBufferLen) return _rxBuffer[_rxBufferPos]; return -1; }
  void flush() { _sslClient->flush(); }
  void stop() { _wsConnected = false; _sslClient->stop(); }
  uint8_t connected() { return _wsConnected && _sslClient->connected(); }
  operator bool() { return _wsConnected; }
};

// ============================================================================
// CLIENTS ET MQTT
// ============================================================================
HardwareSerial SerialAT(1);
TinyGsm modem(SerialAT);
TinyGsmClient gsmClient(modem, 0);
ESP_SSLClient sslClient;
WebSocketClient wsClient(&sslClient);
PubSubClient mqttClient(wsClient);

unsigned long publishIntervalMs = 5000;
unsigned long lastPublishTime = 0;
int lastPirState = LOW;

// ============================================================================
// INITIALISATIONS
// ============================================================================
void initLTE() {
  pinMode(MODEM_PWRKEY, OUTPUT);
  digitalWrite(MODEM_PWRKEY, HIGH); delay(100); digitalWrite(MODEM_PWRKEY, LOW); delay(1000); digitalWrite(MODEM_PWRKEY, HIGH); delay(3000);
  SerialAT.begin(115200, SERIAL_8N1, MODEM_RX, MODEM_TX);
  if (!modem.restart()) return;
  modem.sendAT("+CGDCONT=1,\"IP\",\"", APN, "\""); modem.waitResponse();
  modem.waitForNetwork(60000L);
  modem.gprsConnect(APN, APN_USER, APN_PASS);
}

void mqttCallback(char* topic, byte* payload, unsigned int length) {
  String msg = ""; for (unsigned int i = 0; i < length; i++) msg += (char)payload[i];
  String t = String(topic);
  if (t.endsWith("actuators/led1")) digitalWrite(PIN_LED1, (msg.indexOf("on") >= 0) ? HIGH : LOW);
  else if (t.endsWith("actuators/led2")) digitalWrite(PIN_LED2, (msg.indexOf("on") >= 0) ? HIGH : LOW);
}

bool connectMQTT() {
  if (!wsClient.connected() && !wsClient.connect(MQTT_HOST, MQTT_WSS_PORT)) return false;
  if (mqttClient.connect(MQTT_CLIENT_ID, MQTT_USER, MQTT_PASS)) {
    mqttClient.subscribe("hydro-limoilou/poste-05/actuators/#");
    return true;
  }
  return false;
}

void setup() {
  Serial.begin(115200);
  pinMode(PIN_PIR, INPUT);
  pinMode(PIN_POT1, INPUT);
  pinMode(PIN_POT2, INPUT);
  pinMode(PIN_LED1, OUTPUT);
  pinMode(PIN_LED2, OUTPUT);
  Wire.begin(I2C_SDI, I2C_SCK);
  if (!bme.begin(0x76) && !bme.begin(0x77)) Serial.println("BME280 init failed!");
  
  sslClient.setClient(&gsmClient);
  sslClient.setInsecure();
  mqttClient.setServer(MQTT_HOST, MQTT_WSS_PORT);
  mqttClient.setCallback(mqttCallback);
  initLTE();
  connectMQTT();
}

void loop() {
  if (!mqttClient.connected()) connectMQTT();
  mqttClient.loop();

  unsigned long now = millis();
  if (now - lastPublishTime >= publishIntervalMs) {
    lastPublishTime = now;
    
    // Mesures
    float temp = bme.readTemperature();
    float hum = bme.readHumidity();
    float pres = bme.readPressure() / 100.0F;
    int pirState = digitalRead(PIN_PIR);
    
    // Publication
    mqttClient.publish("hydro-limoilou/poste-05/telemetry/temp", String(temp).c_str());
    mqttClient.publish("hydro-limoilou/poste-05/telemetry/hum", String(hum).c_str());
    mqttClient.publish("hydro-limoilou/poste-05/telemetry/pres", String(pres).c_str());
    mqttClient.publish("hydro-limoilou/poste-05/telemetry/pir", String(pirState).c_str());
    mqttClient.publish("hydro-limoilou/poste-05/telemetry/pot1", String(analogRead(PIN_POT1)).c_str());
    mqttClient.publish("hydro-limoilou/poste-05/telemetry/pot2", String(analogRead(PIN_POT2)).c_str());
    mqttClient.publish("hydro-limoilou/poste-05/status", "{\"status\": \"ok\"}");
    
    // Alarmes
    if (pirState == HIGH && lastPirState == LOW) 
      mqttClient.publish("hydro-limoilou/poste-05/alarm/pir", "{\"event\": \"movement\"}");
    lastPirState = pirState;
  }
}
