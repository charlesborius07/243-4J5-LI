# Projet LoRa Bidirectionnel avec Intelligence Artificielle (LLM)

Ce projet implémente une liaison de données LoRa bidirectionnelle entre deux cartes **LilyGo T-Beam Supreme (ESP32-S3)**. Le système utilise un modèle de langage (LLM via l'API Groq) pour prendre des décisions basées sur des données de capteurs et publie les résultats sur un broker MQTT.

## Architecture du Système

1.  **Émetteur (Node A)** :
    *   **Monitoring** : Lit la valeur d'un potentiomètre en temps réel et l'affiche sur l'OLED.
    *   **Transmission** : Envoie une trame JSON via LoRa. Déclenchement manuel via le bouton **BOOT** ou automatique toutes les **10 secondes** (avec compte à rebours à l'écran).
    *   **Réception** : Affiche la réponse JSON du LLM en plein écran avec une police compacte.
    *   **Action** : Actionne une LED (**GPIO 46**) selon la décision du LLM (`on`/`off`).
    *   **Auto-Reset** : Revient en mode monitoring après 4s d'affichage du résultat.

2.  **Récepteur (Node B)** :
    *   **Écoute** : Reçoit la trame LoRa et mesure les performances radio (**RSSI/SNR**).
    *   **Intelligence** : Interroge l'API **Groq (LLM)**. En cas d'erreur, effectue **3 tentatives** avec un délai croissant. Si l'échec persiste, bascule en mode **Fallback** (`action: off`).
    *   **IoT & Cloud** : Publie la décision sur un broker **MQTT** (WebSockets SSL sur port 443) et la renvoie à l'émetteur via LoRa.
    *   **Interface** : OLED affiche simultanément la trame reçue, la télémétrie et la réponse LLM en police compacte. La LED (**GPIO 46**) clignote lors des échanges (RX, LLM, TX).

## Matériel Requis

*   2x LilyGo T-Beam Supreme (ESP32-S3, SX1262 LoRa, OLED SH1106).
*   1x Potentiomètre (connecté sur GPIO 2 de l'émetteur).
*   LEDs connectées sur GPIO 46 sur les deux cartes.

## Robustesse Logicielle

Le code du récepteur intègre des fonctions avancées de fiabilité :
*   **WiFi & MQTT Watchdog** : Reconnexion automatique et silencieuse en arrière-plan si la connexion est perdue.
*   **LLM Retry Logic** : Gestion des timeouts API avec exponentiel backoff (2s, 4s).
*   **WSS MQTT** : Utilisation du protocole WebSocket Secure pour contourner les restrictions de pare-feu (port 443).

## Installation

### 1. Dépendances Arduino
Installez les bibliothèques suivantes via le Library Manager :
*   **RadioLib**, **ArduinoJson**, **U8g2**, **XPowersLib**, **PubSubClient**.

### 2. Configuration Secrets
Pour chaque module (`emetteur-lora` et `recepteur-lora`) :
1.  Copiez `config_example.h` vers `config.h`.
2.  Remplissez vos identifiants (WiFi, Clé API Groq, Broker MQTT).
> Les fichiers `config.h` sont protégés par le `.gitignore` du projet.

### 3. Compilation
Paramètres requis :
*   **Board** : "ESP32S3 Dev Module"
*   **USB CDC On Boot** : Enabled
*   **PSRAM** : "OPI PSRAM"

## Utilisation

### Émetteur
*   **IDLE** : Affiche "Monitoring Pot" et un décompte de 10s.
*   **Action** : Appuyez sur **BOOT** pour forcer l'envoi immédiat.
*   **Résultat** : Affiche le JSON reçu du LLM. La LED s'allume si `action: on`.

### Récepteur
*   **Attente** : Affiche "LoRA RX Pret! En attente...".
*   **Cycle** : À la réception, la LED clignote et l'écran se remplit avec les données brutes et la décision. Il revient au repos après 5s.

---
*Projet réalisé dans le cadre du cours Objets Connectés - Cégep de Limoilou.*
