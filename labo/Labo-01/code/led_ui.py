import threading
import time
from queue import Queue
import os

import curses
from evdev import InputDevice, ecodes, list_devices


# ---------- GESTION DU TOUCH ----------

class TouchReader(threading.Thread):
    def __init__(self, event_queue: Queue):
        super().__init__(daemon=True)
        self.event_queue = event_queue
        self.device = self._find_touch_device()
        if not self.device:
            raise RuntimeError("Aucun périphérique touchscreen trouvé.")

        # On récupère les infos d’axes pour calibrer
        abs_x = self.device.absinfo(ecodes.ABS_MT_POSITION_X)
        abs_y = self.device.absinfo(ecodes.ABS_MT_POSITION_Y)

        self.min_x, self.max_x = abs_x.min, abs_x.max
        self.min_y, self.max_y = abs_y.min, abs_y.max

        self.current_x = (self.min_x + self.max_x) // 2
        self.current_y = (self.min_y + self.max_y) // 2

    def _find_touch_device(self):
        """
        Essaie de trouver un device dont le nom contient 'touch' ou 'ft5406'
        (fréquent sur les écrans Raspberry Pi).
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

class CoolConsoleUI:
    def __init__(self, stdscr, touch_reader: TouchReader, event_queue: Queue):
        self.stdscr = stdscr
        self.touch_reader = touch_reader
        self.event_queue = event_queue
        self.running = True
        self.status_message = "Prêt. Touchez un bouton."

        self.buttons = []  # rempli à chaque redraw en fonction de la taille écran
        self.serial_fd = None
        self.serial_path = '/dev/ttyACM0'
        
        # Configuration du port série via stty (baudrate 115200)
        os.system(f"stty -F {self.serial_path} 115200 raw -echo")
        
        # Ouverture du port série en mode binaire non-bufferisé
        try:
            # os.open est plus bas niveau et évite les buffers de Python
            self.serial_fd = os.open(self.serial_path, os.O_RDWR | os.O_NOCTTY | os.O_SYNC)
        except Exception as e:
            self.status_message = f"Erreur ouverture Serial: {e}"

    def _init_colors(self):
        curses.start_color()
        curses.use_default_colors()
        curses.init_pair(1, curses.COLOR_BLACK, curses.COLOR_CYAN)   # STATUS
        curses.init_pair(2, curses.COLOR_BLACK, curses.COLOR_GREEN)  # Active (Pressed)
        curses.init_pair(3, curses.COLOR_YELLOW, -1)                 # Texte status
        curses.init_pair(4, curses.COLOR_BLACK, curses.COLOR_MAGENTA)# LOGS
        curses.init_pair(5, curses.COLOR_BLACK, curses.COLOR_YELLOW) # REBOOT
        curses.init_pair(6, curses.COLOR_WHITE, curses.COLOR_RED)    # ROUGE
        curses.init_pair(7, curses.COLOR_BLACK, curses.COLOR_GREEN)  # VERT (Texte noir sur fond vert pour lisibilité)

    def _build_buttons(self, h, w):
        """
        Construit 2 gros boutons (ROUGE et VERT) centrés verticalement.
        """
        self.buttons = []
        btn_width = max(20, w - 4)
        
        # Espace vertical disponible (moins header et status)
        avail_h = max(1, h - 4)
        
        # Hauteur par bouton (on en veut 2)
        btn_height = (avail_h - 2) // 2
        if btn_height < 3:
            btn_height = 3

        # Position de départ
        start_row = 2

        # Bouton ROUGE
        self.buttons.append({
            "label": "ROUGE",
            "row": start_row,
            "col": (w - btn_width) // 2,
            "height": btn_height,
            "width": btn_width,
            "active": False,
            "color_idx": 6, # RED pair
            "command": b"RED\n"
        })

        # Bouton VERT
        self.buttons.append({
            "label": "VERT",
            "row": start_row + btn_height + 1,
            "col": (w - btn_width) // 2,
            "height": btn_height,
            "width": btn_width,
            "active": False,
            "color_idx": 7, # GREEN pair
            "command": b"GREEN\n"
        })

    def _draw(self):
        self.stdscr.erase()
        h, w = self.stdscr.getmaxyx()

        # Titre
        title = " LilyGO LED Control "
        self.stdscr.attron(curses.A_BOLD)
        self.stdscr.addstr(0, max(0, (w - len(title)) // 2), title)
        self.stdscr.attroff(curses.A_BOLD)

        # Status bar
        self.stdscr.attron(curses.color_pair(3))
        try:
            self.stdscr.addstr(h - 1, 1, f"Status: {self.status_message[:w-4]}")
        except curses.error:
            pass # Ignorer si l'écran est trop petit
        self.stdscr.attroff(curses.color_pair(3))

        # Construire les boutons selon la taille écran
        self._build_buttons(h, w)

        # Dessin des boutons
        for btn in self.buttons:
            # Si actif (touché), on utilise la paire 2 (Active/GreenInv), sinon la couleur spécifique
            attr = curses.color_pair(2) if btn["active"] else curses.color_pair(btn["color_idx"])
            
            for r in range(btn["row"], btn["row"] + btn["height"]):
                if 0 <= r < h - 1:
                    try:
                        self.stdscr.attron(attr)
                        self.stdscr.addstr(r, btn["col"], " " * btn["width"])
                        self.stdscr.attroff(attr)
                    except curses.error:
                        pass

            # Label centré
            label = f"[ {btn['label']} ]"
            label_col = btn["col"] + max(0, (btn["width"] - len(label)) // 2)
            label_row = btn["row"] + btn["height"] // 2
            if 0 <= label_row < h - 1:
                try:
                    self.stdscr.addstr(label_row, label_col, label)
                except curses.error:
                    pass

        self.stdscr.refresh()

    def _touch_to_rowcol(self, x_raw, y_raw):
        """
        Map coordonnées brutes evdev -> lignes/colonnes du terminal curses.
        """
        h, w = self.stdscr.getmaxyx()

        # protection division par zéro
        dx = max(1, self.touch_reader.max_x - self.touch_reader.min_x)
        dy = max(1, self.touch_reader.max_y - self.touch_reader.min_y)

        x_norm = (x_raw - self.touch_reader.min_x) / dx
        y_norm = (y_raw - self.touch_reader.min_y) / dy

        col = int(x_norm * (w - 1))
        row = int(y_norm * (h - 1))

        # clamp
        row = max(0, min(h - 1, row))
        col = max(0, min(w - 1, col))
        return row, col

    def _handle_touch_tap(self, x_raw, y_raw):
        row, col = self._touch_to_rowcol(x_raw, y_raw)

        # Vérifier sur quel bouton on a tapé
        clicked_btn = None
        for btn in self.buttons:
            if (btn["row"] <= row < btn["row"] + btn["height"] and
                    btn["col"] <= col < btn["col"] + btn["width"]):
                clicked_btn = btn
                break

        # Réinitialiser les états
        for btn in self.buttons:
            btn["active"] = False

        if not clicked_btn:
            # self.status_message = f"Touch: {row},{col} (vide)"
            return

        clicked_btn["active"] = True
        command = clicked_btn["command"]
        
        # Envoi Série
        try:
            if self.serial_fd:
                os.write(self.serial_fd, command)
                # Pas besoin de flush avec os.write et O_SYNC
                self.status_message = f"ENVOYE: {command.strip()}"
            else:
                self.status_message = "Erreur: Port série non ouvert"
                # Tentative de reconnexion
                self.serial_fd = os.open(self.serial_path, os.O_RDWR | os.O_NOCTTY | os.O_SYNC)
        except Exception as e:
            self.status_message = f"ERREUR ENVOI: {e}"
            self.serial_fd = None


    def run(self):
        self.stdscr.nodelay(True)
        curses.curs_set(0)
        self._init_colors()

        last_redraw = 0

        while self.running:
            now = time.time()
            if now - last_redraw > 0.05:  # ~20 FPS
                self._draw()
                last_redraw = now

            # Lecture touches clavier (pour quitter proprement)
            try:
                ch = self.stdscr.getch()
            except curses.error:
                ch = -1

            if ch == ord('q'):
                self.status_message = "Quitter..."
                self.running = False

            # Gestion des événements tactiles
            try:
                event = self.event_queue.get_nowait()
            except Exception:
                event = None

            if event:
                kind, x_raw, y_raw = event
                if kind == "tap":
                    self._handle_touch_tap(x_raw, y_raw)

            time.sleep(0.01)

        # Nettoyage
        if self.serial_fd:
            try:
                os.close(self.serial_fd)
            except:
                pass


# ---------- ENTRY POINT ----------

def main(stdscr):
    event_queue = Queue()
    
    # Touch Reader start
    try:
        touch_reader = TouchReader(event_queue)
        touch_reader.start()
    except Exception as e:
        # En cas d'erreur touch (ex: pas de device), on log mais on continue si possible
        # Mais ici on a besoin du touch pour l'UI, donc on laisse planter proprement
        raise e

    ui = CoolConsoleUI(stdscr, touch_reader, event_queue)
    ui.run()


if __name__ == "__main__":
    curses.wrapper(main)
