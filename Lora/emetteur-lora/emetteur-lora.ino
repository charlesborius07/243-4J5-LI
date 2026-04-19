/*
 * Activite - Emetteur LoRa avec appel LLM de l'autre cote
 * Cegep de Limoilou - Objets connectes
 *
 * Materiel :
 *   - LilyGO T-Beam Supreme (ESP32-S3, SH1106 OLED, AXP2101 PMU)
 *   - Potentiometre sur GPIO 2 (ADC)
 *   - LED sur GPIO 25
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

// =============================================
// PINS T-Beam Supreme
// =============================================

#define POT_PIN         2     // GPIO ADC pour le potentiometre
#define LED_ACTION      25    // LED d'action
#define PIN_BTN         0     // Bouton integre du T-Beam Supreme

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

#include <SPI.h>

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
// PROTOTYPES
// =============================================

void initPMU();
void oledPrint(String texte);

// =============================================
// SETUP
// =============================================

void setup() {
  Serial.begin(115200);
  delay(500);

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

  // Pins
  pinMode(PIN_BTN, INPUT_PULLUP);
  pinMode(POT_PIN, INPUT);
  pinMode(LED_ACTION, OUTPUT);
  analogReadResolution(12); // 0-4095

  // Initialiser LoRa
  oledPrint("Init LoRa...");
  // Passer -1 pour le CS afin que RadioLib puisse le contrôler (sinon le driver ESP32 le bloque)
  SPI.begin(LORA_SCK, LORA_MISO, LORA_MOSI, -1);
  int state = radio.begin(915.0, 125.0, 9, 7, 0x12, 22, 8);
  if (state == RADIOLIB_ERR_NONE) {
    Serial.println("LoRa init success!");
    oledPrint("LoRa pret!");
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
  JsonDocument doc;
  doc["pot"] = analogRead(POT_PIN);
  doc["millis"] = millis();
  
  String msg; 
  serializeJson(doc, msg);
  
  Serial.println("TX: " + msg);
  radio.transmit(msg);
  oledPrint("TX: " + msg + "\nAttente...");

  // Écouter la décision du LLM (retour LoRa)
  String reply;
  int state = radio.receive(reply, 10000);
  
  if (state == RADIOLIB_ERR_NONE) {
    Serial.println("RX: " + reply);
    JsonDocument r; 
    DeserializationError err = deserializeJson(r, reply);
    
    String action = "none";
    if (!err) {
      action = r["action"] | "none";
    }
    
    digitalWrite(LED_ACTION, action == "on" ? HIGH : LOW);
    oledPrint("TX: " + msg + "\nLLM: " + reply);
  } else if (state == RADIOLIB_ERR_RX_TIMEOUT) {
    Serial.println("RX timeout");
    oledPrint("TX: " + msg + "\nLLM: Timeout!");
  } else {
    Serial.println("RX error, code " + String(state));
    oledPrint("TX: " + msg + "\nRX Erreur: " + String(state));
  }
  
  delay(5000);
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