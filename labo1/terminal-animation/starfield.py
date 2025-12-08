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


class Particle:
    """Particule qui explose depuis un point"""
    def __init__(self, x, y, angle, speed):
        self.x = float(x)
        self.y = float(y)
        self.vx = math.cos(angle) * speed
        self.vy = math.sin(angle) * speed
        self.life = 1.0  # 0 à 1
        self.decay = random.uniform(0.02, 0.05)
        self.char = random.choice(['*', '✦', '✧', '●', '○', '+'])

    def update(self):
        """Met à jour la particule"""
        self.x += self.vx
        self.y += self.vy
        self.life -= self.decay
        # Friction
        self.vx *= 0.95
        self.vy *= 0.95

    def is_alive(self):
        return self.life > 0


class Shockwave:
    """Onde de choc circulaire qui s'agrandit"""
    def __init__(self, x, y):
        self.x = x
        self.y = y
        self.radius = 0.0
        self.max_radius = random.uniform(10, 20)
        self.thickness = 2
        self.life = 1.0
        self.speed = random.uniform(0.5, 1.0)

    def update(self):
        """Met à jour l'onde"""
        self.radius += self.speed
        self.life = 1.0 - (self.radius / self.max_radius)

    def is_alive(self):
        return self.radius < self.max_radius

    def get_points(self):
        """Retourne les points du cercle"""
        points = []
        # Calculer les points sur le cercle
        num_points = int(self.radius * 6)  # Plus de points pour les grands cercles
        for i in range(num_points):
            angle = (i / num_points) * 2 * math.pi
            px = int(self.x + math.cos(angle) * self.radius)
            py = int(self.y + math.sin(angle) * self.radius)
            points.append((px, py))
        return points


class Trail:
    """Traînée lumineuse qui suit le doigt"""
    def __init__(self, x, y):
        self.points = [(x, y)]
        self.max_length = 10
        self.life = 1.0

    def add_point(self, x, y):
        """Ajoute un point à la traînée"""
        self.points.append((x, y))
        if len(self.points) > self.max_length:
            self.points.pop(0)
        self.life = 1.0

    def update(self):
        """Met à jour la traînée"""
        self.life -= 0.05
        if self.life < 0.5 and len(self.points) > 0:
            self.points.pop(0)

    def is_alive(self):
        return len(self.points) > 0


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
        curses.init_pair(5, curses.COLOR_MAGENTA, curses.COLOR_BLACK)
        curses.init_pair(6, curses.COLOR_GREEN, curses.COLOR_BLACK)

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

        # Effets visuels
        self.particles = []
        self.shockwaves = []
        self.trails = []
        self.last_touch_positions = {}  # {touch_id: (x, y)} pour multi-touch
        self.touch_combo = 0
        self.last_touch_time = 0

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

    def create_particle_explosion(self, x, y, count=20):
        """Crée une explosion de particules"""
        for i in range(count):
            angle = (i / count) * 2 * math.pi
            speed = random.uniform(0.5, 2.0)
            self.particles.append(Particle(x, y, angle, speed))

    def create_shockwave(self, x, y):
        """Crée une onde de choc"""
        self.shockwaves.append(Shockwave(x, y))

    def handle_touch(self):
        """Gère les événements tactiles avec support multi-touch"""
        if not self.touch_handler:
            return

        # Vérifier s'il y a un nouveau touch (touch down) - pour les combos et zones
        new_touch_pos = self.touch_handler.get_touch_position()
        if new_touch_pos:
            x, y = new_touch_pos
            current_time = time.time()

            # Détection de combo (touches rapides)
            if current_time - self.last_touch_time < 0.3:
                self.touch_combo += 1
            else:
                self.touch_combo = 1
            self.last_touch_time = current_time

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

        # MULTI-TOUCH: Gérer tous les doigts actuellement sur l'écran
        all_touches = self.touch_handler.get_all_active_touches()
        touch_count = len(all_touches)

        if touch_count > 0:
            # Effets spéciaux selon le nombre de doigts
            for touch_id, (x, y) in enumerate(all_touches):
                # Vérifier si c'est un nouveau touch ou s'il a bougé
                is_new_touch = touch_id not in self.last_touch_positions

                if is_new_touch:
                    # NOUVEAU TOUCH - créer explosion et onde
                    particle_count = min(20 + self.touch_combo * 10, 50)
                    self.create_particle_explosion(x, y, particle_count)

                    # Ondes de choc (plus si plusieurs doigts)
                    num_waves = min(1 + touch_count, 3)
                    for i in range(num_waves):
                        self.shockwaves.append(Shockwave(x, y))
                        if i > 0:
                            self.shockwaves[-1].speed += i * 0.3

                    # Spawner des étoiles
                    star_count = min(5 + self.touch_combo * 2 + touch_count * 3, 20)
                    for _ in range(star_count):
                        self.spawn_star_at(x, y)

                    # Créer une nouvelle traînée pour ce doigt
                    self.trails.append(Trail(x, y))

                else:
                    # DOIGT EXISTANT - mettre à jour la traînée
                    last_pos = self.last_touch_positions[touch_id]
                    dx = x - last_pos[0]
                    dy = y - last_pos[1]
                    distance = math.sqrt(dx*dx + dy*dy)

                    # Si le doigt a bougé
                    if distance > 0.5:
                        # Trouver la traînée la plus récente pour ce doigt
                        # (approximation: on prend la dernière traînée vivante)
                        active_trails = [t for t in self.trails if t.is_alive()]
                        if len(active_trails) >= touch_count and touch_id < len(active_trails):
                            trail_idx = -(touch_count - touch_id)
                            self.trails[trail_idx].add_point(x, y)
                        elif len(active_trails) > 0:
                            active_trails[-1].add_point(x, y)
                        else:
                            self.trails.append(Trail(x, y))

                        # Ajouter des particules le long du mouvement
                        if distance > 2:
                            num_particles = int(distance / 2)
                            for i in range(num_particles):
                                t = i / num_particles
                                px = last_pos[0] + dx * t
                                py = last_pos[1] + dy * t
                                angle = random.uniform(0, 2 * math.pi)
                                # Couleur différente par doigt
                                self.particles.append(Particle(px, py, angle, random.uniform(0.3, 0.8)))

                # Mettre à jour la dernière position
                self.last_touch_positions[touch_id] = (x, y)

            # Effets spéciaux multi-doigts
            if touch_count >= 2:
                # Créer des particules entre les doigts
                for i in range(len(all_touches)):
                    for j in range(i+1, len(all_touches)):
                        x1, y1 = all_touches[i]
                        x2, y2 = all_touches[j]
                        # Particules au milieu
                        mx, my = (x1 + x2) / 2, (y1 + y2) / 2
                        if random.random() < 0.3:  # 30% de chance
                            angle = random.uniform(0, 2 * math.pi)
                            self.particles.append(Particle(mx, my, angle, random.uniform(0.2, 0.6)))

        else:
            # Plus de doigts sur l'écran - nettoyer
            self.last_touch_positions.clear()

    def update(self):
        """Met à jour toutes les étoiles et effets"""
        # Gérer les touches
        self.handle_touch()

        # Mettre à jour les étoiles
        for star in self.stars:
            star.update(self.speed)

        # Mettre à jour les particules
        for particle in self.particles[:]:
            particle.update()
            if not particle.is_alive():
                self.particles.remove(particle)

        # Mettre à jour les ondes de choc
        for wave in self.shockwaves[:]:
            wave.update()
            if not wave.is_alive():
                self.shockwaves.remove(wave)

        # Mettre à jour les traînées
        for trail in self.trails[:]:
            trail.update()
            if not trail.is_alive():
                self.trails.remove(trail)

    def draw(self):
        """Dessine l'animation"""
        self.stdscr.clear()

        # 1. Dessiner les étoiles (background)
        for star in self.stars:
            sx, sy = star.get_screen_pos()
            size = star.get_size()

            # Vérifier que la position est dans l'écran
            if 0 <= sx < self.width and 0 <= sy < self.height - 1:
                try:
                    # Choisir le caractère selon la taille/distance
                    char_idx = min(size, len(self.star_chars) - 1)
                    char = self.star_chars[char_idx]

                    # Choisir la couleur selon la distance
                    color_idx = min(size, len(self.current_colors) - 1)
                    color_pair, attr = self.current_colors[color_idx]

                    self.stdscr.addstr(sy, sx, char, curses.color_pair(color_pair) | attr)
                except curses.error:
                    pass

        # 2. Dessiner les traînées
        for trail in self.trails:
            for i, (tx, ty) in enumerate(trail.points):
                if 0 <= tx < self.width and 0 <= ty < self.height - 1:
                    try:
                        # Intensité selon la position dans la traînée
                        intensity = curses.A_BOLD if i == len(trail.points) - 1 else curses.A_NORMAL
                        char = '●' if i == len(trail.points) - 1 else '○'
                        # Couleur qui change le long de la traînée
                        color = 2 + (i % 4)
                        self.stdscr.addstr(ty, tx, char, curses.color_pair(color) | intensity)
                    except curses.error:
                        pass

        # 3. Dessiner les ondes de choc
        for wave in self.shockwaves:
            points = wave.get_points()
            for px, py in points:
                if 0 <= px < self.width and 0 <= py < self.height - 1:
                    try:
                        # Intensité basée sur la vie de l'onde
                        intensity = curses.A_BOLD if wave.life > 0.7 else curses.A_NORMAL
                        char = '◉' if wave.life > 0.7 else '○'
                        # Couleur cyclique pour effet arc-en-ciel
                        color = 1 + int((wave.radius * 0.5) % 6)
                        self.stdscr.addstr(py, px, char, curses.color_pair(color) | intensity)
                    except curses.error:
                        pass

        # 4. Dessiner les particules
        for particle in self.particles:
            px, py = int(particle.x), int(particle.y)
            if 0 <= px < self.width and 0 <= py < self.height - 1:
                try:
                    # Intensité basée sur la vie
                    if particle.life > 0.7:
                        intensity = curses.A_BOLD
                        color = 2  # Jaune brillant
                    elif particle.life > 0.4:
                        intensity = curses.A_NORMAL
                        color = 1  # Blanc
                    else:
                        intensity = curses.A_DIM
                        color = 1

                    self.stdscr.addstr(py, px, particle.char, curses.color_pair(color) | intensity)
                except curses.error:
                    pass

        # 5. Afficher les informations tactiles en bas
        try:
            if self.touch_handler:
                touch_count = self.touch_handler.get_touch_count()
                combo_text = f" COMBO x{self.touch_combo}!" if self.touch_combo > 1 else ""
                fingers_text = f" | {touch_count} doigt{'s' if touch_count > 1 else ''}" if touch_count > 0 else ""
                info = f"Touch: G=Ralentir | C=Effet | D=Accélérer | Mode: {self.char_mode}/{self.color_mode} | Vitesse: {self.speed:.1f}{combo_text}{fingers_text}"
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
    starfield = Starfield(stdscr)
    starfield.run()


if __name__ == "__main__":
    wrapper(main)
