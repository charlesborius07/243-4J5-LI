// LilyGO T-SIM A7670G - Version LTE/Cellulaire avec MQTT via WebSocket SSL
// Utilise PubSubClient avec wrapper WebSocket pour simplifier le code MQTT

#define TINY_GSM_MODEM_SIM7600
#define TINY_GSM_RX_BUFFER 1024

#include <TinyGsmClient.h>
#include <PubSubClient.h>

// ESP_SSLClient configuration
#define ENABLE_DEBUG
#define ENABLE_ERROR_STRING
#define DEBUG_PORT Serial
#define SSLCLIENT_INSECURE_ONLY

#include <ESP_SSLClient.h>
#include <mbedtls/base64.h>

#include "auth.h"
#include <Wire.h>
#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>

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

// --- Configuration des broches (Pins) ---
const int LED_PINS[] = {12, 13, 14, 15};
const int NUM_LEDS = 4;
const int POT_PINS[] = {32, 33};
const int NUM_POTS = 2;
const int BUTTON1_PIN = 34;
const int BUTTON2_PIN = 35;

Adafruit_MPU6050 mpu;

// Topics
String TOPIC_PUB_BUTTONS;
String TOPIC_PUB_POTS;
String TOPIC_PUB_ACCEL;
String TOPIC_PUB_STATUS;

String TOPIC_SUB_LED1;
String TOPIC_SUB_LED2;
String TOPIC_SUB_LED3;
String TOPIC_SUB_LED4;
String TOPIC_SUB_CONFIG;

// Variables d'état
unsigned long publishIntervalMs = 5000;
unsigned long lastPublishTime = 0;
unsigned long lastGprsCheck = 0;
const unsigned long GPRS_CHECK_INTERVAL = 30000;

int lastButton1State = HIGH;
int lastButton2State = HIGH;

// Serial pour le modem
HardwareSerial SerialAT(1);

// ============================================================================
// CLASSE WRAPPER WEBSOCKET POUR PUBSUBCLIENT
// ============================================================================

class WebSocketClient : public Client {
private:
  ESP_SSLClient* _sslClient;
  bool _wsConnected;

  // Buffer pour les données reçues
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
  WebSocketClient(ESP_SSLClient* sslClient) {
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

// ============================================================================
// CLIENTS ET MQTT
// ============================================================================

TinyGsm modem(SerialAT);
TinyGsmClient gsmClient(modem, 0);
ESP_SSLClient sslClient;
WebSocketClient wsClient(&sslClient);
PubSubClient mqttClient(wsClient);

// ============================================================================
// INITIALISATIONS
// ============================================================================

void initSerial() {
  Serial.begin(115200);
  delay(2000);
  Serial.println("\n=== LilyGo T-SIM A7670G - MQTT via LTE + WebSocket SSL ===");
  
  // Configuration des topics avec la macro STUDENT_TOPIC_ROOT de auth.h
  String root = String(STUDENT_TOPIC_ROOT);
  TOPIC_PUB_BUTTONS = root + "sensors/buttons";
  TOPIC_PUB_POTS    = root + "sensors/pots";
  TOPIC_PUB_ACCEL   = root + "sensors/accel";
  TOPIC_PUB_STATUS  = root + "status";

  TOPIC_SUB_LED1    = root + "actuators/led1";
  TOPIC_SUB_LED2    = root + "actuators/led2";
  TOPIC_SUB_LED3    = root + "actuators/led3";
  TOPIC_SUB_LED4    = root + "actuators/led4";
  TOPIC_SUB_CONFIG  = root + "config";
}

void initGPIO() {
  for (int i = 0; i < NUM_LEDS; i++) {
    pinMode(LED_PINS[i], OUTPUT);
    digitalWrite(LED_PINS[i], LOW);
  }
  pinMode(BUTTON1_PIN, INPUT_PULLUP);
  pinMode(BUTTON2_PIN, INPUT_PULLUP);
  // Les potentiomètres sur 32 et 33 ne nécessitent pas de pinMode pour analogRead
  Serial.println("[INFO] GPIO initialisés");
}

void initI2C() {
  Serial.println("[INFO] Initialisation du MPU6050...");
  if (!mpu.begin()) {
    Serial.println("[ERROR] Accéléromètre non détecté !");
  } else {
    Serial.println("[INFO] Accéléromètre prêt.");
  }
}

void modemPowerOn() {
  Serial.println("[INFO] Allumage du modem...");
  pinMode(MODEM_PWRKEY, OUTPUT);
  digitalWrite(MODEM_PWRKEY, HIGH);
  delay(100);
  digitalWrite(MODEM_PWRKEY, LOW);
  delay(1000);
  digitalWrite(MODEM_PWRKEY, HIGH);
  delay(3000);
  Serial.println("[INFO] Modem allumé");
}

void initLTE() {
  modemPowerOn();
  Serial.println("[INFO] Initialisation modem...");
  SerialAT.begin(115200, SERIAL_8N1, MODEM_RX, MODEM_TX);
  delay(3000);

  if (!modem.restart()) {
    Serial.println("[ERROR] Échec du redémarrage du modem");
    return;
  }
  Serial.println("[INFO] Modem initialisé.");

  Serial.println("[INFO] Configuration de l'APN...");
  modem.sendAT("+CGDCONT=1,\"IP\",\"", APN, "\"");
  modem.waitResponse();
  
  Serial.println("[INFO] Connexion au réseau cellulaire...");
  if (!modem.waitForNetwork(60000L)) {
    Serial.println("[ERROR] Échec de connexion au réseau");
    return;
  }
  Serial.println("[INFO] Réseau connecté.");

  Serial.println("[INFO] Connexion GPRS...");
  if (!modem.gprsConnect(APN, APN_USER, APN_PASS)) {
    Serial.println("[ERROR] Échec de connexion GPRS");
    return;
  }
  Serial.println("[INFO] GPRS connecté avec succès.");
}

// ============================================================================
// MQTT ET GESTION
// ============================================================================

void handleLedCommand(int ledIndex, String msg) {
  if (ledIndex >= 0 && ledIndex < NUM_LEDS) {
    if (msg.indexOf("\"state\": \"on\"") >= 0 || msg.indexOf("\"state\":\"on\"") >= 0) {
      digitalWrite(LED_PINS[ledIndex], HIGH);
      Serial.print("[INFO] LED"); Serial.print(ledIndex + 1); Serial.println(" Allumée");
    } else if (msg.indexOf("\"state\": \"off\"") >= 0 || msg.indexOf("\"state\":\"off\"") >= 0) {
      digitalWrite(LED_PINS[ledIndex], LOW);
      Serial.print("[INFO] LED"); Serial.print(ledIndex + 1); Serial.println(" Éteinte");
    }
  }
}

void mqttCallback(char* topic, byte* payload, unsigned int length) {
  String msg = "";
  for (unsigned int i = 0; i < length; i++) {
    msg += (char)payload[i];
  }

  Serial.print("[MQTT] <- ");
  Serial.print(topic);
  Serial.print(" = ");
  Serial.println(msg);

  String topicStr = String(topic);

  if (topicStr == TOPIC_SUB_LED1) {
    handleLedCommand(0, msg);
  } else if (topicStr == TOPIC_SUB_LED2) {
    handleLedCommand(1, msg);
  } else if (topicStr == TOPIC_SUB_LED3) {
    handleLedCommand(2, msg);
  } else if (topicStr == TOPIC_SUB_LED4) {
    handleLedCommand(3, msg);
  } else if (topicStr == TOPIC_SUB_CONFIG) {
    int idx = msg.indexOf("\"interval\":");
    if (idx >= 0) {
      int startIdx = msg.indexOf(":", idx) + 1;
      int endIdx = msg.indexOf("}", startIdx);
      if (endIdx > startIdx) {
        String valStr = msg.substring(startIdx, endIdx);
        valStr.trim();
        long newInterval = valStr.toInt();
        if (newInterval > 0) {
          publishIntervalMs = newInterval;
          Serial.print("[INFO] Nouvel intervalle de publication: ");
          Serial.println(publishIntervalMs);
        }
      }
    }
  }
}

bool connectMQTT() {
  Serial.println("[INFO] Connexion au broker MQTT...");

  if (!wsClient.connected()) {
    if (!wsClient.connect(MQTT_HOST, MQTT_WSS_PORT)) {
      Serial.println("[ERROR] Échec de connexion WebSocket");
      return false;
    }
  }

  if (mqttClient.connect(MQTT_CLIENT_ID, MQTT_USER, MQTT_PASS)) {
    Serial.println("[INFO] MQTT Connecté avec succès!");

    mqttClient.subscribe(TOPIC_SUB_LED1.c_str());
    mqttClient.subscribe(TOPIC_SUB_LED2.c_str());
    mqttClient.subscribe(TOPIC_SUB_LED3.c_str());
    mqttClient.subscribe(TOPIC_SUB_LED4.c_str());
    mqttClient.subscribe(TOPIC_SUB_CONFIG.c_str());
    Serial.println("[INFO] Souscriptions MQTT effectuées");

    return true;
  }

  Serial.print("[ERROR] Échec de connexion MQTT, code d'erreur: ");
  Serial.println(mqttClient.state());
  return false;
}

void maintainMQTT() {
  unsigned long now = millis();

  // Vérifier la connexion GPRS
  if (now - lastGprsCheck > GPRS_CHECK_INTERVAL) {
    lastGprsCheck = now;
    if (!modem.isGprsConnected()) {
      Serial.println("[ERROR] Connexion GPRS perdue, tentative de reconnexion...");
      if (modem.gprsConnect(APN, APN_USER, APN_PASS)) {
        Serial.println("[INFO] GPRS reconnecté.");
      }
    }
  }

  // Maintenir la connexion MQTT
  if (!mqttClient.connected()) {
    Serial.println("[WARN] Client MQTT déconnecté, tentative de reconnexion...");
    if (modem.isGprsConnected()) {
      connectMQTT();
    }
  }

  mqttClient.loop();
}

void readSensors() {
  int button1State = digitalRead(BUTTON1_PIN);
  int button2State = digitalRead(BUTTON2_PIN);
  
  if (button1State != lastButton1State || button2State != lastButton2State) {
    lastButton1State = button1State;
    lastButton2State = button2State;
    
    // Publication immédiate sur changement d'état
    if (mqttClient.connected()) {
      String payload = "{\"btn1\": " + String(button1State == LOW ? "true" : "false") + 
                       ", \"btn2\": " + String(button2State == LOW ? "true" : "false") + "}";
      mqttClient.publish(TOPIC_PUB_BUTTONS.c_str(), payload.c_str());
      Serial.print("[MQTT] -> "); Serial.print(TOPIC_PUB_BUTTONS); Serial.print(" = "); Serial.println(payload);
    }
  }
}

void publishData() {
  unsigned long now = millis();
  if (now - lastPublishTime >= publishIntervalMs) {
    lastPublishTime = now;

    if (!mqttClient.connected()) return;

    // 1. Potentiomètres
    int valPot1 = analogRead(POT_PINS[0]);
    int valPot2 = analogRead(POT_PINS[1]);
    String payloadPots = "{\"pot1\": " + String(valPot1) + ", \"pot2\": " + String(valPot2) + "}";
    mqttClient.publish(TOPIC_PUB_POTS.c_str(), payloadPots.c_str());
    Serial.print("[MQTT] -> "); Serial.print(TOPIC_PUB_POTS); Serial.print(" = "); Serial.println(payloadPots);

    // 2. Accéléromètre
    sensors_event_t a, g, temp;
    mpu.getEvent(&a, &g, &temp);
    
    // Calcul simple du roll et pitch
    float roll = atan2(a.acceleration.y, a.acceleration.z) * 180.0 / PI;
    float pitch = atan2(-a.acceleration.x, sqrt(a.acceleration.y * a.acceleration.y + a.acceleration.z * a.acceleration.z)) * 180.0 / PI;
    
    String payloadAccel = "{\"x\": " + String(a.acceleration.x, 2) + 
                          ", \"y\": " + String(a.acceleration.y, 2) + 
                          ", \"z\": " + String(a.acceleration.z, 2) + 
                          ", \"roll\": " + String(roll, 2) + 
                          ", \"pitch\": " + String(pitch, 2) + "}";
    mqttClient.publish(TOPIC_PUB_ACCEL.c_str(), payloadAccel.c_str());
    Serial.print("[MQTT] -> "); Serial.print(TOPIC_PUB_ACCEL); Serial.print(" = "); Serial.println(payloadAccel);

    // 3. Statut
    long uptimeSec = millis() / 1000;
    int rssi = modem.getSignalQuality();
    String payloadStatus = "{\"uptime\": " + String(uptimeSec) + ", \"rssi\": " + String(rssi) + "}";
    mqttClient.publish(TOPIC_PUB_STATUS.c_str(), payloadStatus.c_str());
    Serial.print("[MQTT] -> "); Serial.print(TOPIC_PUB_STATUS); Serial.print(" = "); Serial.println(payloadStatus);
  }
}

// ============================================================================
// SETUP & LOOP
// ============================================================================

void setup() {
  initSerial();
  initGPIO();
  initI2C();
  
  // Configuration SSL avant MQTT
  sslClient.setClient(&gsmClient);
  sslClient.setInsecure();
  sslClient.setBufferSizes(2048, 1024);
  sslClient.setDebugLevel(0);

  mqttClient.setServer(MQTT_HOST, MQTT_WSS_PORT);
  mqttClient.setCallback(mqttCallback);
  mqttClient.setKeepAlive(60);

  initLTE();
  connectMQTT();
}

void loop() {
  maintainMQTT();
  readSensors();
  publishData();
}