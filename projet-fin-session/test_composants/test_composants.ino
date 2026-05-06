#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BME280.h>

#define PIN_PIR 13
#define PIN_POT1 34
#define PIN_POT2 35
#define PIN_LED1 14
#define PIN_LED2 15

#define I2C_SCK 22
#define I2C_SDI 21

Adafruit_BME280 bme;

void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println("=== Test des composants LilyGO A7670G ===\n");

  pinMode(PIN_PIR, INPUT);
  pinMode(PIN_POT1, INPUT);
  pinMode(PIN_POT2, INPUT);
  pinMode(PIN_LED1, OUTPUT);
  pinMode(PIN_LED2, OUTPUT);

  Wire.begin(I2C_SDI, I2C_SCK);

  if (!bme.begin(0x76) && !bme.begin(0x77)) {
    Serial.println("ERREUR: BME280 non detecte!");
  } else {
    Serial.println("BME280 detecte avec succes");
  }

  Serial.println("Initialisation terminee\n");
}

void loop() {
  Serial.println("--- Lecture des capteurs ---");

  float temp = bme.readTemperature();
  float hum = bme.readHumidity();
  float pres = bme.readPressure() / 100.0F;

  if (!isnan(temp)) {
    Serial.printf("BME280 - Temp: %.1f C, Hum: %.1f %%, Pres: %.1f hPa\n", temp, hum, pres);
  } else {
    Serial.println("BME280 - Erreur de lecture");
  }

  int pirState = digitalRead(PIN_PIR);
  Serial.printf("PIR (EKMC4607112K) - Etat: %s\n", pirState ? "MOUVEMENT" : "Repos");

  int pot1 = analogRead(PIN_POT1);
  int pot2 = analogRead(PIN_POT2);
  Serial.printf("Potentiometres - Pot1: %d (%.2f V), Pot2: %d (%.2f V)\n", 
                pot1, pot1 * 3.3 / 4095, pot2, pot2 * 3.3 / 4095);

  Serial.println("LEDs - Clignotement...");
  digitalWrite(PIN_LED1, HIGH);
  digitalWrite(PIN_LED2, LOW);
  delay(500);
  digitalWrite(PIN_LED1, LOW);
  digitalWrite(PIN_LED2, HIGH);
  delay(500);

  Serial.println("\n");
  delay(2000);
}
