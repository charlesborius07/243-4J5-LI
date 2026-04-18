#include <RadioLib.h>
#include "soc/soc.h"
#include "soc/rtc_cntl_reg.h"

// Pins SX1262 du T-Supreme
SX1262 radio = new Module(10, 33, 5, 36);

void setup() {
  
  Serial.begin(115200);
  Serial.println("=== EMETTEUR LoRa ===");
  
  // Fréquence 868 MHz (Europe), BW 125 kHz, SF 9, CR 7
  int state = radio.begin(868.0, 125.0, 9, 7, 0x12, 22, 8);
  if (state == RADIOLIB_ERR_NONE) {
    Serial.println("Radio init OK");
  } else {
    Serial.print("Erreur radio: ");
    Serial.println(state);
  }
}

#define POT_PIN 2

void loop() {
  int valeur = analogRead(POT_PIN);
  
  String msg = String(valeur);
  int state = radio.transmit(msg);
  
  if (state == RADIOLIB_ERR_NONE) {
    Serial.println("TX: " + msg);
  } else {
    Serial.print("Erreur TX: ");
    Serial.println(state);
  }
  
  delay(2000);
}