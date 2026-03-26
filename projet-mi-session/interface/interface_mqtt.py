import json
import threading
import time
from queue import Queue

import curses
from evdev import InputDevice, ecodes, list_devices
import paho.mqtt.client as mqtt
import ssl

# Configuration MQTT
from mqtt_config import MQTT_CONFIG


# ---------- GESTION DU TOUCH ----------

class TouchReader(threading.Thread):
    def __init__(self, event_queue: Queue):
        super().__init__(daemon=True)
        self.event_queue = event_queue
        self.device = self._find_touch_device()
        if not self.device:
            raise RuntimeError("Aucun périphérique touchscreen trouvé.")

        # On récupère les infos d'axes pour calibrer
        abs_x = self.device.absinfo(ecodes.ABS_MT_POSITION_X)
        abs_y = self.device.absinfo(ecodes.ABS_MT_POSITION_Y)

        self.min_x, self.max_x = abs_x.min, abs_x.max
        self.min_y, self.max_y = abs_y.min, abs_y.max

        self.current_x = (self.min_x + self.max_x) // 2
        self.current_y = (self.min_y + self.max_y) // 2

    def _find_touch_device(self):
        """
        Essaie de trouver un device dont le nom contient 'touch' ou 'ft5406'
        """
        for path in list_devices():
            dev = InputDevice(path)
            name = dev.name.lower()
            if "touch" in name or "ft5406" in name:
                print(f"[TouchReader] Using device: {dev.name} ({path})")
                return dev
        return None

    def run(self):
        for event in self.device.read_loop():
            if event.type == ecodes.EV_ABS:
                if event.code == ecodes.ABS_MT_POSITION_X:
                    self.current_x = event.value
                elif event.code == ecodes.ABS_MT_POSITION_Y:
                    self.current_y = event.value

            elif event.type == ecodes.EV_KEY and event.code == ecodes.BTN_TOUCH:
                # 1 = touch down, 0 = touch up
                if event.value == 1:
                    # On push un "tap" dans la queue avec les coordonnées brutes
                    self.event_queue.put(("tap", self.current_x, self.current_y))


# ---------- UI CURSES ----------

class LEDControlUI:
    def __init__(self, stdscr, touch_reader: TouchReader, event_queue: Queue, mqtt_config: dict):
        self.stdscr = stdscr
        self.touch_reader = touch_reader
        self.event_queue = event_queue
        self.running = True
        self.status_message = "Prêt - Contrôle des LEDs via MQTT"

        # Configuration MQTT
        self.mqtt_config = mqtt_config
        self.mqtt_client = None
        self.mqtt_connected = False

        # Topics MQTT pour les LEDs du LilyGo
        device_id = mqtt_config.get("device_id", "esp32-XXXX")
        
        # S'assure que le device_id se termine par '/'
        if not device_id.endswith('/'):
            root = f"{device_id}/"
        else:
            root = device_id
            
        self.led_topics = [f"{root}actuators/led{i+1}" for i in range(4)]
        
        self.sensors_buttons_topic = f"{root}sensors/buttons"
        self.sensors_pots_topic = f"{root}sensors/pots"
        self.sensors_accel_topic = f"{root}sensors/accel"

        self._init_mqtt()

        self.buttons = []
        self.led_states = [False] * 4
        
        self.btn1_state = False
        self.btn2_state = False
        self.pot1_val = 0
        self.pot2_val = 0
        self.accel_data = {"x": 0, "y": 0, "z": 0, "roll": 0, "pitch": 0}

    def _init_mqtt(self):
        try:
            client_id = f"python-control-{int(time.time())}"
            self.mqtt_client = mqtt.Client(client_id=client_id, transport="websockets")
            self.mqtt_client.tls_set(cert_reqs=ssl.CERT_REQUIRED)
            self.mqtt_client.username_pw_set(self.mqtt_config.get("username", "esp_user"), self.mqtt_config.get("password", ""))
            self.mqtt_client.on_connect = self._on_mqtt_connect
            self.mqtt_client.on_message = self._on_mqtt_message
            
            broker = self.mqtt_config.get("broker", "mqtt.edxo.ca")
            port = self.mqtt_config.get("port", 443)
            self.mqtt_client.connect(broker, port, 60)
            self.mqtt_client.loop_start()
        except Exception as e:
            self.status_message = f"Erreur MQTT: {str(e)}"

    def _on_mqtt_connect(self, client, userdata, flags, rc):
        if rc == 0:
            self.mqtt_connected = True
            client.subscribe(self.sensors_buttons_topic)
            client.subscribe(self.sensors_pots_topic)
            client.subscribe(self.sensors_accel_topic)

    def _on_mqtt_message(self, client, userdata, msg):
        try:
            payload = json.loads(msg.payload.decode('utf-8'))
            if msg.topic == self.sensors_buttons_topic:
                self.btn1_state = payload.get("btn1", False)
                self.btn2_state = payload.get("btn2", False)
            elif msg.topic == self.sensors_pots_topic:
                self.pot1_val = payload.get("pot1", 0)
                self.pot2_val = payload.get("pot2", 0)
            elif msg.topic == self.sensors_accel_topic:
                self.accel_data = payload
        except: pass

    def _publish_mqtt(self, topic, message):
        if self.mqtt_client and self.mqtt_connected:
            self.mqtt_client.publish(topic, message)

    def _draw_progress_bar(self, row, col, width, value, max_val):
        bar_width = int((value / max_val) * (width - 2))
        bar_width = max(0, min(width - 2, bar_width))
        self.stdscr.addstr(row, col, "[" + "=" * bar_width + " " * (width - 2 - bar_width) + "]")

    def _init_colors(self):
        curses.start_color()
        curses.use_default_colors()
        curses.init_pair(1, curses.COLOR_BLACK, curses.COLOR_YELLOW)   # QUIT
        curses.init_pair(2, curses.COLOR_GREEN, curses.COLOR_BLACK)    # MQTT ON
        curses.init_pair(3, curses.COLOR_YELLOW, curses.COLOR_BLACK)   # Status texte
        curses.init_pair(4, curses.COLOR_RED, curses.COLOR_BLACK)      # MQTT OFF
        # LED OFF pairs
        curses.init_pair(10, curses.COLOR_RED, curses.COLOR_BLACK)     # LED1 OFF
        curses.init_pair(11, curses.COLOR_GREEN, curses.COLOR_BLACK)   # LED2 OFF
        curses.init_pair(12, curses.COLOR_YELLOW, curses.COLOR_BLACK)  # LED3 OFF
        curses.init_pair(13, curses.COLOR_BLUE, curses.COLOR_BLACK)    # LED4 OFF

    def _build_buttons(self, h, w):
        self.buttons = []
        btn_width = min(15, w // 5 - 2)
        btn_height = 5
        start_row = 4
        
        led_pairs = [10, 11, 12, 13]
        for i in range(4):
            self.buttons.append({
                "name": f"LED{i+1}",
                "label": f"LED{i+1}",
                "topic": self.led_topics[i],
                "color_pair": led_pairs[i],
                "row": start_row,
                "col": 2 + i * (btn_width + 2),
                "height": btn_height,
                "width": btn_width,
            })
        self.buttons.append({
            "name": "QUIT", "label": "QUITTER", "topic": None, "color_pair": 1,
            "row": h - 4, "col": w // 2 - 8, "height": 3, "width": 16,
        })

    def _draw(self):
        self.stdscr.erase()
        h, w = self.stdscr.getmaxyx()
        self.stdscr.addstr(0, w // 2 - 8, "DASHBOARD MQTT", curses.A_BOLD)
        status_text = "MQTT: CONNECTÉ" if self.mqtt_connected else "MQTT: DÉCONNECTÉ"
        status_color = curses.color_pair(2) if self.mqtt_connected else curses.color_pair(4)
        self.stdscr.attron(status_color)
        self.stdscr.addstr(0, w - len(status_text) - 2, status_text)
        self.stdscr.attroff(status_color)

        self._build_buttons(h, w)
        for btn in self.buttons:
            attr = curses.color_pair(btn["color_pair"])
            if btn["name"].startswith("LED"):
                idx = int(btn["name"][3:]) - 1
                if self.led_states[idx]: attr |= curses.A_REVERSE
            
            self.stdscr.attron(attr)
            for r in range(btn["row"], btn["row"] + btn["height"]):
                self.stdscr.addstr(r, btn["col"], " " * btn["width"])
            self.stdscr.addstr(btn["row"] + 2, btn["col"] + (btn["width"] - len(btn["label"])) // 2, btn["label"])
            self.stdscr.attroff(attr)

        self.stdscr.addstr(11, 2, f"Buttons: {self.btn1_state} {self.btn2_state}", curses.A_BOLD)
        self.stdscr.addstr(12, 2, "Pot1:")
        self._draw_progress_bar(12, 8, 20, self.pot1_val, 4095)
        self.stdscr.addstr(13, 2, "Pot2:")
        self._draw_progress_bar(13, 8, 20, self.pot2_val, 4095)
        self.stdscr.addstr(15, 2, f"Accel: {self.accel_data}")
        self.stdscr.attron(curses.color_pair(3))
        self.stdscr.addstr(h - 1, 0, self.status_message[:w-1])
        self.stdscr.attroff(curses.color_pair(3))
        self.stdscr.refresh()

    def _touch_to_rowcol(self, x_raw, y_raw):
        h, w = self.stdscr.getmaxyx()
        dx = max(1, self.touch_reader.max_x - self.touch_reader.min_x)
        dy = max(1, self.touch_reader.max_y - self.touch_reader.min_y)
        col = int(((x_raw - self.touch_reader.min_x) / dx) * (w - 1))
        row = int(((y_raw - self.touch_reader.min_y) / dy) * (h - 1))
        return max(0, min(h - 1, row)), max(0, min(w - 1, col))

    def _handle_touch_tap(self, x_raw, y_raw):
        row, col = self._touch_to_rowcol(x_raw, y_raw)
        for btn in self.buttons:
            if (btn["row"] <= row < btn["row"] + btn["height"] and
                    btn["col"] <= col < btn["col"] + btn["width"]):
                if btn["name"] == "QUIT": self.running = False
                elif btn["name"].startswith("LED"):
                    idx = int(btn["name"][3:]) - 1
                    self.led_states[idx] = not self.led_states[idx]
                    self._publish_mqtt(btn["topic"], json.dumps({"state": "on" if self.led_states[idx] else "off"}))

    def run(self):
        self.stdscr.nodelay(True)
        curses.curs_set(0)
        self._init_colors()
        while self.running:
            self._draw()
            try:
                event = self.event_queue.get_nowait()
                if event[0] == "tap": self._handle_touch_tap(event[1], event[2])
            except: pass
            time.sleep(0.05)
        if self.mqtt_client:
            self.mqtt_client.loop_stop()
            self.mqtt_client.disconnect()

def main(stdscr):
    event_queue = Queue()
    touch_reader = TouchReader(event_queue)
    touch_reader.start()
    ui = LEDControlUI(stdscr, touch_reader, event_queue, MQTT_CONFIG)
    ui.run()

if __name__ == "__main__":
    curses.wrapper(main)
