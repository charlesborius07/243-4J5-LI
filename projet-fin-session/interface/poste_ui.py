import curses
import threading
import time
import json
import ssl
import paho.mqtt.client as mqtt
from queue import Queue
from evdev import InputDevice, ecodes, list_devices
from mqtt_config import MQTT_CONFIG

MQTT_BROKER = MQTT_CONFIG["broker"]
MQTT_PORT   = MQTT_CONFIG["port"]
MQTT_USER   = MQTT_CONFIG["username"]
MQTT_PASS   = MQTT_CONFIG["password"]
CLIENT_ID   = "poste-05-ui"
TOPIC_BASE  = f"hydro-limoilou/{MQTT_CONFIG['device_id']}"

class TouchReader(threading.Thread):
    def __init__(self, event_queue: Queue):
        super().__init__(daemon=True)
        self.event_queue = event_queue
        self.device = self._find_touch_device()
        self.min_x, self.max_x, self.min_y, self.max_y = 0, 1000, 0, 1000
        if self.device:
            abs_x = self.device.absinfo(ecodes.ABS_MT_POSITION_X)
            abs_y = self.device.absinfo(ecodes.ABS_MT_POSITION_Y)
            self.min_x, self.max_x = abs_x.min, abs_x.max
            self.min_y, self.max_y = abs_y.min, abs_y.max

    def _find_touch_device(self):
        for path in list_devices():
            dev = InputDevice(path)
            if "touch" in dev.name.lower() or "ft5406" in dev.name.lower(): return dev
        return None

    def run(self):
        if not self.device: return
        for event in self.device.read_loop():
            if event.type == ecodes.EV_ABS:
                if event.code == ecodes.ABS_MT_POSITION_X: self.current_x = event.value
                elif event.code == ecodes.ABS_MT_POSITION_Y: self.current_y = event.value
            elif event.type == ecodes.EV_KEY and event.code == ecodes.BTN_TOUCH and event.value == 1:
                self.event_queue.put(("tap", self.current_x, self.current_y))

class MQTTHandler:
    def __init__(self):
        self.data = {"temp": "N/A", "hum": "N/A", "pres": "N/A", "v_line": "N/A", "i_line": "N/A", "alarms": [], "rssi": "N/A", "uptime": "0s", "led1": "off", "led2": "off", "llm": "En attente..."}
        self.client = mqtt.Client(client_id=CLIENT_ID, transport="websockets")
        self.client.tls_set(cert_reqs=ssl.CERT_REQUIRED, tls_version=ssl.PROTOCOL_TLS)
        self.client.username_pw_set(MQTT_USER, MQTT_PASS)
        self.client.on_message = self.on_message
        self.client.connect(MQTT_BROKER, MQTT_PORT, 60)
        self.client.subscribe(f"{TOPIC_BASE}/#")
        self.client.loop_start()

    def on_message(self, client, userdata, msg):
        try:
            topic = msg.topic
            payload = json.loads(msg.payload.decode())
            if "telemetry/temperature" in topic: self.data["temp"] = f"{payload['value']} {payload['unit']}"
            elif "telemetry/humidity" in topic: self.data["hum"] = f"{payload['value']} {payload['unit']}"
            elif "telemetry/pressure" in topic: self.data["pres"] = f"{payload['value']} {payload['unit']}"
            elif "telemetry/voltage_line" in topic: self.data["v_line"] = f"{payload['value']} {payload['unit']}"
            elif "telemetry/current_line" in topic: self.data["i_line"] = f"{payload['value']} {payload['unit']}"
            elif "status/llm" in topic: self.data["llm"] = payload.get("summary", "...")
            elif "alarm/" in topic: self.data["alarms"].append(f"{topic.split('/')[-1]}: {payload['level']}")
            elif "status" in topic and "llm" not in topic: self.data["rssi"], self.data["uptime"] = payload.get("rssi", "N/A"), f"{payload.get('uptime', 0)}s"
            elif "actuators/led_1" in topic: self.data["led1"] = payload.get("state", "off")
            elif "actuators/led_2" in topic: self.data["led2"] = payload.get("state", "off")
        except: pass

class DashboardUI:
    def __init__(self, stdscr, mqtt_handler, touch_reader, event_queue):
        self.stdscr = stdscr
        self.mqtt = mqtt_handler
        self.touch = touch_reader
        self.queue = event_queue
        self.running = True
        self.page = 0
        self.tabs = ["TÉLÉMÉTRIE", "ALARMES", "LIEN"]
        curses.curs_set(0)
        curses.start_color()
        curses.init_pair(1, curses.COLOR_CYAN, curses.COLOR_BLACK)
        curses.init_pair(2, curses.COLOR_MAGENTA, curses.COLOR_BLACK)
        curses.init_pair(3, curses.COLOR_YELLOW, curses.COLOR_BLACK)
        curses.init_pair(4, curses.COLOR_WHITE, curses.COLOR_BLUE)
        curses.init_pair(5, curses.COLOR_BLACK, curses.COLOR_RED)
        curses.init_pair(6, curses.COLOR_WHITE, curses.COLOR_RED) # LED ON ROUGE
        curses.init_pair(7, curses.COLOR_WHITE, curses.COLOR_GREEN) # LED ON VERTE
        curses.init_pair(8, curses.COLOR_BLACK, curses.COLOR_WHITE) # LED OFF

    def _draw_box(self, r, c, h, w, title, value, color_pair, gauge=None, fill=False):
        self.stdscr.attron(curses.color_pair(color_pair) | curses.A_BOLD)
        # Remplissage si LED allumée
        if fill:
            for i in range(1, h - 1): self.stdscr.addstr(r + i, c + 1, " " * (w - 2))
        
        for i in range(h): self.stdscr.addstr(r + i, c, "|" + " " * (w - 2) + "|")
        self.stdscr.addstr(r, c, "+" + "-" * (w - 2) + "+")
        self.stdscr.addstr(r + h - 1, c, "+" + "-" * (w - 2) + "+")
        
        # Agrandissement des textes
        self.stdscr.addstr(r + 1, c + (w - len(title)) // 2, title.upper())
        self.stdscr.addstr(r + 2, c + (w - len(value)) // 2, value.upper())
        
        if gauge is not None:
            bar_w = w - 4
            filled = int(gauge * bar_w)
            self.stdscr.addstr(r + 3, c + 2, "[" + "#" * filled + "-" * (bar_w - filled) + "]")
        self.stdscr.attroff(curses.color_pair(color_pair) | curses.A_BOLD)

    def _draw(self):
        self.stdscr.erase()
        h, w = self.stdscr.getmaxyx()
        tab_h, tab_w = 5, w // 3
        for i, tab in enumerate(self.tabs):
            attr = curses.A_REVERSE | curses.A_BOLD
            color = [curses.color_pair(1), curses.color_pair(2), curses.color_pair(3)][i]
            for r in range(tab_h): self.stdscr.addstr(r, i * tab_w, " " * tab_w, color | attr)
            self.stdscr.addstr(2, i * tab_w + (tab_w - len(tab)) // 2, tab, color | attr)
        self.stdscr.addstr(h-3, w-12, " [QUIT] ", curses.color_pair(5) | curses.A_REVERSE | curses.A_BOLD)
        
        start_row = 6
        if self.page == 0:
            box_h = (h - start_row - 4) // 4
            box_w = (w - 6) // 2
            self._draw_box(start_row, 2, box_h, box_w, "TEMP", self.mqtt.data['temp'], 1)
            self._draw_box(start_row, 4 + box_w, box_h, box_w, "HUM", self.mqtt.data['hum'], 1)
            self._draw_box(start_row + box_h + 1, 2, box_h, box_w, "PRES", self.mqtt.data['pres'], 1)
            # Gauges for Volt (200-260V) and Curr (0-100A)
            try: v = (float(self.mqtt.data['v_line'].split()[0]) - 200) / 60
            except: v = 0
            self._draw_box(start_row + box_h + 1, 4 + box_w, box_h, box_w, "VOLT", self.mqtt.data['v_line'], 1, gauge=max(0,min(1,v)))
            try: c = float(self.mqtt.data['i_line'].split()[0]) / 100
            except: c = 0
            self._draw_box(start_row + 2*(box_h + 1), 2, box_h, box_w, "CURR", self.mqtt.data['i_line'], 1, gauge=max(0,min(1,c)))
            # LEDs dans la troisième rangée
            led1_active = self.mqtt.data['led1'] == 'on'
            led2_active = self.mqtt.data['led2'] == 'on'
            led1_color = 6 if led1_active else 8
            led2_color = 7 if led2_active else 8
            self._draw_box(start_row + 3*(box_h + 1), 2, box_h, box_w, "LED ROUGE", self.mqtt.data['led1'].upper(), led1_color, fill=led1_active)
            self._draw_box(start_row + 3*(box_h + 1), 4 + box_w, box_h, box_w, "LED VERTE", self.mqtt.data['led2'].upper(), led2_color, fill=led2_active)

        elif self.page == 1:
            self.stdscr.addstr(6, 2, "RESUME LLM:", curses.A_BOLD | curses.color_pair(2))
            self.stdscr.addstr(7, 2, self.mqtt.data['llm'][:w-4])
            self.stdscr.addstr(10, 2, "ALARMES:", curses.A_BOLD | curses.color_pair(2))
            for i, a in enumerate(self.mqtt.data['alarms'][-3:]):
                self._draw_box(12 + i*5, 2, 4, w-4, "ALERTE", a, 2)
            self.stdscr.addstr(h-2, 2, "[TAPER POUR ACK]", curses.color_pair(5) | curses.A_REVERSE)

        elif self.page == 2:
            self._draw_box(6, 2, 8, w-4, "RSSI (Liaison)", f"{self.mqtt.data['rssi']} dBm", 3)
            self._draw_box(15, 2, 6, w-4, "UPTIME", self.mqtt.data['uptime'], 3)
        self.stdscr.refresh()

    def run(self):
        self.stdscr.nodelay(True)
        while self.running:
            self._draw()
            try:
                event = self.queue.get_nowait()
                if event[0] == "tap":
                    y, x = self._touch_to_curses(event[1], event[2])
                    h, w = self.stdscr.getmaxyx()
                    if y < 5: self.page = x // (w // 3)
                    elif y >= h-4 and x >= w-12: self.running = False
                    elif self.page == 1 and y >= h-5: self.mqtt.data['alarms'] = []
            except: pass
            if self.stdscr.getch() == ord('q'): self.running = False
            time.sleep(0.05)

    def _touch_to_curses(self, x, y):
        h, w = self.stdscr.getmaxyx()
        norm_x = (x - self.touch.min_x) / max(1, self.touch.max_x - self.touch.min_x)
        norm_y = (y - self.touch.min_y) / max(1, self.touch.max_y - self.touch.min_y)
        return int(norm_y * h), int(norm_x * w)

def main(stdscr):
    q = Queue()
    t = TouchReader(q); t.start()
    ui = DashboardUI(stdscr, MQTTHandler(), t, q)
    ui.run()

if __name__ == "__main__": curses.wrapper(main)
