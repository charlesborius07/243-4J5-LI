#!/usr/bin/env python3
"""
Animation style Matrix pour terminal
Affiche des caractères qui tombent façon "code Matrix"
Supporte les écrans tactiles pour interaction
"""

import curses
import random
import time
from curses import wrapper
from touch_handler import create_touch_handler


class MatrixRain:
    def __init__(self, stdscr, use_touch=True):
        self.stdscr = stdscr
        self.height, self.width = stdscr.getmaxyx()

        # Initialiser les couleurs
        curses.start_color()
        curses.init_pair(1, curses.COLOR_GREEN, curses.COLOR_BLACK)
        curses.init_pair(2, curses.COLOR_WHITE, curses.COLOR_BLACK)
        curses.init_pair(3, curses.COLOR_CYAN, curses.COLOR_BLACK)
        curses.init_pair(4, curses.COLOR_MAGENTA, curses.COLOR_BLACK)
        curses.init_pair(5, curses.COLOR_YELLOW, curses.COLOR_BLACK)

        # Colonnes de caractères tombants
        self.columns = []
        for x in range(self.width):
            self.columns.append({
                'y': random.randint(-self.height, 0),
                'speed': random.uniform(0.5, 2.0),
                'length': random.randint(5, 20),
                'chars': []
            })

        # Jeux de caractères pour différents modes
        self.char_sets = {
            'matrix': list("ｦｱｳｴｵｶｷｸｹｺｻｼｽｾｿﾀﾁﾂﾃﾄﾅﾆﾇﾈﾉﾊﾋﾌﾍﾎﾏﾐﾑﾒﾓﾔﾕﾖﾗﾘﾙﾚﾛﾜﾝ0123456789@#$%^&*"),
            'binary': list("01"),
            'hex': list("0123456789ABCDEF"),
            'symbols': list("@#$%^&*(){}[]<>?!"),
        }

        # Palettes de couleurs
        self.color_schemes = {
            'green': (1, 2),  # vert et blanc
            'cyan': (3, 2),   # cyan et blanc
            'magenta': (4, 2),  # magenta et blanc
            'yellow': (5, 2),  # jaune et blanc
        }

        # Mode actuel
        self.char_mode = 'matrix'
        self.color_mode = 'green'
        self.chars = self.char_sets[self.char_mode]

        # Vitesse globale (multiplicateur)
        self.global_speed = 1.0

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

    def spawn_column(self, x):
        """Crée une nouvelle colonne à la position x"""
        if 0 <= x < len(self.columns):
            self.columns[x] = {
                'y': 0,
                'speed': random.uniform(1.0, 3.0),
                'length': random.randint(8, 25),
                'chars': []
            }

    def cycle_effect_mode(self):
        """Change le mode d'effet (caractères et couleurs)"""
        # Cycle de modes de caractères
        modes = list(self.char_sets.keys())
        current_idx = modes.index(self.char_mode)
        self.char_mode = modes[(current_idx + 1) % len(modes)]
        self.chars = self.char_sets[self.char_mode]

        # Cycle de couleurs
        colors = list(self.color_schemes.keys())
        current_idx = colors.index(self.color_mode)
        self.color_mode = colors[(current_idx + 1) % len(colors)]

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

            # Diviser l'écran en 3 zones horizontales
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

            # Spawn une colonne à la position touchée
            self.spawn_column(x)

    def update(self):
        """Met à jour la position des colonnes"""
        # Gérer les touches
        self.handle_touch()

        for col in self.columns:
            col['y'] += col['speed'] * 0.5 * self.global_speed

            # Réinitialiser la colonne si elle sort de l'écran
            if col['y'] > self.height + col['length']:
                col['y'] = random.randint(-20, -1)
                col['speed'] = random.uniform(0.5, 2.0)
                col['length'] = random.randint(5, 20)
                col['chars'] = []

    def draw(self):
        """Dessine l'animation"""
        self.stdscr.clear()

        # Obtenir les couleurs du schéma actuel
        main_color, bright_color = self.color_schemes[self.color_mode]

        for x, col in enumerate(self.columns):
            # Générer de nouveaux caractères si nécessaire
            while len(col['chars']) < col['length']:
                col['chars'].append(random.choice(self.chars))

            # Dessiner la traînée de caractères
            for i, char in enumerate(col['chars']):
                y = int(col['y']) - i

                if 0 <= y < self.height - 1 and 0 <= x < self.width:
                    try:
                        # Premier caractère en blanc (lumineux)
                        if i == 0:
                            self.stdscr.addstr(y, x, char, curses.color_pair(bright_color) | curses.A_BOLD)
                        else:
                            # Autres dans la couleur principale
                            self.stdscr.addstr(y, x, char, curses.color_pair(main_color))
                    except curses.error:
                        pass

        # Afficher les informations tactiles en bas
        try:
            if self.touch_handler:
                info = f"Touch: Gauche=Ralentir | Centre=Effet | Droite=Accélérer | Mode: {self.char_mode} | Vitesse: {self.global_speed:.1f}x"
            else:
                info = "Clavier: q=Quitter"

            if len(info) < self.width:
                self.stdscr.addstr(self.height - 1, 0, info, curses.A_DIM)
        except curses.error:
            pass

        self.stdscr.refresh()

    def run(self):
        """Boucle principale de l'animation"""
        try:
            while True:
                # Vérifier si l'utilisateur veut quitter (touche 'q')
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
    matrix = MatrixRain(stdscr)
    matrix.run()


if __name__ == "__main__":
    wrapper(main)
