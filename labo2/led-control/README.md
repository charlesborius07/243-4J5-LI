# Contrôle de LEDs LilyGo via MQTT et Interface Tactile

Ce programme permet de contrôler les LEDs d'un ESP32 LilyGo via MQTT en utilisant une interface tactile.

## 🔧 Configuration

### 1. Installation des dépendances

**Option 1: Via apt (recommandé pour Raspberry Pi OS)**

```bash
sudo apt update
sudo apt install python3-paho-mqtt python3-evdev
```

**Option 2: Via pip (si les paquets apt ne sont pas disponibles)**

```bash
sudo pip3 install -r requirements.txt
# Ou si erreur "externally-managed-environment":
sudo pip3 install --break-system-packages -r requirements.txt
```

### 2. Création du fichier de configuration

Le fichier `mqtt_config.py` contient vos identifiants et n'est **pas versionné** (pour des raisons de sécurité).

```bash
# Copiez le fichier template
cp mqtt_config.py.example mqtt_config.py
```

### 3. Configuration du Device ID et des identifiants

**IMPORTANT**: Vous devez configurer le `device_id` de votre ESP32 et vos identifiants MQTT dans le fichier `mqtt_config.py`.

Pour trouver votre Device ID:

1. Connectez votre ESP32 LilyGo via USB
2. Ouvrez un moniteur série à 115200 bauds:
   ```bash
   # Option 1: Via Arduino IDE
   # Outils > Moniteur série

   # Option 2: Via screen
   screen /dev/ttyACM0 115200

   # Option 3: Via minicom
   minicom -D /dev/ttyACM0 -b 115200
   ```
3. Après la connexion WiFi, cherchez la ligne:
   ```
   Device ID: esp32-XXXXXX
   ```
4. Copiez cette valeur et modifiez le fichier `mqtt_config.py`:
   ```python
   "device_id": "esp32-123456",  # Remplacez par votre ID réel
   "password": "votre_mot_de_passe",  # Votre mot de passe Mosquitto
   ```

### 4. Vérification de la configuration MQTT

Éditez `mqtt_config.py` et vérifiez que tous les paramètres sont corrects:

- `broker`: Adresse du broker MQTT (par défaut: `mqtt.edxo.ca`)
- `port`: Port WSS (par défaut: `443`)
- `username`: Nom d'utilisateur Mosquitto
- `password`: Mot de passe Mosquitto
- `device_id`: **OBLIGATOIRE** - ID de votre ESP32

## 🚀 Utilisation

### Lancement du programme

**Méthode 1: Depuis SSH (pour tests)**

```bash
cd /home/fpoisson/243-4J5-LI/labo2/led-control
sudo python3 touch_ui_mqtt.py
```

**Méthode 2: Sur l'écran tactile local (interface complète)**

Pour lancer directement sur l'écran tactile Raspberry Pi (recommandé):

**Option A: Avec le script helper**
```bash
cd /home/fpoisson/243-4J5-LI/labo2/led-control
./launch_on_screen.sh
```

**Option B: Commande manuelle**
```bash
# Passer sur le terminal virtuel 1 (écran physique)
sudo chvt 1

# Lancer le programme sur tty1
sudo setsid sh -c 'exec </dev/tty1 >/dev/tty1 2>&1 python3 /home/fpoisson/243-4J5-LI/labo2/led-control/touch_ui_mqtt.py'
```

**Pour revenir au bureau graphique:**
- Appuyez sur `Ctrl+Alt+F7` (ou F8 selon la configuration)
- Ou depuis SSH: `sudo chvt 7`

### Interface

L'interface affiche 5 boutons tactiles:

- **LED 1 ON (ROUGE)**: Allume la LED 1 du LilyGo
- **LED 1 OFF**: Éteint la LED 1 du LilyGo
- **LED 2 ON (VERT)**: Allume la LED 2 du LilyGo
- **LED 2 OFF**: Éteint la LED 2 du LilyGo
- **QUIT**: Quitte le programme

### Indicateurs

- **● MQTT: Connecté** (vert): Connexion MQTT établie
- **○ MQTT: Déconnecté** (rouge): Connexion MQTT perdue
- **Feedback MQTT**: Affiche les messages envoyés (→) et reçus (←)
- **Status**: Affiche le dernier événement ou erreur

### Raccourci clavier

- Appuyez sur `q` pour quitter le programme

## 🔍 Dépannage

### Erreur "ModuleNotFoundError: No module named 'paho'"

Les dépendances Python ne sont pas installées. Installez-les avec:

```bash
# Méthode recommandée (apt)
sudo apt update
sudo apt install python3-paho-mqtt python3-evdev

# Alternative (pip)
sudo pip3 install paho-mqtt evdev
# Ou si erreur "externally-managed-environment":
sudo pip3 install --break-system-packages paho-mqtt evdev
```

### Le programme ne trouve pas le touchscreen

Vérifiez les périphériques disponibles:
```bash
ls -l /dev/input/event*
```

### Erreur "MQTT: Déconnecté"

1. Vérifiez que l'ESP32 est bien connecté et fonctionne
2. Vérifiez le Device ID dans `mqtt_config.py`
3. Vérifiez les identifiants MQTT (username/password)
4. Vérifiez que le broker est accessible:
   ```bash
   ping mqtt.edxo.ca
   ```

### Erreur "Username/Password incorrect"

Vérifiez que les identifiants dans `mqtt_config.py` correspondent à ceux configurés sur le broker Mosquitto.

### Les LEDs ne répondent pas

1. Vérifiez que l'ESP32 est connecté au broker MQTT
2. Vérifiez le Device ID (doit correspondre exactement)
3. Ouvrez le moniteur série de l'ESP32 pour voir si les messages MQTT sont reçus

## 📡 Topics MQTT

Le programme utilise automatiquement les topics suivants:

- **Publication** (envoi):
  - `{device_id}/led/1/set` → "ON" ou "OFF"
  - `{device_id}/led/2/set` → "ON" ou "OFF"

- **Souscription** (réception):
  - `{device_id}/button/1/state` → "PRESSED" ou "RELEASED"
  - `{device_id}/button/2/state` → "PRESSED" ou "RELEASED"

Exemple avec `device_id = "esp32-123456"`:
- `esp32-123456/led/1/set`
- `esp32-123456/led/2/set`
- `esp32-123456/button/1/state`
- `esp32-123456/button/2/state`

## 🔒 Sécurité

La connexion MQTT utilise:
- **WebSocket Secure (WSS)** sur le port 443
- **SSL/TLS** pour le chiffrement
- **Authentification** username/password

## 📝 Différences avec la version série

Par rapport au code original (`labo1/led-control/touch_ui_led.py`):

1. ✅ Communication **MQTT** au lieu de port série
2. ✅ Connexion **WSS** (WebSocket Secure) sécurisée
3. ✅ Support de **5 boutons** (2 LEDs × ON/OFF + QUIT)
4. ✅ **Authentification** via username/password
5. ✅ **Feedback bidirectionnel** (réception des messages MQTT)
6. ✅ Configuration **externalisée** (mqtt_config.py)
