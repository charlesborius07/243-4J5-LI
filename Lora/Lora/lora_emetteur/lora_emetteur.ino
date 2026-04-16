#include <RadioLib.h>

// Pins SX1262 du T-Beam Supreme
SX1262 radio = new Module(10, 33, 5, 36);

void setup() {
  Serial.begin(115200);
  // freq, BW, SF, CR, sync, puissance, preamble
  radio.begin(915.0, 125.0, 9, 7, 0x12, 22, 8);
}

#define POT_PIN 2
#define LED_PIN 25

void loop() {
  int valeur = analogRead(POT_PIN);
  radio.transmit(String(valeur));
  Serial.println("TX: " + String(valeur));
  delay(5000);
}

#include <ArduinoJson.h>

void loop() {
  JsonDocument doc;
  doc["pot"] = analogRead(POT_PIN);
  String msg; serializeJson(doc, msg);
  radio.transmit(msg);
  oledPrint("TX: " + msg);

  // Écouter la décision du LLM (retour LoRa)
  String reply;
  if (radio.receive(reply, 10000) == RADIOLIB_ERR_NONE) {
    JsonDocument r; deserializeJson(r, reply);
    String action = r["action"] | "none";
    digitalWrite(LED_ACTION, action == "on" ? HIGH : LOW);
    oledPrint("TX: " + msg + "\nLLM: " + reply);
  }
  delay(5000);
}
