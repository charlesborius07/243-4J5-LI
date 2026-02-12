import threading
import time
from queue import Queue
import serial
import curses
from evdev import InputDevice, ecodes, list_devices

# ---------- GESTION DU TOUCH (Identique à touch_ui.py) ----------

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
                if event.code == ecodes.ABS_MT_POSITION_X:
                    self.current_x = event.value
                elif event.code == ecodes.ABS_MT_POSITION_Y:
                    self.current_y = event.value
            elif event.type == ecodes.EV_KEY and event.code == ecodes.BTN_TOUCH:
                if event.value == 1:
                    self.event_queue.put(("tap", self.current_x, self.current_y))

# ---------- UI CONTROL LED ----------

class LedControlUI:
    def __init__(self, stdscr, touch_reader: TouchReader, event_queue: Queue, ser: serial.Serial):
        self.stdscr = stdscr
        self.touch_reader = touch_reader
        self.event_queue = event_queue
        self.ser = ser
        self.running = True
        self.status_message = "Connecté au LilyGo. En attente..."
        self.buttons = []

    def _init_colors(self):
        curses.start_color()
        curses.use_default_colors()
        # Pair 1: Status (Cyan)
        curses.init_pair(1, curses.COLOR_CYAN, -1)
        # Pair 2: Vert (Texte noir sur fond vert pour bouton actif/couleur)
        curses.init_pair(2, curses.COLOR_BLACK, curses.COLOR_GREEN)
        # Pair 3: Rouge (Texte noir sur fond rouge)
        curses.init_pair(3, curses.COLOR_BLACK, curses.COLOR_RED)
        # Pair 4: Quit (Blanc sur Rouge foncé/Noir)
        curses.init_pair(4, curses.COLOR_WHITE, curses.COLOR_RED)
        # Pair 5: Texte standard bouton (Blanc sur Noir)
        curses.init_pair(5, curses.COLOR_WHITE, curses.COLOR_BLACK)

    def _build_buttons(self, h, w):
        self.buttons = []
        btn_width = max(20, w - 10)
        btn_height = 4  # Plus gros boutons

        # Calcul positions pour 3 boutons (VERT, ROUGE, QUIT)
        # On les centre verticalement
        total_h = 3 * (btn_height + 2)
        start_row = (h - total_h) // 2
        if start_row < 2: start_row = 2

        col = (w - btn_width) // 2

        # Bouton VERT
        self.buttons.append({
            "label": "LED VERTE",
            "cmd": "VERT",
            "row": start_row,
            "col": col,
            "height": btn_height,
            "width": btn_width,
            "color_idx": 2, # Green pair
            "active": False
        })

        # Bouton ROUGE
        self.buttons.append({
            "label": "LED ROUGE",
            "cmd": "ROUGE",
            "row": start_row + btn_height + 2,
            "col": col,
            "height": btn_height,
            "width": btn_width,
            "color_idx": 3, # Red pair
            "active": False
        })

        # Bouton QUIT
        self.buttons.append({
            "label": "QUITTER",
            "cmd": "QUIT",
            "row": start_row + 2 * (btn_height + 2),
            "col": col,
            "height": btn_height,
            "width": btn_width,
            "color_idx": 4, 
            "active": False
        })

    def _draw(self):
        self.stdscr.erase()
        h, w = self.stdscr.getmaxyx()

        # Titre
        title = " CONTROLE LED LILYGO "
        self.stdscr.attron(curses.A_BOLD | curses.A_UNDERLINE)
        self.stdscr.addstr(1, max(0, (w - len(title)) // 2), title)
        self.stdscr.attroff(curses.A_BOLD | curses.A_UNDERLINE)

        # Status
        self.stdscr.attron(curses.color_pair(1))
        self.stdscr.addstr(h - 2, 2, f"Info: {self.status_message[:w-6]}")
        self.stdscr.attroff(curses.color_pair(1))

        self._build_buttons(h, w)

        for btn in self.buttons:
            # Couleur: Si "active" (vient d'être cliqué) on inverse ou on garde la couleur
            # Ici on garde la couleur définie (Vert pour vert, Rouge pour rouge)
            # mais on pourrait changer le style si sélectionné.
            attr = curses.color_pair(btn["color_idx"])
            if btn["active"]:
                attr = attr | curses.A_REVERSE

            # Dessin boîte
            for r in range(btn["row"], btn["row"] + btn["height"]):
                if 0 <= r < h:
                    self.stdscr.attron(attr)
                    self.stdscr.addstr(r, btn["col"], " " * btn["width"])
                    self.stdscr.attroff(attr)

            # Label centré
            label = f"[ {btn['label']} ]"
            label_col = btn["col"] + max(0, (btn["width"] - len(label)) // 2)
            label_row = btn["row"] + btn["height"] // 2
            
            if 0 <= label_row < h:
                self.stdscr.attron(attr | curses.A_BOLD)
                self.stdscr.addstr(label_row, label_col, label)
                self.stdscr.attroff(attr | curses.A_BOLD)

        self.stdscr.refresh()

    def _touch_to_rowcol(self, x_raw, y_raw):
        h, w = self.stdscr.getmaxyx()
        dx = max(1, self.touch_reader.max_x - self.touch_reader.min_x)
        dy = max(1, self.touch_reader.max_y - self.touch_reader.min_y)
        
        x_norm = (x_raw - self.touch_reader.min_x) / dx
        y_norm = (y_raw - self.touch_reader.min_y) / dy

        col = int(x_norm * (w - 1))
        row = int(y_norm * (h - 1))
        return max(0, min(h - 1, row)), max(0, min(w - 1, col))

    def _send_command(self, cmd):
        if self.ser and self.ser.is_open:
            try:
                msg = f"{cmd}\n"
                self.ser.write(msg.encode('utf-8'))
                self.status_message = f"Envoyé: {cmd}"
            except Exception as e:
                self.status_message = f"Erreur Série: {e}"
        else:
            self.status_message = "Erreur: Port série fermé"

    def _handle_tap(self, x, y):
        row, col = self._touch_to_rowcol(x, y)
        
        clicked = None
        for btn in self.buttons:
            if (btn["row"] <= row < btn["row"] + btn["height"] and
                btn["col"] <= col < btn["col"] + btn["width"]):
                clicked = btn
                break
        
        # Reset visual state
        for btn in self.buttons: btn["active"] = False

        if clicked:
            clicked["active"] = True
            cmd = clicked["cmd"]
            
            if cmd == "QUIT":
                self.running = False
            else:
                self._send_command(cmd)

    def run(self):
        self.stdscr.nodelay(True)
        curses.curs_set(0)
        self._init_colors()
        
        last_redraw = 0
        while self.running:
            now = time.time()
            if now - last_redraw > 0.05:
                self._draw()
                last_redraw = now

            try:
                ch = self.stdscr.getch()
                if ch == ord('q'): self.running = False
            except: pass

            try:
                event = self.event_queue.get_nowait()
                if event[0] == "tap":
                    self._handle_tap(event[1], event[2])
            except: pass
            
            time.sleep(0.01)

def main(stdscr):
    # Setup Serial
    try:
        # Tente de se connecter au LilyGo
        ser = serial.Serial('/dev/ttyACM0', 115200, timeout=1)
        time.sleep(2) # Attente reset Arduino
    except Exception as e:
        stdscr.addstr(0, 0, f"Erreur connection Serie: {e}")
        stdscr.refresh()
        time.sleep(2)
        return

    event_queue = Queue()
    try:
        touch_reader = TouchReader(event_queue)
        touch_reader.start()
    except RuntimeError as e:
        stdscr.addstr(0, 0, f"Erreur Touch: {e}")
        stdscr.refresh()
        time.sleep(2)
        return

    ui = LedControlUI(stdscr, touch_reader, event_queue, ser)
    ui.run()
    
    ser.close()

if __name__ == "__main__":
    curses.wrapper(main)
