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

# ---------- GESTION DU TOUCH ----------

class TouchReader(threading.Thread):
    def __init__(self, event_queue: Queue):
        super().__init__(daemon=True)
        self.event_queue = event_queue
        self.device = self._find_touch_device()
        # Calibrage simplifié
        self.min_x, self.max_x = 0, 4096
        self.min_y, self.max_y = 0, 4096
        self.current_x, self.current_y = 2048, 2048

    def _find_touch_device(self):
        for path in list_devices():
            dev = InputDevice(path)
            if "touch" in dev.name.lower() or "ft5406" in dev.name.lower():
                return dev
        return None

    def run(self):
        if not self.device: return
        for event in self.device.read_loop():
            if event.type == ecodes.EV_ABS:
                if event.code == ecodes.ABS_MT_POSITION_X: self.current_x = event.value
                elif event.code == ecodes.ABS_MT_POSITION_Y: self.current_y = event.value
            elif event.type == ecodes.EV_KEY and event.code == ecodes.BTN_TOUCH and event.value == 1:
                self.event_queue.put(("tap", self.current_x, self.current_y))

# ---------- UI DU JEU (Curses) ----------

class GameUI:
    def __init__(self, stdscr, touch_reader, event_queue, mqtt_config):
        self.stdscr = stdscr
        self.running = True
        self.mqtt_connected = False
        self.mqtt_config = mqtt_config
        self.event_queue = event_queue
        
        # État du jeu
        self.player_pos = [10, 10]
        self.score = 0
        
        self._init_mqtt()

    def _init_mqtt(self):
        try:
            client_id = f"python-control-{int(time.time())}"
            self.mqtt_client = mqtt.Client(client_id=client_id, transport="websockets")
            self.mqtt_client.tls_set(cert_reqs=ssl.CERT_REQUIRED)
            self.mqtt_client.username_pw_set(self.mqtt_config.get("username"), self.mqtt_config.get("password"))
            self.mqtt_client.ws_set_options(path="/")
            self.mqtt_client.on_connect = lambda c, u, f, rc: setattr(self, 'mqtt_connected', rc == 0)
            self.mqtt_client.on_message = self._on_message
            self.mqtt_client.connect(self.mqtt_config.get("broker"), self.mqtt_config.get("port", 443), 60)
            self.mqtt_client.loop_start()
        except Exception as e:
            self.status_message = f"Erreur MQTT: {str(e)}"

    def _on_message(self, client, userdata, msg):
        # Logique de réception capteurs pour le jeu (Accel, etc.)
        pass

    def _publish_led(self, led_idx, state):
        topic = f"{self.mqtt_config.get('device_id')}actuators/led{led_idx+1}"
        self.mqtt_client.publish(topic, json.dumps({"state": state}))

    def _draw_maze(self):
        self.stdscr.erase()
        h, w = self.stdscr.getmaxyx()
        
        # Affichage simplifié du labyrinthe
        self.stdscr.addstr(0, 0, "--- MQTT LABYRINTH GAME ---")
        self.stdscr.addstr(self.player_pos[1], self.player_pos[0], "O") # Bille
        
        self.stdscr.addstr(h-1, 0, f"Score: {self.score} | MQTT: {'ON' if self.mqtt_connected else 'OFF'}")
        self.stdscr.refresh()

    def run(self):
        self.stdscr.nodelay(True)
        while self.running:
            self._draw_maze()
            
            # Lecture clavier pour tester sans capteurs
            ch = self.stdscr.getch()
            if ch == ord('q'): self.running = False
            
            time.sleep(0.05)

        self.mqtt_client.loop_stop()
        self.mqtt_client.disconnect()

def main(stdscr):
    event_queue = Queue()
    touch_reader = TouchReader(event_queue)
    touch_reader.start()
    ui = GameUI(stdscr, touch_reader, event_queue, MQTT_CONFIG)
    ui.run()

if __name__ == "__main__":
    curses.wrapper(main)
