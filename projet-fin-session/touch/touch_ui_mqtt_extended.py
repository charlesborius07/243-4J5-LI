import threading
import time
from queue import Queue
import curses
from evdev import InputDevice, ecodes, list_devices
import paho.mqtt.client as mqtt
import ssl
import json

from mqtt_config import MQTT_CONFIG

class TouchReader(threading.Thread):
    def __init__(self, event_queue: Queue):
        super().__init__(daemon=True)
        self.event_queue = event_queue
        self.device = self._find_touch_device()
        if not self.device:
            raise RuntimeError("Aucun périphérique touchscreen trouvé.")

        abs_x = self.device.absinfo(ecodes.ABS_MT_POSITION_X)
        abs_y = self.device.absinfo(ecodes.ABS_MT_POSITION_Y)
        self.min_x, self.max_x = abs_x.min, abs_x.max
        self.min_y, self.max_y = abs_y.min, abs_y.max
        self.current_x = (self.min_x + self.max_x) // 2
        self.current_y = (self.min_y + self.max_y) // 2

    def _find_touch_device(self):
        for path in list_devices():
            dev = InputDevice(path)
            name = dev.name.lower()
            if "touch" in name or "ft5406" in name:
                return dev
        return None

    def run(self):
        for event in self.device.read_loop():
            if event.type == ecodes.EV_ABS:
                if event.code == ecodes.ABS_MT_POSITION_X: self.current_x = event.value
                elif event.code == ecodes.ABS_MT_POSITION_Y: self.current_y = event.value
            elif event.type == ecodes.EV_KEY and event.code == ecodes.BTN_TOUCH and event.value == 1:
                self.event_queue.put(("tap", self.current_x, self.current_y))

class LEDControlUI:
    def __init__(self, stdscr, touch_reader: TouchReader, event_queue: Queue, mqtt_config: dict):
        self.stdscr = stdscr
        self.touch_reader = touch_reader
        self.event_queue = event_queue
        self.running = True
        self.mqtt_config = mqtt_config
        self.mqtt_client = None
        self.mqtt_connected = False
        self.base_topic = mqtt_config.get("device_id", "hydro-limoilou/poste-05")

        self.sensor_data = {"Temp": "N/A", "Hum": "N/A", "Pres": "N/A", "Volt": "N/A", "Curr": "N/A"}
        self.led_states = {"LED1": "OFF", "LED2": "OFF"}
        self.llm_response = "En attente..."
        self.alarm_statuses = {"motion": "OK", "voltage": "OK", "current": "OK"}
        self.status = {"RSS": "N/A", "Uptime": "N/A"}

        self.current_page = "Télémétrie"
        self.pages = ["Télémétrie", "Alarmes", "Lien"]
        
        self._init_mqtt()

    def _init_mqtt(self):
        self.mqtt_client = mqtt.Client(transport="websockets")
        self.mqtt_client.tls_set(cert_reqs=ssl.CERT_REQUIRED)
        self.mqtt_client.username_pw_set(self.mqtt_config.get("username"), self.mqtt_config.get("password"))
        self.mqtt_client.on_connect = lambda c, u, f, rc: setattr(self, 'mqtt_connected', rc==0)
        self.mqtt_client.on_message = self._on_mqtt_message
        self.mqtt_client.connect(self.mqtt_config.get("broker"), self.mqtt_config.get("port"), 60)
        self.mqtt_client.loop_start()
        self.mqtt_client.subscribe(f"{self.base_topic}/#")

    def _on_mqtt_message(self, client, userdata, msg):
        topic, payload = msg.topic, msg.payload.decode('utf-8', errors='ignore')
        try:
            data = json.loads(payload)
            if "telemetry" in topic:
                if "temperature" in topic: self.sensor_data["Temp"] = f"{data.get('value')} {data.get('unit')}"
                elif "humidity" in topic: self.sensor_data["Hum"] = f"{data.get('value')} {data.get('unit')}"
                elif "pressure" in topic: self.sensor_data["Pres"] = f"{data.get('value')} {data.get('unit')}"
                elif "voltage_line" in topic: self.sensor_data["Volt"] = f"{data.get('value')} {data.get('unit')}"
                elif "current_line" in topic: self.sensor_data["Curr"] = f"{data.get('value')} {data.get('unit')}"
            elif "alarm/" in topic:
                a_type = topic.split('/')[-1]
                if a_type in self.alarm_statuses: self.alarm_statuses[a_type] = data.get('level', 'WARNING')
            elif "status/llm" in topic: self.llm_response = data.get('summary', str(data))
            elif "actuators" in topic:
                state = str(data.get('state', '')).upper()
                if state in ["ON", "OFF"]:
                    if "led_1" in topic: self.led_states["LED1"] = state
                    elif "led_2" in topic: self.led_states["LED2"] = state
            elif "status" in topic:
                self.status = {"RSS": f"{data.get('rssi')}dB", "Uptime": f"{data.get('uptime')}s"}
        except: pass

    def _init_colors(self):
        curses.start_color()
        curses.use_default_colors()
        curses.init_pair(1, curses.COLOR_BLACK, curses.COLOR_YELLOW)   # Telemetrie
        curses.init_pair(2, curses.COLOR_BLACK, curses.COLOR_MAGENTA)  # Alarmes
        curses.init_pair(3, curses.COLOR_BLACK, curses.COLOR_BLUE)     # Lien
        curses.init_pair(4, curses.COLOR_BLACK, curses.COLOR_RED)      # Quitter
        
        curses.init_pair(10, curses.COLOR_RED, -1)     # Temp
        curses.init_pair(11, curses.COLOR_CYAN, -1)    # Hum
        curses.init_pair(12, curses.COLOR_MAGENTA, -1) # Pres
        curses.init_pair(13, curses.COLOR_YELLOW, -1)  # Volt
        curses.init_pair(14, curses.COLOR_GREEN, -1)   # Curr
        
        curses.init_pair(6, curses.COLOR_BLACK, curses.COLOR_GREEN)    # LED1 ON (Vert)
        curses.init_pair(7, curses.COLOR_BLACK, curses.COLOR_BLUE)     # LED2 ON (Bleu)
        curses.init_pair(8, curses.COLOR_WHITE, curses.COLOR_RED)      # OFF (Rouge)
        
        curses.init_pair(20, curses.COLOR_WHITE, curses.COLOR_GREEN)   # ALARM OK
        curses.init_pair(21, curses.COLOR_WHITE, curses.COLOR_RED)     # ALARM WARNING

    def _draw_nav(self, w):
        page_width = w // 4
        nav_height = 4
        for i, page in enumerate(self.pages):
            attr = curses.color_pair(i + 1) | curses.A_BOLD
            self.stdscr.attron(attr)
            for r in range(0, nav_height):
                self.stdscr.addstr(r, i * page_width, " " * page_width)
            self.stdscr.addstr(nav_height // 2, i * page_width + (page_width - len(page)) // 2, page)
            self.stdscr.attroff(attr)
        
        self.stdscr.attron(curses.color_pair(4) | curses.A_BOLD)
        for r in range(0, nav_height):
            self.stdscr.addstr(r, 3 * page_width, " " * (w - 3 * page_width))
        self.stdscr.addstr(nav_height // 2, 3 * page_width + (page_width - 7) // 2, "QUITTER")
        self.stdscr.attroff(curses.color_pair(4) | curses.A_BOLD)

    def _draw_gauge(self, row, col, label, value, max_val, w, color_pair):
        width = min(w - 20, 30)
        filled = int((value / max_val) * width) if max_val > 0 else 0
        self.stdscr.attron(curses.color_pair(color_pair) | curses.A_BOLD)
        self.stdscr.addstr(row, col, f"{label:12} : {value:6.1f}")
        self.stdscr.addstr(row + 1, col, "█" * filled + "░" * (width - filled))
        self.stdscr.attroff(curses.color_pair(color_pair) | curses.A_BOLD)

    def _draw_led(self, row, col, name, state, led_type):
        h, w = 6, 20
        color = (curses.color_pair(6) if led_type == "LED1" else curses.color_pair(7)) if state == "ON" else curses.color_pair(8)
        self.stdscr.attron(color)
        for r in range(row, min(row + h, curses.LINES - 1)):
            self.stdscr.addstr(r, col, " " * min(w, curses.COLS - col - 1))
        self.stdscr.attroff(color)
        self.stdscr.attron(curses.A_BOLD)
        if row + 2 < curses.LINES:
            self.stdscr.addstr(row + 2, col + 2, f"{name}: {state}")
        self.stdscr.attroff(curses.A_BOLD)

    def _draw_alarm(self, row, col, name, status):
        h, w = 4, 30
        color = curses.color_pair(20) if status == "OK" else curses.color_pair(21)
        self.stdscr.attron(color | curses.A_BOLD)
        for r in range(row, row + h):
            self.stdscr.addstr(r, col, " " * w)
        self.stdscr.addstr(row + 1, col + 2, f"{name.upper()}")
        self.stdscr.addstr(row + 2, col + 2, f"STATUS: {status}")
        self.stdscr.attroff(color | curses.A_BOLD)

    def _draw(self):
        self.stdscr.erase()
        h, w = self.stdscr.getmaxyx()
        self._draw_nav(w)

        row = 5
        if self.current_page == "Télémétrie":
            sensors = [("Temp", self.sensor_data.get("Temp"), 10), ("Hum", self.sensor_data.get("Hum"), 11), ("Pres", self.sensor_data.get("Pres"), 12)]
            for i, (name, val, col_p) in enumerate(sensors):
                self.stdscr.attron(curses.color_pair(col_p) | curses.A_BOLD)
                self.stdscr.addstr(row + i, 2, f"{name}: {val}")
                self.stdscr.attroff(curses.color_pair(col_p) | curses.A_BOLD)
            v = float(self.sensor_data.get("Volt", "0").split()[0]) if "N/A" not in self.sensor_data["Volt"] else 0
            c = float(self.sensor_data.get("Curr", "0").split()[0]) if "N/A" not in self.sensor_data["Curr"] else 0
            self._draw_gauge(row + 4, 2, "Tension", v, 300, w, 13)
            self._draw_gauge(row + 7, 2, "Courant", c, 100, w, 14)
            self._draw_led(row + 10, 2, "LED1", self.led_states["LED1"], "LED1")
            self._draw_led(row + 17, 2, "LED2", self.led_states["LED2"], "LED2")
            
        elif self.current_page == "Alarmes":
            self._draw_alarm(row, 2, "Motion", self.alarm_statuses["motion"])
            self._draw_alarm(row + 5, 2, "Tension", self.alarm_statuses["voltage"])
            self._draw_alarm(row + 10, 2, "Courant", self.alarm_statuses["current"])
            self.stdscr.addstr(h-2, 2, "[RESET]", curses.color_pair(4) | curses.A_BOLD)
            
        elif self.current_page == "Lien":
            self.stdscr.addstr(row, 2, f"RSSI: {self.status.get('RSS')}  Uptime: {self.status.get('Uptime')}")
            self.stdscr.addstr(row+2, 2, f"LLM: {self.llm_response[:w-5]}")
        self.stdscr.refresh()

    def _touch_to_rowcol(self, x_raw, y_raw):
        h, w = self.stdscr.getmaxyx()
        return int((y_raw-self.touch_reader.min_y)/(self.touch_reader.max_y-self.touch_reader.min_y)*(h-1)), \
               int((x_raw-self.touch_reader.min_x)/(self.touch_reader.max_x-self.touch_reader.min_x)*(w-1))

    def run(self):
        self.stdscr.nodelay(True)
        self._init_colors()
        while self.running:
            self._draw()
            try:
                event = self.event_queue.get_nowait()
                row, col = self._touch_to_rowcol(event[1], event[2])
                if row < 4:
                    w = curses.COLS
                    if col < w // 4: self.current_page = "Télémétrie"
                    elif col < 2 * (w // 4): self.current_page = "Alarmes"
                    elif col < 3 * (w // 4): self.current_page = "Lien"
                    else: self.running = False
                elif self.current_page == "Alarmes" and row >= curses.LINES - 3: self.alarm_statuses = {"motion": "OK", "voltage": "OK", "current": "OK"}
            except: pass
            time.sleep(0.1)

def main(stdscr):
    event_queue = Queue()
    t = TouchReader(event_queue); t.start()
    LEDControlUI(stdscr, t, event_queue, MQTT_CONFIG).run()

if __name__ == "__main__": curses.wrapper(main)
