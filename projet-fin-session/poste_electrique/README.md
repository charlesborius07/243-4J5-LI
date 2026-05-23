# Système de Surveillance de Poste Électrique (Poste-05)

## 1. Mise en situation
Dans le cadre de la modernisation du réseau de distribution d'Hydro-Limoilou, le projet "Poste-05" vise à déployer un système de surveillance IoT autonome. Ce système permet le monitoring en temps réel des conditions environnementales et électriques critiques d'un poste de transformation, tout en assurant une détection d'intrusion physique. Il garantit la sécurité des installations en activant des alertes locales et distantes en cas de dépassement de seuils critiques.

## 2. Schéma de câblage
Le système est articulé autour d'une carte ESP32. Voici la configuration des connexions :

| Composant | Broche(s) |
| :--- | :--- |
| **BME280** (I2C) | SDA: 21, SCL: 22 |
| **Capteur PIR** | GPIO 13 |
| **Potentiomètre 1** (Tension) | GPIO 34 |
| **Potentiomètre 2** (Courant) | GPIO 35 |
| **LED 1** (Alerte Tension) | GPIO 14 |
| **LED 2** (Alerte Courant) | GPIO 25 |

## 3. Contrat MQTT
Tous les topics sont sous la racine : `hydro-limoilou/poste-05/`

| Topic | Type | Description |
| :--- | :--- | :--- |
| `telemetry/temperature` | Publication | Valeur Température (°C) |
| `telemetry/humidity` | Publication | Valeur Humidité (%) |
| `telemetry/pressure` | Publication | Valeur Pression (hPa) |
| `telemetry/vibration` | Publication | Vecteurs vibration (x, y, z) |
| `telemetry/voltage_line` | Publication | Valeur Tension mesurée (V) |
| `telemetry/current_line` | Publication | Valeur Courant mesuré (A) |
| `alarm/motion` | Publication | État intrusion (1: Début, 0: Fin) |
| `alarm/voltage` | Publication | Alerte seuil tension |
| `alarm/current` | Publication | Alerte seuil courant |
| `actuators/led_1` | Pub/Sub | État LED Tension (on/off) |
| `actuators/led_2` | Pub/Sub | État LED Courant (on/off) |
| `status` | Publication | État système (uptime, RSSI) |

## 4. Procédure de démonstration et tests

### Scénario 1 : Fonctionnement normal (Télémétrie)
*   **Action** : Démarrer le système (l'ESP32 se connecte au Wi-Fi, puis à MQTT).
*   **Résultat attendu** : Flux régulier de messages JSON sur les topics `telemetry/*` toutes les 10 secondes. Les LEDs sont éteintes.

### Scénario 2 : Dépassement de seuils électriques (Alarme locale)
*   **Action** : Tourner le **Potentiomètre 1** pour simuler une tension > 250V.
*   **Résultat attendu** :
    *   La **LED 1** s'allume physiquement.
    *   Publication immédiate d'un message `warning` sur `alarm/voltage`.
    *   Publication de l'état `{"state": "on"}` sur `actuators/led_1`.

### Scénario 3 : Détection d'intrusion (Gestion dynamique)
*   **Action** : Activer le capteur PIR (déplacement devant).
*   **Résultat attendu** :
    *   Publication immédiate d'un message `warning` (valeur 1.0) sur `alarm/motion`.
*   **Action** : Arrêter tout mouvement et attendre 5 secondes.
*   **Résultat attendu** :
    *   Publication automatique d'un message d'information (valeur 0.0) sur `alarm/motion` confirmant la fin de l'intrusion.

## 5. Configuration et Compilation

### 5.1 Fichier d'authentification
Le projet nécessite un fichier `auth.h` pour les identifiants Wi-Fi et MQTT. Copiez le fichier d'exemple et remplissez vos informations :
```bash
cp auth.h.example auth.h
# Modifiez ensuite auth.h avec vos identifiants réels
```

### 5.2 Dépendances (Bibliothèques Arduino)
Le code utilise les bibliothèques suivantes. Assurez-vous de les installer via le gestionnaire de bibliothèques Arduino :
- `PubSubClient` (par Nick O'Leary)
- `Adafruit BME280 Library`
- `Adafruit Unified Sensor`
- `ArduinoJson`

### 5.3 Dépannage
*   **Connexion Série** : Le moniteur série est configuré à **115200 bauds**. Si vous voyez des caractères illisibles, vérifiez que votre terminal série est configuré à cette vitesse.
*   **Problème de téléversement** : Si l'upload échoue, assurez-vous que le port `/dev/ttyACM0` (ou équivalent) est accessible par votre utilisateur et que la carte est en mode de programmation (maintenir le bouton BOOT si nécessaire lors de la connexion).
