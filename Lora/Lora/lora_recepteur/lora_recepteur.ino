#include <RadioLib.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include "soc/soc.h"
#include "soc/rtc_cntl_reg.h"


#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_SDA 17
#define OLED_SCL 18

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

#define LED_STATUS 25
#define LED_ACTION 26

// Pins SX1262 du T-Supreme
SX1262 radio = new Module(10, 33, 5, 36);

void setup() {

   WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0); // Désactive le détecteur de brownout
    // Le reste de votre code..

  Serial.begin(115200);
  delay(3000); 
  Serial.println("BOD désactivé, le système démarre...");

  Serial.println("=== RECEPTEUR LoRa ===");
  
  Wire.begin(OLED_SDA, OLED_SCL);
  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);
  display.clearDisplay();
  display.setTextSize(1);
  display.setCursor(0, 0);
  display.println("Recepteur LoRa");
  display.display();
  
  pinMode(LED_STATUS, OUTPUT);
  pinMode(LED_ACTION, OUTPUT);
  
  int state = radio.begin(868.0, 125.0, 9, 7, 0x12, 22, 8);
  if (state == RADIOLIB_ERR_NONE) {
    Serial.println("Radio init OK");
    display.println("Radio OK");
  } else {
    Serial.print("Erreur radio: ");
    Serial.println(state);
  }
  display.display();
}

void oledPrint(String text) {
  display.clearDisplay();
  display.setCursor(0, 0);
  int line = 0;
  int pos = 0;
  while (pos < text.length() && line < 8) {
    int nextLine = text.indexOf('\n', pos);
    if (nextLine == -1) nextLine = text.length();
    display.println(text.substring(pos, nextLine));
    pos = nextLine + 1;
    line++;
  }
  display.display();
}

void processMessage(String received) {
  Serial.println("Recu: " + received);
  
  int valeur = received.toInt();
  String action = "none";
  String status = "normal";
  
  if (valeur > 3000) {
    status = "urgent";
    action = "on";
  } else if (valeur > 1500) {
    status = "attention";
    action = "none";
  } else {
    status = "normal";
    action = "off";
  }
  
  // Répondre à l'émetteur
  String reply = "{\"status\":\"" + status + "\",\"action\":\"" + action + "\"}";
  radio.transmit(reply);
  Serial.println("TX: " + reply);
  
  // Allumer LED selon action
  digitalWrite(LED_ACTION, action == "on" ? HIGH : LOW);
  oledPrint("RX: " + received + "\nRSSI: " + String(radio.getRSSI()) + "\nAction: " + action);
}

void loop() {
  String received;
  int state = radio.receive(received);
  
  if (state == RADIOLIB_ERR_NONE) {
    digitalWrite(LED_STATUS, HIGH);
    processMessage(received);
    delay(100);
    digitalWrite(LED_STATUS, LOW);
  }
}