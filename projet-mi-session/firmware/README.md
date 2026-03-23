# Projet Mi-Session - Firmware

Ce répertoire contient le firmware principal pour le projet basé sur la carte LilyGO (A7670G) et les composants suivants:
- 4x LEDs
- 1x Bouton
- 2x Potentiomètres
- 1x Accéléromètre (MPU6050)

## Structure des Fichiers

- `firmware.ino` : Sketch Arduino principal.
- `auth.h` : Identifiants réseau et MQTT (Ne pas commiter. Remplir à partir de `auth.h.example`).
- `trust_anchors.h` : Certificats TLS si une connexion MQTT sécurisée est nécessaire.
- `tests/checklist.md` : Grille de validation des critères du projet.

## Dépendances

Les bibliothèques suivantes sont nécessaires pour compiler ce projet dans l'IDE Arduino :
- **TinyGSM** par Volodymyr Shymanskyy (pour la communication LTE)
- **PubSubClient** par Nick O'Leary (pour le protocole MQTT)
- **Adafruit MPU6050** (pour l'accéléromètre)
- **Adafruit Unified Sensor** (dépendance requise pour MPU6050)
- **Wire** (incluse avec l'ESP32)

## Configuration et Téléversement

1. Copiez le fichier `auth.h.example` vers `auth.h` et renseignez les valeurs propres à votre environnement (APN, Serveur MQTT, etc.).
2. Ouvrez le fichier `firmware.ino` dans l'IDE Arduino.
3. Sélectionnez le type de carte approprié (ex: *ESP32 Wrover Module* ou *LilyGo T-SIM A7670* selon votre configuration).
4. Assurez-vous que la vitesse du port série est configurée à `115200` bauds.
5. Compilez et téléversez le code.
6. Ouvrez le moniteur série pour valider la séquence de démarrage (`[INFO] Démarrage...`, `[INFO] LTE Connecté`, etc.).

## Note sur les broches

Ce firmware s'inspire du schéma de broches provenant de `test_mpu6050.ino`.
- **LEDs** : 12, 13, 14, 15
- **Potentiomètres** : 26, 27
- **Bouton** : 34

*Avertissement :* Les broches `26` et `27` sont également définies par défaut pour `MODEM_TX` et `MODEM_RX` dans la bibliothèque LTE. Si vous utilisez physiquement les potentiomètres sur 26 et 27, veillez à ne pas créer de conflit matériel avec les broches UART du modem. En cas de conflit, il faudra adapter le câblage et les constantes des broches.
