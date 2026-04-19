/*
 * Activite - Recepteur LoRa avec appel LLM de l'autre cote
 * Cegep de Limoilou - Objets connectes
 *
 * Materiel :
 *   - LilyGO T-Beam Supreme (ESP32-S3, SH1106 OLED, AXP2101 PMU)
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

#include <WiFi.h>
#include <esp_wpa2.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <U8g2lib.h>
#include <Wire.h>
#include <XPowersLib.h>
#include <RadioLib.h>
#include <SPI.h>

#include "config.h"

// =============================================
// PINS T-Beam Supreme
// =============================================

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
void connecterWiFi();
String appelLLM(int valeurPot);

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

  // WiFi
  connecterWiFi();

  // Initialiser LoRa
  oledPrint("Init LoRa...");
  // Passer -1 pour le CS afin que RadioLib puisse le contrôler
  SPI.begin(LORA_SCK, LORA_MISO, LORA_MOSI, -1);
  int state = radio.begin(915.0, 125.0, 9, 7, 0x12, 22, 8);
  if (state == RADIOLIB_ERR_NONE) {
    Serial.println("LoRa init success!");
    oledPrint("LoRa pret!\nAttente Rx...");
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
  String receivedMsg;
  // Attente bloquante d'un message (avec timeout de 1 seconde pour garder la boucle active)
  int state = radio.receive(receivedMsg, 1000);
  
  if (state == RADIOLIB_ERR_NONE) {
    float rssi = radio.getRSSI();
    float snr = radio.getSNR();
    Serial.println("RX: " + receivedMsg + " RSSI: " + String(rssi) + " SNR: " + String(snr));
    
    oledPrint("RX: " + receivedMsg + "\nRSSI:" + String(rssi, 1) + " SNR:" + String(snr, 1) + "\nTraitement...");
    
    // Parser le JSON pour trouver la valeur du potentiometre
    JsonDocument doc;
    DeserializationError err = deserializeJson(doc, receivedMsg);
    int pot = 0;
    if (!err) {
      pot = doc["pot"] | 0;
    }
    
    // Appel du LLM
    String llmReply = appelLLM(pot);
    Serial.println("LLM: " + llmReply);
    
    // Renvoi de la reponse a l'emetteur
    int txState = radio.transmit(llmReply);
    
    if (txState == RADIOLIB_ERR_NONE) {
      oledPrint("TX: " + llmReply + "\nSucces!");
    } else {
      oledPrint("TX Err: " + String(txState));
    }
    
    // Petite pause pour bien lire l'ecran
    delay(2000);
    oledPrint("Attente Rx...");
  }
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
// CONNEXION WIFI
// =============================================

void connecterWiFi() {
  Serial.println("Connexion WiFi...");
  oledPrint("WiFi...\n" + String(WIFI_SSID));

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
    Serial.print(".");
    tentatives++;
  }

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\nWiFi connecte : " + WiFi.localIP().toString());
    oledPrint("WiFi OK\n" + WiFi.localIP().toString());
    delay(1000);
  } else {
    Serial.println("\nErreur WiFi !");
    oledPrint("Erreur WiFi\nVerifie config");
    // On ne bloque pas forcement pour que LoRa puisse marcher sans WiFi meme si le LLM va echouer
  }
}

// =============================================
// APPEL API OPENWEBUI / GROQ
// =============================================

String appelLLM(int valeurPot) {
  if (WiFi.status() != WL_CONNECTED) {
    return "{\"action\":\"none\",\"msg\":\"No WiFi\"}";
  }

  HTTPClient http;
  http.begin(OPENWEBUI_URL);
  http.addHeader("Content-Type", "application/json");
  http.addHeader("Authorization", String("Bearer ") + API_KEY);
  http.setTimeout(30000);

  // Construction du JSON
  JsonDocument doc;
  doc["model"] = MODEL_NAME;

  JsonArray messages = doc["messages"].to<JsonArray>();

  JsonObject systemMsg = messages.add<JsonObject>();
  systemMsg["role"]    = "system";
  systemMsg["content"] = SYSTEM_PROMPT;

  JsonObject userMsg = messages.add<JsonObject>();
  userMsg["role"]    = "user";
  userMsg["content"] = "potentiometre: " + String(valeurPot);

  String payload;
  serializeJson(doc, payload);

  Serial.println("Payload: " + payload);

  int httpCode = http.POST(payload);
  String reponse = "";

  if (httpCode == 200) {
    String body = http.getString();

    JsonDocument rep;
    DeserializationError err = deserializeJson(rep, body);

    if (!err) {
      reponse = rep["choices"][0]["message"]["content"].as<String>();
      // Nettoyer d'eventuelles balises markdown (Groq a tendance a encadrer les JSON)
      reponse.replace("```json", "");
      reponse.replace("```", "");
      reponse.trim();
    } else {
      reponse = "{\"action\":\"none\",\"msg\":\"JSON Err\"}";
    }
  } else {
    reponse = "{\"action\":\"none\",\"msg\":\"HTTP " + String(httpCode) + "\"}";
    Serial.println(http.getString());
  }

  http.end();
  return reponse;
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