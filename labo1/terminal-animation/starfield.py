#!/usr/bin/env python3
"""
Animation de champ d'étoiles (starfield) pour terminal
Simule un voyage dans l'espace avec des étoiles qui défilent
Supporte les écrans tactiles pour interaction
"""

import curses
import random
import time
import math
from curses import wrapper
from touch_handler import create_touch_handler


class Star:
    def __init__(self, width, height, x=None, y=None):
        self.reset(width, height, randomize_z=True, x=x, y=y)

    def reset(self, width, height, randomize_z=False, x=None, y=None):
        """Réinitialise l'étoile au centre"""
        # Position 3D
        if x is not None and y is not None:
            # Position spécifique pour spawn tactile
            self.x = x - width // 2
            self.y = y - height // 2
        else:
            # Position aléatoire
            self.x = random.uniform(-width, width)
            self.y = random.uniform(-height, height)

        self.z = random.uniform(1, width) if randomize_z else width

        self.width = width
        self.height = height

    def update(self, speed):
        """Met à jour la position de l'étoile"""
        self.z -= speed

        # Réinitialiser si l'étoile est trop proche
        if self.z <= 0:
            self.reset(self.width, self.height)

    def get_screen_pos(self):
        """Calcule la position 2D sur l'écran"""
        # Projection perspective
        k = 128 / self.z
        sx = int(self.x * k + self.width // 2)
        sy = int(self.y * k + self.height // 2)

        return sx, sy

    def get_size(self):
        """Calcule la taille de l'étoile basée sur la distance"""
        return int((1 - self.z / self.width) * 3)


class Starfield:
    def __init__(self, stdscr, use_touch=True):
        self.stdscr = stdscr
        self.height, self.width = stdscr.getmaxyx()

        # Initialiser les couleurs
        curses.start_color()
        curses.init_pair(1, curses.COLOR_WHITE, curses.COLOR_BLACK)
        curses.init_pair(2, curses.COLOR_YELLOW, curses.COLOR_BLACK)
        curses.init_pair(3, curses.COLOR_CYAN, curses.COLOR_BLACK)
        curses.init_pair(4, curses.COLOR_RED, curses.COLOR_BLACK)
        curses.init_pair(5, curses.COLOR_GREEN, curses.COLOR_BLACK)
        curses.init_pair(6, curses.COLOR_MAGENTA, curses.COLOR_BLACK)

        # Créer les étoiles
        self.star_count = 200
        self.stars = [Star(self.width, self.height) for _ in range(self.star_count)]

        # Vitesse de déplacement
        self.speed = 2.0

        # Modes de caractères
        self.char_sets = {
            'classic': ['.', '*', '✦', '✧', '★'],
            'dots': ['.', '·', '•', '●', '⬤'],
            'plus': ['.', '+', '✦', '✚', '✛'],
            'mixed': ['.', '*', '+', '○', '◉'],
        }

        # Palettes de couleurs
        self.color_schemes = {
            'white': [(1, curses.A_NORMAL), (1, curses.A_BOLD), (2, curses.A_BOLD)],
            'rainbow': [(1, curses.A_BOLD), (2, curses.A_BOLD), (3, curses.A_BOLD),
                       (4, curses.A_BOLD), (5, curses.A_BOLD), (6, curses.A_BOLD)],
            'warm': [(2, curses.A_NORMAL), (2, curses.A_BOLD), (4, curses.A_BOLD)],
            'cool': [(3, curses.A_NORMAL), (3, curses.A_BOLD), (1, curses.A_BOLD)],
        }

        # Modes actuels
        self.char_mode = 'classic'
        self.color_mode = 'white'
        self.star_chars = self.char_sets[self.char_mode]
        self.current_colors = self.color_schemes[self.color_mode]

        # Gestionnaire tactile
        self.touch_handler = None
        if use_touch:
            self.touch_handler = create_touch_handler(self.width, self.height)
            if self.touch_handler.start():
                print("Support tactile activé!")
            else:
                self.touch_handler = None

        # Masquer le curseur
        curses.curs_set(0)
        stdscr.nodelay(1)
        stdscr.clear()

    def spawn_star_at(self, x, y):
        """Spawne une nouvelle étoile à une position spécifique"""
        # Remplacer une étoile aléatoire
        idx = random.randint(0, len(self.stars) - 1)
        self.stars[idx] = Star(self.width, self.height, x, y)

    def cycle_effect_mode(self):
        """Change le mode d'effet (caractères et couleurs)"""
        if random.random() < 0.5:
            # Changer les caractères
            modes = list(self.char_sets.keys())
            current_idx = modes.index(self.char_mode)
            self.char_mode = modes[(current_idx + 1) % len(modes)]
            self.star_chars = self.char_sets[self.char_mode]
        else:
            # Changer la palette de couleurs
            palettes = list(self.color_schemes.keys())
            current_idx = palettes.index(self.color_mode)
            self.color_mode = palettes[(current_idx + 1) % len(palettes)]
            self.current_colors = self.color_schemes[self.color_mode]

    def adjust_speed(self, delta):
        """Ajuste la vitesse"""
        self.speed = max(0.5, min(10.0, self.speed + delta))

    def handle_touch(self):
        """Gère les événements tactiles"""
        if not self.touch_handler:
            return

        touch_pos = self.touch_handler.get_touch_position()
        if touch_pos:
            x, y = touch_pos

            # Diviser l'écran en 3 zones
            zone_width = self.width // 3

            if x < zone_width:
                # Zone gauche: ralentir
                self.adjust_speed(-0.5)
            elif x >= 2 * zone_width:
                # Zone droite: accélérer
                self.adjust_speed(0.5)
            else:
                # Zone centrale: changer d'effet
                self.cycle_effect_mode()

            # Spawner plusieurs étoiles à la position touchée
            for _ in range(5):
                self.spawn_star_at(x, y)

    def update(self):
        """Met à jour toutes les étoiles"""
        # Gérer les touches
        self.handle_touch()

        for star in self.stars:
            star.update(self.speed)

    def draw(self):
        """Dessine l'animation"""
        self.stdscr.clear()

        for star in self.stars:
            sx, sy = star.get_screen_pos()

            # Vérifier si l'étoile est visible
            if 0 <= sx < self.width and 0 <= sy < self.height - 1:
                try:
                    size = star.get_size()

                    # Choisir le caractère selon la taille
                    if size <= 0:
                        char_idx = 0
                    elif size == 1:
                        char_idx = 1
                    else:
                        char_idx = min(size + 1, len(self.star_chars) - 1)

                    char = self.star_chars[char_idx]

                    # Choisir la couleur et l'intensité depuis le schéma actuel
                    color_idx = min(size, len(self.current_colors) - 1)
                    color_pair, intensity = self.current_colors[color_idx]
                    color = curses.color_pair(color_pair) | intensity

                    self.stdscr.addstr(sy, sx, char, color)

                except curses.error:
                    pass

        # Afficher les instructions
        try:
            if self.touch_handler:
                info = f"Touch: G=Ralentir | C=Effet | D=Accélérer | Mode: {self.char_mode}/{self.color_mode} | Vitesse: {self.speed:.1f}"
            else:
                info = "↑/↓: Vitesse | q: Quitter | Vitesse: {:.1f}".format(self.speed)

            if len(info) < self.width:
                self.stdscr.addstr(self.height - 1, 0, info, curses.A_DIM)
        except curses.error:
            pass

        self.stdscr.refresh()

    def run(self):
        """Boucle principale de l'animation"""
        try:
            while True:
                # Vérifier les touches
                key = self.stdscr.getch()
                if key == ord('q') or key == ord('Q'):
                    break
                elif key == curses.KEY_UP:
                    self.adjust_speed(0.5)
                elif key == curses.KEY_DOWN:
                    self.adjust_speed(-0.5)

                self.update()
                self.draw()
                time.sleep(0.05)  # ~20 FPS
        finally:
            # Arrêter le gestionnaire tactile
            if self.touch_handler:
                self.touch_handler.stop()


def main(stdscr):
    starfield = Starfield(stdscr)
    starfield.run()


if __name__ == "__main__":
    wrapper(main)
