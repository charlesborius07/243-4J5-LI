# Projet de Mi-Session - Objets Connectés (243-4J5-LI)

Ce dépôt contient l'ensemble des composants pour le projet de mi-session du cours d'objets connectés. Le système comprend un firmware embarqué, une interface tactile utilisateur, ainsi que la conception matérielle d'une carte d'extension (shield).

## Structure du Projet

- **`firmware/`** : Code source du firmware pour le microcontrôleur LilyGo.
  - Projet au format Arduino (`firmware.ino`).
  - *Consultez le fichier `firmware/README.md` pour les instructions détaillées de compilation et la configuration des identifiants.*

- **`interface/`** : Code de l'interface tactile.
  - Application développée en Python (`main.py`).
  - Comprend les dépendances (`requirements.txt`) et un service systemd (`iot-interface.service`) pour l'exécution en arrière-plan.

- **`kicad/`** : Fichiers de conception matérielle.
  - Projet KiCad contenant les schémas et le routage du circuit imprimé (shield) personnalisé.

- **`fabrication/`** : Fichiers de production.
  - Contient les fichiers Gerbers, la nomenclature (BOM) et les fichiers de position (CPL) nécessaires à la fabrication du PCB.

## Démarrage Rapide

### 1. Configuration du Firmware
1. Installez l'environnement Arduino IDE.
2. Copiez le fichier `firmware/auth.h.example` vers `firmware/auth.h` et renseignez vos informations de connexion.
3. Ouvrez `firmware/firmware.ino` et téléversez le code sur votre carte LilyGo.

### 2. Lancement de l'Interface
Assurez-vous d'avoir Python installé sur votre machine cible (ex: Raspberry Pi).

```bash
cd interface
pip install -r requirements.txt
python main.py
```
*(Optionnel : configurez le service `iot-interface.service` pour un lancement automatique au démarrage).*
