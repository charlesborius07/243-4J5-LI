/*
   LoRa Receiver - Recepteur avec appel Groq LLM et MQTT

   Fonctionnement:
   - Recoit la trame LoRa (JSON)
   - Mesure RSSI/SNR et affiche sur OLED
   - DEL status clignote a chaque reception
   - Appelle Groq API
   - Publie le resultat sur MQTT (status, action)
   - Retourne la reponse via LoRa

   Modeles Groq disponibles:
   - llama-3.1-8b-instant (rapide, gratuit)
   - llama-3.2-1b-preview
   - llama-3.2-3b-preview
   - mixtral-8x7b-32768
   - gemma2-9b-it
*/

#include "LoRaBoards.h"
#include <RadioLib.h>
#include <U8g2lib.h>
#include <Wire.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>

#if     defined(USING_SX1276)
#ifndef CONFIG_RADIO_FREQ
#define CONFIG_RADIO_FREQ           868.0
#endif
#ifndef CONFIG_RADIO_BW
#define CONFIG_RADIO_BW             125.0
#endif
SX1276 radio = new Module(RADIO_CS_PIN, RADIO_DIO0_PIN, RADIO_RST_PIN, RADIO_DIO1_PIN);

#elif   defined(USING_SX1278)
#ifndef CONFIG_RADIO_FREQ
#define CONFIG_RADIO_FREQ           433.0
#endif
#ifndef CONFIG_RADIO_BW
#define CONFIG_RADIO_BW             125.0
#endif
SX1278 radio = new Module(RADIO_CS_PIN, RADIO_DIO0_PIN, RADIO_RST_PIN, RADIO_DIO1_PIN);

#elif   defined(USING_SX1262)
#ifndef CONFIG_RADIO_FREQ
#define CONFIG_RADIO_FREQ           850.0
#endif
#ifndef CONFIG_RADIO_BW
#define CONFIG_RADIO_BW             125.0
#endif
SX1262 radio = new Module(RADIO_CS_PIN, RADIO_DIO1_PIN, RADIO_RST_PIN, RADIO_BUSY_PIN);

#elif   defined(USING_SX1268)
#ifndef CONFIG_RADIO_FREQ
#define CONFIG_RADIO_FREQ           433.0
#endif
#ifndef CONFIG_RADIO_BW
#define CONFIG_RADIO_BW             125.0
#endif
SX1268 radio = new Module(RADIO_CS_PIN, RADIO_DIO1_PIN, RADIO_RST_PIN, RADIO_BUSY_PIN);

#elif   defined(USING_SX1280)
#ifndef CONFIG_RADIO_FREQ
#define CONFIG_RADIO_FREQ           2400.0
#endif
#ifndef CONFIG_RADIO_BW
#define CONFIG_RADIO_BW             203.125
#endif
SX1280 radio = new Module(RADIO_CS_PIN, RADIO_DIO1_PIN, RADIO_RST_PIN, RADIO_BUSY_PIN);

#elif  defined(USING_SX1280PA)
#ifndef CONFIG_RADIO_FREQ
#define CONFIG_RADIO_FREQ           2400.0
#endif
#ifndef CONFIG_RADIO_BW
#define CONFIG_RADIO_BW             203.125
#endif
SX1280 radio = new Module(RADIO_CS_PIN, RADIO_DIO1_PIN, RADIO_RST_PIN, RADIO_BUSY_PIN);

#elif   defined(USING_LR1121)
#define CONFIG_RADIO_FREQ           2450.0
#define CONFIG_RADIO_BW             125.0
LR1121 radio = new Module(RADIO_CS_PIN, RADIO_DIO9_PIN, RADIO_RST_PIN, RADIO_BUSY_PIN);
#endif

#define STATUS_LED      45

#ifndef WIFI_SSID
#define WIFI_SSID       "1975 grandville"
#define WIFI_PASSWORD   "grandville1975"
#endif

#ifndef GROQ_API_URL
#define GROQ_API_URL    "https://api.groq.com/openai/v1/chat/completions"
#endif

#ifndef GROQ_API_KEY
#define GROQ_API_KEY    "REMOVED"
#endif

#ifndef GROQ_MODEL
#define GROQ_MODEL      "llama-3.1-8b-instant"
#endif

#ifndef SYSTEM_PROMPT
#define SYSTEM_PROMPT   "Tu es un assistant IoT. Reponds en une phrase courte et en francais."
#endif

#ifndef MQTT_BROKER
#define MQTT_BROKER     "mqtt.charlesborius07.com"
#define MQTT_PORT       443
#define MQTT_TOPIC      "lora/result"
#define MQTT_USER       "telecom"
#define MQTT_PASSWORD   "Teladmin1$"
#endif

U8G2_SH1106_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0, U8X8_PIN_NONE);

WiFiClient espClient;
PubSubClient mqttClient(espClient);

static volatile bool receivedFlag = false;
static volatile bool transmittedFlag = false;
static int potValue = 0;
static String packetId = "";

void setRxFlag(void) {
    receivedFlag = true;
}

void setTxFlag(void) {
    transmittedFlag = true;
}

void blinkLed(int times, int delayMs) {
    for (int i = 0; i < times; i++) {
        digitalWrite(STATUS_LED, HIGH);
        delay(delayMs);
        digitalWrite(STATUS_LED, LOW);
        delay(delayMs);
    }
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

void drawWrapText(const char* text, int startY) {
    u8g2.setFont(u8g2_font_5x7_tf);
    int y = startY;
    int maxWidth = 128;
    const char* p = text;
    
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
        y += 9;
    }
}

String appelLLM(int valeurPot) {
    if (WiFi.status() != WL_CONNECTED) {
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

    Serial.println("Payload: " + payload);

    int httpCode = http.POST(payload);
    String reponse = "";

    if (httpCode == 200) {
        String body = http.getString();
        Serial.println("Groq Response: " + body);

        JsonDocument rep;
        DeserializationError err = deserializeJson(rep, body);

        if (!err) {
            reponse = rep["choices"][0]["message"]["content"].as<String>();
        } else {
            reponse = "Erreur JSON";
        }
    } else {
        Serial.println("Groq Error: " + http.getString());
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
        Serial.println("\nWiFiOK: " + WiFi.localIP().toString());
        drawScreen("WiFi OK!", WiFi.localIP().toString().c_str(), "", "");
    } else {
        Serial.println("\nErreur WiFi!");
        drawScreen("Erreur WiFi", "", "", "");
    }
}

void reconnectMQTT() {
    if (!mqttClient.connected()) {
        Serial.print("MQTT connexion...");
        if (strlen(MQTT_USER) > 0) {
            mqttClient.connect("lora-receiver", MQTT_USER, MQTT_PASSWORD);
        } else {
            mqttClient.connect("lora-receiver");
        }

        if (mqttClient.connected()) {
            Serial.println("OK");
        } else {
            Serial.print("ECHEC ");
            Serial.println(mqttClient.state());
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

        String actionLower = llmResponse;
        actionLower.toLowerCase();

        if (actionLower.indexOf("eteint") >= 0 || actionLower.indexOf("off") >= 0) {
            doc["status"] = "warning";
            doc["action"] = "eteindre";
        } else if (actionLower.indexOf("allume") >= 0 || actionLower.indexOf("on") >= 0 || actionLower.indexOf("active") >= 0) {
            doc["status"] = "ok";
            doc["action"] = "allumer";
        } else if (actionLower.indexOf("augmente") >= 0 || actionLower.indexOf("plus") >= 0) {
            doc["status"] = "ok";
            doc["action"] = "augmenter";
        } else if (actionLower.indexOf("diminue") >= 0 || actionLower.indexOf("moins") >= 0 || actionLower.indexOf("reduire") >= 0) {
            doc["status"] = "warning";
            doc["action"] = "diminuer";
        } else if (actionLower.startsWith("erreur")) {
            doc["status"] = "error";
            doc["action"] = "aucune";
        } else {
            doc["status"] = "ok";
            doc["action"] = "observer";
        }

        String mqttPayload;
        serializeJson(doc, mqttPayload);

        Serial.println("MQTT: " + mqttPayload);
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
    
    pinMode(STATUS_LED, OUTPUT);
    digitalWrite(STATUS_LED, LOW);
    
    int state = radio.begin();
    Serial.printf("[%s]: Radio Init ... %s\n", RADIO_TYPE_STR,
                  state == RADIOLIB_ERR_NONE ? "OK" : "ERREUR");
    
    radio.setPacketReceivedAction(setRxFlag);
    radio.setPacketSentAction(setTxFlag);
    
    radio.setFrequency(CONFIG_RADIO_FREQ);
    radio.setBandwidth(CONFIG_RADIO_BW);
    radio.setSpreadingFactor(12);
    radio.setCodingRate(6);
    radio.setSyncWord(0xAB);
    radio.setPreambleLength(16);
    radio.setCRC(false);
    
#if !defined(USING_SX1280) && !defined(USING_LR1121) && !defined(USING_SX1280PA)
    radio.setCurrentLimit(140);
#endif
    
    connecterWiFi();

    mqttClient.setServer(MQTT_BROKER, MQTT_PORT);
    reconnectMQTT();

    Serial.println("LoRa RX Pret - En attente...");
    radio.startReceive();
    
    blinkLed(2, 200);
}

void loop() {
    if (receivedFlag) {
        receivedFlag = false;
        
        String receivedData;
        int state = radio.readData(receivedData);
        
        blinkLed(3, 100);
        
        if (state == RADIOLIB_ERR_NONE) {
            int rssi = radio.getRSSI();
            float snr = radio.getSNR();
            
            Serial.println("=== RECEPTION ===");
            Serial.println("RX: " + receivedData);
            Serial.println("RSSI: " + String(rssi) + " dBm");
            Serial.println("SNR: " + String(snr, 1) + " dB");
            
            drawScreen("RX OK!", receivedData.c_str(),
                      ("RSSI:" + String(rssi) + " SNR:" + String(snr, 1)).c_str(),
                      "LLM en cours...");
            
            StaticJsonDocument<64> doc;
            DeserializationError err = deserializeJson(doc, receivedData);
            
            if (!err && doc.containsKey("pot")) {
                potValue = doc["pot"].as<int>();
                if (doc.containsKey("id")) {
                    packetId = doc["id"].as<String>();
                }
                
                Serial.println("Pot value: " + String(potValue));

                String llmResponse = appelLLM(potValue);
                Serial.println("LLM Response: " + llmResponse);

                String action = "observer";
                String status = "ok";

                String actionLower = llmResponse;
                actionLower.toLowerCase();

                if (actionLower.indexOf("eteint") >= 0 || actionLower.indexOf("off") >= 0) {
                    status = "warning";
                    action = "eteindre";
                } else if (actionLower.indexOf("allume") >= 0 || actionLower.indexOf("on") >= 0 || actionLower.indexOf("active") >= 0) {
                    status = "ok";
                    action = "allumer";
                } else if (actionLower.indexOf("augmente") >= 0 || actionLower.indexOf("plus") >= 0) {
                    status = "ok";
                    action = "augmenter";
                } else if (actionLower.indexOf("diminue") >= 0 || actionLower.indexOf("moins") >= 0 || actionLower.indexOf("reduire") >= 0) {
                    status = "warning";
                    action = "diminuer";
                } else if (actionLower.startsWith("erreur")) {
                    status = "error";
                    action = "aucune";
                }

                publierMQTT(potValue, llmResponse, rssi, snr);

                String responsePayload = "{";
                responsePayload += "\"from\":\"rx\",";
                responsePayload += "\"pot\":" + String(potValue) + ",";
                responsePayload += "\"rssi\":" + String(rssi) + ",";
                responsePayload += "\"snr\":" + String(snr, 1) + ",";
                responsePayload += "\"status\":\"" + status + "\",";
                responsePayload += "\"action\":\"" + action + "\",";
                responsePayload += "\"llm\":\"" + llmResponse + "\"";
                responsePayload += "}";

                Serial.println("TX Response: " + responsePayload);

                char buf1[32], buf2[32];
                snprintf(buf1, sizeof(buf1), "Action: %s", action.c_str());
                snprintf(buf2, sizeof(buf2), "Status: %s", status.c_str());
                drawScreen(buf1, buf2, "Envoi...", "");

                radio.startTransmit(responsePayload.c_str());
            }
        } else {
            Serial.print("RX Erreur: ");
            Serial.println(state);
            drawScreen("RX Erreur", String(state).c_str(), "", "");
            radio.startReceive();
        }
    }
    
    if (transmittedFlag) {
        transmittedFlag = false;
        
        Serial.println("TX Response OK!");
        blinkLed(2, 50);
        drawScreen("Reponse envoyee!", "", "En attente...", "");
        
        radio.startReceive();
    }

    mqttClient.loop();
    delay(10);
}
