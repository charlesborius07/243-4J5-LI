#include <Wire.h>
#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>

#define TINY_GSM_MODEM_SIM7600
#define TINY_GSM_RX_BUFFER 1024
#include <TinyGsmClient.h>
#include <PubSubClient.h>

#include "auth.h"

// ============================================================================
// CONFIGURATION DES BROCHES (Hardware Pinout)
// ============================================================================
// NOTE : D'après test_mpu6050.ino, on utilise :
const int LED_PINS[] = {12, 13, 14, 15};
const int POT_PINS[] = {32, 33};
const int BUTTON_PIN = 34;

// CONFIG MODEM A7670G
// Attention: 26 et 27 sont souvent les broches TX/RX par défaut du modem.
// S'il y a un conflit matériel avec les potentiomètres (26, 27), il faudra
// réassigner physiquement les broches. On laisse les définitions standard ici.
#define MODEM_TX 26
#define MODEM_RX 27
#define MODEM_PWRKEY 4

// ============================================================================
// CONSTANTES ET VARIABLES GLOBALES
// ============================================================================
const unsigned long PUBLISH_INTERVAL_MS = 5000;
const unsigned long DEBOUNCE_DELAY_MS = 50;
const unsigned long MQTT_RECONNECT_INTERVAL_MS = 5000;

unsigned long lastPublishTime = 0;
unsigned long lastButtonCheckTime = 0;
unsigned long lastMqttReconnectAttempt = 0;

int lastButtonState = HIGH;

// Objets matériels et réseau
Adafruit_MPU6050 mpu;
HardwareSerial SerialAT(1);

#define ENABLE_DEBUG
#define ENABLE_ERROR_STRING
#define DEBUG_PORT Serial
#define SSLCLIENT_INSECURE_ONLY
#include <ESP_SSLClient.h>
#include "WebSocketClient.h"

TinyGsm modem(SerialAT);
TinyGsmClient gsmClient(modem, 0);
ESP_SSLClient sslClient;
WebSocketClient wsClient(&sslClient, "/");
PubSubClient mqttClient(wsClient);

// Buffers pour les topics
char topicSensorsButtons[100];
char topicSensorsPots[100];
char topicSensorsAccel[100];
char topicStatus[100];
char topicActuatorsLed[4][100];

// ============================================================================
// FONCTIONS DE DÉMARRAGE (Setup)
// ============================================================================

void initSerial() {
    Serial.begin(115200);
    delay(1000);
    Serial.println("\n=============================================");
    Serial.println("[INFO] Démarrage du système LilyGO Firmware");
    Serial.println("=============================================");
}

void initGPIO() {
    Serial.println("[INFO] Initialisation des GPIOs (LEDs, Boutons, Pots)");
    
    // LEDs
    for (int i = 0; i < 4; i++) {
        pinMode(LED_PINS[i], OUTPUT);
        digitalWrite(LED_PINS[i], LOW);
    }
    
    // Bouton (Pas de pull-up interne sur 34)
    pinMode(BUTTON_PIN, INPUT);
    
    // Les potentiomètres utilisent l'ADC, pas besoin de pinMode explicite en ESP32
    // analogRead(POT_PINS[i]) gèrera le mode de fonctionnement.
}

void initI2C() {
    Serial.println("[INFO] Initialisation I2C pour MPU6050...");
    if (!mpu.begin()) {
        Serial.println("[ERROR] Accéléromètre MPU6050 introuvable ! Vérifiez le câblage.");
    } else {
        Serial.println("[INFO] Accéléromètre MPU6050 prêt.");
        mpu.setAccelerometerRange(MPU6050_RANGE_8_G);
        mpu.setGyroRange(MPU6050_RANGE_500_DEG);
        mpu.setFilterBandwidth(MPU6050_BAND_21_HZ);
    }
}

void initLTE() {
    Serial.println("[INFO] Allumage du Modem LTE A7670G...");
    
    pinMode(MODEM_PWRKEY, OUTPUT);
    digitalWrite(MODEM_PWRKEY, HIGH);
    delay(100);
    digitalWrite(MODEM_PWRKEY, LOW);
    delay(1000);
    digitalWrite(MODEM_PWRKEY, HIGH);
    delay(3000);
    
    Serial.println("[INFO] Initialisation de la communication série Modem");
    SerialAT.begin(115200, SERIAL_8N1, MODEM_RX, MODEM_TX);
    delay(3000);
    
    if (!modem.restart()) {
        Serial.println("[ERROR] Échec du redémarrage du modem.");
        return;
    }
    
    Serial.println("[INFO] Recherche du réseau cellulaire...");
    if (!modem.waitForNetwork(60000L)) {
        Serial.println("[ERROR] Échec de la connexion au réseau.");
        return;
    }
    
    Serial.println("[INFO] Connexion au réseau GPRS...");
    if (!modem.gprsConnect(APN, APN_USER, APN_PASS)) {
        Serial.println("[ERROR] Échec d'activation du contexte GPRS.");
        return;
    }
    
    Serial.println("[INFO] LTE/GPRS connecté avec succès.");
}

void mqttCallback(char* topic, byte* payload, unsigned int length) {
    String msg = "";
    for (unsigned int i = 0; i < length; i++) {
        msg += (char)payload[i];
    }
    
    Serial.print("[MQTT] Message reçu sur: ");
    Serial.print(topic);
    Serial.print(" => ");
    Serial.println(msg);

    // Vérifier si le message correspond à une commande de LED
    for (int i = 0; i < 4; i++) {
        if (strcmp(topic, topicActuatorsLed[i]) == 0) {
            // Recherche basique de l'état "on" ou "off"
            if (msg.indexOf("\"state\":\"on\"") >= 0 || msg.indexOf("\"state\": \"on\"") >= 0) {
                digitalWrite(LED_PINS[i], HIGH);
                Serial.printf("[INFO] Action: LED %d ALLUMÉE\n", i + 1);
            } else if (msg.indexOf("\"state\":\"off\"") >= 0 || msg.indexOf("\"state\": \"off\"") >= 0) {
                digitalWrite(LED_PINS[i], LOW);
                Serial.printf("[INFO] Action: LED %d ÉTEINTE\n", i + 1);
            }
        }
    }
}

void connectMQTT() {
    Serial.println("[INFO] Configuration des Topics MQTT...");
    
    snprintf(topicSensorsButtons, sizeof(topicSensorsButtons), "%ssensors/buttons", STUDENT_TOPIC_ROOT);
    snprintf(topicSensorsPots, sizeof(topicSensorsPots), "%ssensors/pots", STUDENT_TOPIC_ROOT);
    snprintf(topicSensorsAccel, sizeof(topicSensorsAccel), "%ssensors/accel", STUDENT_TOPIC_ROOT);
    snprintf(topicStatus, sizeof(topicStatus), "%sstatus", STUDENT_TOPIC_ROOT);
    
    for (int i = 0; i < 4; i++) {
        snprintf(topicActuatorsLed[i], sizeof(topicActuatorsLed[i]), "%sactuators/led%d", STUDENT_TOPIC_ROOT, i + 1);
    }
    
    // Configuration SSL
    Serial.println("[SSL] Configuration du client SSL...");
    sslClient.setClient(&gsmClient);
    sslClient.setInsecure();
    sslClient.setBufferSizes(2048, 1024);
    sslClient.setDebugLevel(1);

    mqttClient.setServer(MQTT_BROKER, 443); // Port WSS (443) pour contourner Cloudflare
    mqttClient.setCallback(mqttCallback);
    mqttClient.setKeepAlive(60);
}

// ============================================================================
// FONCTIONS DE LA BOUCLE (Loop)
// ============================================================================

void maintainMQTT() {
    if (!mqttClient.connected()) {
        unsigned long now = millis();
        if (now - lastMqttReconnectAttempt > MQTT_RECONNECT_INTERVAL_MS) {
            lastMqttReconnectAttempt = now;
            
            Serial.println("[MQTT] Tentative de connexion au broker...");
            if (mqttClient.connect(MQTT_CLIENT_ID, MQTT_USER, MQTT_PASS)) {
                Serial.println("[INFO] Connecté au broker MQTT.");
                
                // Souscrire aux topics de contrôle pour les 4 LEDs
                for (int i = 0; i < 4; i++) {
                    mqttClient.subscribe(topicActuatorsLed[i]);
                    Serial.print("[MQTT] Souscription à: ");
                    Serial.println(topicActuatorsLed[i]);
                }
            } else {
                Serial.print("[ERROR] Échec de la connexion MQTT, rc=");
                Serial.println(mqttClient.state());
            }
        }
    }
    // Traiter les événements MQTT entrants (nécessaire à chaque boucle)
    mqttClient.loop();
}

void readSensors() {
    unsigned long now = millis();
    
    // Lecture du bouton avec anti-rebond (debounce) non bloquant
    if (now - lastButtonCheckTime > DEBOUNCE_DELAY_MS) {
        int currentButtonState = digitalRead(BUTTON_PIN);
        
        // Si l'état a changé
        if (currentButtonState != lastButtonState) {
            lastButtonState = currentButtonState;
            lastButtonCheckTime = now;
            
            // Logique LOW = pressé (PULLUP)
            bool isPressed = (currentButtonState == LOW);
            char payload[64];
            snprintf(payload, sizeof(payload), "{\"btn1\": %s}", isPressed ? "true" : "false");
            
            if (mqttClient.connected()) {
                mqttClient.publish(topicSensorsButtons, payload);
                Serial.print("[MQTT] Publié sur Boutons: ");
                Serial.println(payload);
            }
        }
    }
}

void publishData() {
    unsigned long now = millis();
    
    // Publication périodique basée sur un timestamp, sans delay() bloquant
    if (now - lastPublishTime >= PUBLISH_INTERVAL_MS) {
        lastPublishTime = now;
        
        if (mqttClient.connected()) {
            char payload[150];
            
            // 1. Lecture et publication des potentiomètres
            int pot1 = analogRead(POT_PINS[0]);
            int pot2 = analogRead(POT_PINS[1]);
            snprintf(payload, sizeof(payload), "{\"pot1\": %d, \"pot2\": %d}", pot1, pot2);
            mqttClient.publish(topicSensorsPots, payload);
            
            // 2. Lecture et publication de l'accéléromètre
            sensors_event_t a, g, temp;
            mpu.getEvent(&a, &g, &temp);
            snprintf(payload, sizeof(payload), "{\"x\": %.2f, \"y\": %.2f, \"z\": %.2f, \"roll\": %.2f, \"pitch\": %.2f}", 
                     a.acceleration.x, a.acceleration.y, a.acceleration.z, g.gyro.x, g.gyro.y);
            mqttClient.publish(topicSensorsAccel, payload);
            
            // 3. Publication de l'état système (Status)
            int signalQuality = modem.getSignalQuality();
            unsigned long uptimeSeconds = now / 1000;
            snprintf(payload, sizeof(payload), "{\"uptime\": %lu, \"rssi\": %d}", uptimeSeconds, signalQuality);
            mqttClient.publish(topicStatus, payload);
            
            Serial.println("[INFO] Données périodiques publiées (Pots, Accel, Status)");
        }
    }
}

// ============================================================================
// POINTS D'ENTRÉE ARDUINO
// ============================================================================

void setup() {
    initSerial();      // Debug UART
    initGPIO();        // Boutons, LEDs, potentiomètres
    initI2C();         // MPU6050
    initLTE();         // Modem A7670G
    connectMQTT();     // Configuration du client MQTT
}

void loop() {
    maintainMQTT();    // Gère la reconnexion automatique MQTT
    readSensors();     // Surveille les boutons (événements immédiats)
    publishData();     // Publie la télémétrie périodique (non-bloquant)
    
    // Petite pause pour stabiliser la boucle matérielle (au lieu du delay complet)
    delay(10);
}
