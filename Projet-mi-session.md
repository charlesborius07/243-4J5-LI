# Projet de mi-session — Système IoT complet avec Shield LilyGO A7670G

> Objectif : concevoir un **système IoT complet** intégrant un shield PCB pour le LilyGO A7670G, un programme embarqué, la communication MQTT et une interface sur Raspberry Pi 5.

---

## Vue d'ensemble

Ce projet combine toutes les compétences acquises depuis le début de la session:

```
┌─────────────────────┐         MQTT/LTE          ┌─────────────────────┐
│  LilyGO A7670G      │ ─────────────────────────▶│  Raspberry Pi 5     │
│  + Shield PCB       │                           │                     │
│                     │                           │  • Broker Mosquitto │
│  • 2 DELs           │◀───────────────────────── │  • Interface Python │
│  • 2 Boutons        │      Commandes MQTT       │  • Cloudflare Tunnel│
│  • 2 Potentiomètres │                           │                     │
│  • Accéléromètre    │                           └─────────────────────┘
└─────────────────────┘
```

---

## Partie 1 : Shield PCB

### Cahier des charges matériel

Concevez un shield qui intègre:

| Composant | Quantité | Spécifications |
|-----------|----------|----------------|
| DELs | 2 | Avec résistances de limitation (330Ω) |
| Boutons poussoirs | 2 | Avec pull-up/pull-down appropriés |
| Potentiomètres | 2 | 10kΩ pour entrées analogiques |
| Accéléromètre | 1 | MPU6050 ou ADXL345 (I2C) |
| Connecteurs | - | Headers compatibles LilyGO A7670G |

### Livrables PCB

1. **Prototype breadboard** fonctionnel
2. **Schéma KiCad** (ERC sans erreurs)
3. **Layout PCB** (DRC sans erreurs, Gerbers générés)
4. **Documentation** (schéma, rendu 3D, BOM)

---

## Partie 2 : Programme embarqué (LilyGO A7670G)

### Exigences

Le programme sur le LilyGO A7670G doit:

1. **Lire les capteurs** du shield (boutons, potentiomètres, accéléromètre)
2. **Contrôler les actionneurs** (DELs)
3. **Publier les données** vers le broker MQTT sur le Raspberry Pi 5 via LTE
4. **Recevoir des commandes** MQTT pour contrôler les DELs ou modifier le comportement

### Structure MQTT suggérée

```
Votre topic racine: etudiant/{prenom-nom}/

Publications (LilyGO → Broker):
  etudiant/{prenom-nom}/sensors/buttons      → {"btn1": true, "btn2": false}
  etudiant/{prenom-nom}/sensors/pots         → {"pot1": 512, "pot2": 1023}
  etudiant/{prenom-nom}/sensors/accel        → {"x": 0.12, "y": -0.05, "z": 9.81}
  etudiant/{prenom-nom}/status               → {"uptime": 3600, "rssi": -65}

Souscriptions (Broker → LilyGO):
  etudiant/{prenom-nom}/actuators/led1       → {"state": "on"} ou {"state": "off"}
  etudiant/{prenom-nom}/actuators/led2       → {"state": "on"} ou {"state": "off"}
  etudiant/{prenom-nom}/config               → {"interval": 1000}
```

---

## Partie 3 : Interface Raspberry Pi 5

### Exigences

L'interface Python sur le Raspberry Pi 5 doit:

1. **Afficher les données** reçues des capteurs en temps réel
2. **Permettre le contrôle** des DELs via l'interface
3. **Être fonctionnelle** sur l'écran tactile du Raspberry Pi

### Fonctionnalités minimales

- Affichage des valeurs des potentiomètres
- Affichage de l'état des boutons
- Affichage des données de l'accéléromètre
- Boutons pour contrôler les DELs à distance

---

## Partie 4 : Idées de projets (au choix)

La fonctionnalité exacte de votre système est **libre**. Voici des idées pour vous inspirer:

### Idées simples

| Projet | Description |
|--------|-------------|
| **Télécommande IoT** | Les boutons et potentiomètres contrôlent des paramètres affichés sur l'interface Pi |
| **Moniteur d'inclinaison** | L'accéléromètre détecte l'orientation, les DELs indiquent si l'appareil est à niveau |
| **Compteur d'événements** | Chaque pression de bouton incrémente un compteur affiché sur le Pi |
| **Dimmer à distance** | Les potentiomètres ajustent l'intensité (PWM) des DELs |

### Idées intermédiaires

| Projet | Description |
|--------|-------------|
| **Détecteur de mouvement** | L'accéléromètre déclenche une alerte (DEL + notification) lors d'un mouvement |
| **Jeu Simon** | Séquence de DELs à reproduire avec les boutons, score affiché sur le Pi |
| **Contrôleur de seuils** | Les potentiomètres définissent des seuils, l'accéléromètre déclenche des alertes |
| **Tableau de bord véhicule** | Simulation d'indicateurs (vitesse=pot1, régime=pot2, alertes=accéléromètre) |

### Idées avancées

| Projet | Description |
|--------|-------------|
| **Podomètre IoT** | L'accéléromètre compte les pas, historique affiché sur le Pi |
| **Système d'alarme** | Armement par bouton, détection par accéléromètre, notification sur le Pi |
| **Contrôleur MIDI** | Les potentiomètres et boutons envoient des valeurs interprétables comme contrôleur |
| **Station météo mobile** | Ajout d'un capteur temp/humidité, données GPS incluses |

### Votre propre idée

Vous pouvez proposer votre propre projet. Il doit:
- Utiliser **au moins 3 des 4 types d'entrées** (boutons, potentiomètres, accéléromètre)
- Utiliser **les 2 DELs** comme indicateurs
- Avoir une **logique applicative** cohérente
- Être **documenté** clairement

---

## Structure de remise

```bash
~/243-4J5-LI/projet-mi-session/
├── kicad/                      # Schéma et PCB
│   ├── shield.kicad_pro
│   ├── shield.kicad_sch
│   └── shield.kicad_pcb
├── firmware/                   # Code LilyGO A7670G
│   ├── src/
│   │   └── main.cpp
│   ├── platformio.ini
│   └── README.md               # Instructions de compilation
├── interface/                  # Interface Python Raspberry Pi
│   ├── main.py
│   ├── requirements.txt
│   └── README.md               # Instructions d'exécution
├── fabrication/                # Fichiers de fabrication PCB
│   ├── gerbers/
│   ├── bom.csv
│   └── fabrication-readme.md
├── docs/                       # Documentation
│   ├── photos/
│   ├── screenshots/
│   └── demo-video.mp4          # Vidéo de démonstration (optionnel)
└── README.md                   # Vue d'ensemble du projet
```

---

## Critères d'évaluation

| Critère | Pondération | Description |
|---------|-------------|-------------|
| **Shield PCB** | 30% | Schéma complet, ERC/DRC propres, routage cohérent, prototype fonctionnel |
| **Programme embarqué** | 30% | Lecture des capteurs, publication MQTT, réception de commandes |
| **Interface Raspberry Pi** | 20% | Affichage des données, contrôle des actionneurs, ergonomie |
| **Documentation** | 20% | README clair, schémas inclus, instructions, vidéo/photos |

---

## Livraison

```bash
cd ~/243-4J5-LI/projet-mi-session
git add .
git commit -m "Projet mi-session : système IoT complet"
git push origin prenom-nom/projet-mi-session
```

**Date de remise:** Semaine 7

---

## Ressources utiles

### Code de base (à partir des labos 1 et 2)

Votre projet doit **partir du code développé dans les labos précédents**:

#### Firmware LilyGO A7670G (Labo 2)

| Fichier | Description |
|---------|-------------|
| `labo2/code/lilygo_lte_mqtt/lilygo_lte_mqtt.ino` | Communication MQTT via LTE — **point de départ pour le firmware** |
| `labo2/code/lilygo_wifi_mschapv2/lilygo_wifi_mschapv2.ino` | Alternative WiFi si disponible |
| `labo1/lilygo-test/lilygo-test.ino` | Test de base du LilyGO |

**À ajouter au code du Labo 2:**
- Lecture des entrées analogiques (potentiomètres) avec `analogRead()`
- Lecture des entrées numériques (boutons) avec `digitalRead()`
- Communication I2C avec l'accéléromètre (bibliothèque MPU6050 ou ADXL345)
- Publication des données capteurs sur vos topics MQTT
- Souscription aux topics de commande pour les DELs

#### Interface Python Raspberry Pi (Labos 1 et 2)

| Fichier | Description |
|---------|-------------|
| `labo2/led-control/touch_ui_mqtt.py` | Interface tactile avec MQTT — **point de départ pour l'interface** |
| `labo1/led-control/touch_ui_led.py` | Interface tactile de base |
| `labo1/code/touch_ui.py` | Exemple d'interface pygame |

**À ajouter au code du Labo 2:**
- Affichage des données des capteurs (potentiomètres, boutons, accéléromètre)
- Graphiques ou jauges pour visualiser les valeurs
- Boutons de contrôle pour les DELs distantes
- Logique applicative selon votre projet choisi

### Outils

| Outil | Usage |
|-------|-------|
| KiCad | Schéma et PCB |
| Arduino IDE / PlatformIO | Firmware LilyGO |
| Python 3 + pygame + paho-mqtt | Interface Raspberry Pi |

### Documentation externe

- [LilyGO A7670G Wiki](https://github.com/Xinyuan-LilyGO/LilyGO-T-A7670)
- [MPU6050 Arduino Library](https://github.com/ElectronicCats/mpu6050)
- [ADXL345 Arduino Library](https://github.com/adafruit/Adafruit_ADXL345)
- [Paho MQTT Python](https://pypi.org/project/paho-mqtt/)
