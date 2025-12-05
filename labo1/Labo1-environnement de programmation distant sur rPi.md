<div style="background: linear-gradient(90deg, #0ea5e9, #6366f1); padding: 18px 20px; color: #f8fafc; border-radius: 14px; box-shadow: 0 4px 12px rgba(0,0,0,0.15);">
  <h1 style="margin: 0; font-size: 28px;">Labo 1 — Environnement de programmation distant pour objets connectés</h1>
  <p style="margin: 6px 0 0; font-size: 15px;">Guide pas à pas pour préparer un Raspberry Pi 5, le connecter au réseau et déployer les outils nécessaires.</p>
</div>

---

## 🧭 Plan du guide
- [Matériel requis](#-matériel-requis)
- [Installation Ubuntu Server](#1-installation-ubuntu-server)
- [Configuration réseau](#2-configuration-réseau)
- [Connexion à distance via Cloudflare Tunnel](#3-connexion-à-distance-via-cloudflare-tunnel)
- [Configuration Git](#4-configuration-git)
- [Interface tactile en mode console](#5-interface-tactile-distante-en-mode-console)
- [Installation Node.js et outils CLI](#6-installation-nodejs-et-outils-cli)
- [Programmation du LilyGO A7670E](#7-programmation-du-lilygo-a7670e)
- [Notes importantes](#-notes-importantes)
- [Commandes de vérification](#-commandes-de-vérification-utiles)

<div style="height: 6px; background: linear-gradient(90deg, #22d3ee, #22c55e); border-radius: 999px; margin: 18px 0;"></div>

## 🎒 Matériel requis
<div style="background:#ecfeff; border:1px solid #06b6d4; padding:12px 14px; border-radius:10px;">
<ul style="margin:0;">
  <li>Clavier Raspberry Pi</li>
  <li>Raspberry Pi 5</li>
  <li>Écran tactile pour Raspberry Pi 5</li>
  <li>Alimentation USB-C pour Raspberry Pi 5</li>
  <li>Câble micro-USB pour clavier</li>
  <li>Carte micro SD 64 GB</li>
  <li>LilyGO A7670E avec antenne GPS et LTE</li>
  <li>Carte SIM</li>
  <li>Câble USB-A à USB-C</li>
</ul>
</div>

<div style="height: 6px; background: linear-gradient(90deg, #22c55e, #84cc16); border-radius: 999px; margin: 22px 0;"></div>

## 1. Installation Ubuntu Server
> 🎯 **Objectif :** préparer la carte SD avec Ubuntu Server, SSH et l'écran tactile.

### Préparation de la carte SD
1. Installer Ubuntu Server (dernière version LTS) sur le Raspberry Pi 5
2. Lors de la préparation de la carte micro-SD:
   - Activer SSH
   - Configurer username, password et hostname
3. Pendant la préparation, installer l'écran sur le Raspberry Pi 5 (suivre les instructions attentivement)
4. Brancher le clavier sur le Raspberry Pi 5

<div style="height: 5px; background: linear-gradient(90deg, #f59e0b, #fb7185); border-radius: 999px; margin: 22px 0;"></div>

## 2. Configuration réseau
> 🌐 **Objectif :** disposer d'une connexion filaire fixe et d'un WiFi prêt pour le réseau du Cégep.

### 2.1 Adresse IP statique (Ethernet)

#### Créer/éditer le fichier Netplan
```bash
sudo nano /etc/netplan/01-ethernet.yaml
```

#### Configuration
```yaml
network:
  version: 2
  renderer: networkd
  ethernets:
    eth0:
      dhcp4: false
      addresses:
        - 192.168.1.9/24
      gateway4: 192.168.1.1
      nameservers:
        addresses:
          - 1.1.1.1
          - 8.8.8.8
```

<div style="background:#fef9c3; border:1px solid #facc15; padding:10px 12px; border-radius:10px;">
<strong>Paramètres</strong>
<ul>
  <li><code>192.168.1.9</code> → Votre IP fixe</li>
  <li><code>/24</code> → Masque 255.255.255.0</li>
  <li><code>gateway4: 192.168.1.1</code> → Votre passerelle (modem/routeur)</li>
  <li>DNS: Cloudflare (1.1.1.1) + Google (8.8.8.8)</li>
</ul>
</div>

#### Appliquer la configuration
```bash
sudo netplan apply
```

Ou avec debug:
```bash
sudo netplan --debug apply
```

#### Vérification
```bash
ip a
```
Vous devriez voir: `inet 192.168.1.9/24`

**Test Internet:**
```bash
ping 1.1.1.1
ping google.com
```

### 2.2 Connexion SSH locale
1. Brancher le câble réseau entre votre RPi et votre PC
2. Se connecter en SSH au Raspberry Pi

### 2.3 Configuration WiFi WPA-EAP (Réseau Cégep)

#### Créer le fichier de configuration WiFi
```bash
sudo nano /etc/netplan/01-wifi.yaml
```
```yaml
network:
  version: 2
  renderer: networkd
  wifis:
    wlan0:
      dhcp4: true
      access-points:
        "MonSSID":
          mode: infrastructure
          auth:
            key-management: wpa-eap
            eap-method: peap
            identity: "mon_user"
            password: "mon_password"
            phase2-auth: mschapv2
```

#### Appliquer la configuration WiFi
```bash
sudo netplan generate
sudo netplan apply
```

#### Debug si la connexion échoue
```bash
sudo netplan --debug apply
sudo journalctl -u systemd-networkd -f
```

#### Test de connectivité
```bash
ping www.google.ca
```

<div style="height: 5px; background: linear-gradient(90deg, #22d3ee, #3b82f6); border-radius: 999px; margin: 22px 0;"></div>

## 3. Connexion à distance via Cloudflare Tunnel
> 🔒 **Objectif :** sécuriser l'accès SSH via un tunnel Cloudflare et Zero Trust.

### 3.1 Prérequis
1. Se créer un compte gratuit sur Cloudflare
2. Acheter un nom de domaine public

### 3.2 Installation et configuration sur le Raspberry Pi

#### Authentification Cloudflare
```bash
cloudflared login
```

#### Créer un tunnel nommé
```bash
cloudflared tunnel create rpi-ssh
```

La commande affichera:
- Un **UUID** (ex: `12345678-abcd-...`) - **gardez-le précieusement**
- Créera un fichier JSON de credentials dans: `/home/fpoisson/.cloudflared/<UUID>.json`

#### Créer le fichier de configuration
```bash
nano /home/fpoisson/.cloudflared/config.yml
```

**Contenu:**
```yaml
tunnel: <TON-UUID-ICI>
credentials-file: /home/fpoisson/.cloudflared/<TON-UUID-ICI>.json

ingress:
  - hostname: rpi.edxo.ca
    service: ssh://localhost:22
  - service: http_status:404
```

<div style="background:#fdf2f8; border:1px solid #ec4899; padding:10px 12px; border-radius:10px;">
<strong>Remplacer</strong>
<ul>
  <li><code>&lt;TON-UUID-ICI&gt;</code> par l'UUID du tunnel</li>
  <li><code>rpi.edxo.ca</code> par votre sous-domaine</li>
</ul>
</div>

#### Lier le tunnel au DNS
```bash
cloudflared tunnel route dns rpi-ssh rpi.edxo.ca
```
- `rpi-ssh` → nom du tunnel
- `rpi.edxo.ca` → hostname externe

Cela crée automatiquement l'entrée DNS dans votre compte Cloudflare.

#### Tester le tunnel manuellement
```bash
cloudflared tunnel run rpi-ssh
```
Laissez cette commande tourner (utilisez `tmux` ou `screen` si nécessaire).

Si tout fonctionne, vous verrez des logs: `"Connection established"` / `"Proxying tunnel"`

#### Installer le service (démarrage automatique)
Une fois le test réussi:
```bash
sudo cloudflared service install
```

### 3.3 Configuration Cloudflare Zero Trust (Dashboard web)
1. Aller sur le dashboard Cloudflare
2. Accéder à **Zero Trust** (ou "Cloudflare One")
3. Naviguer vers: **Access → Applications → Add an application**

**Configuration de l'application:**
- **Type:** Self-hosted
- **Application name:** rpi-ssh (ou autre nom)
- **Domain:** rpi.edxo.ca
- **Session duration:** 24h (ou selon préférence)

**Configuration des Policies:**
- **Action:** Allow
- **Include:** Emails → Votre email Cloudflare (ex: francis.poisson2@...)
- Enregistrer

**Résultat:** Seul un utilisateur autorisé (vous) pourra utiliser `rpi.edxo.ca` en SSH via Access.

### 3.4 Connexion SSH via Cloudflare Access

#### Configuration SSH locale (sur votre PC)
Éditer `~/.ssh/config`:
```bash
nano ~/.ssh/config
```

**Ajouter:**
```
Host rpi
  HostName rpi.edxo.ca
  User fpoisson
  ProxyCommand cloudflared access ssh --hostname %h
```

#### Se connecter
```bash
ssh rpi
```

Vous verrez soit:
- `fpoisson@rpi's password:` (authentification par mot de passe)
- `Authenticated with public key...` (si clé SSH configurée)

<div style="height: 5px; background: linear-gradient(90deg, #c084fc, #22d3ee); border-radius: 999px; margin: 22px 0;"></div>


## 4. Configuration Git
> 🔧 **Objectif :** configurer Git et GitHub pour collaborer sur le projet du cours.

### 4.1 Création du compte GitHub et token d'accès

#### Créer un compte GitHub
1. Si vous n'avez pas de compte, allez sur [github.com](https://github.com) et créez-en un
2. Vérifiez votre adresse email

#### Créer un Personal Access Token (Classic)
1. Connectez-vous à GitHub
2. Allez dans **Settings** (en haut à droite, cliquez sur votre avatar)
3. Dans le menu de gauche, en bas, cliquez sur **Developer settings**
4. Cliquez sur **Personal access tokens** → **Tokens (classic)**
5. Cliquez sur **Generate new token** → **Generate new token (classic)**
6. Configurez le token:
   - **Note:** `Raspberry Pi - 243-4J5-LI`
   - **Expiration:** 90 days (ou selon préférence)
   - **Scopes:** Cochez au minimum `repo` (accès complet aux dépôts privés et publics)
7. Cliquez sur **Generate token**
8. **⚠️ IMPORTANT:** Copiez le token immédiatement, vous ne pourrez plus le voir!

### 4.2 Configuration Git sur le Raspberry Pi

#### Configuration de l'identité
```bash
git config --global user.name "Votre Nom"
git config --global user.email "votre.email@example.com"
```

#### Configurer le credential store
Pour éviter de retaper le token à chaque fois:
```bash
git config --global credential.helper store
```

<div style="background:#fee2e2; border:1px solid #ef4444; padding:10px 12px; border-radius:10px;">
<strong>⚠️ Attention sécurité</strong>
<ul>
  <li>Le mode <code>store</code> enregistre le token en <strong>texte clair</strong> dans <code>~/.git-credentials</code></li>
  <li>Sur un système partagé, préférez <code>cache</code> : <code>git config --global credential.helper cache</code></li>
  <li>Pour un timeout de 1h : <code>git config --global credential.helper 'cache --timeout=3600'</code></li>
</ul>
</div>

### 4.3 Cloner le dépôt du cours

#### Cloner le repository
```bash
cd ~
git clone https://github.com/fpoisson2/243-4J5-LI.git
cd 243-4J5-LI
```

Lors du premier clone, Git vous demandera:
- **Username:** Votre nom d'utilisateur GitHub
- **Password:** Collez votre **token** (pas votre mot de passe!)

Le credential helper sauvegarde ces informations pour les prochaines fois.

### 4.4 Travailler avec les branches

#### Créer votre branche personnelle
```bash
git checkout -b prenom-nom/labo1
```

Exemple: `git checkout -b francis-poisson/labo1`

#### Vérifier votre branche actuelle
```bash
git branch
```

L'astérisque `*` indique la branche active.

#### Faire des modifications et les sauvegarder

**Vérifier l'état:**
```bash
git status
```

**Ajouter vos modifications:**
```bash
git add .
```

Ou pour ajouter un fichier spécifique:
```bash
git add chemin/vers/fichier.py
```

**Créer un commit:**
```bash
git commit -m "Description de vos changements"
```

Exemple: `git commit -m "Ajout de l'interface tactile avec trois boutons"`

**Pousser vers GitHub:**
```bash
git push origin prenom-nom/labo1
```

Si c'est le premier push de cette branche:
```bash
git push -u origin prenom-nom/labo1
```

Le flag `-u` (upstream) établit le lien entre votre branche locale et la branche distante.

### 4.5 Synchroniser avec le dépôt principal

#### Récupérer les dernières modifications
```bash
git fetch origin
```

#### Mettre à jour votre branche locale depuis main
```bash
git checkout main
git pull origin main
```

#### Fusionner main dans votre branche
```bash
git checkout prenom-nom/labo1
git merge main
```

<div style="background:#dbeafe; border:1px solid #3b82f6; padding:10px 12px; border-radius:10px;">
<strong>💡 Bonnes pratiques</strong>
<ul>
  <li>Faites des commits fréquents avec des messages clairs</li>
  <li>Synchronisez régulièrement avec <code>main</code> pour éviter les conflits</li>
  <li>Nommez vos branches de façon descriptive: <code>prenom-nom/feature-description</code></li>
  <li>Ne travaillez jamais directement sur <code>main</code></li>
</ul>
</div>

<div style="height: 5px; background: linear-gradient(90deg, #10b981, #06b6d4); border-radius: 999px; margin: 22px 0;"></div>


## 5. Interface tactile distante en mode console
> 📱 **Objectif :** afficher un tableau de bord tactile minimal directement sur la console du Raspberry Pi (TTY1) via `curses` et `evdev`.

### 5.1 Code prêt à l'emploi
- Le script se trouve dans `~/243-4J5-LI/labo1/code/touch_ui.py`.
- Il affiche trois boutons (STATUS, LOGS, QUIT) et réagit aux taps du panneau tactile sans serveur X.
- `q` ou le bouton **QUIT** ferment l'application.

### 5.2 Dépendances requises
```bash
sudo apt update
sudo apt install -y python3 python3-evdev
```

### 5.3 Lancer l'UI sur l'écran distant
Exécuter depuis une session SSH (le Pi doit avoir l'écran tactile branché) :
```bash
sudo chvt 1
sudo setsid sh -c 'exec </dev/tty1 >/dev/tty1 2>&1 python3 /home/fpoisson/243-4J5-LI/labo1/code/touch_ui.py'
```
- `chvt 1` bascule l'affichage sur la console locale (TTY1).
- `setsid` démarre le script dans un nouveau groupe de sessions et redirige STDIN/STDOUT/STDERR vers l'écran, ce qui permet de voir et toucher l'interface à distance.

<div style="height: 5px; background: linear-gradient(90deg, #f59e0b, #f97316); border-radius: 999px; margin: 22px 0;"></div>


## 6. Installation Node.js et outils CLI
> 🛠️ **Objectif :** installer Node.js 22 avec NVM puis la Gemini CLI.

### 6.1 Installation de base
```bash
sudo apt install npm
```

### 6.2 Configuration NVM (Node Version Manager)

#### Activer NVM dans la session
```bash
export NVM_DIR="$HOME/.nvm"
[ -s "$NVM_DIR/nvm.sh" ] && . "$NVM_DIR/nvm.sh"
[ -s "$NVM_DIR/bash_completion" ] && . "$NVM_DIR/bash_completion"
```

#### Vérifier NVM
```bash
command -v nvm
```

Devrait répondre: `nvm` ou `/home/fpoisson/.nvm/nvm.sh`

#### Installer Node.js 22
```bash
nvm install 22
nvm use 22
```

#### Vérification
```bash
node -v    # Devrait afficher v22.x.x
npm -v
```

**Important:** Avec NVM, pas besoin de `sudo` pour `node`/`npm`. Tout est dans votre `$HOME`.

### 6.3 Installation Gemini CLI

#### Installation
```bash
npm install -g @google/gemini-cli
```

#### Vérification
```bash
gemini --help
```

Vous ne devriez plus voir l'erreur: `SyntaxError: Invalid regular expression flags`

#### Utilisation
```bash
gemini
```
Lancer dans le dossier du code Python créé pour assistance.

### 6.4 Nettoyage (optionnel)
Pour supprimer l'ancienne installation globale:
```bash
nvm use system
npm uninstall -g @google/gemini-cli
nvm use 22
```

**Astuce:** Ajoutez `nvm use 22` dans votre `~/.bashrc` pour en faire la version par défaut.

### 6.5 Exercice pratique avec Gemini CLI

Maintenant que vous avez installé Gemini CLI, testez-le pour améliorer votre code!

**Exemple d'utilisation:**
1. Naviguez vers votre code:
   ```bash
   cd ~/243-4J5-LI/labo1/code
   ```

2. Lancez Gemini et demandez-lui d'ajouter une fonctionnalité:
   ```bash
   gemini
   ```

3. **Suggestions de requêtes:**
   - "Ajoute un quatrième bouton 'REBOOT' qui affiche un message de confirmation"
   - "Ajoute des couleurs différentes pour chaque bouton"
   - "Crée une fonction qui affiche l'heure actuelle dans le coin supérieur droit"
   - "Ajoute un indicateur de batterie factice qui change de couleur"

4. Testez le code modifié:
   ```bash
   sudo chvt 1
   sudo setsid sh -c 'exec </dev/tty1 >/dev/tty1 2>&1 python3 /home/fpoisson/243-4J5-LI/labo1/code/touch_ui.py'
   ```

5. Sauvegardez vos changements avec Git:
   ```bash
   git add .
   git commit -m "Ajout de fonctionnalité via Gemini: [décrivez ce que vous avez ajouté]"
   git push origin prenom-nom/labo1
   ```

<div style="background:#f0fdf4; border:1px solid #22c55e; padding:10px 12px; border-radius:10px;">
<strong>✅ À remettre:</strong>
<ul>
  <li>Capturez une photo de votre écran tactile montrant la nouvelle fonctionnalité</li>
  <li>Notez la requête Gemini que vous avez utilisée</li>
  <li>Décrivez brièvement ce qui fonctionne et ce qui ne fonctionne pas</li>
</ul>
</div>

<div style="height: 5px; background: linear-gradient(90deg, #34d399, #fbbf24); border-radius: 999px; margin: 22px 0;"></div>


## 7. Programmation du LilyGO A7670E
> 🚀 **Objectif :** installer Arduino CLI et programmer le module LilyGO pour communiquer via LTE.

### 7.1 Installation Arduino CLI

#### Télécharger et installer Arduino CLI
```bash
cd ~
curl -fsSL https://raw.githubusercontent.com/arduino/arduino-cli/master/install.sh | sh
```

#### Ajouter Arduino CLI au PATH
```bash
echo 'export PATH=$PATH:$HOME/bin' >> ~/.bashrc
source ~/.bashrc
```

#### Vérifier l'installation
```bash
arduino-cli version
```

#### Initialiser la configuration
```bash
arduino-cli config init
```

#### Mettre à jour l'index des boards
```bash
arduino-cli core update-index
```

### 7.2 Configuration pour ESP32

#### Ajouter l'URL des ESP32
```bash
arduino-cli config add board_manager.additional_urls https://raw.githubusercontent.com/espressif/arduino-esp32/gh-pages/package_esp32_index.json
```

#### Mettre à jour l'index
```bash
arduino-cli core update-index
```

#### Installer le support ESP32
```bash
arduino-cli core install esp32:esp32
```

#### Lister les boards disponibles
```bash
arduino-cli board listall esp32
```

### 7.3 Installation des bibliothèques requises

Pour le LilyGO A7670E, installer les bibliothèques nécessaires:
```bash
arduino-cli lib install "TinyGSM"
arduino-cli lib install "ArduinoJson"
arduino-cli lib install "PubSubClient"
```

### 7.4 Premier programme simple

#### Créer un dossier pour le projet
```bash
mkdir -p ~/243-4J5-LI/labo1/lilygo-test
cd ~/243-4J5-LI/labo1/lilygo-test
```

#### Créer le sketch Arduino
```bash
nano lilygo-test.ino
```

**Code de test simple:**
```cpp
// Test basique pour LilyGO A7670E
// Vérifie la communication série et allume la LED

#define LED_PIN 12  // LED intégrée sur le LilyGO

void setup() {
  // Initialiser la communication série
  Serial.begin(115200);
  delay(1000);

  // Configurer la LED
  pinMode(LED_PIN, OUTPUT);

  Serial.println("=========================");
  Serial.println("LilyGO A7670E - Test");
  Serial.println("=========================");
  Serial.println("Démarrage...");
}

void loop() {
  // Faire clignoter la LED
  digitalWrite(LED_PIN, HIGH);
  Serial.println("LED ON");
  delay(1000);

  digitalWrite(LED_PIN, LOW);
  Serial.println("LED OFF");
  delay(1000);
}
```

### 7.5 Compilation et téléversement

#### Connecter le LilyGO
1. Brancher le câble USB-A vers USB-C entre le Raspberry Pi et le LilyGO
2. Vérifier la connexion:
```bash
arduino-cli board list
```

Vous devriez voir un port comme `/dev/ttyUSB0` ou `/dev/ttyACM0`

#### Compiler le sketch
```bash
arduino-cli compile --fqbn esp32:esp32:esp32 lilygo-test.ino
```

#### Téléverser vers le LilyGO
```bash
arduino-cli upload -p /dev/ttyUSB0 --fqbn esp32:esp32:esp32 lilygo-test.ino
```

**Note:** Remplacez `/dev/ttyUSB0` par le port détecté sur votre système.

#### Moniteur série pour voir les messages
```bash
arduino-cli monitor -p /dev/ttyUSB0 -c baudrate=115200
```

Vous devriez voir:
```
=========================
LilyGO A7670E - Test
=========================
Démarrage...
LED ON
LED OFF
LED ON
LED OFF
...
```

Pour quitter le moniteur série: `Ctrl+C`

### 7.6 Prochaines étapes

Une fois le test de base réussi:
1. Tester la communication avec le module A7670E (AT commands)
2. Configurer la connexion LTE avec votre carte SIM
3. Établir une connexion MQTT pour envoyer des données
4. Intégrer le GPS pour la géolocalisation

<div style="background:#fef3c7; border:1px solid #f59e0b; padding:10px 12px; border-radius:10px;">
<strong>⚡ Dépannage</strong>
<ul>
  <li>Si <code>/dev/ttyUSB0</code> n'apparaît pas, vérifiez le câble USB</li>
  <li>Ajoutez votre utilisateur au groupe dialout: <code>sudo usermod -a -G dialout $USER</code> puis redémarrez</li>
  <li>Si l'upload échoue, appuyez sur le bouton BOOT du LilyGO pendant l'upload</li>
  <li>Pour voir tous les ports: <code>ls -la /dev/tty*</code></li>
</ul>
</div>

<div style="height: 5px; background: linear-gradient(90deg, #a855f7, #ec4899); border-radius: 999px; margin: 22px 0;"></div>
