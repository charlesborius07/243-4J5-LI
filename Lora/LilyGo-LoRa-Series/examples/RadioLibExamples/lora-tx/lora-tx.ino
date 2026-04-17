/*
   LoRa Transmitter - Emetteur avec potentiometre et reponse du recepteur

   Fonctionnement:
   - Lit la valeur du potentiometre
   - Transmet la valeur via LoRa (JSON)
   - Affiche la trame sur OLED
   - Recoit la decision du recepteur (action/status LLM)
   - Affiche la decision sur OLED et DEL
*/

#include "LoRaBoards.h"
#include <RadioLib.h>
#include <U8g2lib.h>
#include <Wire.h>
#include <ArduinoJson.h>

#if     defined(USING_SX1276)
#ifndef CONFIG_RADIO_FREQ
#define CONFIG_RADIO_FREQ           868.0
#endif
#ifndef CONFIG_RADIO_OUTPUT_POWER
#define CONFIG_RADIO_OUTPUT_POWER   17
#endif
#ifndef CONFIG_RADIO_BW
#define CONFIG_RADIO_BW             125.0
#endif
SX1276 radio = new Module(RADIO_CS_PIN, RADIO_DIO0_PIN, RADIO_RST_PIN, RADIO_DIO1_PIN);

#elif   defined(USING_SX1278)
#ifndef CONFIG_RADIO_FREQ
#define CONFIG_RADIO_FREQ           433.0
#endif
#ifndef CONFIG_RADIO_OUTPUT_POWER
#define CONFIG_RADIO_OUTPUT_POWER   17
#endif
#ifndef CONFIG_RADIO_BW
#define CONFIG_RADIO_BW             125.0
#endif
SX1278 radio = new Module(RADIO_CS_PIN, RADIO_DIO0_PIN, RADIO_RST_PIN, RADIO_DIO1_PIN);

#elif   defined(USING_SX1262)
#ifndef CONFIG_RADIO_FREQ
#define CONFIG_RADIO_FREQ           850.0
#endif
#ifndef CONFIG_RADIO_OUTPUT_POWER
#define CONFIG_RADIO_OUTPUT_POWER   22
#endif
#ifndef CONFIG_RADIO_BW
#define CONFIG_RADIO_BW             125.0
#endif
SX1262 radio = new Module(RADIO_CS_PIN, RADIO_DIO1_PIN, RADIO_RST_PIN, RADIO_BUSY_PIN);

#elif   defined(USING_SX1268)
#ifndef CONFIG_RADIO_FREQ
#define CONFIG_RADIO_FREQ           433.0
#endif
#ifndef CONFIG_RADIO_OUTPUT_POWER
#define CONFIG_RADIO_OUTPUT_POWER   22
#endif
#ifndef CONFIG_RADIO_BW
#define CONFIG_RADIO_BW             125.0
#endif
SX1268 radio = new Module(RADIO_CS_PIN, RADIO_DIO1_PIN, RADIO_RST_PIN, RADIO_BUSY_PIN);

#elif   defined(USING_SX1280)
#ifndef CONFIG_RADIO_FREQ
#define CONFIG_RADIO_FREQ           2400.0
#endif
#ifndef CONFIG_RADIO_OUTPUT_POWER
#define CONFIG_RADIO_OUTPUT_POWER   13
#endif
#ifndef CONFIG_RADIO_BW
#define CONFIG_RADIO_BW             203.125
#endif
SX1280 radio = new Module(RADIO_CS_PIN, RADIO_DIO1_PIN, RADIO_RST_PIN, RADIO_BUSY_PIN);

#elif  defined(USING_SX1280PA)
#ifndef CONFIG_RADIO_FREQ
#define CONFIG_RADIO_FREQ           2400.0
#endif
#ifndef CONFIG_RADIO_OUTPUT_POWER
#define CONFIG_RADIO_OUTPUT_POWER   3
#endif
#ifndef CONFIG_RADIO_BW
#define CONFIG_RADIO_BW             203.125
#endif
SX1280 radio = new Module(RADIO_CS_PIN, RADIO_DIO1_PIN, RADIO_RST_PIN, RADIO_BUSY_PIN);

#elif   defined(USING_LR1121)
#define CONFIG_RADIO_FREQ           2450.0
#define CONFIG_RADIO_OUTPUT_POWER   LILYGO_RADIO_2G4_TX_POWER_LIMIT
#define CONFIG_RADIO_BW             125.0
LR1121 radio = new Module(RADIO_CS_PIN, RADIO_DIO9_PIN, RADIO_RST_PIN, RADIO_BUSY_PIN);
#endif

#define PIN_POT         2
#define PIN_BTN         0
#define STATUS_LED      45

U8G2_SH1106_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0, U8X8_PIN_NONE);

static volatile bool receivedFlag = false;
static volatile bool transmittedFlag = false;
static int transmissionState = RADIOLIB_ERR_NONE;
static int receptionState = RADIOLIB_ERR_NONE;

String txPayload = "";
String rxPayload = "";
String rxRssi = "";
String rxSnr = "";

void setTxFlag(void) {
    transmittedFlag = true;
}

void setRxFlag(void) {
    receivedFlag = true;
}

void drawScreen(const char* line1, const char* line2, const char* line3, const char* line4) {
    u8g2.clearBuffer();
    u8g2.setFont(u8g2_font_helvB10_tf);
    u8g2.drawUTF8(0, 15, line1);
    
    u8g2.setFont(u8g2_font_helvR08_tf);
    if (line2) u8g2.drawUTF8(0, 30, line2);
    if (line3) u8g2.drawUTF8(0, 45, line3);
    if (line4) u8g2.drawUTF8(0, 60, line4);
    
    u8g2.sendBuffer();
}

void setup() {
    Serial.begin(115200);
    delay(1500);

    setupBoards();

    Wire.begin();
    u8g2.begin();
    u8g2.enableUTF8Print();

    pinMode(PIN_BTN, INPUT_PULLUP);
    pinMode(PIN_POT, INPUT);
    pinMode(STATUS_LED, OUTPUT);
    analogReadResolution(12);

    int state = radio.begin();
    Serial.printf("[%s]: Radio Initializing ... %s\n", RADIO_TYPE_STR,
                  state == RADIOLIB_ERR_NONE ? "success!" : "failed");

    radio.setPacketSentAction(setTxFlag);
    radio.setPacketReceivedAction(setRxFlag);

    radio.setFrequency(CONFIG_RADIO_FREQ);
    radio.setBandwidth(CONFIG_RADIO_BW);
    radio.setSpreadingFactor(12);
    radio.setCodingRate(6);
    radio.setSyncWord(0xAB);
    radio.setOutputPower(CONFIG_RADIO_OUTPUT_POWER);
    radio.setPreambleLength(16);
    radio.setCRC(false);

#if !defined(USING_SX1280) && !defined(USING_LR1121) && !defined(USING_SX1280PA)
    radio.setCurrentLimit(140);
#endif

    drawScreen("LoRa TX", "Pret", "Appuie pour envoyer", "");
    Serial.println("LoRa TX Pret - Appuie sur le bouton pour envoyer");
}

void loop() {
    static bool btnPrecedent = HIGH;
    static bool enAttente = false;
    static int dernierPotAffiche = -1;
    bool btnActuel = digitalRead(PIN_BTN);

    if (!enAttente) {
        int valeurPot = analogRead(PIN_POT);
        
        if (abs(valeurPot - dernierPotAffiche) > 20 || dernierPotAffiche == -1) {
            dernierPotAffiche = valeurPot;
            char buf[32];
            snprintf(buf, sizeof(buf), "Pot: %d", valeurPot);
            drawScreen("LoRa TX", buf, "Appuie pour envoyer", "");
        }

        if (btnPrecedent == HIGH && btnActuel == LOW) {
            digitalWrite(STATUS_LED, HIGH);
            txPayload = "{\"pot\":" + String(valeurPot) + ",\"id\":" + String(millis()) + "}";
            
            Serial.println("=== TRANSMISSION ===");
            Serial.println("Pot: " + String(valeurPot));
            Serial.println("TX: " + txPayload);
            
            char buf[32];
            snprintf(buf, sizeof(buf), "TX: %d", valeurPot);
            drawScreen("Envoi...", buf, txPayload.c_str(), "");
            
            enAttente = true;
            transmissionState = radio.startTransmit(txPayload.c_str());
        }
    }

    if (transmittedFlag) {
        transmittedFlag = false;
        digitalWrite(STATUS_LED, LOW);
        
        if (transmissionState == RADIOLIB_ERR_NONE) {
            Serial.println("TX OK - En attente reponse...");
            drawScreen("TX OK", "Attente reponse...", "", "");
        } else {
            Serial.print("TX Erreur: ");
            Serial.println(transmissionState);
            drawScreen("TX Erreur", String(transmissionState).c_str(), "", "");
        }
        
        radio.startReceive();
    }

    if (receivedFlag) {
        receivedFlag = false;
        receptionState = radio.readData(rxPayload);

        if (receptionState == RADIOLIB_ERR_NONE) {
            int rssi = radio.getRSSI();
            float snr = radio.getSNR();

            rxRssi = String(rssi) + " dBm";
            rxSnr = String(snr, 1) + " dB";

            Serial.println("=== RECEPTION ===");
            Serial.println("RX: " + rxPayload);
            Serial.println("RSSI: " + rxRssi);
            Serial.println("SNR: " + rxSnr);

            StaticJsonDocument<256> doc;
            DeserializationError err = deserializeJson(doc, rxPayload);

            String action = "";
            String status = "";
            String llmResponse = "";

            if (!err) {
                if (doc.containsKey("action")) action = doc["action"].as<String>();
                if (doc.containsKey("status")) status = doc["status"].as<String>();
                if (doc.containsKey("llm")) llmResponse = doc["llm"].as<String>();

                Serial.println("Action: " + action);
                Serial.println("Status: " + status);
                Serial.println("LLM: " + llmResponse);
            }
            Serial.println("=================");

            if (action == "allumer") {
                digitalWrite(STATUS_LED, HIGH);
            } else if (action == "eteindre") {
                digitalWrite(STATUS_LED, LOW);
            }

            char buf1[32], buf2[32], buf3[32], buf4[32];
            snprintf(buf1, sizeof(buf1), "Action: %s", action.length() > 0 ? action.c_str() : "---");
            snprintf(buf2, sizeof(buf2), "Status: %s", status.length() > 0 ? status.c_str() : "---");
            snprintf(buf3, sizeof(buf3), "RSSI: %s", rxRssi.c_str());
            snprintf(buf4, sizeof(buf4), "SNR: %s", rxSnr.c_str());
            drawScreen(buf1, buf2, buf3, buf4);
        }

        enAttente = false;
    }

    btnPrecedent = btnActuel;
    delay(50);
}
