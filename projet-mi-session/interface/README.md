# Interface Tactile IoT

Cette interface graphique tactile en Python (`pygame`) permet de visualiser en temps réel les données de télémétrie du projet mi-session (boutons, potentiomètres, accéléromètre) et de contrôler 4 LEDs à distance via MQTT.

## Prérequis

1. Installez les dépendances nécessaires à l'aide de `pip` :

```bash
pip install -r requirements.txt
```

2. Assurez-vous d'avoir un broker MQTT opérationnel (par exemple, Mosquitto) tournant sur votre machine ou réseau.

## Exécution manuelle

Pour lancer l'interface manuellement :

```bash
python3 main.py
```

*Note : Appuyez sur la touche `ESC` ou `q` pour fermer l'application.*

## Configuration du service Systemd (Démarrage automatique)

Pour que l'interface se lance automatiquement au démarrage du Raspberry Pi, suivez ces étapes :

1. Copiez le fichier de service vers le répertoire systemd :
```bash
sudo cp iot-interface.service /etc/systemd/system/
```

2. Rechargez la configuration des services :
```bash
sudo systemctl daemon-reload
```

3. Activez le service pour qu'il démarre au boot :
```bash
sudo systemctl enable iot-interface.service
```

4. Démarrez le service manuellement (pour tester) :
```bash
sudo systemctl start iot-interface.service
```

5. Vérifiez le statut du service :
```bash
sudo systemctl status iot-interface.service
```

### Visualiser les journaux

Si l'application plante ou ne s'affiche pas, vous pouvez consulter les logs en temps réel via :
```bash
journalctl -u iot-interface.service -f
```

## Structure du code
- `main.py` : Le fichier principal contenant la logique MVC et graphique (Pygame).
- `requirements.txt` : Versions des packages nécessaires (`pygame`, `paho-mqtt`).
- `iot-interface.service` : Configuration Systemd.
- `tests/checklist.md` : Checklist de tests à valider lors de la recette de l'interface.
