#include "LoRaBoards.h"
#include <RadioLib.h>
#include <U8g2lib.h>
#include <Wire.h>
#include <ArduinoJson.h>
#include <HTTPClient.h>
#include <WiFiClientSecure.h>
#include <PubSubClient.h>
#include "config.h"

#if     defined(USING_SX1276)
SX1276 radio = new Module(RADIO_CS_PIN, RADIO_DIO0_PIN, RADIO_RST_PIN, RADIO_DIO1_PIN);
#elif   defined(USING_SX1278)
SX1278 radio = new Module(RADIO_CS_PIN, RADIO_DIO0_PIN, RADIO_RST_PIN, RADIO_DIO1_PIN);
#elif   defined(USING_SX1262)
SX1262 radio = new Module(RADIO_CS_PIN, RADIO_DIO1_PIN, RADIO_RST_PIN, RADIO_BUSY_PIN);
#elif   defined(USING_SX1268)
SX1268 radio = new Module(RADIO_CS_PIN, RADIO_DIO1_PIN, RADIO_RST_PIN, RADIO_BUSY_PIN);
#elif   defined(USING_SX1280)
SX1280 radio = new Module(RADIO_CS_PIN, RADIO_DIO1_PIN, RADIO_RST_PIN, RADIO_BUSY_PIN);
#elif  defined(USING_SX1280PA)
SX1280 radio = new Module(RADIO_CS_PIN, RADIO_DIO1_PIN, RADIO_RST_PIN, RADIO_BUSY_PIN);
#elif   defined(USING_LR1121)
LR1121 radio = new Module(RADIO_CS_PIN, RADIO_DIO9_PIN, RADIO_RST_PIN, RADIO_BUSY_PIN);
#endif

#ifndef MQTT_BROKER
#define MQTT_BROKER     "mqtt.charlesborius07.com"
#define MQTT_PORT       1883
#define MQTT_TOPIC      "lora/result"
#define MQTT_USER       "telecom"
#define MQTT_PASSWORD   "Teladmin1$"
#endif

U8G2_SH1106_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0, U8X8_PIN_NONE);
WiFiClient wifiClient;
PubSubClient mqttClient(wifiClient);

static volatile bool receivedFlag = false;
static volatile bool transmittedFlag = false;
int potValue = 0;
String packetId = "";

void setRxFlag(void) {
    receivedFlag = true;
}

void setTxFlag(void) {
    transmittedFlag = true;
}

void blinkLed(int times, int ms) {
#ifdef BOARD_LED
    for (int i = 0; i < times; i++) {
        digitalWrite(BOARD_LED, LED_ON);
        delay(ms);
        digitalWrite(BOARD_LED, !LED_ON);
        delay(ms);
    }
#endif
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

String appelLLM(int valeurPot) {
    Serial.println("[LLM] === Debut appel LLM ===");
    if (WiFi.status() != WL_CONNECTED) {
        Serial.println("[LLM] ERREUR: WiFi non connecte!");
        return "Erreur: WiFi";
    }
    
    HTTPClient http;
    http.begin(GROQ_API_URL);
    http.addHeader("Content-Type", "application/json");
    http.addHeader("Authorization", String("Bearer ") + GROQ_API_KEY);
    http.setTimeout(30000);

    JsonDocument doc;
    doc["model"] = GROQ_MODEL;
    doc["temperature"] = 0.7;
    doc["max_tokens"] = 100;
    JsonArray messages = doc["messages"].to<JsonArray>();
    JsonObject systemMsg = messages.add<JsonObject>();
    systemMsg["role"] = "system";
    systemMsg["content"] = SYSTEM_PROMPT;
    JsonObject userMsg = messages.add<JsonObject>();
    userMsg["role"] = "user";
    userMsg["content"] = "Valeur potentiometre: " + String(valeurPot);

    String payload;
    serializeJson(doc, payload);
    int httpCode = http.POST(payload);
    String reponse = "";

    if (httpCode == 200) {
        String body = http.getString();
        JsonDocument rep;
        DeserializationError err = deserializeJson(rep, body);
        if (!err) {
            reponse = rep["choices"][0]["message"]["content"].as<String>();
        } else {
            reponse = "Erreur JSON";
        }
    } else {
        reponse = "Erreur Groq: " + String(httpCode);
    }
    http.end();
    return reponse;
}

void connecterWiFi() {
    Serial.println("Connexion WiFi...");
    drawScreen("WiFi...", WIFI_SSID, "", "");
    WiFi.disconnect(true);
    WiFi.mode(WIFI_STA);
    delay(100);
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    int tentatives = 0;
    while (WiFi.status() != WL_CONNECTED && tentatives < 40) {
        delay(500);
        Serial.print(".");
        tentatives++;
    }
    if (WiFi.status() == WL_CONNECTED) {
        drawScreen("WiFi OK!", WiFi.localIP().toString().c_str(), "", "");
    } else {
        drawScreen("Erreur WiFi", "", "", "");
    }
}

void reconnectMQTT() {
    if (!mqttClient.connected()) {
        if (strlen(MQTT_USER) > 0) {
            mqttClient.connect("lora-receiver", MQTT_USER, MQTT_PASSWORD);
        } else {
            mqttClient.connect("lora-receiver");
        }
    }
}

void publierMQTT(int potValue, const String& llmResponse, int rssi, float snr) {
    if (!mqttClient.connected()) {
        reconnectMQTT();
    }
    if (mqttClient.connected()) {
        JsonDocument doc;
        doc["timestamp"] = millis();
        doc["potValue"] = potValue;
        doc["rssi"] = rssi;
        doc["snr"] = snr;
        doc["llmResponse"] = llmResponse;
        String mqttPayload;
        serializeJson(doc, mqttPayload);
        mqttClient.publish(MQTT_TOPIC, mqttPayload.c_str());
    }
}

void setup() {
    Serial.begin(115200);
    delay(1500);
    setupBoards();
    Wire.begin();
    u8g2.begin();
    u8g2.enableUTF8Print();
    
#ifdef BOARD_LED
    pinMode(BOARD_LED, OUTPUT);
    digitalWrite(BOARD_LED, !LED_ON);
#endif
    
    int state = radio.begin();
    if (state == RADIOLIB_ERR_NONE) {
        radio.setTCXO(1.8);
        radio.setFrequency(868.0);
        radio.setBandwidth(125.0);
        radio.setSpreadingFactor(12);
        radio.setCodingRate(6);
        radio.setSyncWord(0xAB);
        radio.setPreambleLength(16);
        radio.setCRC(false);
        radio.setPacketReceivedAction(setRxFlag);
        radio.setPacketSentAction(setTxFlag);
    } else {
        drawScreen("Erreur Radio", String(state).c_str(), "", "");
        while(1) delay(100);
    }
    
#if !defined(USING_SX1280) && !defined(USING_LR1121) && !defined(USING_SX1280PA)
    radio.setCurrentLimit(140);
#endif
    
    connecterWiFi();
    drawScreen("LoRa RX", "Pret", "868.0 MHz", "Attente...");
    radio.startReceive();
}

void loop() {
    if (receivedFlag) {
        receivedFlag = false;
        String receivedData;
        int state = radio.readData(receivedData);
        if (state == RADIOLIB_ERR_NONE) {
            int rssi = radio.getRSSI();
            float snr = radio.getSNR();
            drawScreen("RX OK!", receivedData.c_str(), "LLM en cours...", "");
            
            StaticJsonDocument<128> doc;
            DeserializationError err = deserializeJson(doc, receivedData);
            if (!err && doc.containsKey("pot")) {
                potValue = doc["pot"].as<int>();
                String llmResponse = appelLLM(potValue);
                
                String action = "observer";
                String status = "ok";
                String actionLower = llmResponse;
                actionLower.toLowerCase();
                if (actionLower.indexOf("eteint") >= 0 || actionLower.indexOf("off") >= 0) {
                    status = "warning"; action = "eteindre";
                } else if (actionLower.indexOf("allume") >= 0 || actionLower.indexOf("on") >= 0) {
                    status = "ok"; action = "allumer";
                }

                publierMQTT(potValue, llmResponse, rssi, snr);

                String responsePayload = "{\"from\":\"rx\",\"pot\":" + String(potValue) + ",\"status\":\"" + status + "\",\"action\":\"" + action + "\",\"llm\":\"" + llmResponse + "\"}";
                drawScreen(("Action: " + action).c_str(), ("Status: " + status).c_str(), "Envoi reponse...", "");
                
                radio.transmit(responsePayload.c_str());
                drawScreen("Reponse envoyee!", ("Action: " + action).c_str(), "En attente...", "");
            }
        }
        radio.startReceive();
    }
    mqttClient.loop();
    delay(10);
}
