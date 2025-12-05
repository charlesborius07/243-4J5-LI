// Test basique pour LilyGO A7670E
// Vérifie la communication série et allume la LED

#define LED_PIN 12  // LED intégrée sur le LilyGO

void setup() {
  // Initialiser la communication série
  Serial.begin(115200);
  delay(1000);

  // Configurer la LED
  pinMode(LED_PIN, OUTPUT);

  Serial.println("=========================");
  Serial.println("LilyGO A7670E - Test");
  Serial.println("=========================");
  Serial.println("Démarrage...");
}

void loop() {
  // Faire clignoter la LED
  digitalWrite(LED_PIN, HIGH);
  Serial.println("LED ON");
  delay(1000);

  digitalWrite(LED_PIN, LOW);
  Serial.println("LED OFF");
  delay(1000);
}
