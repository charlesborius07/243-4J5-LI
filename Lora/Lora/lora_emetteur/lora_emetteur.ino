#include <RadioLib.h>

// Pins SX1262 du T-Beam SUPREME (correctes)
// NSS=10, DIO1=1, NRST=5, BUSY=4
// SPI: SCK=12, MISO=13, MOSI=11
SX1262 radio = new Module(10, 1, 5, 4);

void setup() {
  delay(500);
  Serial.begin(115200);
  Serial.println("=== EMETTEUR LoRa ===");
  Serial.flush();
  
  delay(1000);
  
  // Fréquence 868 MHz (Europe), BW 125 kHz, SF 9, CR 7
  // Puissance réduite à 2 dBm pour éviter le brownout
  int state = radio.begin(868.0, 125.0, 9, 7, 0x12, 2, 8);
  if (state == RADIOLIB_ERR_NONE) {
    Serial.println("Radio init OK");
  } else {
    Serial.print("Erreur radio: ");
    Serial.println(state);
  }
  Serial.flush();
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
  Serial.flush();
  
  delay(2000);
}