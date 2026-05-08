import sys
import os
import threading
import time
import json
import curses
import paho.mqtt.client as mqtt
from queue import Queue

# Configuration importée
from mqtt_config import MQTT_CONFIG

MQTT_BROKER = MQTT_CONFIG["broker"]
MQTT_USER   = MQTT_CONFIG["username"]
MQTT_PASS   = MQTT_CONFIG["password"]
CLIENT_ID   = "poste-05-ui"
TOPIC_BASE  = f"hydro-limoilou/{MQTT_CONFIG['device_id']}"

class MQTTHandler:
    def __init__(self):
        self.data = {
            "temp": "N/A", "hum": "N/A", "pres": "N/A",
            "v_line": "N/A", "i_line": "N/A", "llm": "En attente..."
        }
        self.client = mqtt.Client(CLIENT_ID)
        self.client.username_pw_set(MQTT_USER, MQTT_PASS)
        self.client.on_message = self.on_message
        
        # Connexion sur port 1883 pour MQTT standard
        self.client.connect(MQTT_BROKER, 1883, 60)
        self.client.subscribe(f"{TOPIC_BASE}/#")
        self.client.loop_start()

    def on_message(self, client, userdata, msg):
        try:
            topic = msg.topic
            payload = json.loads(msg.payload.decode())
            
            if "temperature" in topic: self.data["temp"] = f"{payload['value']} {payload['unit']}"
            elif "humidity" in topic: self.data["hum"] = f"{payload['value']} {payload['unit']}"
            elif "pressure" in topic: self.data["pres"] = f"{payload['value']} {payload['unit']}"
            elif "voltage_line" in topic: self.data["v_line"] = f"{payload['value']} {payload['unit']}"
            elif "current_line" in topic: self.data["i_line"] = f"{payload['value']} {payload['unit']}"
            elif "status/llm" in topic: self.data["llm"] = payload.get("summary", "...")
        except: pass

class DashboardUI:
    def __init__(self, stdscr, mqtt_handler):
        self.stdscr = stdscr
        self.mqtt = mqtt_handler
        self.running = True

    def _draw(self):
        self.stdscr.erase()
        h, w = self.stdscr.getmaxyx()
        
        # En-tête
        self.stdscr.addstr(0, 0, "=== DASHBOARD POSTE ÉLECTRIQUE 05 ===", curses.A_BOLD)
        
        # Affichage Télémétrie
        self.stdscr.addstr(2, 0, f"Température: {self.mqtt.data['temp']}")
        self.stdscr.addstr(3, 0, f"Humidité   : {self.mqtt.data['hum']}")
        self.stdscr.addstr(4, 0, f"Pression   : {self.mqtt.data['pres']}")
        self.stdscr.addstr(6, 0, f"Tension    : {self.mqtt.data['v_line']}")
        self.stdscr.addstr(7, 0, f"Courant    : {self.mqtt.data['i_line']}")
        
        # Affichage LLM
        self.stdscr.addstr(9, 0, "--- Résumé LLM ---", curses.A_UNDERLINE)
        # Tronquer le résumé pour tenir dans l'écran
        self.stdscr.addstr(10, 0, self.mqtt.data['llm'][:w-1])
        
        self.stdscr.addstr(h-1, 0, "Appuyez sur 'q' pour quitter")
        self.stdscr.refresh()

    def run(self):
        self.stdscr.nodelay(True)
        curses.curs_set(0)
        while self.running:
            self._draw()
            if self.stdscr.getch() == ord('q'): self.running = False
            time.sleep(0.5)

def main(stdscr):
    mqtt_handler = MQTTHandler()
    ui = DashboardUI(stdscr, mqtt_handler)
    ui.run()

if __name__ == "__main__":
    try:
        curses.wrapper(main)
    except curses.error as e:
        print(f"\n--- ERREUR CURSES ---")
        print(f"Détail: {e}")
        print(f"Variable TERM actuelle: {os.environ.get('TERM')}")
        print(f"Assurez-vous d'être dans un terminal interactif.")
    except Exception as e:
        print(f"\n--- ERREUR INCONNUE ---")
        print(f"{e}")
