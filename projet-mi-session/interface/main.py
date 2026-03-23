import pygame
import sys
import json
import paho.mqtt.client as mqtt

# --- Configuration ---
MQTT_BROKER = "localhost" # Remplacer par l'IP du Raspberry Pi si broker distant
MQTT_PORT = 1883
TOPIC_ROOT = "etudiant/prenom-nom/" # À configurer selon votre projet

# --- Couleurs ---
BG_COLOR = (30, 30, 30)
TEXT_COLOR = (255, 255, 255)
GREEN = (76, 175, 80)
RED = (244, 67, 54)
BLUE = (33, 150, 243)
ORANGE = (255, 152, 0)
GRAY = (100, 100, 100)

class SensorData:
    """Structure de données pour les capteurs."""
    def __init__(self):
        self.btn1 = False
        self.pot1 = 0
        self.pot2 = 0
        self.accel = {"x": 0.0, "y": 0.0, "z": 0.0, "roll": 0.0, "pitch": 0.0}

class SensorDisplay:
    """Affichage des données capteurs."""
    def __init__(self, font, small_font):
        self.font = font
        self.small_font = small_font

    def update(self, data: dict) -> None:
        """Méthode de compatibilité demandée."""
        pass

    def draw(self, surface, data, start_x, start_y):
        # Titre
        title = self.font.render("Capteurs", True, TEXT_COLOR)
        surface.blit(title, (start_x, start_y))
        
        y = start_y + 40
        
        # Bouton
        btn_text = self.small_font.render("Bouton 1:", True, TEXT_COLOR)
        surface.blit(btn_text, (start_x, y))
        btn_color = GREEN if data.btn1 else RED
        pygame.draw.circle(surface, btn_color, (start_x + 120, y + 10), 12)
        
        y += 40
        
        # Potentiomètres (Barres de progression 0-4095)
        self._draw_progress_bar(surface, "Pot 1", data.pot1, 4095, start_x, y)
        y += 40
        self._draw_progress_bar(surface, "Pot 2", data.pot2, 4095, start_x, y)
        y += 50
        
        # Accéléromètre
        accel_title = self.small_font.render("Accéléromètre & Gyro:", True, TEXT_COLOR)
        surface.blit(accel_title, (start_x, y))
        y += 30
        
        # X, Y, Z
        xyz_text = self.small_font.render(f"X: {data.accel['x']:.2f}  Y: {data.accel['y']:.2f}  Z: {data.accel['z']:.2f}", True, TEXT_COLOR)
        surface.blit(xyz_text, (start_x, y))
        y += 30
        
        # Jauges pour Roll / Pitch (-90 à 90)
        self._draw_gauge(surface, "Roll", data.accel['roll'], -90, 90, start_x, y)
        y += 40
        self._draw_gauge(surface, "Pitch", data.accel['pitch'], -90, 90, start_x, y)

    def _draw_progress_bar(self, surface, label, value, max_val, x, y):
        text = self.small_font.render(f"{label}: {value}", True, TEXT_COLOR)
        surface.blit(text, (x, y))
        
        bar_x = x + 100
        bar_w = 200
        bar_h = 20
        
        pygame.draw.rect(surface, GRAY, (bar_x, y, bar_w, bar_h))
        fill_w = int((value / max_val) * bar_w)
        fill_w = max(0, min(bar_w, fill_w))
        if fill_w > 0:
            pygame.draw.rect(surface, BLUE, (bar_x, y, fill_w, bar_h))

    def _draw_gauge(self, surface, label, value, min_val, max_val, x, y):
        text = self.small_font.render(f"{label}: {value:.1f}°", True, TEXT_COLOR)
        surface.blit(text, (x, y))
        
        bar_x = x + 100
        bar_w = 200
        bar_h = 20
        
        pygame.draw.rect(surface, GRAY, (bar_x, y, bar_w, bar_h))
        
        # Centre de la jauge
        center_x = bar_x + bar_w // 2
        pygame.draw.line(surface, TEXT_COLOR, (center_x, y), (center_x, y + bar_h), 2)
        
        # Remplissage
        val_range = max_val - min_val
        val_norm = (value - min_val) / val_range
        val_norm = max(0.0, min(1.0, val_norm))
        
        dot_x = bar_x + int(val_norm * bar_w)
        pygame.draw.circle(surface, ORANGE, (dot_x, y + bar_h // 2), 8)

class LEDControl:
    """Contrôle des LEDs via MQTT."""
    def __init__(self, font, mqtt_client):
        self.font = font
        self.mqtt_client = mqtt_client
        self.states = [False, False, False, False]
        self.buttons = [] # Store rects for collision

    def draw(self, surface, start_x, start_y):
        title = self.font.render("Contrôle LEDs", True, TEXT_COLOR)
        surface.blit(title, (start_x, start_y))
        
        self.buttons.clear()
        y = start_y + 50
        
        # Dessiner 4 boutons (2x2 grid, zones > 44x44 px)
        for i in range(4):
            col = i % 2
            row = i // 2
            
            bx = start_x + col * 160
            by = y + row * 100
            
            rect = pygame.Rect(bx, by, 140, 80)
            self.buttons.append((rect, i))
            
            color = GREEN if self.states[i] else GRAY
            pygame.draw.rect(surface, color, rect, border_radius=10)
            
            # Texte centré
            text = self.font.render(f"LED {i+1}", True, TEXT_COLOR)
            text_rect = text.get_rect(center=rect.center)
            surface.blit(text, text_rect)

    def toggle(self, led_id: int) -> None:
        if 0 <= led_id < 4:
            self.states[led_id] = not self.states[led_id]
            state_str = "on" if self.states[led_id] else "off"
            topic = f"{TOPIC_ROOT}actuators/led{led_id+1}"
            payload = json.dumps({"state": state_str})
            if self.mqtt_client:
                self.mqtt_client.publish(topic, payload)

    def handle_click(self, pos):
        for rect, led_id in self.buttons:
            if rect.collidepoint(pos):
                self.toggle(led_id)
                return True
        return False

class App:
    """Application principale."""
    def __init__(self):
        pygame.init()
        # Option plein écran disponible : décommenter pygame.FULLSCREEN
        self.screen_width = 800
        self.screen_height = 480
        # self.screen = pygame.display.set_mode((self.screen_width, self.screen_height), pygame.FULLSCREEN)
        self.screen = pygame.display.set_mode((self.screen_width, self.screen_height))
        pygame.display.set_caption("Interface IoT")
        
        self.clock = pygame.time.Clock()
        self.font = pygame.font.SysFont(None, 32)
        self.small_font = pygame.font.SysFont(None, 24)
        
        self.sensor_data = SensorData()
        self.sensor_display = SensorDisplay(self.font, self.small_font)
        
        self.mqtt_client = mqtt.Client()
        self.mqtt_client.on_connect = self.on_mqtt_connect
        self.mqtt_client.on_disconnect = self.on_mqtt_disconnect
        self.mqtt_client.on_message = self.on_mqtt_message
        
        self.led_control = LEDControl(self.font, self.mqtt_client)
        self.mqtt_connected = False
        
        self.running = True

    def on_mqtt_connect(self, client, userdata, flags, rc):
        if rc == 0:
            self.mqtt_connected = True
            client.subscribe(f"{TOPIC_ROOT}sensors/#")
            
    def on_mqtt_disconnect(self, client, userdata, rc):
        self.mqtt_connected = False
        
    def on_mqtt_message(self, client, userdata, msg):
        try:
            payload = json.loads(msg.payload.decode())
            if msg.topic.endswith("sensors/buttons"):
                if "btn1" in payload:
                    self.sensor_data.btn1 = payload["btn1"]
            elif msg.topic.endswith("sensors/pots"):
                if "pot1" in payload: self.sensor_data.pot1 = payload["pot1"]
                if "pot2" in payload: self.sensor_data.pot2 = payload["pot2"]
            elif msg.topic.endswith("sensors/accel"):
                for k in ["x", "y", "z", "roll", "pitch"]:
                    if k in payload:
                        self.sensor_data.accel[k] = payload[k]
        except Exception as e:
            print(f"Erreur décodage JSON: {e}")

    def run(self) -> None:
        try:
            self.mqtt_client.connect(MQTT_BROKER, MQTT_PORT, 60)
            self.mqtt_client.loop_start()
        except Exception as e:
            print(f"Erreur de connexion MQTT: {e}")
            
        while self.running:
            for event in pygame.event.get():
                if event.type == pygame.QUIT:
                    self.running = False
                elif event.type == pygame.KEYDOWN:
                    if event.key == pygame.K_ESCAPE or event.key == pygame.K_q:
                        self.running = False
                elif event.type == pygame.MOUSEBUTTONDOWN:
                    if event.button == 1: # Clic principal / Touch
                        self.led_control.handle_click(event.pos)
                        
            self.draw()
            pygame.display.flip()
            self.clock.tick(30) # 30 FPS maximum
            
        self.mqtt_client.loop_stop()
        self.mqtt_client.disconnect()
        pygame.quit()
        sys.exit()

    def draw(self):
        self.screen.fill(BG_COLOR)
        
        # En-tête
        title = self.font.render("Interface IoT - Projet Mi-Session", True, TEXT_COLOR)
        self.screen.blit(title, (20, 20))
        
        # Statut MQTT
        status_text = "Connecté" if self.mqtt_connected else "Déconnecté"
        status_color = GREEN if self.mqtt_connected else RED
        status_surf = self.font.render(f"MQTT: {status_text}", True, status_color)
        self.screen.blit(status_surf, (self.screen_width - status_surf.get_width() - 20, 20))
        
        # Ligne de séparation
        pygame.draw.line(self.screen, GRAY, (20, 60), (self.screen_width - 20, 60), 2)
        
        # Zones principales
        self.sensor_display.draw(self.screen, self.sensor_data, 30, 90)
        self.led_control.draw(self.screen, 450, 90)
        
        # Indication Quitter
        quit_text = self.small_font.render("Appuyez sur 'q' ou ESC pour quitter", True, GRAY)
        self.screen.blit(quit_text, (20, self.screen_height - 30))

if __name__ == "__main__":
    app = App()
    app.run()
