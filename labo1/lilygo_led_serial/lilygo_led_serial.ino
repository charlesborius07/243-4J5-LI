#define LED_VERT 12
#define LED_ROUGE 15

void setup() {
  Serial.begin(115200);
  
  pinMode(LED_VERT, OUTPUT);
  pinMode(LED_ROUGE, OUTPUT);
  
  // Test au démarrage : tout allumé 1sec
  digitalWrite(LED_VERT, HIGH);
  digitalWrite(LED_ROUGE, HIGH);
  delay(1000);
  digitalWrite(LED_VERT, LOW);
  digitalWrite(LED_ROUGE, LOW);
  
  Serial.println("READY");
}

void loop() {
  if (Serial.available() > 0) {
    String command = Serial.readStringUntil('\n');
    command.trim(); // Enlever les espaces/sauts de ligne

    if (command == "VERT") {
      digitalWrite(LED_VERT, HIGH);
      digitalWrite(LED_ROUGE, LOW);
      Serial.println("OK: VERT ON");
    } 
    else if (command == "ROUGE") {
      digitalWrite(LED_VERT, LOW);
      digitalWrite(LED_ROUGE, HIGH);
      Serial.println("OK: ROUGE ON");
    }
  }
}
