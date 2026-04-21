# Projet LoRa Bidirectionnel avec Intelligence Artificielle (LLM)

Ce projet implémente une liaison de données LoRa bidirectionnelle entre deux cartes **LilyGo T-Beam Supreme (ESP32-S3)**. Le système utilise un modèle de langage (LLM via l'API Groq) pour prendre des décisions basées sur des données de capteurs et publie les résultats sur un broker MQTT.

## Architecture du Système

1.  **Émetteur (Node A)** :
    *   Lit la valeur d'un potentiomètre en temps réel.
    *   Envoie une trame JSON via LoRa (déclenchement par bouton BOOT ou automatique toutes les 10s).
    *   Affiche la réponse du LLM sur son écran OLED.
    *   Actionne une LED (GPIO 46) selon la décision reçue.

2.  **Récepteur (Node B)** :
    *   Reçoit la trame LoRa et mesure les performances radio (RSSI/SNR).
    *   Interroge l'API **Groq (LLM)** avec la valeur reçue.
    *   Publie la décision du LLM sur un broker **MQTT** (via WebSockets SSL/443).
    *   Renvoie la décision à l'émetteur via LoRa.
    *   Indique son activité via des clignotements de LED (GPIO 46).

## Matériel Requis

*   2x LilyGo T-Beam Supreme (ESP32-S3, SX1262 LoRa, OLED SH1106).
*   1x Potentiomètre (connecté sur GPIO 2 de l'émetteur).
*   LEDs connectées sur GPIO 46 (pour le retour d'action).

## Installation

### 1. Dépendances Arduino
Installez les bibliothèques suivantes via le Library Manager :
*   **RadioLib** (jgromes)
*   **ArduinoJson** (Benoit Blanchon)
*   **U8g2** (olikraus)
*   **XPowersLib** (Lewis He)
*   **PubSubClient** (Nick O'Leary)

### 2. Configuration
Pour chaque module (émetteur et récepteur) :
1.  Allez dans son dossier respectif (`emetteur-lora` ou `recepteur-lora`).
2.  Copiez le fichier `config_example.h` vers `config.h`.
3.  Remplissez vos identifiants (WiFi, Clé API Groq, Broker MQTT).

> **Note :** Les fichiers `config.h` sont ignorés par Git pour protéger vos données secrètes.

### 3. Compilation et Téléversement
Utilisez les paramètres suivants dans Arduino IDE :
*   **Board** : "ESP32S3 Dev Module"
*   **USB CDC On Boot** : Enabled (Crucial pour le moniteur série)
*   **PSRAM** : "OPI PSRAM"
*   **Flash Mode** : QIO 80MHz

Via `arduino-cli` :
```bash
arduino-cli compile --fqbn esp32:esp32:esp32s3:CDCOnBoot=cdc,PSRAM=opi --upload -p /dev/ttyACMX
```

## Utilisation

### Émetteur
*   **Au démarrage** : L'OLED affiche la valeur du potentiomètre en direct ("Monitoring").
*   **Envoi** : Appuyez sur le bouton **BOOT (0)** ou attendez le décompte de 10s.
*   **Résultat** : La trame envoyée s'affiche, puis la réponse du LLM apparaît après réception. La LED 46 s'allume si l'action est "on".
*   **Reset** : Le système revient en mode monitoring après 4s (ou via un appui bouton).

### Récepteur
*   **Attente** : Affiche "LoRA RX Pret! En attente...".
*   **Traitement** : À la réception, il affiche le JSON reçu, le RSSI/SNR et lance l'appel LLM.
*   **Retour** : Il affiche la réponse générée, la renvoie à l'émetteur et publie sur le topic MQTT : `etudiant/VOTRE_NOM/lora/decision`.

## Structure des Dossiers
*   `emetteur-lora/` : Code source du noeud capteur.
*   `recepteur-lora/` : Code source du noeud passerelle LLM/MQTT.
*   `llm-t-beam-supreme/` : Code de référence original.

---
*Projet réalisé dans le cadre du cours Objets Connectés - Cégep de Limoilou.*
