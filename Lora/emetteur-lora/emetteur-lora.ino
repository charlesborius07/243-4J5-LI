/*
 * Activite - Emetteur LoRa avec appel LLM de l'autre cote
 * Cegep de Limoilou - Objets connectes
 *
 * Materiel :
 *   - LilyGO T-Beam Supreme (ESP32-S3, SH1106 OLED, AXP2101 PMU)
 *   - Potentiometre sur GPIO 2 (ADC)
 *   - LED sur GPIO 46
 *   - Bouton BOOT sur GPIO 0
 *
 * Dependances (Arduino Library Manager) :
 *   - ArduinoJson (Benoit Blanchon)
 *   - U8g2 (olikraus)
 *   - XPowersLib (Lewis He)
 *   - RadioLib (jgromes)
 *
 * Board dans Arduino IDE :
 *   - ESP32S3 Dev Module (esp32:esp32:esp32s3)
 *   - USB CDC On Boot : Enabled
 *   - PSRAM : OPI PSRAM
 */

#include <ArduinoJson.h>
#include <U8g2lib.h>
#include <Wire.h>
#include <XPowersLib.h>
#include <RadioLib.h>
#include <SPI.h>

// =============================================
// PINS T-Beam Supreme
// =============================================

#define POT_PIN         2     
#define LED_ACTION      46    
#define PIN_BTN         0     

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
// ÉTATS ET VARIABLES
// =============================================

enum State {
  IDLE,           // Lecture pot en temps réel
  WAITING_REPLY,  // Trame envoyée affichée, attente réponse LoRa
  SHOW_RESULT     // Réponse reçue affichée, attente bouton ou timeout pour reset
};

State currentState = IDLE;
bool btnPrecedent = HIGH;
int dernierPot = -1;
String trameEnvoyee = "";
String reponseLLM = "";

unsigned long lastCycleTime = 0;
const unsigned long CYCLE_INTERVAL = 10000; // 10 secondes
const unsigned long DISPLAY_DURATION = 4000; // Afficher le résultat 4s avant de reset auto

// =============================================
// PROTOTYPES
// =============================================

void initPMU();
void oledPrint(String texte, bool small = false);
void executerTX();

// =============================================
// SETUP
// =============================================

void setup() {
  Serial.begin(115200);
  delay(500);

  Wire.begin(OLED_SDA, OLED_SCL);
  Wire1.begin(PMU_SDA, PMU_SCL);

  initPMU();

  u8g2.begin();
  u8g2.enableUTF8Print();

  pinMode(PIN_BTN, INPUT_PULLUP);
  pinMode(POT_PIN, INPUT);
  pinMode(LED_ACTION, OUTPUT);
  digitalWrite(LED_ACTION, LOW);
  analogReadResolution(12);

  oledPrint("Init LoRa...");
  SPI.begin(LORA_SCK, LORA_MISO, LORA_MOSI, -1);
  int state = radio.begin(915.0, 125.0, 9, 7, 0x12, 22, 8);
  if (state != RADIOLIB_ERR_NONE) {
    oledPrint("Erreur LoRa:\n" + String(state));
    while (true);
  }

  lastCycleTime = millis();
}

// =============================================
// LOOP
// =============================================

void loop() {
  bool btnActuel = digitalRead(PIN_BTN);
  unsigned long now = millis();

  // Détection appui bouton
  if (btnPrecedent == HIGH && btnActuel == LOW) {
    if (currentState == IDLE) {
      executerTX();
    } else if (currentState == SHOW_RESULT) {
      currentState = IDLE;
      dernierPot = -1; 
      lastCycleTime = now;
      Serial.println("Reset manuel - Monitoring");
    }
    delay(200);
  }
  btnPrecedent = btnActuel;

  // Logique de cycle automatique (10s)
  if (currentState == IDLE) {
    // Monitoring Pot
    int pot = analogRead(POT_PIN);
    if (abs(pot - dernierPot) > 15) {
      dernierPot = pot;
      String info = "Monitoring Pot\nVAL: " + String(pot) + "\n\nEnvoi auto dans\n" + String((CYCLE_INTERVAL - (now - lastCycleTime))/1000) + "s";
      oledPrint(info);
    }

    // Trigger automatique
    if (now - lastCycleTime >= CYCLE_INTERVAL) {
      executerTX();
    }
  } 
  else if (currentState == SHOW_RESULT) {
    // Retour automatique à l'état initial après DISPLAY_DURATION
    if (now - lastCycleTime >= DISPLAY_DURATION) {
      currentState = IDLE;
      dernierPot = -1;
      lastCycleTime = now;
      Serial.println("Reset automatique - Monitoring");
    }
  }

  delay(20);
}

// =============================================
// CYCLE TRANSMISSION
// =============================================

void executerTX() {
  currentState = WAITING_REPLY;
  int pot = analogRead(POT_PIN);
  
  JsonDocument doc;
  doc["pot"] = pot;
  doc["millis"] = millis();
  
  trameEnvoyee = "";
  serializeJson(doc, trameEnvoyee);
  
  Serial.println("TX LoRa: " + trameEnvoyee);
  oledPrint("ENVOI LORA:\n" + trameEnvoyee + "\n\nAttente reponse...", true);
  
  int txState = radio.transmit(trameEnvoyee);
  if (txState != RADIOLIB_ERR_NONE) {
    oledPrint("Erreur TX: " + String(txState));
    lastCycleTime = millis();
    currentState = SHOW_RESULT;
    return;
  }

  // Écouter la réponse du récepteur
  String reply;
  int rxState = radio.receive(reply, 10000); 
  
  if (rxState == RADIOLIB_ERR_NONE) {
    reponseLLM = reply;
    Serial.println("RX LoRa: " + reponseLLM);
    
    JsonDocument r; 
    DeserializationError err = deserializeJson(r, reponseLLM);
    
    String action = "none";
    if (!err) {
      action = r["action"] | "none";
    }
    
    digitalWrite(LED_ACTION, (action == "on") ? HIGH : LOW);
    
    // Affiche SEULEMENT la réponse LLM du récepteur
    oledPrint("LLM RECU:\n" + reponseLLM, true);
  } else {
    String errStr = (rxState == RADIOLIB_ERR_RX_TIMEOUT) ? "Timeout!" : "Erreur " + String(rxState);
    oledPrint("TX OK\n\nResultat RX:\n" + errStr, true);
  }
  
  lastCycleTime = millis(); 
  currentState = SHOW_RESULT;
}

// =============================================
// INITIALISATION PMU AXP2101
// =============================================

void initPMU() {
  if (!pmu.init(Wire1, AXP2101_SLAVE_ADDRESS, PMU_SDA, PMU_SCL)) return;
  pmu.setALDO1Voltage(3300);  pmu.enableALDO1();
  pmu.setALDO2Voltage(3300);  pmu.enableALDO2();
  pmu.setALDO3Voltage(3300);  pmu.enableALDO3();
  pmu.setALDO4Voltage(3300);  pmu.enableALDO4();
  pmu.setBLDO1Voltage(3300);  pmu.enableBLDO1();
  pmu.setBLDO2Voltage(3300);  pmu.enableBLDO2();
  pmu.setDC3Voltage(3300);    pmu.enableDC3();
  pmu.setDC5Voltage(3300);    pmu.enableDC5();
  pmu.setChargingLedMode(XPOWERS_CHG_LED_CTRL_CHG);
}

// =============================================
// AFFICHAGE OLED MULTILIGNES
// =============================================

void oledPrint(String texte, bool small) {
  u8g2.clearBuffer();
  
  int lineStep = 11;
  if (small) {
    u8g2.setFont(u8g2_font_6x10_tf);
    lineStep = 10;
  } else {
    u8g2.setFont(u8g2_font_helvB08_tf);
    lineStep = 11;
  }

  int y = lineStep;
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
    y += lineStep;
  }
  u8g2.sendBuffer();
}
