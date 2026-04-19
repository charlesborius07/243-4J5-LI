#include "LoRaBoards.h"
#include <RadioLib.h>
#include <U8g2lib.h>
#include <Wire.h>
#include <ArduinoJson.h>

#if     defined(USING_SX1262)
SX1262 radio = new Module(RADIO_CS_PIN, RADIO_DIO1_PIN, RADIO_RST_PIN, RADIO_BUSY_PIN);
#endif

// Pins corrects pour T-Beam S3 Supreme
#define STATUS_LED 7
#define PIN_BTN 0
#define PIN_POT 2

U8G2_SH1106_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0, U8X8_PIN_NONE);

static volatile bool receivedFlag = false;
static volatile bool transmittedFlag = false;

void setTxFlag(void) { transmittedFlag = true; }
void setRxFlag(void) { receivedFlag = true; }

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
    // 1. Initialisation immediate de la LED pour debug visuel
    pinMode(STATUS_LED, OUTPUT);
    for(int i=0; i<3; i++) {
        digitalWrite(STATUS_LED, HIGH);
        delay(100);
        digitalWrite(STATUS_LED, LOW);
        delay(100);
    }

    Serial.begin(115200);
    delay(1000);
    Serial.println("\n\n--- EMETTEUR START ---");

    // 2. Initialisation Hardware LilyGo (Power, Display, etc.)
    setupBoards(); 
    Serial.println("setupBoards() OK");

    // 3. Initialisation OLED (SH1106)
    Wire.begin();
    u8g2.begin();
    
    pinMode(PIN_BTN, INPUT_PULLUP);
    pinMode(PIN_POT, INPUT);
    analogReadResolution(12);

    // 4. Initialisation LoRa
    Serial.println("Initialisation Radio...");
    int state = radio.begin();
    if (state == RADIOLIB_ERR_NONE) {
        Serial.println("Radio OK");
        radio.setTCXO(1.8);
        radio.setFrequency(868.0);
        radio.setBandwidth(125.0);
        radio.setSpreadingFactor(12);
        radio.setSyncWord(0xAB);
        radio.setPacketSentAction(setTxFlag);
        radio.setPacketReceivedAction(setRxFlag);
        radio.startReceive();
    } else {
        Serial.printf("Erreur Radio: %d\n", state);
        drawScreen("Erreur Radio", String(state).c_str(), "", "");
    }

    drawScreen("LoRa TX", "Pret", "868.0 MHz", "Bouton: GPIO0");
    Serial.println("Init Terminee");
}

void loop() {
    static bool lastBtn = HIGH;
    static bool enAttente = false;
    static unsigned long startWait = 0;
    
    bool currentBtn = digitalRead(PIN_BTN);

    if (currentBtn == LOW && lastBtn == HIGH) {
        Serial.println("Bouton APPUYE!");
        digitalWrite(STATUS_LED, HIGH);
        
        int val = analogRead(PIN_POT);
        String txData = "{\"pot\":" + String(val) + "}";
        Serial.print("Envoi: ");
        Serial.println(txData);
        
        drawScreen("Envoi...", txData.c_str(), "", "");
        
        int state = radio.startTransmit(txData.c_str());
        if (state == RADIOLIB_ERR_NONE) {
            enAttente = true;
            startWait = millis();
        } else {
            Serial.printf("Erreur TX: %d\n", state);
            digitalWrite(STATUS_LED, LOW);
        }
    }
    lastBtn = currentBtn;

    if (transmittedFlag) {
        transmittedFlag = false;
        digitalWrite(STATUS_LED, LOW);
        Serial.println("TX Termine");
        drawScreen("TX OK", "Attente reponse...", "", "");
        radio.startReceive();
    }

    if (receivedFlag) {
        receivedFlag = false;
        String rxData;
        radio.readData(rxData);
        Serial.print("RX: ");
        Serial.println(rxData);
        drawScreen("RECU!", rxData.c_str(), "", "");
        enAttente = false;
        radio.startReceive();
    }

    if (enAttente && (millis() - startWait > 20000)) {
        Serial.println("Timeout");
        enAttente = false;
        digitalWrite(STATUS_LED, LOW);
        drawScreen("LoRa TX", "Timeout", "Appuie pour reessayer", "");
        radio.startReceive();
    }
    
    delay(10);
}
