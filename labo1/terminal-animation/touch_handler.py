#!/usr/bin/env python3
"""
Module de gestion des événements tactiles pour les animations terminal
Supporte les écrans tactiles connectés au Raspberry Pi
"""

import os
import select
import threading
from typing import Optional, Tuple, List


class TouchHandler:
    """Gestionnaire d'événements tactiles pour écran tactile"""

    def __init__(self, screen_width: int, screen_height: int):
        """
        Initialise le gestionnaire tactile

        Args:
            screen_width: Largeur du terminal en caractères
            screen_height: Hauteur du terminal en caractères
        """
        self.screen_width = screen_width
        self.screen_height = screen_height
        self.touch_device = None
        self.running = False
        self.thread = None

        # État tactile actuel (ancien système - un seul touch)
        self.current_x = 0
        self.current_y = 0
        self.touch_active = False
        self.last_touch = None  # (x, y) en coordonnées terminal

        # Multi-touch support
        self.current_slot = 0
        self.touches = {}  # {slot: {'x': x, 'y': y, 'tracking_id': id}}
        self.new_touches = []  # Liste des nouvelles touches à consommer

        # Paramètres de calibration (seront ajustés selon l'écran)
        self.touch_max_x = 4095  # Valeur max typique pour écran tactile
        self.touch_max_y = 4095

        # Tenter d'initialiser evdev si disponible
        self.evdev_available = False
        try:
            import evdev
            self.evdev = evdev
            self.evdev_available = True
            self._find_touch_device()
        except ImportError:
            print("Module evdev non disponible. Installez avec: sudo apt-get install python3-evdev")

    def _find_touch_device(self):
        """Trouve automatiquement le périphérique tactile"""
        if not self.evdev_available:
            return

        devices = [self.evdev.InputDevice(path) for path in self.evdev.list_devices()]

        # Chercher un périphérique avec capacités tactiles
        for device in devices:
            caps = device.capabilities(verbose=False)
            # Vérifier si le périphérique supporte les événements absolus (écran tactile)
            if 3 in caps:  # EV_ABS
                abs_events = caps[3]
                # Vérifier présence de ABS_X et ABS_Y
                if any(code in [0, 1] for code, _ in abs_events):  # ABS_X=0, ABS_Y=1
                    self.touch_device = device
                    print(f"Périphérique tactile détecté: {device.name}")

                    # Calibrer selon les valeurs max du périphérique
                    for code, absinfo in abs_events:
                        if code == 0:  # ABS_X
                            self.touch_max_x = absinfo.max
                        elif code == 1:  # ABS_Y
                            self.touch_max_y = absinfo.max

                    return

        print("Aucun périphérique tactile trouvé")

    def _touch_to_terminal_coords(self, touch_x: int, touch_y: int) -> Tuple[int, int]:
        """
        Convertit les coordonnées tactiles brutes en coordonnées terminal

        Args:
            touch_x: Position X brute du touch
            touch_y: Position Y brute du touch

        Returns:
            Tuple (x, y) en coordonnées de caractères terminal
        """
        # Normaliser les coordonnées (0.0 à 1.0)
        norm_x = touch_x / self.touch_max_x
        norm_y = touch_y / self.touch_max_y

        # Convertir en coordonnées terminal
        term_x = int(norm_x * self.screen_width)
        term_y = int(norm_y * self.screen_height)

        # Limiter aux bornes
        term_x = max(0, min(term_x, self.screen_width - 1))
        term_y = max(0, min(term_y, self.screen_height - 1))

        return term_x, term_y

    def _read_touch_events(self):
        """Thread de lecture des événements tactiles (evdev)"""
        if not self.touch_device:
            return

        try:
            for event in self.touch_device.read_loop():
                if not self.running:
                    break

                # EV_ABS = 3, événements de position absolue
                if event.type == 3:
                    # Ancien système (un seul touch)
                    if event.code == 0:  # ABS_X
                        self.current_x = event.value
                    elif event.code == 1:  # ABS_Y
                        self.current_y = event.value

                    # Multi-touch
                    elif event.code == 47:  # ABS_MT_SLOT
                        self.current_slot = event.value
                    elif event.code == 57:  # ABS_MT_TRACKING_ID
                        if event.value == -1:  # Touch terminé
                            if self.current_slot in self.touches:
                                del self.touches[self.current_slot]
                        else:  # Nouveau touch
                            if self.current_slot not in self.touches:
                                self.touches[self.current_slot] = {
                                    'x': 0, 'y': 0, 'tracking_id': event.value
                                }
                    elif event.code == 53:  # ABS_MT_POSITION_X
                        if self.current_slot in self.touches:
                            self.touches[self.current_slot]['x'] = event.value
                    elif event.code == 54:  # ABS_MT_POSITION_Y
                        if self.current_slot in self.touches:
                            self.touches[self.current_slot]['y'] = event.value

                # EV_KEY = 1, événements de bouton (touch down/up)
                elif event.type == 1:
                    if event.code == 330:  # BTN_TOUCH
                        if event.value == 1:  # Touch down
                            self.touch_active = True
                            self.last_touch = self._touch_to_terminal_coords(
                                self.current_x, self.current_y
                            )
                            # Aussi ajouter aux nouvelles touches
                            self.new_touches.append(self.last_touch)
                        elif event.value == 0:  # Touch up
                            self.touch_active = False

                # EV_SYN = 0, événement de synchronisation (fin d'une frame)
                elif event.type == 0 and event.code == 0:  # SYN_REPORT
                    # Une frame complète a été reçue, on peut traiter
                    pass

        except OSError:
            pass  # Périphérique déconnecté

    def start(self):
        """Démarre la lecture des événements tactiles en arrière-plan"""
        if not self.evdev_available or not self.touch_device:
            return False

        self.running = True
        self.thread = threading.Thread(target=self._read_touch_events, daemon=True)
        self.thread.start()
        return True

    def stop(self):
        """Arrête la lecture des événements tactiles"""
        self.running = False
        if self.thread:
            self.thread.join(timeout=1.0)

    def get_touch_position(self) -> Optional[Tuple[int, int]]:
        """
        Récupère la dernière position touchée et la consomme

        Returns:
            Tuple (x, y) en coordonnées terminal, ou None si pas de nouveau touch
        """
        if self.last_touch:
            touch = self.last_touch
            self.last_touch = None
            return touch
        return None

    def is_touch_active(self) -> bool:
        """Retourne True si l'écran est actuellement touché"""
        return self.touch_active

    def get_current_position(self) -> Tuple[int, int]:
        """
        Récupère la position actuelle du touch (sans consommer)

        Returns:
            Tuple (x, y) en coordonnées terminal
        """
        return self._touch_to_terminal_coords(self.current_x, self.current_y)

    def get_all_active_touches(self) -> List[Tuple[int, int]]:
        """
        Récupère toutes les positions tactiles actuellement actives

        Returns:
            Liste de tuples (x, y) en coordonnées terminal
        """
        positions = []
        for slot, touch in self.touches.items():
            pos = self._touch_to_terminal_coords(touch['x'], touch['y'])
            positions.append(pos)
        return positions

    def get_touch_count(self) -> int:
        """
        Retourne le nombre de touches actuellement actives

        Returns:
            Nombre de doigts sur l'écran
        """
        return len(self.touches)


class MockTouchHandler(TouchHandler):
    """Version mock du gestionnaire tactile pour tests sans écran tactile"""

    def __init__(self, screen_width: int, screen_height: int):
        super().__init__(screen_width, screen_height)
        self.evdev_available = False
        print("Mode mock: utilisez le clavier pour simuler le touch")
        print("  Touches numériques 1-9: touch dans différentes zones")
        print("  Espace: touch aléatoire")

    def simulate_touch(self, x: int, y: int):
        """Simule un événement tactile à la position spécifiée"""
        self.last_touch = (x, y)

    def start(self):
        return True


def create_touch_handler(screen_width: int, screen_height: int,
                         mock: bool = False) -> TouchHandler:
    """
    Factory pour créer un gestionnaire tactile approprié

    Args:
        screen_width: Largeur du terminal
        screen_height: Hauteur du terminal
        mock: Si True, utilise la version mock pour les tests

    Returns:
        Instance de TouchHandler ou MockTouchHandler
    """
    if mock:
        return MockTouchHandler(screen_width, screen_height)
    else:
        return TouchHandler(screen_width, screen_height)
