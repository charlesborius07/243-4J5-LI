// Programme de diagnostic pour modem A7670G
// Teste la connexion réseau cellulaire et affiche des informations détaillées

#define TINY_GSM_MODEM_SIM7600
#define TINY_GSM_RX_BUFFER 1024
#define TINY_GSM_DEBUG Serial

#include <TinyGsmClient.h>

// CONFIG MODEM A7670G
#define MODEM_TX 26
#define MODEM_RX 27
#define MODEM_PWRKEY 4
#define MODEM_DTR 32
#define MODEM_RI 33
#define MODEM_FLIGHT 25
#define MODEM_STATUS 34

// Configuration APN
const char APN[] = "hologram";
const char APN_USER[] = "";
const char APN_PASS[] = "";

HardwareSerial SerialAT(1);
TinyGsm modem(SerialAT);

void modemPowerOn() {
  Serial.println("[MODEM] Allumage du modem...");
  pinMode(MODEM_PWRKEY, OUTPUT);
  digitalWrite(MODEM_PWRKEY, HIGH);
  delay(100);
  digitalWrite(MODEM_PWRKEY, LOW);
  delay(1000);
  digitalWrite(MODEM_PWRKEY, HIGH);
  delay(3000);
  Serial.println("[MODEM] ✓ Modem allumé");
}

void sendATCommand(const char* cmd, int delayMs = 1000) {
  Serial.print("[AT] Envoi: ");
  Serial.println(cmd);
  SerialAT.println(cmd);
  delay(delayMs);

  while (SerialAT.available()) {
    String response = SerialAT.readStringUntil('\n');
    Serial.print("[AT] Réponse: ");
    Serial.println(response);
  }
}

void setup() {
  Serial.begin(115200);
  delay(2000);

  Serial.println();
  Serial.println("=== DIAGNOSTIC MODEM A7670G ===");
  Serial.println();

  // Démarrage du modem
  modemPowerOn();

  SerialAT.begin(115200, SERIAL_8N1, MODEM_RX, MODEM_TX);
  delay(3000);

  Serial.println();
  Serial.println("=== TEST 1: Communication de base ===");
  sendATCommand("AT");
  sendATCommand("AT+CGMI");  // Fabricant
  sendATCommand("AT+CGMM");  // Modèle
  sendATCommand("AT+CGMR");  // Version firmware
  sendATCommand("AT+CGSN");  // IMEI

  Serial.println();
  Serial.println("=== TEST 2: État de la carte SIM ===");
  sendATCommand("AT+CPIN?"); // État du PIN
  sendATCommand("AT+CCID");  // ICCID de la SIM
  sendATCommand("AT+CIMI");  // IMSI

  Serial.println();
  Serial.println("=== TEST 3: Configuration réseau ===");
  sendATCommand("AT+COPS?");    // Opérateur sélectionné
  sendATCommand("AT+COPS=?", 60000);   // Liste des opérateurs disponibles (peut prendre du temps)
  sendATCommand("AT+CSQ");      // Qualité du signal
  sendATCommand("AT+CREG?");    // Statut enregistrement réseau GSM
  sendATCommand("AT+CGREG?");   // Statut enregistrement réseau GPRS
  sendATCommand("AT+CEREG?");   // Statut enregistrement réseau LTE

  Serial.println();
  Serial.println("=== TEST 4: Mode réseau ===");
  sendATCommand("AT+CNMP?");    // Mode réseau préféré
  sendATCommand("AT+CMNB?");    // Mode réseau LTE

  Serial.println();
  Serial.println("=== TEST 5: Forcer la recherche réseau ===");
  sendATCommand("AT+COPS=0", 30000);  // Sélection automatique de l'opérateur

  Serial.println();
  Serial.println("=== TEST 6: Vérification connexion réseau ===");
  delay(5000);
  sendATCommand("AT+CREG?");
  sendATCommand("AT+COPS?");
  sendATCommand("AT+CSQ");

  Serial.println();
  Serial.println("=== TEST 7: Tentative de connexion GPRS ===");
  sendATCommand("AT+CGATT=1");  // Attacher au GPRS
  delay(2000);
  sendATCommand("AT+CGATT?");   // Vérifier l'attachement

  String apnCmd = "AT+CGDCONT=1,\"IP\",\"" + String(APN) + "\"";
  sendATCommand(apnCmd.c_str());

  sendATCommand("AT+CGACT=1,1"); // Activer le contexte PDP
  delay(2000);
  sendATCommand("AT+CGACT?");    // Vérifier l'activation

  sendATCommand("AT+CGPADDR=1"); // Obtenir l'adresse IP

  Serial.println();
  Serial.println("=== DIAGNOSTIC TERMINÉ ===");
  Serial.println();
  Serial.println("Analysez les résultats ci-dessus:");
  Serial.println("1. AT+CPIN? doit retourner 'READY' (SIM déverrouillée)");
  Serial.println("2. AT+CSQ doit retourner un signal > 10 (qualité acceptable)");
  Serial.println("3. AT+CREG? doit retourner '+CREG: 0,1' ou '+CREG: 0,5' (enregistré)");
  Serial.println("4. AT+COPS? doit afficher le nom de votre opérateur");
  Serial.println("5. AT+CGATT? doit retourner '+CGATT: 1' (attaché au GPRS)");
  Serial.println();
  Serial.println("Si l'un de ces tests échoue, il y a un problème à ce niveau.");
}

void loop() {
  // Passthrough mode: permet d'envoyer des commandes AT manuellement
  if (SerialAT.available()) {
    Serial.write(SerialAT.read());
  }
  if (Serial.available()) {
    SerialAT.write(Serial.read());
  }
}
