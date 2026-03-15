#include <Wire.h>
#include <Adafruit_MPU6050.h> // Nécessite la bibliothèque Adafruit MPU6050
#include <Adafruit_Sensor.h>

// Définition des broches
const int ledPins[] = {12, 13, 14, 15};
const int potPins[] = {26, 27};
const int buttonPin = 34;

Adafruit_MPU6050 mpu;

void setup() {
  Serial.begin(115200);
  
  // Configuration des LEDs
  for (int i = 0; i < 4; i++) {
    pinMode(ledPins[i], OUTPUT);
  }

  // Configuration du bouton (Pull-up interne)
  pinMode(buttonPin, INPUT_PULLUP);

  // Initialisation I2C pour l'accéléromètre
  if (!mpu.begin()) {
    Serial.println("Erreur: Accéléromètre non détecté !");
  } else {
    Serial.println("Accéléromètre prêt.");
  }
}

void loop() {
  // 1. Lecture des Potentiomètres
  int valPot1 = analogRead(potPins[0]);
  int valPot2 = analogRead(potPins[1]);

  // 2. Lecture de l'accéléromètre
  sensors_event_t a, g, temp;
  mpu.getEvent(&a, &g, &temp);

  // 3. Lecture du bouton (LOW si pressé avec INPUT_PULLUP)
  bool buttonState = digitalRead(buttonPin) == LOW;

  // 4. Animation des LEDs (chenillard rapide)
  for (int i = 0; i < 4; i++) {
    digitalWrite(ledPins[i], HIGH);
    delay(50);
    digitalWrite(ledPins[i], LOW);
  }

  // 5. Affichage des données
  Serial.print("Pots: "); Serial.print(valPot1); Serial.print(" | "); Serial.println(valPot2);
  Serial.print("Accel X: "); Serial.print(a.acceleration.x); 
  Serial.print(" Y: "); Serial.print(a.acceleration.y);
  Serial.print(" Z: "); Serial.println(a.acceleration.z);
  
  if (buttonState) {
    Serial.println("--- BOUTON PRESSÉ ---");
    // Allume toutes les LEDs si on appuie
    for(int i=0; i<4; i++) digitalWrite(ledPins[i], HIGH);
    delay(200);
  }

  delay(200);
}
