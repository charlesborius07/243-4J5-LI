#!/usr/bin/env python3
"""
Animation de pluie arc-en-ciel pour terminal
Affiche des gouttes de pluie colorées qui tombent
Supporte les écrans tactiles pour interaction
"""

import curses
import random
import time
from curses import wrapper
from touch_handler import create_touch_handler


class RainbowRain:
    def __init__(self, stdscr, use_touch=True):
        self.stdscr = stdscr
        self.height, self.width = stdscr.getmaxyx()

        # Initialiser les couleurs
        curses.start_color()
        curses.init_pair(1, curses.COLOR_RED, curses.COLOR_BLACK)
        curses.init_pair(2, curses.COLOR_YELLOW, curses.COLOR_BLACK)
        curses.init_pair(3, curses.COLOR_GREEN, curses.COLOR_BLACK)
        curses.init_pair(4, curses.COLOR_CYAN, curses.COLOR_BLACK)
        curses.init_pair(5, curses.COLOR_BLUE, curses.COLOR_BLACK)
        curses.init_pair(6, curses.COLOR_MAGENTA, curses.COLOR_BLACK)
        curses.init_pair(7, curses.COLOR_WHITE, curses.COLOR_BLACK)

        # Liste des gouttes de pluie
        self.drops = []

        # Ensembles de caractères pour différents effets
        self.drop_char_sets = {
            'lines': ['|', '¦', '│', '║'],
            'dots': ['•', '·', '●', '○'],
            'stars': ['*', '✦', '✧', '✶'],
            'mixed': ['|', '•', '*', '~', '÷'],
        }

        # Mode de caractères actuel
        self.char_mode = 'lines'
        self.drop_chars = self.drop_char_sets[self.char_mode]

        # Palettes de couleurs
        self.color_palettes = {
            'rainbow': [1, 2, 3, 4, 5, 6],  # arc-en-ciel complet
            'warm': [1, 2],  # rouge et jaune
            'cool': [4, 5, 6],  # cyan, bleu, magenta
            'mono': [7],  # blanc seulement
        }

        self.color_mode = 'rainbow'
        self.current_colors = self.color_palettes[self.color_mode]

        # Vitesse globale
        self.global_speed = 1.0
        self.spawn_rate = 0.3  # Probabilité de spawner une goutte

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

    def add_drop(self, x=None):
        """Ajoute une nouvelle goutte de pluie"""
        if random.random() < self.spawn_rate:
            if x is None:
                x = random.randint(0, self.width - 1)

            self.drops.append({
                'x': x,
                'y': 0,
                'speed': random.uniform(1.0, 3.0),
                'color': random.choice(self.current_colors),
                'char': random.choice(self.drop_chars),
                'trail': []
            })

    def spawn_drop_at(self, x, y=0):
        """Spawne une goutte à une position spécifique"""
        if 0 <= x < self.width:
            self.drops.append({
                'x': x,
                'y': y,
                'speed': random.uniform(2.0, 4.0),  # Plus rapide pour les spawns tactiles
                'color': random.choice(self.current_colors),
                'char': random.choice(self.drop_chars),
                'trail': []
            })

    def cycle_effect_mode(self):
        """Change le mode d'effet (caractères et couleurs)"""
        # Alterner entre changement de caractères et de couleurs
        if random.random() < 0.5:
            # Changer les caractères
            modes = list(self.drop_char_sets.keys())
            current_idx = modes.index(self.char_mode)
            self.char_mode = modes[(current_idx + 1) % len(modes)]
            self.drop_chars = self.drop_char_sets[self.char_mode]
        else:
            # Changer la palette de couleurs
            palettes = list(self.color_palettes.keys())
            current_idx = palettes.index(self.color_mode)
            self.color_mode = palettes[(current_idx + 1) % len(palettes)]
            self.current_colors = self.color_palettes[self.color_mode]

    def adjust_speed(self, delta):
        """Ajuste la vitesse globale"""
        self.global_speed = max(0.1, min(5.0, self.global_speed + delta))

    def handle_touch(self):
        """Gère les événements tactiles"""
        if not self.touch_handler:
            return

        touch_pos = self.touch_handler.get_touch_position()
        if touch_pos:
            x, y = touch_pos

            # Diviser l'écran en 3 zones verticales
            zone_width = self.width // 3

            if x < zone_width:
                # Zone gauche: ralentir
                self.adjust_speed(-0.2)
            elif x >= 2 * zone_width:
                # Zone droite: accélérer
                self.adjust_speed(0.2)
            else:
                # Zone centrale: changer d'effet
                self.cycle_effect_mode()

            # Toujours spawner une goutte à la position touchée
            self.spawn_drop_at(x, y)

    def update(self):
        """Met à jour la position des gouttes"""
        # Gérer les touches
        self.handle_touch()

        self.add_drop()

        for drop in self.drops[:]:
            # Ajouter la position actuelle à la traînée
            drop['trail'].append(drop['y'])
            if len(drop['trail']) > 5:
                drop['trail'].pop(0)

            # Déplacer la goutte avec vitesse globale
            drop['y'] += drop['speed'] * 0.5 * self.global_speed

            # Retirer la goutte si elle sort de l'écran
            if drop['y'] > self.height:
                self.drops.remove(drop)

    def draw(self):
        """Dessine l'animation"""
        self.stdscr.clear()

        for drop in self.drops:
            x = drop['x']

            # Dessiner la traînée
            for i, trail_y in enumerate(drop['trail']):
                y = int(trail_y)
                if 0 <= y < self.height - 1 and 0 <= x < self.width:
                    try:
                        intensity = curses.A_NORMAL if i < len(drop['trail']) - 1 else curses.A_BOLD
                        self.stdscr.addstr(y, x, drop['char'],
                                         curses.color_pair(drop['color']) | intensity)
                    except curses.error:
                        pass

        # Afficher les instructions en bas
        try:
            if self.touch_handler:
                info = f"Touch: G=Ralentir | C=Effet | D=Accélérer | Mode: {self.char_mode}/{self.color_mode} | Vitesse: {self.global_speed:.1f}x"
            else:
                info = "Appuyez sur 'q' pour quitter"

            if len(info) < self.width:
                self.stdscr.addstr(self.height - 1, 0, info, curses.A_DIM)
        except curses.error:
            pass

        self.stdscr.refresh()

    def run(self):
        """Boucle principale de l'animation"""
        try:
            while True:
                # Vérifier si l'utilisateur veut quitter
                key = self.stdscr.getch()
                if key == ord('q') or key == ord('Q'):
                    break

                self.update()
                self.draw()
                time.sleep(0.05)  # ~20 FPS
        finally:
            # Arrêter le gestionnaire tactile
            if self.touch_handler:
                self.touch_handler.stop()


def main(stdscr):
    rain = RainbowRain(stdscr)
    rain.run()


if __name__ == "__main__":
    wrapper(main)
