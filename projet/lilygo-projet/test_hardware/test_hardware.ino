#include <Arduino.h>
#include <Wire.h>

// Définition des broches (Pins)
const int LED_PINS[] = {12, 13, 14, 15};
const int NUM_LEDS = 4;
const int POT1_PIN = 26;
const int POT2_PIN = 27;
const int BUTTON_PIN = 34;

// I2C Acceleromètre MPU6886 (Adresse standard 0x68)
const int I2C_SDA = 22;
const int I2C_SCL = 21;
const uint8_t MPU6886_ADDR = 0x68;

// Variables globales pour le chenillard (LEDs)
int currentLed = 0;
unsigned long lastLedTime = 0;
const unsigned long LED_INTERVAL = 200; // 200 ms entre chaque LED

// Variables pour l'affichage sur le moniteur série
unsigned long lastPrintTime = 0;
const unsigned long PRINT_INTERVAL = 250; // Affichage toutes les 250 ms

void setup() {
  // 1. Initialisation du port série
  Serial.begin(115200);
  delay(1000); // Attendre que le port série soit prêt (recommandé pour l'ESP32)
  Serial.println("\n--- Démarrage du Test Matériel ---");

  // 2. Configuration des LEDs
  for (int i = 0; i < NUM_LEDS; i++) {
    pinMode(LED_PINS[i], OUTPUT);
    digitalWrite(LED_PINS[i], LOW); // S'assurer que les LEDs sont éteintes au départ
  }

  // 3. Configuration du bouton 
  // La broche 34 de l'ESP32 est en entrée uniquement et n'a pas de résistance de pull-up interne.
  // Vous avez précisé utiliser une résistance de pull-up externe, on configure donc simplement en INPUT.
  pinMode(BUTTON_PIN, INPUT);

  // 4. Configuration du bus I2C (SDA = 22, SCL = 21)
  Wire.begin(I2C_SDA, I2C_SCL);

  // 5. Initialisation de l'accéléromètre (Réveil du composant MPU6886 / MPU6050)
  Wire.beginTransmission(MPU6886_ADDR);
  Wire.write(0x6B); // Registre PWR_MGMT_1 (Gestion de l'alimentation)
  Wire.write(0x00); // Mettre à 0 pour sortir du mode veille (wake up)
  byte error = Wire.endTransmission();

  if (error == 0) {
    Serial.println("Accéléromètre I2C détecté avec succès !");
  } else {
    Serial.println("ERREUR: Accéléromètre introuvable (Vérifiez les connexions SDA/SCL et l'adresse).");
  }
}

void loop() {
  unsigned long currentMillis = millis();

  // --- 1. Gestion des LEDs en séquence (Chenillard non bloquant) ---
  if (currentMillis - lastLedTime >= LED_INTERVAL) {
    lastLedTime = currentMillis;

    // Éteindre la LED actuelle
    digitalWrite(LED_PINS[currentLed], LOW);
    
    // Passer à la suivante
    currentLed = (currentLed + 1) % NUM_LEDS;
    
    // Allumer la nouvelle LED
    digitalWrite(LED_PINS[currentLed], HIGH);
  }

  // --- 2. Lecture et Affichage périodique des capteurs ---
  // Nous utilisons un intervalle de temps pour ne pas saturer le moniteur série
  if (currentMillis - lastPrintTime >= PRINT_INTERVAL) {
    lastPrintTime = currentMillis;

    // A. Lecture des Potentiomètres (L'ADC de l'ESP32 renvoie une valeur sur 12 bits : 0 à 4095)
    int valPot1 = analogRead(POT1_PIN);
    int valPot2 = analogRead(POT2_PIN);

    // B. Lecture du Bouton (Avec un pull-up, le signal est à l'état BAS lorsqu'on appuie)
    bool isButtonPressed = (digitalRead(BUTTON_PIN) == LOW);

    // C. Lecture de l'Accéléromètre (valeurs brutes X, Y, Z)
    int16_t accelX = 0, accelY = 0, accelZ = 0;
    
    Wire.beginTransmission(MPU6886_ADDR);
    Wire.write(0x3B); // Registre de départ : ACCEL_XOUT_H
    Wire.endTransmission(false);
    Wire.requestFrom((int)MPU6886_ADDR, 6, (int)true); // Demande de 6 octets (2 octets par axe)
    
    if (Wire.available() == 6) {
      // Recomposition des valeurs sur 16 bits
      accelX = Wire.read() << 8 | Wire.read();
      accelY = Wire.read() << 8 | Wire.read();
      accelZ = Wire.read() << 8 | Wire.read();
    }

    // --- Formatage de l'affichage sur le moniteur série ---
    Serial.print("Bouton : ");
    Serial.print(isButtonPressed ? "[ APPUYE ]" : "[ RELACHE ]");
    
    Serial.print(" | Pot1 : ");
    Serial.print(valPot1);
    
    Serial.print("\t| Pot2 : ");
    Serial.print(valPot2);
    
    Serial.print("\t| Accel (X,Y,Z) : ");
    Serial.print(accelX); Serial.print(", ");
    Serial.print(accelY); Serial.print(", ");
    Serial.println(accelZ);
  }
}
