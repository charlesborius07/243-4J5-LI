#include <RadioLib.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SH110X.h>

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1

Adafruit_SH1106G display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

#define LED_STATUS 37

// Pins SX1262 du T-Beam SUPREME (correctes)
// NSS=10, DIO1=1, NRST=5, BUSY=4
// SPI: SCK=12, MISO=13, MOSI=11
SX1262 radio = new Module(10, 1, 5, 4);

void setup() {
  delay(500);
  Serial.begin(115200);
  Serial.println("=== RECEPTEUR LoRa ===");
  Serial.flush();

  delay(500);

  Wire.begin(OLED_SDA, OLED_SCL);
  display.begin(0x3C, true);
  delay(100);
  display.clearDisplay();
  display.setTextSize(1);
  display.setCursor(0, 0);
  display.println("Recepteur LoRa");
  display.display();
  
  pinMode(LED_STATUS, OUTPUT);
  
  int state = radio.begin(868.0, 125.0, 9, 7, 0x12, 2, 8);
  if (state == RADIOLIB_ERR_NONE) {
    Serial.println("Radio init OK");
    display.println("Radio OK");
  } else {
    Serial.print("Erreur radio: ");
    Serial.println(state);
  }
  Serial.flush();
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
  digitalWrite(LED_STATUS, HIGH);
  oledPrint("RX: " + received + "\nRSSI: " + String(radio.getRSSI()) + "\nStatus: " + status);
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