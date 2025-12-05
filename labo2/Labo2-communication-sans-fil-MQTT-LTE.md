<div style="background: linear-gradient(90deg, #0ea5e9, #6366f1); padding: 18px 20px; color: #f8fafc; border-radius: 14px; box-shadow: 0 4px 12px rgba(0,0,0,0.15);">
  <h1 style="margin: 0; font-size: 28px;">Labo 2 — Communication sans fil et télémétrie IoT</h1>
  <p style="margin: 6px 0 0; font-size: 15px;">Du câble série au réseau cellulaire : communication MQTT via WiFi et LTE avec géolocalisation GPS.</p>
</div>

---

## 📐 Architecture du système

```mermaid
graph TD
    %% ==== STYLES ====
    classDef zoneClient fill:#f0fdf4,stroke:#16a34a,stroke-width:2px,color:#052e16;
    classDef zoneAccess fill:#eff6ff,stroke:#2563eb,stroke-width:2px,color:#0f172a;
    classDef zoneLab fill:#f9fafb,stroke:#4b5563,stroke-width:2px,color:#020617;
    classDef zoneCloud fill:#fefce8,stroke:#d97706,stroke-width:2px,color:#451a03;
    classDef componentCore fill:#e5e7eb,stroke:#4b5563,stroke-width:1.5px;
    classDef componentService fill:#eef2ff,stroke:#6366f1,stroke-width:1.5px;
    classDef componentDevice fill:#ecfeff,stroke:#06b6d4,stroke-width:1.5px;
    classDef securityEdge fill:#fef2f2,stroke:#b91c1c,stroke-width:1.5px,stroke-dasharray:4 3;
    classDef wireless fill:#fae8ff,stroke:#a855f7,stroke-width:1.5px;

    %% ==== ZONE CLIENT ====
    subgraph Zone_Client ["💻 Poste de développement"]
        Dev_PC["Terminal SSH + Navigateur"]:::zoneClient
    end

    %% ==== ZONE D'ACCÈS / EDGE ====
    subgraph Zone_Access ["🔒 Accès sécurisé"]
        CF_ZT["Cloudflare Zero Trust"]:::securityEdge
        CF_Tunnel["cloudflared"]:::componentService
    end

    %% ==== ZONE LAB / ON-PREM ====
    subgraph Zone_Lab ["🏠 Lab On-Prem"]
        subgraph RPi5_Core ["🍓 Raspberry Pi 5"]
            SSHD["SSH Server"]:::componentCore
            MQTT_Broker["Mosquitto Broker"]:::componentService

            subgraph Dev_Stack ["🛠️ Outils Dev"]
                Git_CLI["Git CLI"]:::componentService
                Node_Gemini["Node + Gemini"]:::componentService
                Python_Env["Python + evdev"]:::componentService
                Arduino_CLI["Arduino CLI"]:::componentService
            end
        end

        subgraph Lab_Devices ["📱 Périphériques"]
            Touchscreen["Écran tactile"]:::componentDevice
            LilyGO_A7670E["LilyGO A7670E<br/>(ESP32 + LTE + GPS)"]:::wireless
        end
    end

    %% ==== ZONE CLOUD / SAAS ====
    subgraph Zone_Cloud ["☁️ Services Cloud"]
        GitHub_SaaS["GitHub"]:::zoneCloud
        Gemini_API["Gemini API"]:::zoneCloud
        Cellular_Network["Réseau Cellulaire<br/>(LTE Cat-1)"]:::zoneCloud
        GPS_Satellites["Satellites GPS"]:::zoneCloud
    end

    %% ==== FLUX PRINCIPAUX ====

    %% 1. ACCÈS DISTANT (vertical)
    Dev_PC -->|"HTTPS / SSH"| CF_ZT
    CF_ZT -->|"Tunnel Cloudflare<br/>(mTLS + Auth)"| CF_Tunnel
    CF_Tunnel -->|"TCP SSH :22"| SSHD
    SSHD --> Dev_Stack

    %% 2. DEV & GESTION DE CODE
    Git_CLI -.->|"git clone/pull/push"| GitHub_SaaS

    %% 3. APPELS IA
    Node_Gemini -.->|"API REST"| Gemini_API

    %% 4. COMMUNICATION MQTT
    Python_Env -->|"MQTT Publish/Subscribe<br/>(WiFi local)"| MQTT_Broker
    LilyGO_A7670E -->|"MQTT via WiFi"| MQTT_Broker
    LilyGO_A7670E -->|"MQTT via LTE"| Cellular_Network
    Cellular_Network -->|"Internet"| MQTT_Broker

    %% 5. PROGRAMMATION ET MONITORING
    Arduino_CLI -->|"Flash USB<br/>/dev/ttyUSB0"| LilyGO_A7670E

    %% 6. INTERACTIONS TACTILES
    Python_Env -->|"UI tactile<br/>/dev/input"| Touchscreen

    %% 7. GPS
    LilyGO_A7670E -.->|"Réception signaux<br/>satellites"| GPS_Satellites

    %% ==== CLASS ZONES ====
    class Dev_PC zoneClient;
    class CF_ZT,CF_Tunnel zoneAccess;
    class SSHD,Dev_Stack componentCore;
    class Touchscreen componentDevice;
    class LilyGO_A7670E wireless;
    class GitHub_SaaS,Gemini_API,Cellular_Network,GPS_Satellites zoneCloud;
```

Ce diagramme illustre la nouvelle architecture avec communication sans fil:
- **Zone Client (vert):** Votre poste de développement
- **Zone d'accès sécurisé (bleu):** Cloudflare Zero Trust et tunnel
- **Zone Lab (gris):** Raspberry Pi 5 avec broker MQTT et périphériques IoT
- **Zone Cloud (jaune):** Services externes incluant le réseau cellulaire et GPS
- **Communication sans fil (violet):** LilyGO communique via WiFi ou LTE

---

## 🧭 Plan du guide
- [Prérequis](#-prérequis)
- [Introduction au protocole MQTT](#1-introduction-au-protocole-mqtt)
- [Communication MQTT via WiFi](#2-communication-mqtt-via-wifi)
- [Activation du modem LTE](#3-activation-du-modem-lte)
- [Communication MQTT via LTE](#4-communication-mqtt-via-lte)
- [Intégration GPS](#5-intégration-gps)
- [Projet intégrateur](#6-projet-intégrateur)
- [Au prochain laboratoire](#-au-prochain-laboratoire)
- [Commandes de vérification](#-commandes-de-vérification-utiles)

<div style="height: 6px; background: linear-gradient(90deg, #22d3ee, #22c55e); border-radius: 999px; margin: 18px 0;"></div>

## 📋 Prérequis

<div style="background:#fef9c3; border:1px solid #facc15; padding:12px 14px; border-radius:10px;">
<strong>⚠️ Avant de commencer</strong>
<p>Assurez-vous d'avoir complété:</p>
<ul>
  <li>✅ Le Labo 1 complet (environnement de programmation distant)</li>
  <li>✅ Le devoir de préparation (installation Mosquitto et test WiFi)</li>
  <li>✅ Accès SSH distant à votre Raspberry Pi via Cloudflare Tunnel</li>
  <li>✅ Arduino CLI configuré pour ESP32</li>
  <li>✅ Carte SIM activée et insérée dans le LilyGO</li>
  <li>✅ Antennes GPS et LTE correctement branchées</li>
</ul>
</div>

### Vérification rapide

```bash
# Vérifier que Mosquitto est actif
sudo systemctl status mosquitto

# Vérifier Arduino CLI
arduino-cli version

# Vérifier les bibliothèques nécessaires
arduino-cli lib list | grep -E "TinyGSM|PubSubClient|ArduinoJson"
```

<div style="height: 5px; background: linear-gradient(90deg, #f59e0b, #fb7185); border-radius: 999px; margin: 22px 0;"></div>

## 1. Introduction au protocole MQTT
> 🎯 **Objectif :** comprendre MQTT et ses avantages pour l'IoT.

### 💡 Concepts clés

**Qu'est-ce que MQTT?**

MQTT (Message Queuing Telemetry Transport) est un protocole de messagerie léger conçu pour l'IoT. Développé à l'origine par IBM en 1999 pour surveiller des pipelines de pétrole via liaison satellite, il est devenu le standard de facto pour la communication entre appareils connectés.

**Pourquoi MQTT plutôt que HTTP?**

| Caractéristique | HTTP | MQTT |
|-----------------|------|------|
| Architecture | Requête/Réponse | Publish/Subscribe |
| Connexion | Nouvelle connexion par requête | Connexion persistante |
| Overhead | Headers volumineux (~200-800 bytes) | Headers minimaux (~2 bytes) |
| Bande passante | Élevée | Très faible |
| Latence | Moyenne à élevée | Faible |
| QoS | Non natif | 3 niveaux intégrés |
| Hors ligne | Pas de gestion | Messages en attente |
| Batterie | Consommation élevée | Optimisé pour économie |

**Architecture Publish/Subscribe:**

Dans MQTT, les appareils ne communiquent pas directement entre eux. Ils passent par un **broker** (courtier) central:

```
[Publisher] --publish--> [Broker] --deliver--> [Subscriber(s)]
                           ↕
                    [Topics/Routes]
```

**Exemple concret:**
```
LilyGO (publisher) → Topic: "iot/temperature" → Broker → Raspberry Pi (subscriber)
                                                       → Application mobile
                                                       → Cloud dashboard
```

**Avantages:**
- ✅ **Découplage:** Le publisher ne sait pas qui reçoit ses messages
- ✅ **Scalabilité:** Un message peut avoir 0, 1 ou 1000 abonnés
- ✅ **Flexibilité:** On peut ajouter/retirer des abonnés sans toucher au publisher

**Topics (sujets):**

Les topics sont des chaînes hiérarchiques qui organisent les messages, comme un système de fichiers:

```
iot/                          # Racine
├── sensors/                  # Groupe de capteurs
│   ├── temperature           # Température
│   ├── humidity              # Humidité
│   └── pressure              # Pression
├── gps/                      # Géolocalisation
│   ├── latitude
│   ├── longitude
│   └── altitude
└── status/                   # État système
    ├── battery
    └── signal
```

**Wildcards (jokers):**
- `+` : un niveau quelconque → `iot/sensors/+` écoute température, humidité, pression
- `#` : tous les sous-niveaux → `iot/#` écoute TOUT sous iot/

**Quality of Service (QoS):**

MQTT offre 3 niveaux de garantie de livraison:

| QoS | Nom | Garantie | Usage |
|-----|-----|----------|-------|
| 0 | At most once | Au plus une fois (peut être perdu) | Données temps réel non critiques (température, GPS toutes les secondes) |
| 1 | At least once | Au moins une fois (peut être dupliqué) | Données importantes mais tolérantes aux doublons |
| 2 | Exactly once | Exactement une fois (garanti unique) | Données critiques (alertes, commandes de contrôle) |

**Retained Messages:**

Un message **retained** est stocké par le broker et envoyé immédiatement à tout nouvel abonné. Utile pour:
- État actuel d'un système (ON/OFF)
- Dernière valeur connue d'un capteur
- Configuration persistante

**Last Will and Testament (LWT):**

Le "testament" MQTT permet à un appareil de préparer un message automatique envoyé par le broker si l'appareil se déconnecte brutalement (batterie vide, perte réseau, crash).

**Exemple:**
```
Topic LWT: "iot/lilygo/status"
Message LWT: "offline"
```

Si le LilyGO perd la connexion, le broker publie automatiquement `"offline"` sur ce topic.

### 1.1 Test du broker Mosquitto

#### Vérifier que Mosquitto est actif

```bash
sudo systemctl status mosquitto
```

Vous devriez voir: `Active: active (running)`

#### Test pub/sub local

**Terminal 1 - Subscriber:**
```bash
mosquitto_sub -h localhost -t "test/demo" -v
```

**Terminal 2 - Publisher:**
```bash
mosquitto_pub -h localhost -t "test/demo" -m "Hello MQTT!"
```

**Explication:**
- `-h localhost` : se connecter au broker local
- `-t "test/demo"` : topic utilisé
- `-m "Hello MQTT!"` : message à publier
- `-v` : verbose (affiche le topic avec le message)

#### Test avec QoS

**Subscriber avec QoS 1:**
```bash
mosquitto_sub -h localhost -t "test/qos" -q 1 -v
```

**Publisher avec QoS 1:**
```bash
mosquitto_pub -h localhost -t "test/qos" -q 1 -m "Message garanti!"
```

#### Test de retained message

**Publier un message retained:**
```bash
mosquitto_pub -h localhost -t "test/retained" -r -m "Je persiste!"
```

**S'abonner APRÈS la publication:**
```bash
mosquitto_sub -h localhost -t "test/retained" -v
```

Vous recevrez immédiatement `"Je persiste!"` même si le message a été publié avant votre abonnement.

#### Test de wildcards

**Terminal 1:**
```bash
mosquitto_sub -h localhost -t "sensors/#" -v
```

**Terminal 2:**
```bash
mosquitto_pub -h localhost -t "sensors/temperature" -m "22.5"
mosquitto_pub -h localhost -t "sensors/humidity" -m "65"
mosquitto_pub -h localhost -t "sensors/pressure" -m "1013"
```

Le subscriber reçoit tous les messages car `#` capture tous les sous-topics.

<div style="background:#dbeafe; border:1px solid #3b82f6; padding:10px 12px; border-radius:10px;">
<strong>💡 Astuce tmux</strong>
<p>Pour gérer plusieurs terminaux simultanément:</p>
<pre><code>tmux
# Nouvelle fenêtre: Ctrl+b puis c
# Naviguer: Ctrl+b puis n (next) / p (previous)
# Split vertical: Ctrl+b puis %
# Split horizontal: Ctrl+b puis "
# Naviguer entre panes: Ctrl+b puis flèches</code></pre>
</div>

<div style="height: 5px; background: linear-gradient(90deg, #22c55e, #84cc16); border-radius: 999px; margin: 22px 0;"></div>

## 2. Communication MQTT via WiFi
> 📡 **Objectif :** remplacer la communication série par MQTT via WiFi.

### 💡 Concepts clés

**Évolution de l'architecture:**

**Labo 1 (Série):**
```
[Interface tactile] → [Python Serial] → [Câble USB] → [LilyGO]
```

**Labo 2 (MQTT WiFi):**
```
[Interface tactile] → [MQTT Pub] → [Broker] → [MQTT Sub] → [LilyGO (WiFi)]
```

**Avantages du passage à MQTT:**
- ✅ **Sans fil:** Plus besoin de câble USB
- ✅ **Bidirectionnel:** L'interface et le LilyGO peuvent tous deux publier et s'abonner
- ✅ **Multi-clients:** Plusieurs interfaces peuvent contrôler le LilyGO
- ✅ **Logs centralisés:** Tous les messages passent par le broker
- ✅ **Extensible:** Facile d'ajouter un dashboard web, une app mobile, etc.

**Bibliothèque PubSubClient:**

PubSubClient est la bibliothèque Arduino de référence pour MQTT. Elle gère:
- Connexion au broker
- Publish de messages
- Subscribe à des topics
- Callback automatique sur réception
- Reconnexion automatique

### 2.1 Code Arduino - MQTT via WiFi

#### Créer le projet

```bash
mkdir -p ~/243-4J5-LI/labo2/mqtt-wifi
cd ~/243-4J5-LI/labo2/mqtt-wifi
nano mqtt-wifi.ino
```

#### Code complet

```cpp
#include <WiFi.h>
#include <PubSubClient.h>

// Configuration WiFi
const char* ssid = "VOTRE_SSID";
const char* password = "VOTRE_PASSWORD";

// Configuration MQTT
const char* mqtt_server = "192.168.1.9";  // IP de votre Raspberry Pi
const int mqtt_port = 1883;
const char* mqtt_client_id = "lilygo-esp32";

// Topics MQTT
const char* topic_command = "iot/led/command";      // Commandes vers LilyGO
const char* topic_status = "iot/led/status";        // Status depuis LilyGO
const char* topic_lwt = "iot/lilygo/status";        // Last Will

// Pins LED
#define LED_RED 25
#define LED_GREEN 26

// Clients
WiFiClient espClient;
PubSubClient mqttClient(espClient);

void setup() {
  Serial.begin(115200);
  delay(1000);

  // Configuration LEDs
  pinMode(LED_RED, OUTPUT);
  pinMode(LED_GREEN, OUTPUT);
  digitalWrite(LED_RED, LOW);
  digitalWrite(LED_GREEN, LOW);

  Serial.println("=========================");
  Serial.println("LilyGO - MQTT via WiFi");
  Serial.println("=========================");

  // Connexion WiFi
  connectWiFi();

  // Configuration MQTT
  mqttClient.setServer(mqtt_server, mqtt_port);
  mqttClient.setCallback(mqttCallback);

  // Connexion MQTT
  connectMQTT();
}

void loop() {
  // Maintenir la connexion MQTT
  if (!mqttClient.connected()) {
    connectMQTT();
  }
  mqttClient.loop();

  // Heartbeat toutes les 10 secondes
  static unsigned long lastHeartbeat = 0;
  if (millis() - lastHeartbeat > 10000) {
    mqttClient.publish(topic_lwt, "online", true);  // Retained
    lastHeartbeat = millis();
  }
}

void connectWiFi() {
  Serial.print("Connexion WiFi à: ");
  Serial.println(ssid);

  WiFi.begin(ssid, password);

  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 20) {
    delay(500);
    Serial.print(".");
    attempts++;
  }

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\n✓ WiFi connecté!");
    Serial.print("IP: ");
    Serial.println(WiFi.localIP());
    Serial.print("Signal: ");
    Serial.print(WiFi.RSSI());
    Serial.println(" dBm");
  } else {
    Serial.println("\n✗ Échec connexion WiFi");
    Serial.println("Redémarrage dans 5s...");
    delay(5000);
    ESP.restart();
  }
}

void connectMQTT() {
  while (!mqttClient.connected()) {
    Serial.print("Connexion MQTT au broker ");
    Serial.print(mqtt_server);
    Serial.print(":");
    Serial.print(mqtt_port);
    Serial.print("...");

    // Connexion avec Last Will
    if (mqttClient.connect(mqtt_client_id, topic_lwt, 1, true, "offline")) {
      Serial.println(" ✓ Connecté!");

      // S'abonner aux commandes
      mqttClient.subscribe(topic_command, 1);  // QoS 1
      Serial.print("Abonné au topic: ");
      Serial.println(topic_command);

      // Publier le status online
      mqttClient.publish(topic_lwt, "online", true);  // Retained
      mqttClient.publish(topic_status, "{\"status\":\"ready\",\"leds\":{\"red\":\"off\",\"green\":\"off\"}}");

    } else {
      Serial.print(" ✗ Échec, rc=");
      Serial.println(mqttClient.state());
      Serial.println("Nouvelle tentative dans 5s...");
      delay(5000);
    }
  }
}

void mqttCallback(char* topic, byte* payload, unsigned int length) {
  // Convertir payload en String
  String message = "";
  for (int i = 0; i < length; i++) {
    message += (char)payload[i];
  }

  Serial.print("Message reçu [");
  Serial.print(topic);
  Serial.print("]: ");
  Serial.println(message);

  // Traiter les commandes
  if (String(topic) == topic_command) {
    handleCommand(message);
  }
}

void handleCommand(String command) {
  command.trim();
  command.toLowerCase();

  if (command == "red") {
    digitalWrite(LED_RED, HIGH);
    digitalWrite(LED_GREEN, LOW);
    mqttClient.publish(topic_status, "{\"led\":\"red\",\"state\":\"on\"}");
    Serial.println("→ LED ROUGE allumée");

  } else if (command == "green") {
    digitalWrite(LED_RED, LOW);
    digitalWrite(LED_GREEN, HIGH);
    mqttClient.publish(topic_status, "{\"led\":\"green\",\"state\":\"on\"}");
    Serial.println("→ LED VERTE allumée");

  } else if (command == "off") {
    digitalWrite(LED_RED, LOW);
    digitalWrite(LED_GREEN, LOW);
    mqttClient.publish(topic_status, "{\"led\":\"all\",\"state\":\"off\"}");
    Serial.println("→ LEDs éteintes");

  } else {
    Serial.print("✗ Commande inconnue: ");
    Serial.println(command);
    mqttClient.publish(topic_status, "{\"error\":\"unknown_command\"}");
  }
}
```

<div style="background:#fef9c3; border:1px solid #facc15; padding:10px 12px; border-radius:10px;">
<strong>📖 Explication du code</strong>
<ul>
  <li><code>WiFi.begin()</code> → Connexion au réseau WiFi</li>
  <li><code>mqttClient.setServer()</code> → Configuration du broker MQTT</li>
  <li><code>mqttClient.setCallback()</code> → Fonction appelée lors de réception de message</li>
  <li><code>mqttClient.connect()</code> → Connexion avec Last Will Testament</li>
  <li><code>mqttClient.subscribe()</code> → S'abonner au topic des commandes</li>
  <li><code>mqttClient.publish()</code> → Publier un message (status, heartbeat)</li>
  <li><code>mqttClient.loop()</code> → Traiter les messages entrants (à appeler dans loop())</li>
  <li>Paramètre <code>true</code> dans publish → Message retained</li>
  <li>QoS 1 → Garantie de livraison au moins une fois</li>
</ul>

<strong>⚙️ À personnaliser</strong>
<ul>
  <li><code>ssid</code> et <code>password</code> → Vos identifiants WiFi</li>
  <li><code>mqtt_server</code> → IP de votre Raspberry Pi (192.168.1.9 ou autre)</li>
  <li><code>LED_RED</code> et <code>LED_GREEN</code> → Pins GPIO selon votre circuit</li>
</ul>
</div>

#### Compilation et téléversement

```bash
arduino-cli compile --fqbn esp32:esp32:esp32 mqtt-wifi.ino
arduino-cli upload -p /dev/ttyUSB0 --fqbn esp32:esp32:esp32 mqtt-wifi.ino
```

#### Monitoring

```bash
arduino-cli monitor -p /dev/ttyUSB0 -c baudrate=115200
```

### 2.2 Test de la communication MQTT

#### Tester depuis la ligne de commande

**Surveiller tous les messages:**
```bash
mosquitto_sub -h localhost -t "iot/#" -v
```

**Envoyer des commandes:**
```bash
# Allumer LED rouge
mosquitto_pub -h localhost -t "iot/led/command" -m "red"

# Allumer LED verte
mosquitto_pub -h localhost -t "iot/led/command" -m "green"

# Éteindre toutes les LEDs
mosquitto_pub -h localhost -t "iot/led/command" -m "off"
```

Vous devriez voir:
- Les messages de commande sur `iot/led/command`
- Les réponses de status sur `iot/led/status`
- Les heartbeats sur `iot/lilygo/status`

#### Vérifier le Last Will Testament

**S'abonner au status:**
```bash
mosquitto_sub -h localhost -t "iot/lilygo/status" -v
```

**Débrancher le LilyGO** (ou appuyer sur RESET):
Le broker devrait automatiquement publier `"offline"` sur le topic.

### 2.3 Interface Python MQTT

#### Modifier l'interface tactile

Adaptez votre `touch_ui.py` du Labo 1 pour utiliser MQTT au lieu du port série.

**Installer la bibliothèque MQTT Python:**
```bash
sudo apt install -y python3-paho-mqtt
```

**Extrait de code pour intégration:**
```python
import paho.mqtt.client as mqtt

# Configuration MQTT
MQTT_BROKER = "localhost"
MQTT_PORT = 1883
MQTT_TOPIC_COMMAND = "iot/led/command"
MQTT_TOPIC_STATUS = "iot/led/status"

# Créer client MQTT
mqtt_client = mqtt.Client("touchscreen-ui")

def on_connect(client, userdata, flags, rc):
    print(f"Connecté au broker MQTT avec code: {rc}")
    client.subscribe(MQTT_TOPIC_STATUS)

def on_message(client, userdata, msg):
    print(f"Message reçu: {msg.topic} -> {msg.payload.decode()}")

mqtt_client.on_connect = on_connect
mqtt_client.on_message = on_message
mqtt_client.connect(MQTT_BROKER, MQTT_PORT, 60)
mqtt_client.loop_start()  # Thread séparé pour MQTT

# Dans votre gestionnaire de bouton tactile:
def handle_red_button():
    mqtt_client.publish(MQTT_TOPIC_COMMAND, "red")

def handle_green_button():
    mqtt_client.publish(MQTT_TOPIC_COMMAND, "green")
```

<div style="background:#dbeafe; border:1px solid #3b82f6; padding:10px 12px; border-radius:10px;">
<strong>💡 Exercice</strong>
<p>Modifiez complètement <code>touch_ui.py</code> pour:</p>
<ul>
  <li>Remplacer la communication série par MQTT</li>
  <li>Afficher le status de connexion du LilyGO (online/offline)</li>
  <li>Afficher les réponses de status reçues</li>
  <li>Ajouter un compteur de messages reçus</li>
</ul>
</div>

<div style="height: 5px; background: linear-gradient(90deg, #f59e0b, #f97316); border-radius: 999px; margin: 22px 0;"></div>

## 3. Activation du modem LTE
> 📶 **Objectif :** configurer le modem A7670E pour la connectivité cellulaire.

### 💡 Concepts clés

**Qu'est-ce que le modem A7670E?**

Le **A7670E** est un modem cellulaire multi-bande supportant:
- **2G:** GSM 850/900/1800/1900 MHz
- **3G:** UMTS/HSPA+ (bandes 1/2/4/5/6/8)
- **4G LTE Cat-1:** Jusqu'à 10 Mbps DL / 5 Mbps UL (bandes 1/2/3/4/5/7/8/12/13/18/19/20/25/26/28/66)

**LTE Cat-1 vs autres catégories:**

| Catégorie | Débit descendant | Débit montant | Usage |
|-----------|------------------|---------------|-------|
| LTE Cat-1 | 10 Mbps | 5 Mbps | IoT, M2M, télémétrie |
| LTE Cat-4 | 150 Mbps | 50 Mbps | Smartphones |
| LTE Cat-M1 (eMTC) | 1 Mbps | 1 Mbps | IoT basse consommation |
| NB-IoT | 250 kbps | 250 kbps | Capteurs ultra basse consommation |

**Cat-1** est idéal pour l'IoT car:
- ✅ Débit suffisant pour télémétrie, GPS, images basse résolution
- ✅ Couverture mondiale (fallback 2G/3G)
- ✅ Coût modéré
- ✅ Consommation raisonnable

**Communication AT Commands:**

Les modems cellulaires se contrôlent via **commandes AT** (ATtention commands), héritées des modems téléphoniques des années 80.

**Format:**
```
AT+COMMANDE=PARAMETRE
```

**Exemples:**
- `AT` → Test de communication
- `AT+CPIN?` → Vérifier le PIN de la SIM
- `AT+CREG?` → État de l'enregistrement réseau
- `AT+CSQ` → Qualité du signal

**Communication série avec le modem:**

Sur le LilyGO, l'ESP32 communique avec le A7670E via **UART série**:
```
ESP32 (Serial1) ←→ A7670E Modem
  TX → RX
  RX ← TX
```

**TinyGSM:** Bibliothèque qui abstrait les commandes AT et facilite l'utilisation du modem.

**APN (Access Point Name):**

L'**APN** est le point d'accès réseau de votre opérateur cellulaire. C'est comme le "SSID" du réseau cellulaire.

**APNs courants au Canada:**
- **Rogers:** `internet.com`
- **Bell/Virgin:** `inet.bell.ca` ou `pda.bell.ca`
- **Telus/Koodo:** `sp.telus.com`
- **Fido:** `internet.fido.ca`
- **Chatr:** `chatrweb.apn`

### 3.1 Vérification matérielle

**Avant de commencer:**

<div style="background:#fee2e2; border:1px solid #ef4444; padding:10px 12px; border-radius:10px;">
<strong>⚠️ Checklist matériel</strong>
<ul>
  <li>✅ Carte SIM activée et insérée correctement (coin coupé vers le connecteur)</li>
  <li>✅ Antenne LTE vissée sur le connecteur LTE (pas GPS!)</li>
  <li>✅ Antenne GPS vissée sur le connecteur GPS</li>
  <li>✅ Alimentation stable (USB-C du PC ou adaptateur)</li>
  <li>✅ PIN de la SIM désactivé (ou connu)</li>
</ul>

<strong>🔧 Désactiver le PIN de la SIM</strong>
<p>Insérez la SIM dans un téléphone et allez dans:</p>
<code>Réglages → Sécurité → Carte SIM → Verrouillage PIN → Désactiver</code>
<p>Cela évite d'avoir à saisir le PIN à chaque connexion.</p>
</div>

**Identifier les connecteurs:**

```
LilyGO A7670E (vue de dessus):
┌─────────────────────────────┐
│  [ANT GPS]      [ANT LTE]   │ ← Connecteurs antennes (à visser)
│                              │
│      [Slot SIM] →            │ ← Carte SIM (face vers le bas)
│                              │
│         [ESP32]              │
│                              │
│      [USB-C]                 │ ← Alimentation et programmation
└─────────────────────────────┘
```

### 3.2 Code de test du modem

#### Créer le projet

```bash
mkdir -p ~/243-4J5-LI/labo2/modem-test
cd ~/243-4J5-LI/labo2/modem-test
nano modem-test.ino
```

#### Code de diagnostic

```cpp
// Test du modem A7670E
// Vérifie la SIM, le signal et l'enregistrement réseau

#define MODEM_TX 27
#define MODEM_RX 26
#define MODEM_PWRKEY 4
#define MODEM_DTR 32
#define MODEM_RI 33
#define MODEM_FLIGHT 25
#define MODEM_STATUS 34

HardwareSerial SerialAT(1);  // UART1 pour le modem

void setup() {
  Serial.begin(115200);
  delay(2000);

  Serial.println("=========================");
  Serial.println("Test Modem A7670E");
  Serial.println("=========================");

  // Configuration pins
  pinMode(MODEM_PWRKEY, OUTPUT);
  pinMode(MODEM_FLIGHT, OUTPUT);
  digitalWrite(MODEM_FLIGHT, HIGH);  // Mode normal

  // Initialiser communication série avec modem
  SerialAT.begin(115200, SERIAL_8N1, MODEM_RX, MODEM_TX);

  Serial.println("Démarrage du modem...");
  powerOnModem();

  delay(5000);  // Laisser le modem s'initialiser

  // Tests
  testModem();
}

void loop() {
  // Relayer les commandes AT manuelles
  if (Serial.available()) {
    SerialAT.write(Serial.read());
  }
  if (SerialAT.available()) {
    Serial.write(SerialAT.read());
  }
}

void powerOnModem() {
  // Séquence de démarrage
  digitalWrite(MODEM_PWRKEY, HIGH);
  delay(100);
  digitalWrite(MODEM_PWRKEY, LOW);
  delay(1000);
  digitalWrite(MODEM_PWRKEY, HIGH);

  Serial.println("✓ Séquence power ON envoyée");
}

void testModem() {
  Serial.println("\n--- Tests de communication ---");

  // Test 1: Communication basique
  Serial.print("Test AT... ");
  sendAT("AT");
  delay(500);

  // Test 2: Identité du modem
  Serial.print("\nInfo modem: ");
  sendAT("ATI");
  delay(500);

  // Test 3: Vérifier SIM
  Serial.print("\nTest SIM: ");
  sendAT("AT+CPIN?");
  delay(500);

  // Test 4: Qualité signal
  Serial.print("\nQualité signal: ");
  sendAT("AT+CSQ");
  delay(500);

  // Test 5: Enregistrement réseau
  Serial.print("\nEnregistrement réseau: ");
  sendAT("AT+CREG?");
  delay(500);

  // Test 6: Opérateur
  Serial.print("\nOpérateur: ");
  sendAT("AT+COPS?");
  delay(1000);

  Serial.println("\n--- Tests terminés ---");
  Serial.println("Vous pouvez maintenant envoyer des commandes AT manuellement.");
}

void sendAT(const char* cmd) {
  SerialAT.println(cmd);
  delay(100);

  // Lire réponse
  unsigned long timeout = millis() + 2000;
  while (millis() < timeout) {
    if (SerialAT.available()) {
      String response = SerialAT.readString();
      Serial.println(response);
      return;
    }
  }
  Serial.println("✗ Timeout");
}
```

#### Compiler et téléverser

```bash
arduino-cli compile --fqbn esp32:esp32:esp32 modem-test.ino
arduino-cli upload -p /dev/ttyUSB0 --fqbn esp32:esp32:esp32 modem-test.ino
arduino-cli monitor -p /dev/ttyUSB0 -c baudrate=115200
```

### 3.3 Interpréter les résultats

**Réponses attendues:**

```
Test AT...
OK

Info modem:
A7670E

Test SIM:
+CPIN: READY
OK

Qualité signal:
+CSQ: 18,99
OK

Enregistrement réseau:
+CREG: 0,1
OK

Opérateur:
+COPS: 0,0,"Rogers",7
OK
```

<div style="background:#fef9c3; border:1px solid #facc15; padding:10px 12px; border-radius:10px;">
<strong>📖 Interprétation</strong>

<strong>+CPIN: READY</strong>
<ul>
  <li>✅ SIM détectée et prête</li>
  <li>Si <code>+CPIN: SIM PIN</code> → PIN requis</li>
  <li>Si <code>+CME ERROR: 10</code> → SIM absente</li>
</ul>

<strong>+CSQ: 18,99</strong>
<ul>
  <li>Premier nombre (18) = RSSI (Received Signal Strength Indicator)</li>
  <li>0-31 = Signal valide (18 = -89 dBm, bon signal)</li>
  <li>99 = Pas de signal</li>
  <li>Qualité: 0-9: mauvais, 10-14: moyen, 15-19: bon, 20-31: excellent</li>
</ul>

<strong>+CREG: 0,1</strong>
<ul>
  <li>Deuxième nombre (1) = État d'enregistrement</li>
  <li>0 = Non enregistré, pas de recherche</li>
  <li>1 = Enregistré, réseau domestique ✅</li>
  <li>2 = Recherche en cours</li>
  <li>3 = Enregistrement refusé</li>
  <li>5 = Enregistré, itinérance (roaming)</li>
</ul>

<strong>+COPS: 0,0,"Rogers",7</strong>
<ul>
  <li>"Rogers" = Nom de l'opérateur</li>
  <li>7 = LTE (mode réseau)</li>
  <li>Autres: 0=GSM, 2=UMTS, 3=EDGE, 7=LTE</li>
</ul>
</div>

**Si les tests échouent:**

<div style="background:#fee2e2; border:1px solid #ef4444; padding:10px 12px; border-radius:10px;">
<strong>⚡ Dépannage</strong>
<ul>
  <li><strong>Pas de réponse AT:</strong> Vérifier pins TX/RX, baudrate (essayer 9600), séquence de power</li>
  <li><strong>SIM PIN requis:</strong> Désactiver le PIN dans un téléphone</li>
  <li><strong>+CSQ: 99,99:</strong> Vérifier antenne LTE, position (près d'une fenêtre), attendre 1-2 minutes</li>
  <li><strong>+CREG: 0,2 (searching):</strong> Patienter, peut prendre 30s-2min</li>
  <li><strong>+CREG: 0,3 (denied):</strong> SIM non activée, APN incorrect, plan de données requis</li>
</ul>
</div>

<div style="height: 5px; background: linear-gradient(90deg, #22d3ee, #3b82f6); border-radius: 999px; margin: 22px 0;"></div>

## 4. Communication MQTT via LTE
> 🌍 **Objectif :** publier des messages MQTT via le réseau cellulaire.

### 💡 Concepts clés

**Architecture finale:**

```
[LilyGO] → [Modem A7670E] → [Tour cellulaire] → [Internet] → [Broker MQTT Raspberry Pi]
                                                                      ↓
                                                              [Interface tactile]
```

**Avantages:**
- ✅ **Mobilité totale:** Fonctionne n'importe où avec couverture cellulaire
- ✅ **Indépendant du WiFi:** Pas besoin de configuration réseau
- ✅ **Toujours connecté:** Roaming automatique entre antennes
- ✅ **Cas d'usage réels:** Véhicules, zones rurales, déploiements temporaires

**Défis:**
- ❌ Consommation électrique plus élevée que WiFi
- ❌ Latence légèrement supérieure
- ❌ Coût données cellulaires (généralement faible pour MQTT)

### 4.1 Configuration Cloudflare Tunnel pour MQTT

Pour que le LilyGO puisse se connecter au broker MQTT du Raspberry Pi depuis l'extérieur, il faut exposer le port 1883.

#### Option 1: Tunnel MQTT direct (recommandé)

**Sur le Raspberry Pi, éditer la config Cloudflare:**
```bash
nano /home/fpoisson/.cloudflared/config.yml
```

**Ajouter la règle MQTT:**
```yaml
tunnel: <TON-UUID-ICI>
credentials-file: /home/fpoisson/.cloudflared/<TON-UUID-ICI>.json

ingress:
  - hostname: rpi.edxo.ca
    service: ssh://localhost:22
  - hostname: mqtt.edxo.ca
    service: tcp://localhost:1883
  - service: http_status:404
```

**Créer l'entrée DNS:**
```bash
cloudflared tunnel route dns rpi-ssh mqtt.edxo.ca
```

**Redémarrer le tunnel:**
```bash
sudo systemctl restart cloudflared
```

#### Option 2: Broker MQTT public (pour tests)

Utilisez un broker public comme:
- `test.mosquitto.org:1883` (non sécurisé)
- `broker.hivemq.com:1883`

**⚠️ Attention:** Les brokers publics sont **non sécurisés** et visibles par tous. Ne pas utiliser en production!

### 4.2 Code Arduino - MQTT via LTE

#### Créer le projet

```bash
mkdir -p ~/243-4J5-LI/labo2/mqtt-lte
cd ~/243-4J5-LI/labo2/mqtt-lte
nano mqtt-lte.ino
```

#### Code complet

```cpp
#define TINY_GSM_MODEM_SIM7600  // Compatible avec A7670E
#include <TinyGsmClient.h>
#include <PubSubClient.h>

// Pins modem
#define MODEM_TX 27
#define MODEM_RX 26
#define MODEM_PWRKEY 4
#define MODEM_DTR 32
#define MODEM_RI 33
#define MODEM_FLIGHT 25

// Pins LED
#define LED_RED 25
#define LED_GREEN 26

// Configuration réseau cellulaire
const char* apn = "internet.com";  // Rogers - À adapter selon opérateur
const char* gprsUser = "";
const char* gprsPass = "";

// Configuration MQTT
const char* mqtt_server = "mqtt.edxo.ca";  // Ou broker public
const int mqtt_port = 1883;
const char* mqtt_client_id = "lilygo-lte";

// Topics MQTT
const char* topic_command = "iot/led/command";
const char* topic_status = "iot/led/status";
const char* topic_lwt = "iot/lilygo/status";
const char* topic_network = "iot/lilygo/network";

// Clients
HardwareSerial SerialAT(1);
TinyGsm modem(SerialAT);
TinyGsmClient gsmClient(modem);
PubSubClient mqttClient(gsmClient);

void setup() {
  Serial.begin(115200);
  delay(2000);

  // Configuration LEDs
  pinMode(LED_RED, OUTPUT);
  pinMode(LED_GREEN, OUTPUT);
  digitalWrite(LED_RED, LOW);
  digitalWrite(LED_GREEN, LOW);

  // Configuration modem pins
  pinMode(MODEM_PWRKEY, OUTPUT);
  pinMode(MODEM_FLIGHT, OUTPUT);
  digitalWrite(MODEM_FLIGHT, HIGH);

  Serial.println("=========================");
  Serial.println("LilyGO - MQTT via LTE");
  Serial.println("=========================");

  // Initialiser modem
  SerialAT.begin(115200, SERIAL_8N1, MODEM_RX, MODEM_TX);

  Serial.println("Démarrage modem...");
  powerOnModem();
  delay(3000);

  // Connexion réseau cellulaire
  connectCellular();

  // Configuration MQTT
  mqttClient.setServer(mqtt_server, mqtt_port);
  mqttClient.setCallback(mqttCallback);
  mqttClient.setKeepAlive(60);

  // Connexion MQTT
  connectMQTT();
}

void loop() {
  // Maintenir connexions
  if (!modem.isNetworkConnected()) {
    Serial.println("✗ Réseau cellulaire perdu, reconnexion...");
    connectCellular();
  }

  if (!mqttClient.connected()) {
    connectMQTT();
  }
  mqttClient.loop();

  // Heartbeat toutes les 30 secondes
  static unsigned long lastHeartbeat = 0;
  if (millis() - lastHeartbeat > 30000) {
    publishNetworkInfo();
    mqttClient.publish(topic_lwt, "online", true);
    lastHeartbeat = millis();
  }
}

void powerOnModem() {
  digitalWrite(MODEM_PWRKEY, HIGH);
  delay(100);
  digitalWrite(MODEM_PWRKEY, LOW);
  delay(1000);
  digitalWrite(MODEM_PWRKEY, HIGH);
}

void connectCellular() {
  Serial.println("Initialisation du modem...");

  if (!modem.restart()) {
    Serial.println("✗ Échec restart modem");
    delay(5000);
    ESP.restart();
  }

  String modemInfo = modem.getModemInfo();
  Serial.print("Modem: ");
  Serial.println(modemInfo);

  // Attendre enregistrement réseau
  Serial.print("Attente réseau cellulaire");
  if (!modem.waitForNetwork(60000L)) {
    Serial.println("\n✗ Échec enregistrement réseau");
    delay(5000);
    ESP.restart();
  }
  Serial.println(" ✓");

  // Connexion GPRS
  Serial.print("Connexion GPRS");
  if (!modem.gprsConnect(apn, gprsUser, gprsPass)) {
    Serial.println(" ✗ Échec");
    delay(5000);
    ESP.restart();
  }
  Serial.println(" ✓");

  // Afficher info réseau
  printNetworkInfo();
}

void printNetworkInfo() {
  Serial.println("\n--- Info réseau ---");

  int csq = modem.getSignalQuality();
  Serial.print("Signal (CSQ): ");
  Serial.println(csq);

  String cop = modem.getOperator();
  Serial.print("Opérateur: ");
  Serial.println(cop);

  IPAddress ip = modem.localIP();
  Serial.print("IP locale: ");
  Serial.println(ip);

  Serial.println("-------------------\n");
}

void publishNetworkInfo() {
  int csq = modem.getSignalQuality();
  String cop = modem.getOperator();
  IPAddress ip = modem.localIP();

  String payload = "{";
  payload += "\"signal\":" + String(csq) + ",";
  payload += "\"operator\":\"" + cop + "\",";
  payload += "\"ip\":\"" + ip.toString() + "\"";
  payload += "}";

  mqttClient.publish(topic_network, payload.c_str());
}

void connectMQTT() {
  int attempts = 0;
  while (!mqttClient.connected() && attempts < 3) {
    Serial.print("Connexion MQTT à ");
    Serial.print(mqtt_server);
    Serial.print("...");

    if (mqttClient.connect(mqtt_client_id, topic_lwt, 1, true, "offline")) {
      Serial.println(" ✓");

      mqttClient.subscribe(topic_command, 1);
      mqttClient.publish(topic_lwt, "online", true);
      mqttClient.publish(topic_status, "{\"status\":\"ready\",\"connection\":\"lte\"}");

      publishNetworkInfo();

    } else {
      Serial.print(" ✗ rc=");
      Serial.println(mqttClient.state());
      attempts++;
      delay(5000);
    }
  }

  if (!mqttClient.connected()) {
    Serial.println("Échec connexion MQTT, redémarrage...");
    delay(5000);
    ESP.restart();
  }
}

void mqttCallback(char* topic, byte* payload, unsigned int length) {
  String message = "";
  for (int i = 0; i < length; i++) {
    message += (char)payload[i];
  }

  Serial.print("MQTT [");
  Serial.print(topic);
  Serial.print("]: ");
  Serial.println(message);

  if (String(topic) == topic_command) {
    handleCommand(message);
  }
}

void handleCommand(String command) {
  command.trim();
  command.toLowerCase();

  if (command == "red") {
    digitalWrite(LED_RED, HIGH);
    digitalWrite(LED_GREEN, LOW);
    mqttClient.publish(topic_status, "{\"led\":\"red\",\"state\":\"on\"}");
    Serial.println("→ LED ROUGE");

  } else if (command == "green") {
    digitalWrite(LED_RED, LOW);
    digitalWrite(LED_GREEN, HIGH);
    mqttClient.publish(topic_status, "{\"led\":\"green\",\"state\":\"on\"}");
    Serial.println("→ LED VERTE");

  } else if (command == "off") {
    digitalWrite(LED_RED, LOW);
    digitalWrite(LED_GREEN, LOW);
    mqttClient.publish(topic_status, "{\"led\":\"all\",\"state\":\"off\"}");
    Serial.println("→ LEDs OFF");

  } else if (command == "status") {
    publishNetworkInfo();

  } else {
    mqttClient.publish(topic_status, "{\"error\":\"unknown_command\"}");
  }
}
```

<div style="background:#fef9c3; border:1px solid #facc15; padding:10px 12px; border-radius:10px;">
<strong>⚙️ À personnaliser</strong>
<ul>
  <li><code>apn</code> → APN de votre opérateur cellulaire</li>
  <li><code>mqtt_server</code> → Votre domaine Cloudflare (mqtt.edxo.ca) ou broker public</li>
  <li><code>LED_RED</code> / <code>LED_GREEN</code> → Pins selon votre circuit</li>
</ul>
</div>

#### Compilation et test

```bash
arduino-cli compile --fqbn esp32:esp32:esp32 mqtt-lte.ino
arduino-cli upload -p /dev/ttyUSB0 --fqbn esp32:esp32:esp32 mqtt-lte.ino
arduino-cli monitor -p /dev/ttyUSB0 -c baudrate=115200
```

**Sortie attendue:**
```
=========================
LilyGO - MQTT via LTE
=========================
Démarrage modem...
Initialisation du modem...
Modem: A7670E
Attente réseau cellulaire ✓
Connexion GPRS ✓

--- Info réseau ---
Signal (CSQ): 18
Opérateur: Rogers
IP locale: 10.177.xxx.xxx
-------------------

Connexion MQTT à mqtt.edxo.ca... ✓
```

### 4.3 Test de bout en bout

**Sur le Raspberry Pi (ou PC distant):**

```bash
# S'abonner aux topics
mosquitto_sub -h localhost -t "iot/#" -v
```

**Envoyer une commande:**
```bash
mosquitto_pub -h localhost -t "iot/led/command" -m "red"
```

**Vous devriez voir:**
- Le LilyGO reçoit la commande (même connecté via LTE!)
- La LED rouge s'allume
- Un message de status est publié sur `iot/led/status`
- Les infos réseau sur `iot/lilygo/network`

**Tester en mobilité:**
- Débranchez le LilyGO de l'USB (utilisez une batterie externe)
- Déplacez-vous dans une autre pièce
- Les commandes fonctionnent toujours via le réseau cellulaire!

<div style="height: 5px; background: linear-gradient(90deg, #c084fc, #22d3ee); border-radius: 999px; margin: 22px 0;"></div>

## 5. Intégration GPS
> 📍 **Objectif :** récupérer la position GPS et l'envoyer via MQTT.

### 💡 Concepts clés

**GNSS vs GPS:**

**GPS** (Global Positioning System) est le système américain. **GNSS** (Global Navigation Satellite System) inclut:
- GPS (USA)
- GLONASS (Russie)
- Galileo (Europe)
- BeiDou (Chine)

Le modem A7670E supporte **GPS + GLONASS + Galileo** simultanément pour une meilleure précision.

**Format NMEA:**

Les données GPS sont transmises au format **NMEA** (National Marine Electronics Association), une norme de communication série.

**Exemple de trame NMEA:**
```
$GNGGA,123519,4807.038,N,01131.000,E,1,08,0.9,545.4,M,46.9,M,,*47
```

**Décodage:**
- `GNGGA` → Type de trame (Global Navigation - GPS Fix Data)
- `123519` → Heure UTC (12:35:19)
- `4807.038,N` → Latitude 48°07.038' Nord
- `01131.000,E` → Longitude 11°31.000' Est
- `1` → Fix quality (1 = GPS fix)
- `08` → Nombre de satellites
- `0.9` → HDOP (précision horizontale)
- `545.4,M` → Altitude 545.4 mètres

**TinyGPSPlus:** Bibliothèque qui parse les trames NMEA et extrait les données facilement.

**Cold start vs Warm start:**
- **Cold start:** Première acquisition GPS, peut prendre 30s-5min (téléchargement almanach satellites)
- **Warm start:** GPS utilisé récemment, acquisition en 5-30s
- **Hot start:** GPS juste éteint, acquisition en <5s

### 5.1 Installation bibliothèque GPS

```bash
arduino-cli lib install "TinyGPSPlus"
```

### 5.2 Code intégré GPS + MQTT LTE

#### Créer le projet

```bash
mkdir -p ~/243-4J5-LI/labo2/gps-mqtt-lte
cd ~/243-4J5-LI/labo2/gps-mqtt-lte
nano gps-mqtt-lte.ino
```

#### Code complet

```cpp
#define TINY_GSM_MODEM_SIM7600
#include <TinyGsmClient.h>
#include <PubSubClient.h>
#include <TinyGPSPlus.h>

// Pins modem
#define MODEM_TX 27
#define MODEM_RX 26
#define MODEM_PWRKEY 4
#define MODEM_FLIGHT 25

// Pins LED
#define LED_RED 25
#define LED_GREEN 26

// Configuration
const char* apn = "internet.com";
const char* mqtt_server = "mqtt.edxo.ca";
const int mqtt_port = 1883;
const char* mqtt_client_id = "lilygo-gps-lte";

// Topics
const char* topic_command = "iot/led/command";
const char* topic_status = "iot/led/status";
const char* topic_gps = "iot/gps/location";
const char* topic_lwt = "iot/lilygo/status";

// Clients
HardwareSerial SerialAT(1);
TinyGsm modem(SerialAT);
TinyGsmClient gsmClient(modem);
PubSubClient mqttClient(gsmClient);
TinyGPSPlus gps;

// GPS tracking
unsigned long lastGPSPublish = 0;
const unsigned long GPS_PUBLISH_INTERVAL = 10000;  // Publier GPS toutes les 10s

void setup() {
  Serial.begin(115200);
  delay(2000);

  pinMode(LED_RED, OUTPUT);
  pinMode(LED_GREEN, OUTPUT);
  pinMode(MODEM_PWRKEY, OUTPUT);
  pinMode(MODEM_FLIGHT, OUTPUT);
  digitalWrite(MODEM_FLIGHT, HIGH);

  Serial.println("=========================");
  Serial.println("LilyGO - GPS + MQTT LTE");
  Serial.println("=========================");

  SerialAT.begin(115200, SERIAL_8N1, MODEM_RX, MODEM_TX);

  powerOnModem();
  delay(3000);

  connectCellular();
  enableGPS();

  mqttClient.setServer(mqtt_server, mqtt_port);
  mqttClient.setCallback(mqttCallback);
  mqttClient.setKeepAlive(60);

  connectMQTT();
}

void loop() {
  // Maintenir connexions
  if (!modem.isNetworkConnected()) {
    connectCellular();
  }
  if (!mqttClient.connected()) {
    connectMQTT();
  }
  mqttClient.loop();

  // Lire données GPS
  readGPS();

  // Publier position GPS périodiquement
  if (millis() - lastGPSPublish > GPS_PUBLISH_INTERVAL) {
    if (gps.location.isValid()) {
      publishGPS();
    } else {
      Serial.println("GPS: En attente de fix...");
      mqttClient.publish(topic_gps, "{\"status\":\"no_fix\"}");
    }
    lastGPSPublish = millis();
  }
}

void powerOnModem() {
  digitalWrite(MODEM_PWRKEY, HIGH);
  delay(100);
  digitalWrite(MODEM_PWRKEY, LOW);
  delay(1000);
  digitalWrite(MODEM_PWRKEY, HIGH);
}

void connectCellular() {
  Serial.println("Connexion réseau cellulaire...");

  if (!modem.restart()) {
    Serial.println("✗ Échec restart");
    delay(5000);
    ESP.restart();
  }

  if (!modem.waitForNetwork(60000L)) {
    Serial.println("✗ Pas de réseau");
    delay(5000);
    ESP.restart();
  }

  if (!modem.gprsConnect(apn, "", "")) {
    Serial.println("✗ Échec GPRS");
    delay(5000);
    ESP.restart();
  }

  Serial.println("✓ Réseau cellulaire connecté");
  Serial.print("IP: ");
  Serial.println(modem.localIP());
}

void enableGPS() {
  Serial.println("Activation GPS...");

  // Activer GPS
  SerialAT.println("AT+CGPS=1,1");
  delay(200);

  // Lire réponse
  while (SerialAT.available()) {
    Serial.write(SerialAT.read());
  }

  Serial.println("✓ GPS activé (acquisition en cours...)");
  Serial.println("Patientez 30s-2min pour le premier fix GPS");
}

void readGPS() {
  // Demander position GPS
  SerialAT.println("AT+CGPSINFO");
  delay(100);

  // Lire et parser réponse NMEA
  while (SerialAT.available()) {
    char c = SerialAT.read();
    gps.encode(c);
  }
}

void publishGPS() {
  String payload = "{";
  payload += "\"latitude\":" + String(gps.location.lat(), 6) + ",";
  payload += "\"longitude\":" + String(gps.location.lng(), 6) + ",";
  payload += "\"altitude\":" + String(gps.altitude.meters(), 1) + ",";
  payload += "\"speed\":" + String(gps.speed.kmph(), 1) + ",";
  payload += "\"satellites\":" + String(gps.satellites.value()) + ",";
  payload += "\"hdop\":" + String(gps.hdop.value() / 100.0, 2) + ",";
  payload += "\"timestamp\":\"" + getGPSTimestamp() + "\"";
  payload += "}";

  mqttClient.publish(topic_gps, payload.c_str());

  Serial.println("GPS publié:");
  Serial.print("  Lat: ");
  Serial.println(gps.location.lat(), 6);
  Serial.print("  Lon: ");
  Serial.println(gps.location.lng(), 6);
  Serial.print("  Alt: ");
  Serial.print(gps.altitude.meters());
  Serial.println(" m");
  Serial.print("  Satellites: ");
  Serial.println(gps.satellites.value());
}

String getGPSTimestamp() {
  if (!gps.time.isValid() || !gps.date.isValid()) {
    return "invalid";
  }

  char timestamp[32];
  snprintf(timestamp, sizeof(timestamp), "%04d-%02d-%02dT%02d:%02d:%02dZ",
           gps.date.year(), gps.date.month(), gps.date.day(),
           gps.time.hour(), gps.time.minute(), gps.time.second());

  return String(timestamp);
}

void connectMQTT() {
  Serial.print("Connexion MQTT...");
  if (mqttClient.connect(mqtt_client_id, topic_lwt, 1, true, "offline")) {
    Serial.println(" ✓");
    mqttClient.subscribe(topic_command, 1);
    mqttClient.publish(topic_lwt, "online", true);
  } else {
    Serial.print(" ✗ rc=");
    Serial.println(mqttClient.state());
    delay(5000);
  }
}

void mqttCallback(char* topic, byte* payload, unsigned int length) {
  String message = "";
  for (int i = 0; i < length; i++) {
    message += (char)payload[i];
  }

  Serial.print("MQTT [");
  Serial.print(topic);
  Serial.print("]: ");
  Serial.println(message);

  if (String(topic) == topic_command) {
    handleCommand(message);
  }
}

void handleCommand(String command) {
  command.trim();
  command.toLowerCase();

  if (command == "red") {
    digitalWrite(LED_RED, HIGH);
    digitalWrite(LED_GREEN, LOW);
    mqttClient.publish(topic_status, "{\"led\":\"red\"}");
  } else if (command == "green") {
    digitalWrite(LED_RED, LOW);
    digitalWrite(LED_GREEN, HIGH);
    mqttClient.publish(topic_status, "{\"led\":\"green\"}");
  } else if (command == "off") {
    digitalWrite(LED_RED, LOW);
    digitalWrite(LED_GREEN, LOW);
    mqttClient.publish(topic_status, "{\"led\":\"off\"}");
  } else if (command == "gps") {
    if (gps.location.isValid()) {
      publishGPS();
    } else {
      mqttClient.publish(topic_gps, "{\"status\":\"no_fix\"}");
    }
  }
}
```

#### Compilation et test

```bash
arduino-cli compile --fqbn esp32:esp32:esp32 gps-mqtt-lte.ino
arduino-cli upload -p /dev/ttyUSB0 --fqbn esp32:esp32:esp32 gps-mqtt-lte.ino
arduino-cli monitor -p /dev/ttyUSB0 -c baudrate=115200
```

### 5.3 Visualisation GPS

**S'abonner aux données GPS:**
```bash
mosquitto_sub -h localhost -t "iot/gps/location" -v
```

**Exemple de message reçu:**
```json
{
  "latitude": 45.501689,
  "longitude": -73.567256,
  "altitude": 35.2,
  "speed": 0.0,
  "satellites": 8,
  "hdop": 1.2,
  "timestamp": "2025-12-05T18:23:45Z"
}
```

**Visualiser sur une carte:**

Utilisez un service comme:
- **Google Maps:** `https://www.google.com/maps?q=45.501689,-73.567256`
- **OpenStreetMap:** `https://www.openstreetmap.org/?mlat=45.501689&mlon=-73.567256&zoom=15`

**Créer un tracker en temps réel:**

Script Python simple pour afficher les positions:

```python
import paho.mqtt.client as mqtt
import json

def on_message(client, userdata, msg):
    data = json.loads(msg.payload.decode())
    lat = data['latitude']
    lon = data['longitude']
    alt = data.get('altitude', 0)
    sats = data.get('satellites', 0)

    print(f"\n📍 Position GPS:")
    print(f"   Lat: {lat:.6f}")
    print(f"   Lon: {lon:.6f}")
    print(f"   Alt: {alt:.1f} m")
    print(f"   Satellites: {sats}")
    print(f"   Google Maps: https://www.google.com/maps?q={lat},{lon}")

client = mqtt.Client("gps-tracker")
client.on_message = on_message
client.connect("localhost", 1883, 60)
client.subscribe("iot/gps/location")
client.loop_forever()
```

<div style="height: 5px; background: linear-gradient(90deg, #10b981, #06b6d4); border-radius: 999px; margin: 22px 0;"></div>

## 6. Projet intégrateur
> 🎯 **Objectif :** combiner tous les éléments dans un système complet.

### 💡 Cahier des charges

Créez un système de tracking IoT complet avec les fonctionnalités suivantes:

<div style="background:#f0fdf4; border:1px solid #22c55e; padding:12px 14px; border-radius:10px;">
<strong>✅ Fonctionnalités requises</strong>

<strong>1. LilyGO (Arduino):</strong>
<ul>
  <li>Connexion LTE avec fallback 3G/2G si nécessaire</li>
  <li>Publication position GPS toutes les 10 secondes</li>
  <li>Contrôle de 2 LEDs via MQTT (rouge/verte)</li>
  <li>Publication de métriques système (signal, batterie si applicable)</li>
  <li>Gestion Last Will Testament</li>
  <li>Reconnexion automatique en cas de perte réseau</li>
</ul>

<strong>2. Interface tactile (Python):</strong>
<ul>
  <li>Affichage de la dernière position GPS</li>
  <li>Affichage du statut de connexion (online/offline)</li>
  <li>Boutons de contrôle LED (rouge/vert/off)</li>
  <li>Affichage de la qualité du signal cellulaire</li>
  <li>Historique des 5 dernières positions</li>
  <li>Bouton pour forcer une mise à jour GPS</li>
</ul>

<strong>3. Logging (Python CLI ou script):</strong>
<ul>
  <li>Enregistrement de toutes les positions GPS dans un fichier CSV</li>
  <li>Timestamp, latitude, longitude, altitude, vitesse, nombre de satellites</li>
  <li>Rotation des logs (nouveau fichier par jour)</li>
</ul>
</div>

### 6.1 Structure de fichiers

```bash
~/243-4J5-LI/labo2/
├── projet-final/
│   ├── lilygo-tracker.ino       # Code Arduino complet
│   ├── touch_ui_tracker.py      # Interface tactile améliorée
│   ├── gps_logger.py            # Script de logging GPS
│   └── README.md                # Documentation du projet
└── logs/
    └── gps_YYYY-MM-DD.csv       # Logs GPS quotidiens
```

### 6.2 Exemple de logger GPS

```python
#!/usr/bin/env python3
# gps_logger.py - Enregistrement des positions GPS depuis MQTT

import paho.mqtt.client as mqtt
import json
import csv
from datetime import datetime
import os

LOG_DIR = os.path.expanduser("~/243-4J5-LI/labo2/logs")
os.makedirs(LOG_DIR, exist_ok=True)

def get_log_filename():
    """Retourne le nom du fichier log du jour"""
    today = datetime.now().strftime("%Y-%m-%d")
    return os.path.join(LOG_DIR, f"gps_{today}.csv")

def log_gps(data):
    """Enregistre une position GPS dans le CSV"""
    filename = get_log_filename()
    file_exists = os.path.isfile(filename)

    with open(filename, 'a', newline='') as csvfile:
        fieldnames = ['timestamp', 'latitude', 'longitude', 'altitude',
                      'speed', 'satellites', 'hdop']
        writer = csv.DictWriter(csvfile, fieldnames=fieldnames)

        if not file_exists:
            writer.writeheader()

        writer.writerow({
            'timestamp': data.get('timestamp', datetime.now().isoformat()),
            'latitude': data.get('latitude', 0),
            'longitude': data.get('longitude', 0),
            'altitude': data.get('altitude', 0),
            'speed': data.get('speed', 0),
            'satellites': data.get('satellites', 0),
            'hdop': data.get('hdop', 0)
        })

    print(f"✓ Position enregistrée: {data['latitude']:.6f}, {data['longitude']:.6f}")

def on_connect(client, userdata, flags, rc):
    print(f"Connecté au broker MQTT (rc: {rc})")
    client.subscribe("iot/gps/location")
    print("En attente de positions GPS...")

def on_message(client, userdata, msg):
    try:
        data = json.loads(msg.payload.decode())
        if 'latitude' in data and 'longitude' in data:
            log_gps(data)
    except json.JSONDecodeError as e:
        print(f"✗ Erreur parsing JSON: {e}")
    except Exception as e:
        print(f"✗ Erreur: {e}")

# Configuration
client = mqtt.Client("gps-logger")
client.on_connect = on_connect
client.on_message = on_message

try:
    client.connect("localhost", 1883, 60)
    print("=========================")
    print("GPS Logger démarré")
    print(f"Fichier: {get_log_filename()}")
    print("=========================")
    client.loop_forever()
except KeyboardInterrupt:
    print("\nArrêt du logger")
    client.disconnect()
```

**Lancer le logger:**
```bash
chmod +x ~/243-4J5-LI/labo2/projet-final/gps_logger.py
python3 ~/243-4J5-LI/labo2/projet-final/gps_logger.py
```

**Pour lancer en arrière-plan:**
```bash
nohup python3 ~/243-4J5-LI/labo2/projet-final/gps_logger.py &
```

### 6.3 Critères d'évaluation

<div style="background:#dbeafe; border:1px solid#3b82f6; padding:12px 14px; border-radius:10px;">
<strong>📊 Grille d'évaluation</strong>

<strong>Fonctionnement (50%):</strong>
<ul>
  <li>Connexion LTE stable (10%)</li>
  <li>GPS fonctionnel avec position précise (15%)</li>
  <li>Communication MQTT bidirectionnelle (10%)</li>
  <li>Contrôle LEDs opérationnel (10%)</li>
  <li>Gestion des déconnexions et reconnexions (5%)</li>
</ul>

<strong>Code (30%):</strong>
<ul>
  <li>Qualité et lisibilité du code Arduino (10%)</li>
  <li>Qualité et lisibilité du code Python (10%)</li>
  <li>Gestion des erreurs (5%)</li>
  <li>Commentaires et documentation (5%)</li>
</ul>

<strong>Documentation (20%):</strong>
<ul>
  <li>README.md complet avec instructions (10%)</li>
  <li>Photos du circuit et du système en fonctionnement (5%)</li>
  <li>Logs GPS sur au moins 30 minutes (5%)</li>
</ul>
</div>

### 6.4 Livraison

**À remettre sur Git:**
```bash
cd ~/243-4J5-LI/labo2/projet-final
git add .
cd ~/243-4J5-LI/labo2/logs
git add *.csv
cd ~/243-4J5-LI
git commit -m "Labo 2 - Projet final: GPS tracker avec MQTT LTE"
git push origin prenom-nom/labo2
```

**Contenu du README.md:**
- Description du projet
- Photos du circuit
- Instructions de déploiement
- Exemples de commandes MQTT
- Capture d'écran de l'interface tactile
- Extrait des logs GPS
- Difficultés rencontrées et solutions

<div style="height: 5px; background: linear-gradient(90deg, #f59e0b, #f97316); border-radius: 999px; margin: 22px 0;"></div>

## 🔮 Au prochain laboratoire

### Évolution vers une architecture cloud

**Labo 3 (aperçu) - Cloud IoT et stockage de données:**

Au prochain laboratoire, vous allez faire évoluer votre système local vers le cloud:

**1. Broker MQTT cloud:**
- Migration de Mosquitto local vers un broker cloud (AWS IoT Core, HiveMQ Cloud, ou CloudMQTT)
- Configuration TLS/SSL pour sécuriser les communications
- Authentification par certificats X.509

**2. Base de données TimeSeries:**
- Stockage des positions GPS dans InfluxDB ou TimescaleDB
- Requêtes temporelles sur l'historique
- Agrégations et analyses

**3. Dashboard web:**
- Interface Node-RED ou Grafana
- Carte interactive avec trajet en temps réel
- Graphiques de métriques (signal, vitesse, altitude)
- Alertes configurables (géofencing, perte de signal)

**4. API REST:**
- Endpoints pour récupérer l'historique
- Webhooks pour notifications
- Intégration avec services tiers

**Architecture cible:**
```
[LilyGO] → [LTE] → [Broker MQTT Cloud] → [InfluxDB]
                           ↓                  ↓
                     [Dashboard Web]    [API REST]
                           ↓                  ↓
                      [Alertes]         [Webhooks]
```

<div style="height: 5px; background: linear-gradient(90deg, #22d3ee, #a855f7); border-radius: 999px; margin: 22px 0;"></div>

## 📚 Commandes de vérification utiles

### MQTT

```bash
# Vérifier Mosquitto
sudo systemctl status mosquitto

# Logs Mosquitto
sudo journalctl -u mosquitto -f

# Tester pub/sub local
mosquitto_sub -h localhost -t "test" -v
mosquitto_pub -h localhost -t "test" -m "hello"

# Écouter tous les topics
mosquitto_sub -h localhost -t "#" -v

# Publier avec QoS et retained
mosquitto_pub -h localhost -t "test" -q 1 -r -m "persistent"
```

### Réseau LTE

```bash
# Vérifier ports série
ls -la /dev/ttyUSB* /dev/ttyACM*

# Permissions port série
sudo usermod -a -G dialout $USER

# Surveiller connexions réseau (sur Raspberry Pi)
watch -n 1 'netstat -an | grep 1883'

# Test de latence vers broker
ping -c 10 mqtt.edxo.ca
```

### Git

```bash
# Statut
git status

# Voir les modifications
git diff

# Historique
git log --oneline --graph --all

# Créer une branche
git checkout -b prenom-nom/labo2

# Pousser une branche
git push -u origin prenom-nom/labo2

# Synchroniser avec main
git fetch origin
git merge origin/main
```

### Arduino CLI

```bash
# Lister les boards connectées
arduino-cli board list

# Lister les bibliothèques installées
arduino-cli lib list

# Chercher une bibliothèque
arduino-cli lib search GPS

# Voir les logs de compilation détaillés
arduino-cli compile --verbose --fqbn esp32:esp32:esp32 sketch.ino

# Monitor avec filtre
arduino-cli monitor -p /dev/ttyUSB0 | grep -E "GPS|MQTT"
```

### Utilitaires système

```bash
# Espace disque
df -h

# Mémoire
free -h

# Processus
htop

# Logs système
sudo journalctl -f

# Température Raspberry Pi
vcgencmd measure_temp

# tmux (sessions multiples)
tmux new -s mqtt
tmux attach -t mqtt
tmux ls
```

---

<div style="background: linear-gradient(90deg, #0ea5e9, #6366f1); padding: 18px 20px; color: #f8fafc; border-radius: 14px; text-align: center;">
  <h2 style="margin: 0;">Félicitations!</h2>
  <p style="margin: 8px 0 0;">Vous maîtrisez maintenant la communication sans fil IoT via MQTT, WiFi, LTE et GPS.</p>
</div>
