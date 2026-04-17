// LilyGO T-SIM A7670G - Version Projet Complet
// Lecture capteurs, LEDs, MPU6050, MQTT via LTE

#define TINY_GSM_MODEM_SIM7600
#define TINY_GSM_RX_BUFFER 1024

#include <TinyGsmClient.h>
#include <PubSubClient.h>
#include <ESP_SSLClient.h>
#include <mbedtls/base64.h>
#include <Wire.h>
#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>

#include "auth.h"

// ====== CONFIG MODEM A7670G ======
#define MODEM_TX 26
#define MODEM_RX 27
#define MODEM_PWRKEY 4

// ====== CONFIG MQTT/WSS ======
const char* MQTT_HOST = MQTT_BROKER;
const int MQTT_WSS_PORT = 443;
const char* MQTT_PATH = "/";

// Topic racine
const char* STUDENT_PREFIX = "etudiant/charlesboris-charlesfeugang";

// Configuration des broches (Pins)
const int LED_PINS[] = {12, 13, 14, 15};
const int POT_PINS[] = {32, 33}; // Remplacés pour éviter le conflit modem
const int BUTTON_PIN = 34;

// Variables Globales
unsigned long PUBLISH_INTERVAL_MS = 5000;
unsigned long lastPublishTime = 0;
unsigned long lastGprsCheckTime = 0;
const unsigned long GPRS_CHECK_INTERVAL_MS = 30000;

bool buttonState = false;
int pot1Value = 0;
int pot2Value = 0;
float accelX = 0, accelY = 0, accelZ = 0;
float roll = 0, pitch = 0;

Adafruit_MPU6050 mpu;

// Topics
char topicSensorsButtons[100];
char topicSensorsPots[100];
char topicSensorsAccel[100];
char topicStatus[100];
char topicActuatorsLed[4][100];
char topicConfig[100];

HardwareSerial SerialAT(1);

// ============================================================================
// CLASSE WRAPPER WEBSOCKET POUR PUBSUBCLIENT (Conservation code original)
// ============================================================================
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
    }
    else if (opcode == 0x08) {
      Serial.println("[WSS] Serveur a ferme la connexion");
      _wsConnected = false;
      return false;
    }
    else if (opcode == 0x09) {
      uint8_t pong[2] = {0x8A, 0x00};
      _sslClient->write(pong, 2);
      return false;
    }
    return false;
  }

public:
  WebSocketClient(ESP_SSLClient* sslClient) {
    _sslClient = sslClient;
    _wsConnected = false;
    _rxBufferLen = 0;
    _rxBufferPos = 0;
  }

  int connect(IPAddress ip, uint16_t port) { return 0; }
  int connect(const char *host, uint16_t port) {
    if (!_sslClient->connect(host, port)) return 0;
    String wsKey = generateWebSocketKey();
    _sslClient->print("GET "); _sslClient->print(MQTT_PATH);
    _sslClient->print(" HTTP/1.1\r\nHost: "); _sslClient->print(host);
    _sslClient->print("\r\nUpgrade: websocket\r\nConnection: Upgrade\r\nSec-WebSocket-Key: ");
    _sslClient->print(wsKey);
    _sslClient->print("\r\nSec-WebSocket-Protocol: mqtt\r\nSec-WebSocket-Version: 13\r\n\r\n");

    unsigned long timeout = millis();
    while (!_sslClient->available() && millis() - timeout < 5000) delay(10);
    if (!_sslClient->available()) return 0;

    String response = "";
    while (_sslClient->available()) {
      response += (char)_sslClient->read();
      if (response.endsWith("\r\n\r\n")) break;
    }

    if (response.indexOf("101") > 0 && response.indexOf("Switching Protocols") > 0) {
      _wsConnected = true;
      return 1;
    }
    return 0;
  }

  size_t write(uint8_t b) { return write(&b, 1); }
  size_t write(const uint8_t *buf, size_t size) {
    if (!_wsConnected) return 0;
    uint8_t header[14];
    int headerLen = 2;
    header[0] = 0x82;

    if (size < 126) { header[1] = 0x80 | size; }
    else if (size < 65536) {
      header[1] = 0x80 | 126; header[2] = (size >> 8) & 0xFF; header[3] = size & 0xFF; headerLen = 4;
    } else {
      header[1] = 0x80 | 127; for(int i=0; i<8; i++) header[2+i] = 0;
      header[6] = (size >> 24) & 0xFF; header[7] = (size >> 16) & 0xFF;
      header[8] = (size >> 8) & 0xFF; header[9] = size & 0xFF; headerLen = 10;
    }

    uint8_t mask[4];
    for(int i = 0; i < 4; i++) { mask[i] = random(0, 256); header[headerLen + i] = mask[i]; }
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
    if (_sslClient->available() && readWebSocketFrame()) return _rxBufferLen - _rxBufferPos;
    return 0;
  }

  int read() {
    if (_rxBufferPos < _rxBufferLen) return _rxBuffer[_rxBufferPos++];
    if (_sslClient->available() && readWebSocketFrame() && _rxBufferPos < _rxBufferLen) return _rxBuffer[_rxBufferPos++];
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

  int peek() { return (_rxBufferPos < _rxBufferLen) ? _rxBuffer[_rxBufferPos] : -1; }
  void flush() { _sslClient->flush(); }
  void stop() { _wsConnected = false; _sslClient->stop(); }
  uint8_t connected() { return _wsConnected && _sslClient->connected(); }
  operator bool() { return _wsConnected; }
};

// Clients
TinyGsm modem(SerialAT);
TinyGsmClient gsmClient(modem, 0);
ESP_SSLClient sslClient;
WebSocketClient wsClient(&sslClient);
PubSubClient mqttClient(wsClient);

// ============================================================================
// CALLBACK MQTT
// ============================================================================
void mqttCallback(char* topic, byte* payload, unsigned int length) {
    String msg = "";
    for (unsigned int i = 0; i < length; i++) {
        msg += (char)payload[i];
    }
    Serial.print("[MQTT] <- ");
    Serial.print(topic);
    Serial.print(" = ");
    Serial.println(msg);

    // Contrôle LEDs
    bool stateOn = (msg.indexOf("\"on\"") > 0 || msg.indexOf("\"ON\"") > 0);
    
    for (int i = 0; i < 4; i++) {
        if (strcmp(topic, topicActuatorsLed[i]) == 0) {
            digitalWrite(LED_PINS[i], stateOn ? HIGH : LOW);
            Serial.print("[INFO] LED"); Serial.print(i + 1);
            Serial.println(stateOn ? " -> ON" : " -> OFF");
            return;
        }
    }
    
    // Configuration
    if (strcmp(topic, topicConfig) == 0) {
        int idx = msg.indexOf("\"interval\"");
        if (idx > 0) {
            int colonIdx = msg.indexOf(":", idx);
            if (colonIdx > 0) {
                int newVal = msg.substring(colonIdx + 1).toInt();
                if (newVal > 0) {
                    PUBLISH_INTERVAL_MS = newVal;
                    Serial.print("[INFO] Nouvel intervalle de publication: ");
                    Serial.println(PUBLISH_INTERVAL_MS);
                }
            }
        }
    }
}

// ============================================================================
// INITIALISATIONS
// ============================================================================
void initSerial() {
    Serial.begin(115200);
    delay(2000);
    Serial.println("\n=== LilyGo T-SIM A7670G - Projet IoT MQTT ===");
}

void initGPIO() {
    Serial.println("[INFO] Initialisation GPIO");
    for (int i = 0; i < 4; i++) {
        pinMode(LED_PINS[i], OUTPUT);
        digitalWrite(LED_PINS[i], LOW);
    }
    pinMode(BUTTON_PIN, INPUT_PULLUP);
}

void initI2C() {
    Serial.println("[INFO] Initialisation I2C (MPU6050)");
    if (!mpu.begin()) {
        Serial.println("[ERROR] Accéléromètre non détecté !");
    } else {
        Serial.println("[INFO] Accéléromètre prêt.");
        mpu.setAccelerometerRange(MPU6050_RANGE_8_G);
        mpu.setGyroRange(MPU6050_RANGE_500_DEG);
        mpu.setFilterBandwidth(MPU6050_BAND_21_HZ);
    }
}

void initLTE() {
    Serial.println("[INFO] Allumage du modem...");
    pinMode(MODEM_PWRKEY, OUTPUT);
    digitalWrite(MODEM_PWRKEY, HIGH);
    delay(100);
    digitalWrite(MODEM_PWRKEY, LOW);
    delay(1000);
    digitalWrite(MODEM_PWRKEY, HIGH);
    delay(3000);
    
    SerialAT.begin(115200, SERIAL_8N1, MODEM_RX, MODEM_TX);
    delay(3000);

    if (!modem.restart()) {
        Serial.println("[ERROR] Echec redemarrage modem");
        return;
    }
    
    Serial.println("[INFO] Configuration APN...");
    modem.sendAT("+CGDCONT=1,\"IP\",\"", APN, "\"");
    modem.waitResponse();
    
    Serial.println("[INFO] Connexion reseau...");
    if (!modem.waitForNetwork(60000L)) {
        Serial.println("[ERROR] Echec reseau");
        return;
    }
    
    Serial.println("[INFO] Connexion GPRS...");
    if (!modem.gprsConnect(APN, APN_USER, APN_PASS)) {
        Serial.println("[ERROR] Echec GPRS");
        return;
    }
    
    Serial.print("[INFO] IP: ");
    Serial.println(modem.localIP());
}

void connectMQTT() {
    // Configuration des topics avec le prefix étudiant
    snprintf(topicSensorsButtons, sizeof(topicSensorsButtons), "%s/sensors/buttons", STUDENT_PREFIX);
    snprintf(topicSensorsPots, sizeof(topicSensorsPots), "%s/sensors/pots", STUDENT_PREFIX);
    snprintf(topicSensorsAccel, sizeof(topicSensorsAccel), "%s/sensors/accel", STUDENT_PREFIX);
    snprintf(topicStatus, sizeof(topicStatus), "%s/status", STUDENT_PREFIX);
    
    for (int i = 0; i < 4; i++) {
        snprintf(topicActuatorsLed[i], sizeof(topicActuatorsLed[i]), "%s/actuators/led%d", STUDENT_PREFIX, i + 1);
    }
    snprintf(topicConfig, sizeof(topicConfig), "%s/config", STUDENT_PREFIX);

    // Setup SSL & MQTT
    sslClient.setClient(&gsmClient);
    sslClient.setInsecure();
    sslClient.setBufferSizes(2048, 1024);
    
    mqttClient.setServer(MQTT_HOST, MQTT_WSS_PORT);
    mqttClient.setCallback(mqttCallback);
    mqttClient.setKeepAlive(60);

    Serial.println("[INFO] Connexion WSS...");
    if (!wsClient.connect(MQTT_HOST, MQTT_WSS_PORT)) {
        Serial.println("[ERROR] Echec WSS");
        return;
    }

    Serial.println("[INFO] Connexion MQTT...");
    if (mqttClient.connect(MQTT_CLIENT_ID, MQTT_USER, MQTT_PASS)) {
        Serial.println("[INFO] Connecte MQTT!");
        for (int i = 0; i < 4; i++) {
            mqttClient.subscribe(topicActuatorsLed[i]);
        }
        mqttClient.subscribe(topicConfig);
    } else {
        Serial.print("[ERROR] Echec MQTT, code: ");
        Serial.println(mqttClient.state());
    }
}

// ============================================================================
// BOUCLE PRINCIPALE
// ============================================================================
void maintainMQTT() {
    unsigned long now = millis();
    if (now - lastGprsCheckTime > GPRS_CHECK_INTERVAL_MS) {
        lastGprsCheckTime = now;
        if (!modem.isGprsConnected()) {
            Serial.println("[ERROR] GPRS perdu, reconnexion...");
            modem.gprsConnect(APN, APN_USER, APN_PASS);
        }
    }

    if (!mqttClient.connected()) {
        Serial.println("[INFO] Reconnexion MQTT...");
        if (!wsClient.connected()) {
            wsClient.connect(MQTT_HOST, MQTT_WSS_PORT);
        }
        if (wsClient.connected()) {
            if (mqttClient.connect(MQTT_CLIENT_ID, MQTT_USER, MQTT_PASS)) {
                Serial.println("[INFO] Reconnecte MQTT!");
                for (int i = 0; i < 4; i++) mqttClient.subscribe(topicActuatorsLed[i]);
                mqttClient.subscribe(topicConfig);
            }
        }
    }
    
    mqttClient.loop();
}

void readSensors() {
    // Bouton
    buttonState = (digitalRead(BUTTON_PIN) == LOW);
    
    // Potentiometres
    pot1Value = analogRead(POT_PINS[0]);
    pot2Value = analogRead(POT_PINS[1]);
    
    // Accelerometre
    sensors_event_t a, g, temp;
    mpu.getEvent(&a, &g, &temp);
    
    accelX = a.acceleration.x;
    accelY = a.acceleration.y;
    accelZ = a.acceleration.z;
    
    roll = atan2(accelY, accelZ) * 180.0 / PI;
    pitch = atan2(-accelX, sqrt(accelY * accelY + accelZ * accelZ)) * 180.0 / PI;
}

void publishData() {
    unsigned long now = millis();
    if (now - lastPublishTime >= PUBLISH_INTERVAL_MS) {
        lastPublishTime = now;

        if (!mqttClient.connected()) return;

        char payload[200];
        
        // Boutons
        snprintf(payload, sizeof(payload), "{\"btn1\": %s}", buttonState ? "true" : "false");
        mqttClient.publish(topicSensorsButtons, payload);
        
        // Potentiomètres
        snprintf(payload, sizeof(payload), "{\"pot1\": %d, \"pot2\": %d}", pot1Value, pot2Value);
        mqttClient.publish(topicSensorsPots, payload);
        
        // Accéléromètre
        snprintf(payload, sizeof(payload), "{\"x\": %.2f, \"y\": %.2f, \"z\": %.2f, \"roll\": %.2f, \"pitch\": %.2f}", 
                 accelX, accelY, accelZ, roll, pitch);
        mqttClient.publish(topicSensorsAccel, payload);
        
        // Statut
        int rssi = modem.getSignalQuality();
        snprintf(payload, sizeof(payload), "{\"uptime\": %lu, \"rssi\": %d}", now / 1000, rssi);
        mqttClient.publish(topicStatus, payload);
        
        Serial.println("[INFO] Donnees publiees.");
    }
}

void setup() {
    initSerial();
    initGPIO();
    initI2C();
    initLTE();
    connectMQTT();
}

void loop() {
    maintainMQTT();
    readSensors();
    publishData();
    delay(10); // Petit délai pour laisser respirer le processeur
}