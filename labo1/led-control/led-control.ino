// Contrôle de LEDs via commandes série
// Pour LilyGO A7670G - Exercice 7.6
// Commandes acceptées: "rouge" ou "vert"

// Définition des pins pour les LEDs
#define LED_ROUGE 25  // Pin GPIO pour LED rouge
#define LED_VERTE 26  // Pin GPIO pour LED verte

String commandeRecue = "";  // Buffer pour stocker la commande

void setup() {
  // Initialiser la communication série
  Serial.begin(115200);
  delay(1000);

  // Configurer les pins des LEDs en sortie
  pinMode(LED_ROUGE, OUTPUT);
  pinMode(LED_VERTE, OUTPUT);

  // Éteindre toutes les LEDs au démarrage
  digitalWrite(LED_ROUGE, LOW);
  digitalWrite(LED_VERTE, LOW);

  Serial.println("========================================");
  Serial.println("Contrôle de LEDs via Port Série");
  Serial.println("========================================");
  Serial.println("Commandes disponibles:");
  Serial.println("  - rouge : Allume LED rouge, éteint LED verte");
  Serial.println("  - vert  : Allume LED verte, éteint LED rouge");
  Serial.println("========================================");
  Serial.println("En attente de commandes...");
}

void loop() {
  // Vérifier si des données sont disponibles sur le port série
  if (Serial.available() > 0) {
    // Lire la commande jusqu'au caractère de fin de ligne
    commandeRecue = Serial.readStringUntil('\n');

    // Nettoyer la commande (enlever espaces et retours chariot)
    commandeRecue.trim();
    commandeRecue.toLowerCase();  // Convertir en minuscules

    // Afficher la commande reçue
    Serial.print("Commande reçue: '");
    Serial.print(commandeRecue);
    Serial.println("'");

    // Traiter la commande
    if (commandeRecue == "rouge") {
      // Allumer LED rouge, éteindre LED verte
      digitalWrite(LED_ROUGE, HIGH);
      digitalWrite(LED_VERTE, LOW);
      Serial.println("→ LED ROUGE allumée, LED VERTE éteinte");

    } else if (commandeRecue == "vert" || commandeRecue == "verte") {
      // Allumer LED verte, éteindre LED rouge
      digitalWrite(LED_ROUGE, LOW);
      digitalWrite(LED_VERTE, HIGH);
      Serial.println("→ LED VERTE allumée, LED ROUGE éteinte");

    } else if (commandeRecue == "off" || commandeRecue == "eteindre") {
      // Éteindre toutes les LEDs
      digitalWrite(LED_ROUGE, LOW);
      digitalWrite(LED_VERTE, LOW);
      Serial.println("→ Toutes les LEDs éteintes");

    } else {
      // Commande non reconnue
      Serial.println("→ Commande non reconnue!");
      Serial.println("   Utilisez: rouge, vert, ou off");
    }

    // Vider le buffer
    commandeRecue = "";
  }

  // Petite pause pour éviter de surcharger le processeur
  delay(10);
}
