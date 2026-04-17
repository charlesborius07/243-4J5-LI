#!/usr/bin/env python3
"""
Script de diagnostic pour tester l'écran tactile
Affiche les coordonnées brutes et mappées lorsqu'on touche l'écran
"""

import sys
import os
import time
from queue import Queue, Empty

# Import de la config MQTT (pour récupérer le device_id si nécessaire)
sys.path.append(os.path.abspath(os.path.join(os.path.dirname(__file__), '..', 'interface')))
from mqtt_config import MQTT_CONFIG

try:
    from evdev import list_devices, InputDevice, ecodes
    print("evdev disponible")
except ImportError:
    print("ERREUR: evdev non disponible")
    sys.exit(1)

class TouchDiagnostic:
    def __init__(self):
        self.running = True
        self.device = self._find_touch_device()
        if not self.device:
            print("ERREUR: Aucun périphérique tactile trouvé")
            sys.exit(1)
            
        # Calibrage comme dans interface_mqtt.py
        abs_x = self.device.absinfo(ecodes.ABS_MT_POSITION_X)
        abs_y = self.device.absinfo(ecodes.ABS_MT_POSITION_Y)
        self.min_x, self.max_x = abs_x.min, abs_x.max
        self.min_y, self.max_y = abs_y.min, abs_y.max
        
        print(f"Périphérique tactile trouvé: {self.device.name}")
        print(f"Plage X: {self.min_x} - {self.max_x}")
        print(f"Plage Y: {self.min_y} - {self.max_y}")
        print(f"Centre estimé: X={self.min_x + (self.max_x - self.min_x)//2}, Y={self.min_y + (self.max_y - self.min_y)//2}")
        print("\nTouchez l'écran pour voir les coordonnées...")
        print("Appuyez sur Ctrl+C pour quitter\n")
        
        # File d'événements tactiles
        self.touch_event_queue = Queue()
        
        # Démarrer le lecteur tactile dans un thread
        import threading
        self.touch_thread = threading.Thread(target=self._touch_reader, daemon=True)
        self.touch_thread.start()
    
    def _find_touch_device(self):
        """Trouve un périphérique tactile"""
        for path in list_devices():
            dev = InputDevice(path)
            name = dev.name.lower()
            if "touch" in name or "ft5406" in name or "touchscreen" in name:
                print(f"[Touch] Using device: {dev.name} ({path})")
                return dev
        return None
    
    def _touch_reader(self):
        """Lit les événements tactiles en arrière-plan"""
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
                    self.touch_event_queue.put(("tap", self.current_x, self.current_y))
    
    def map_to_screen(self, x_raw, y_raw, screen_width=800, screen_height=480):
        """Mappe les coordonnées brutes vers des coordonnées d'écran"""
        # Éviter la division par zéro
        dx = max(1, self.max_x - self.min_x)
        dy = max(1, self.max_y - self.min_y)
        
        # Normaliser vers [0, 1] puis étendre à la taille de l'écran
        x_norm = (x_raw - self.min_x) / dx
        y_norm = (y_raw - self.min_y) / dy
        
        # Mapper vers les coordonnées d'écran (origine en haut à gauche)
        x_screen = int(x_norm * (screen_width - 1))
        y_screen = int(y_norm * (screen_height - 1))
        
        # S'assurer que les valeurs sont dans les limites
        x_screen = max(0, min(screen_width - 1, x_screen))
        y_screen = max(0, min(screen_height - 1, y_screen))
        
        return x_screen, y_screen
    
    def run(self):
        """Boucle principale de diagnostic"""
        try:
            while self.running:
                try:
                    # Timeout court pour ne pas bloquer indéfiniment
                    event_type, x_raw, y_raw = self.touch_event_queue.get_nowait()
                    if event_type == "tap":
                        # Mapper vers des coordonnées d'écran (supposant 800x480 comme common)
                        x_screen, y_screen = self.map_to_screen(x_raw, y_raw, 800, 480)
                        
                        print(f"[TOUCH] Brut: ({x_raw:4d}, {y_raw:4d}) -> Écran: ({x_screen:3d}, {y_screen:3d})")
                        
                        # Afficher également les coordonnées relatives pour aider au calibrage
                        x_pct = (x_raw - self.min_x) / (self.max_x - self.min_x) * 100
                        y_pct = (y_raw - self.min_y) / (self.max_y - self.min_y) * 100
                        print(f"         Pourcentage: ({x_pct:5.1f}%, {y_pct:5.1f}%)")
                        
                except Empty:
                    pass  # Pas d'événement tactile, continuer
                except Exception as e:
                    print(f"[WARN] Erreur traitement tactile: {e}")
                
                time.sleep(0.05)  # Rafraîchissement fréquent pour la réactivité
        except KeyboardInterrupt:
            print("\nDiagnostic arrêté.")
        finally:
            self.running = False

def main():
    print("=== DIAGNOSTIC ÉCRAN TACTILE ===")
    diagnostic = TouchDiagnostic()
    diagnostic.run()

if __name__ == "__main__":
    main()