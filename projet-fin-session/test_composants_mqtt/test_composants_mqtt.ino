// ============================================================================
// TEST COMPOSANTS LILYGO A7670G AVEC MQTT
// Topics:
//   - hydro-limoilou/poste-05/telemetry/{capteur}  (mesures periodiques)
//   - hydro-limoilou/poste-05/status               (etat periodique du noeud)
//   - hydro-limoilou/poste-05/alarm/{type}         (evenements ponctuels)
//   - hydro-limoilou/poste-05/actuators/{nom}       (commandes descendantes)
// ============================================================================

#define TINY_GSM_MODEM_SIM7600
#define TINY_GSM_RX_BUFFER 1024

#include <TinyGsmClient.h>
#include <PubSubClient.h>
#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BME280.h>
#include <Adafruit_MPU6050.h>

#include "auth.h"

// ====== CONFIG MODEM A7670G ======
#define MODEM_TX 26
#define MODEM_RX 27
#define MODEM_PWRKEY 4
#define MODEM_DTR 12
#define MODEM_RI 13
#define MODEM_FLIGHT 25
#define MODEM_STATUS 0

// ====== CONFIG MQTT ======
const char* MQTT_HOST = MQTT_BROKER;
const int   MQTT_PORT = 1883;  // Port non-SSL pour test

// ====== CAPTEURS ======
#define PIN_PIR 13
#define PIN_POT1 34
#define PIN_POT2 35
#define PIN_LED1 14
#define PIN_LED2 15

#define I2C_SCK 22
#define I2C_SDI 21

Adafruit_BME280 bme;
Adafruit_MPU6050 mpu;

// ====== LED & BOUTONS ======
const int LED_PINS[] = {12, 14, 15};
const int NUM_LEDS = 3;

const int BUTTON1_PIN = 34;
const int BUTTON2_PIN = 35;

// ====== TOPICS MQTT ======
const char* TOPIC_TELEMETRY_TEMP = "hydro-limoilou/poste-05/telemetry/temperature";
const char* TOPIC_TELEMETRY_HUM = "hydro-limoilou/poste-05/telemetry/humidity";
const char* TOPIC_TELEMETRY_PRES = "hydro-limoilou/poste-05/telemetry/pressure";
const char* TOPIC_TELEMETRY_PIR = "hydro-limoilou/poste-05/telemetry/pir";
const char* TOPIC_TELEMETRY_POT1 = "hydro-limoilou/poste-05/telemetry/pot1";
const char* TOPIC_TELEMETRY_POT2 = "hydro-limoilou/poste-05/telemetry/pot2";
const char* TOPIC_TELEMETRY_ACCEL = "hydro-limoilou/poste-05/telemetry/accel";

const char* TOPIC_STATUS = "hydro-limoilou/poste-05/status";

const char* TOPIC_ALARM_MOVEMENT = "hydro-limoilou/poste-05/alarm/movement";

const char* TOPIC_ACTUATOR_LED1 = "hydro-limoilou/poste-05/actuators/led1";
const char* TOPIC_ACTUATOR_LED2 = "hydro-limoilou/poste-05/actuators/led2";
const char* TOPIC_ACTUATOR_LED3 = "hydro-limoilou/poste-05/actuators/led3";
const char* TOPIC_ACTUATOR_CONFIG = "hydro-limoilou/poste-05/actuators/config";

// ====== VARIABLES ======
unsigned long publishIntervalMs = 5000;
unsigned long lastPublishTime = 0;
unsigned long lastGprsCheck = 0;
const unsigned long GPRS_CHECK_INTERVAL = 30000;

int lastButton1State = HIGH;
int lastButton2State = HIGH;
int lastPirState = LOW;

HardwareSerial SerialAT(1);

// ============================================================================
// CLIENTS - MQTT sur GSM (non-SSL pour test)
// ============================================================================

TinyGsm modem(SerialAT);
TinyGsmClient gsmClient(modem, 0);
PubSubClient mqttClient(gsmClient);

// ============================================================================
// INITIALISATIONS
// ============================================================================

void initSerial() {
  Serial.begin(115200);
  delay(2000);
  Serial.println("\n=== LilyGO A7670G - Test Composants + MQTT ===");
}

void initGPIO() {
  pinMode(PIN_PIR, INPUT);
  pinMode(PIN_POT1, INPUT);
  pinMode(PIN_POT2, INPUT);
  pinMode(PIN_LED1, OUTPUT);
  pinMode(PIN_LED2, OUTPUT);
  digitalWrite(PIN_LED1, LOW);
  digitalWrite(PIN_LED2, LOW);

  for (int i = 0; i < NUM_LEDS; i++) {
    pinMode(LED_PINS[i], OUTPUT);
    digitalWrite(LED_PINS[i], LOW);
  }

  pinMode(BUTTON1_PIN, INPUT_PULLUP);
  pinMode(BUTTON2_PIN, INPUT_PULLUP);

  Serial.println("[INFO] GPIO initialises");
}

void initI2C() {
  Wire.begin(I2C_SDI, I2C_SCK);
  
  if (!bme.begin(0x76) && !bme.begin(0x77)) {
    Serial.println("[WARN] BME280 non detecte!");
  } else {
    Serial.println("[INFO] BME280 detecte");
  }

  Serial.println("[INFO] Initialisation MPU6050...");
  if (!mpu.begin()) {
    Serial.println("[WARN] MPU6050 non detecte!");
  } else {
    Serial.println("[INFO] MPU6050 detecte");
    mpu.setAccelerometerRange(MPU6050_RANGE_2_G);
    mpu.setGyroRange(MPU6050_RANGE_250_DEG);
    mpu.setFilterBandwidth(MPU6050_BAND_5_HZ);
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
  Serial.println("[INFO] Modem allume");
}

void initLTE() {
  modemPowerOn();
  Serial.println("[INFO] Initialisation modem...");
  SerialAT.begin(115200, SERIAL_8N1, MODEM_RX, MODEM_TX);
  delay(3000);

  if (!modem.restart()) {
    Serial.println("[ERROR] Echec du redemarrage du modem");
    return;
  }
  Serial.println("[INFO] Modem initialise.");

  Serial.println("[INFO] Configuration de l'APN...");
  modem.sendAT("+CGDCONT=1,\"IP\",\"", APN, "\"");
  modem.waitResponse();
  
  Serial.println("[INFO] Connexion au reseau cellulaire...");
  if (!modem.waitForNetwork(60000L)) {
    Serial.println("[ERROR] Echec de connexion au reseau");
    return;
  }
  Serial.println("[INFO] Reseau connecte.");

  Serial.println("[INFO] Connexion GPRS...");
  if (!modem.gprsConnect(APN, APN_USER, APN_PASS)) {
    Serial.println("[ERROR] Echec de connexion GPRS");
    return;
  }
  Serial.println("[INFO] GPRS connecte avec succes.");
}

// ============================================================================
// MQTT
// ============================================================================

void handleLedCommand(int ledIndex, String msg) {
  if (ledIndex >= 0 && ledIndex < NUM_LEDS) {
    if (msg.indexOf("\"state\": \"on\"") >= 0 || msg.indexOf("\"state\":\"on\"") >= 0) {
      digitalWrite(LED_PINS[ledIndex], HIGH);
      Serial.print("[INFO] LED"); Serial.print(ledIndex + 1); Serial.println(" Allumee");
    } else if (msg.indexOf("\"state\": \"off\"") >= 0 || msg.indexOf("\"state\":\"off\"") >= 0) {
      digitalWrite(LED_PINS[ledIndex], LOW);
      Serial.print("[INFO] LED"); Serial.print(ledIndex + 1); Serial.println(" Eteinte");
    }
  }
}

void mqttCallback(char* topic, byte* payload, unsigned int length) {
  String msg = "";
  for (unsigned int i = 0; i < length; i++) msg += (char)payload[i];

  Serial.print("[MQTT] <- ");
  Serial.print(topic);
  Serial.print(" = ");
  Serial.println(msg);

  String topicStr = String(topic);

  if (topicStr == TOPIC_ACTUATOR_LED1) {
    handleLedCommand(0, msg);
  } else if (topicStr == TOPIC_ACTUATOR_LED2) {
    handleLedCommand(1, msg);
  } else if (topicStr == TOPIC_ACTUATOR_LED3) {
    handleLedCommand(2, msg);
  } else if (topicStr == TOPIC_ACTUATOR_CONFIG) {
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
          Serial.print("[INFO] Nouvel intervalle: ");
          Serial.println(publishIntervalMs);
        }
      }
    }
  }
}

bool connectMQTT() {
  Serial.println("[INFO] Connexion au broker MQTT...");
  Serial.print("[INFO] Host: "); Serial.println(MQTT_HOST);
  Serial.print("[INFO] Port: "); Serial.println(MQTT_PORT);

  if (mqttClient.connect(MQTT_CLIENT_ID, MQTT_USER, MQTT_PASS)) {
    Serial.println("[INFO] MQTT Connecte!");

    mqttClient.subscribe(TOPIC_ACTUATOR_LED1);
    mqttClient.subscribe(TOPIC_ACTUATOR_LED2);
    mqttClient.subscribe(TOPIC_ACTUATOR_LED3);
    mqttClient.subscribe(TOPIC_ACTUATOR_CONFIG);
    Serial.println("[INFO] Souscriptions effectuees");

    return true;
  }

  Serial.print("[ERROR] Echec connexion MQTT, code: ");
  Serial.println(mqttClient.state());
  return false;
}

void maintainMQTT() {
  unsigned long now = millis();

  if (now - lastGprsCheck > GPRS_CHECK_INTERVAL) {
    lastGprsCheck = now;
    if (!modem.isGprsConnected()) {
      Serial.println("[ERROR] Connexion GPRS perdue, reconnexion...");
      if (modem.gprsConnect(APN, APN_USER, APN_PASS)) {
        Serial.println("[INFO] GPRS reconnecte.");
      }
    }
  }

  if (!mqttClient.connected()) {
    Serial.println("[WARN] MQTT deconnecte, reconnexion...");
    if (modem.isGprsConnected()) connectMQTT();
  }

  mqttClient.loop();
}

void checkPirAndButtons() {
  int pirState = digitalRead(PIN_PIR);
  int button1State = digitalRead(BUTTON1_PIN);
  int button2State = digitalRead(BUTTON2_PIN);

  if (pirState != lastPirState) {
    lastPirState = pirState;
    if (mqttClient.connected()) {
      String payload = "{\"detected\": " + String(pirState ? "true" : "false") + "}";
      mqttClient.publish(TOPIC_TELEMETRY_PIR, payload.c_str());
      Serial.print("[MQTT] -> "); Serial.print(TOPIC_TELEMETRY_PIR); Serial.print(" = "); Serial.println(payload);

      if (pirState) {
        String alarmPayload = "{\"timestamp\": " + String(millis()) + ", \"event\": \"movement_detected\"}";
        mqttClient.publish(TOPIC_ALARM_MOVEMENT, alarmPayload.c_str());
        Serial.print("[MQTT] -> "); Serial.print(TOPIC_ALARM_MOVEMENT); Serial.print(" = "); Serial.println(alarmPayload);
      }
    }
  }

  if (button1State != lastButton1State || button2State != lastButton2State) {
    lastButton1State = button1State;
    lastButton2State = button2State;
  }
}

void publishData() {
  unsigned long now = millis();
  if (now - lastPublishTime >= publishIntervalMs) {
    lastPublishTime = now;

    if (!mqttClient.connected()) return;

    float temp = bme.readTemperature();
    float hum = bme.readHumidity();
    float pres = bme.readPressure() / 100.0F;

    if (!isnan(temp)) {
      String payloadTemp = "{\"value\": " + String(temp, 1) + ", \"unit\": \"C\"}";
      mqttClient.publish(TOPIC_TELEMETRY_TEMP, payloadTemp.c_str());
      Serial.print("[MQTT] -> "); Serial.print(TOPIC_TELEMETRY_TEMP); Serial.print(" = "); Serial.println(payloadTemp);
    }

    if (!isnan(hum)) {
      String payloadHum = "{\"value\": " + String(hum, 1) + ", \"unit\": \"%\"}";
      mqttClient.publish(TOPIC_TELEMETRY_HUM, payloadHum.c_str());
      Serial.print("[MQTT] -> "); Serial.print(TOPIC_TELEMETRY_HUM); Serial.print(" = "); Serial.println(payloadHum);
    }

    if (!isnan(pres)) {
      String payloadPres = "{\"value\": " + String(pres, 1) + ", \"unit\": \"hPa\"}";
      mqttClient.publish(TOPIC_TELEMETRY_PRES, payloadPres.c_str());
      Serial.print("[MQTT] -> "); Serial.print(TOPIC_TELEMETRY_PRES); Serial.print(" = "); Serial.println(payloadPres);
    }

    int pot1 = analogRead(PIN_POT1);
    int pot2 = analogRead(PIN_POT2);
    String payloadPot1 = "{\"value\": " + String(pot1) + "}";
    String payloadPot2 = "{\"value\": " + String(pot2) + "}";
    mqttClient.publish(TOPIC_TELEMETRY_POT1, payloadPot1.c_str());
    mqttClient.publish(TOPIC_TELEMETRY_POT2, payloadPot2.c_str());
    Serial.print("[MQTT] -> "); Serial.print(TOPIC_TELEMETRY_POT1); Serial.print(" = "); Serial.println(payloadPot1);
    Serial.print("[MQTT] -> "); Serial.print(TOPIC_TELEMETRY_POT2); Serial.print(" = "); Serial.println(payloadPot2);

    sensors_event_t a, g, tempEvent;
    if (mpu.getEvent(&a, &g, &tempEvent)) {
      float roll = atan2(a.acceleration.y, a.acceleration.z) * 180.0 / PI;
      float pitch = atan2(-a.acceleration.x, sqrt(a.acceleration.y * a.acceleration.y + a.acceleration.z * a.acceleration.z)) * 180.0 / PI;

      String payloadAccel = "{\"x\": " + String(a.acceleration.x, 2) + 
                            ", \"y\": " + String(a.acceleration.y, 2) + 
                            ", \"z\": " + String(a.acceleration.z, 2) + 
                            ", \"roll\": " + String(roll, 2) + 
                            ", \"pitch\": " + String(pitch, 2) + "}";
      mqttClient.publish(TOPIC_TELEMETRY_ACCEL, payloadAccel.c_str());
      Serial.print("[MQTT] -> "); Serial.print(TOPIC_TELEMETRY_ACCEL); Serial.print(" = "); Serial.println(payloadAccel);
    }

    long uptimeSec = millis() / 1000;
    int rssi = modem.getSignalQuality();
    String payloadStatus = "{\"uptime\": " + String(uptimeSec) + 
                           ", \"rssi\": " + String(rssi) + 
                           ", \"type\": \"node_status\"}";
    mqttClient.publish(TOPIC_STATUS, payloadStatus.c_str());
    Serial.print("[MQTT] -> "); Serial.print(TOPIC_STATUS); Serial.print(" = "); Serial.println(payloadStatus);
  }
}

// ============================================================================
// SETUP & LOOP
// ============================================================================

void setup() {
  initSerial();
  initGPIO();
  initI2C();

  mqttClient.setServer(MQTT_HOST, MQTT_PORT);
  mqttClient.setCallback(mqttCallback);
  mqttClient.setKeepAlive(60);

  initLTE();
  connectMQTT();
}

void loop() {
  maintainMQTT();
  checkPirAndButtons();
  publishData();
}