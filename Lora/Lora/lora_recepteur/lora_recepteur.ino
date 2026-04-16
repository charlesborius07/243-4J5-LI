#define LED_STATUS 25  // clignote à chaque réception / appel LLM

void loop() {
  String received;
  if (radio.receive(received) == RADIOLIB_ERR_NONE) {
    digitalWrite(LED_STATUS, HIGH);
    oledPrint("RX: " + received +
              "\nRSSI: " + String(radio.getRSSI()));
    processMessage(received);
    digitalWrite(LED_STATUS, LOW);
  }
}

#include <WiFi.h>
#include "config.h"   // WIFI_SSID, WIFI_PASS, GROQ_API_KEY

void setupWiFi() {
  WiFi.begin(WIFI_SSID, WIFI_PASS);
  while (WiFi.status() != WL_CONNECTED) delay(500);
  oledPrint("WiFi OK\n" + WiFi.localIP().toString());
}

// Schéma JSON (exemple — à adapter) — structured output OBLIGATOIRE
const char* SCHEMA = R"({
  "type":"json_schema",
  "json_schema":{
    "name":"decision","strict":true,
    "schema":{
      "type":"object","additionalProperties":false,
      "required":["status","action"],
      "properties":{
        "status":{"enum":["normal","attention","urgent"]},
        "action":{"enum":["on","off","none"]}}}}})";

String callLLM(String data) {
  JsonDocument req;
  req["model"] = LLM_MODEL;
  deserializeJson(req["response_format"].to<JsonObject>(), SCHEMA);
  auto m = req["messages"].to<JsonArray>();
}

void processMessage(String received) {
  String resp = callLLM(received);

  // Extraire {status, action} du JSON retourné par le LLM
  JsonDocument doc;
  deserializeJson(doc, resp);
  String content = doc["choices"][0]["message"]["content"];
  JsonDocument decision;
  deserializeJson(decision, content);

  // Renvoyer la décision à l'émetteur via LoRa
  String reply; serializeJson(decision, reply);
  radio.transmit(reply);
  oledPrint("LLM: " + reply);

  // Publier sur MQTT (semaine 12)
}
