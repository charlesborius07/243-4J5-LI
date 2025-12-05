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
- [Interface tactile Kivy](#4-interface-graphique-tactile-avec-kivy)
- [Installation Node.js et outils CLI](#5-installation-nodejs-et-outils-cli)
- [Configuration Git](#6-configuration-git)
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

## 4. Interface graphique tactile avec Kivy
> 📱 **Objectif :** afficher un dashboard tactile en plein écran sur le Raspberry Pi.

### 4.1 Installation des dépendances
```bash
sudo apt update

# Serveur X + Window Manager minimal
sudo apt install -y xserver-xorg xinit openbox

# Python et environnement virtuel
sudo apt install -y python3 python3.12-venv

# Kivy pour interface tactile
sudo apt install -y python3-kivy
```

**Composants installés:**
- Serveur X (pour affichage graphique)
- Openbox (gestionnaire de fenêtres léger)
- Kivy (framework interface tactile)

### 4.2 Créer l'application dashboard tactile

#### Créer le répertoire du projet
```bash
mkdir -p ~/touch-kiosk
nano ~/touch-kiosk/app.py
```

#### Code de l'application
```python
from kivy.app import App
from kivy.uix.boxlayout import BoxLayout
from kivy.uix.button import Button
from kivy.uix.label import Label
from kivy.core.window import Window

# Plein écran
Window.fullscreen = True

class Dashboard(BoxLayout):
    def __init__(self, **kwargs):
        super().__init__(orientation="vertical", **kwargs)

        # Zone d'affichage principale
        self.label = Label(
            text="Dashboard tactile\nTouchez un bouton",
            font_size="40sp",
            halign="center",
            valign="middle"
        )
        self.label.bind(size=self._update_text_size)
        self.add_widget(self.label)

        # Barre de boutons en bas
        btn_bar = BoxLayout(size_hint_y=0.25)

        btn1 = Button(text="Statut", font_size="28sp")
        btn2 = Button(text="Redémarrer service", font_size="22sp")
        btn3 = Button(text="Quitter", font_size="28sp")

        btn1.bind(on_press=self.show_status)
        btn2.bind(on_press=self.restart_service)
        btn3.bind(on_press=self.quit_app)

        btn_bar.add_widget(btn1)
        btn_bar.add_widget(btn2)
        btn_bar.add_widget(btn3)

        self.add_widget(btn_bar)

    def _update_text_size(self, *args):
        # Pour que le texte se centre bien
        self.label.text_size = self.label.size

    def show_status(self, instance):
        self.label.text = "Statut :\nTout va bien 😄"

    def restart_service(self, instance):
        # Ici vous pouvez appeler un script shell / API, etc.
        self.label.text = "Action :\nRedémarrage du service… (simulé)"

    def quit_app(self, instance):
        App.get_running_app().stop()

class KioskApp(App):
    def build(self):
        self.title = "Dashboard tactile"
        return Dashboard()

if __name__ == "__main__":
    KioskApp().run()
```

**Note:** Vous pouvez remplacer `show_status` / `restart_service` par des appels à vos scripts, APIs, etc.

### 4.3 Configuration du serveur X

#### Créer le fichier xinitrc
```bash
nano ~/.xinitrc
```

**Contenu:**
```bash
#!/bin/sh
# Lancer openbox (WM léger)
openbox-session &

# Lancer le dashboard Kivy
python3 /home/$USER/touch-kiosk/app.py
```

#### Rendre exécutable
```bash
chmod +x ~/.xinitrc
```

### 4.4 Test manuel du dashboard
```bash
startx
```

L'écran du Pi devrait:
- Quitter le terminal texte
- Afficher le dashboard Kivy en plein écran
- Répondre aux touches tactiles

**Pour quitter:**
- Toucher le bouton "Quitter"
- `Ctrl+Alt+Backspace` (si clavier branché)
- `killall Xorg` via SSH

### 4.5 Démarrage automatique (optionnel)

#### Créer le script de démarrage
```bash
nano ~/start-kiosk.sh
```

**Contenu:**
```bash
#!/bin/bash
cd /home/$USER
/usr/bin/startx
```
```bash
chmod +x ~/start-kiosk.sh
```

#### Créer le service systemd (user)
```bash
mkdir -p ~/.config/systemd/user
nano ~/.config/systemd/user/kiosk.service
```

**Contenu:**
```ini
[Unit]
Description=Kiosk tactile Dashboard

[Service]
Type=simple
ExecStart=/home/%u/start-kiosk.sh
Restart=always
RestartSec=5

[Install]
WantedBy=default.target
```

#### Activer le service
```bash
systemctl --user daemon-reload
systemctl --user enable kiosk.service
systemctl --user start kiosk.service
```

#### Autoriser le démarrage user sans login
```bash
sudo loginctl enable-linger $USER
```

**Au prochain reboot:**
- Le Pi bootera sur console
- Lancera X automatiquement
- Affichera directement le dashboard tactile

<div style="height: 5px; background: linear-gradient(90deg, #f59e0b, #f97316); border-radius: 999px; margin: 22px 0;"></div>

## 5. Installation Node.js et outils CLI
> 🛠️ **Objectif :** installer Node.js 22 avec NVM puis la Gemini CLI.

### 5.1 Installation de base
```bash
sudo apt install npm
```

### 5.2 Configuration NVM (Node Version Manager)

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

### 5.3 Installation Gemini CLI

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

### 5.4 Nettoyage (optionnel)
Pour supprimer l'ancienne installation globale:
```bash
nvm use system
npm uninstall -g @google/gemini-cli
nvm use 22
```

**Astuce:** Ajoutez `nvm use 22` dans votre `~/.bashrc` pour en faire la version par défaut.

<div style="height: 5px; background: linear-gradient(90deg, #22d3ee, #34d399); border-radius: 999px; margin: 22px 0;"></div>

## 6. Configuration Git
> 📝 **Objectif :** configurer Git globalement.

*(Section à compléter selon vos besoins)*
```bash
# Configuration de base Git
git config --global user.name "Votre Nom"
git config --global user.email "votre.email@example.com"
```

<div style="height: 5px; background: linear-gradient(90deg, #22c55e, #a855f7); border-radius: 999px; margin: 22px 0;"></div>

## 7. Programmation du LilyGO A7670E
> 🚀 **Objectif :** compléter avec les instructions spécifiques au module.

*(Section à compléter avec les instructions spécifiques)*

<div style="height: 5px; background: linear-gradient(90deg, #a855f7, #ec4899); border-radius: 999px; margin: 22px 0;"></div>

## 🔖 Notes importantes
- **NVM:** Avec NVM, toutes les commandes `node` et `npm` s'exécutent sans `sudo`
- **Sécurité:** Le tunnel Cloudflare chiffre tout le trafic SSH
- **Performance:** Openbox est un WM léger idéal pour Raspberry Pi
- **Tactile:** Kivy gère automatiquement les événements tactiles
- **Débogage:** Utilisez `journalctl` et `systemctl status` pour diagnostiquer les problèmes

<div style="height: 5px; background: linear-gradient(90deg, #06b6d4, #0ea5e9); border-radius: 999px; margin: 22px 0;"></div>

## 🧪 Commandes de vérification utiles
```bash
# Vérifier NVM
command -v nvm

# Vérifier Node
node -v

# Vérifier Gemini
gemini --version

# Vérifier service kiosk
systemctl --user status kiosk.service

# Vérifier tunnel Cloudflare
sudo systemctl status cloudflared
```
