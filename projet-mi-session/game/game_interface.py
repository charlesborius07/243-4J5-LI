import json
import threading
import time
from queue import Queue
import curses
from evdev import InputDevice, ecodes, list_devices
import paho.mqtt.client as mqtt
import ssl
import sys
import os
# Ajouter le répertoire parent au chemin de recherche pour importer mqtt_config
sys.path.append(os.path.abspath(os.path.join(os.path.dirname(__file__), '..', 'interface')))
from mqtt_config import MQTT_CONFIG

# ---------- VERSION CONSOLE SANS CURSES ----------

class GameConsole:
    def __init__(self, mqtt_config):
        self.running = True
        self.mqtt_connected = False
        self.mqtt_config = mqtt_config
        
        # État du jeu
        self.player_pos = [10, 10]
        self.speed_multiplier = 1.0
        
        self._init_mqtt()

    def _init_mqtt(self):
        try:
            client_id = f"python-control-{int(time.time())}"
            self.mqtt_client = mqtt.Client(client_id=client_id, transport="websockets")
            self.mqtt_client.tls_set(cert_reqs=ssl.CERT_REQUIRED)
            self.mqtt_client.username_pw_set(self.mqtt_config.get("username"), self.mqtt_config.get("password"))
            self.mqtt_client.ws_set_options(path="/")
            self.mqtt_client.on_connect = self._on_connect
            self.mqtt_client.on_message = self._on_message
            self.mqtt_client.connect(self.mqtt_config.get("broker"), self.mqtt_config.get("port", 443), 60)
            self.mqtt_client.loop_start()
            print("MQTT connecté.")
        except Exception as e:
            print(f"Erreur MQTT: {str(e)}")

    def _on_connect(self, client, userdata, flags, rc):
        if rc == 0:
            self.mqtt_connected = True
            device_id = self.mqtt_config.get("device_id", "")
            root = device_id if device_id.endswith('/') else f"{device_id}/"
            self.mqtt_client.subscribe(f"{root}sensors/accel")
            self.mqtt_client.subscribe(f"{root}sensors/pots")
            print("Abonné aux capteurs.")

    def _on_message(self, client, userdata, msg):
        try:
            payload = json.loads(msg.payload.decode('utf-8'))
            topic = msg.topic
            
            if "sensors/accel" in topic:
                dx = int(payload.get('x', 0) * 2.0 * self.speed_multiplier)
                dy = int(payload.get('y', 0) * 2.0 * self.speed_multiplier)
                self.player_pos[0] += dx
                self.player_pos[1] += dy
                # Affichage simple de la position
                print(f"Bille en: {self.player_pos}", end='\r')
                
            elif "sensors/pots" in topic:
                self.speed_multiplier = (payload.get('pot1', 85) / 4095.0) * 5.0
                
        except Exception as e:
            pass

    def run(self):
        print("Jeu lancé (Console). Appuyez sur Ctrl+C pour quitter.")
        while self.running:
            time.sleep(1)

def main():
    ui = GameConsole(MQTT_CONFIG)
    ui.run()

if __name__ == "__main__":
    main()

