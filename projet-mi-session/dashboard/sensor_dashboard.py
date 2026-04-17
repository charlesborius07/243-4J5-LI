#!/usr/bin/env python3
"""
Tableau de bord interactif pour afficher les données des capteurs en temps réel
et contrôler les LEDs via écran tactile
"""

import json
import time
import sys
import os
import threading
import paho.mqtt.client as mqtt
import ssl
from queue import Queue, Empty

# Import de la config MQTT
sys.path.append(os.path.abspath(os.path.join(os.path.dirname(__file__), '..', 'interface')))
from mqtt_config import MQTT_CONFIG

# Classe pour lire les événements tactiles (adaptée depuis game_interface.py)
class TouchReader(threading.Thread):
    def __init__(self, event_queue: Queue):
        super().__init__(daemon=True)
        self.event_queue = event_queue
        self.device = self._find_touch_device()
        # Calibrage simplifié - à ajuster selon votre écran
        self.min_x, self.max_x = 0, 4095
        self.min_y, self.max_y = 0, 4095
        self.current_x, self.current_y = 2048, 2048

    def _find_touch_device(self):
        try:
            from evdev import list_devices, InputDevice, ecodes
            for path in list_devices():
                dev = InputDevice(path)
                if "touch" in dev.name.lower() or "ft5406" in dev.name.lower() or "touchscreen" in dev.name.lower():
                    return dev
            return None
        except ImportError:
            print("[WARN] evdev non disponible, lecture tactile désactivée")
            return None

    def run(self):
        if not self.device: 
            print("[INFO] Aucun périphérique tactile trouvé")
            return
        print(f"[INFO] Lecture tactile depuis: {self.device.name}")
        for event in self.device.read_loop():
            if event.type == ecodes.EV_ABS:
                if event.code == ecodes.ABS_MT_POSITION_X: 
                    self.current_x = event.value
                elif event.code == ecodes.ABS_MT_POSITION_Y: 
                    self.current_y = event.value
            elif event.type == ecodes.EV_KEY and event.code == ecodes.BTN_TOUCH and event.value == 1:
                # Événement de tap - envoyer les coordonnées
                self.event_queue.put(("tap", self.current_x, self.current_y))

class SensorDashboard:
    def __init__(self):
        self.running = True
        self.mqtt_connected = False
        
        # Import de la config MQTT ici pour éviter les problèmes de portée
        sys.path.append(os.path.abspath(os.path.join(os.path.dirname(__file__), '..', 'interface')))
        from mqtt_config import MQTT_CONFIG
        self.mqtt_config = MQTT_CONFIG
        
        # Données des capteurs
        self.pot1 = 0
        self.pot2 = 0
        self.accel = {"x": 0.0, "y": 0.0, "z": 0.0, "roll": 0.0, "pitch": 0.0}
        self.button1 = False  # Un seul bouton
        self.status = {"uptime": 0, "rssi": 0}
        
        # État des LEDs (basé sur les dernières commandes reçues)
        self.led_states = {1: False, 2: False, 3: False, 4: False}
        
        # File d'événements tactiles
        self.touch_event_queue = Queue()
        
        # Démarrer le lecteur tactile
        self.touch_reader = TouchReader(self.touch_event_queue)
        self.touch_reader.start()
        
        self._init_mqtt()
    
    def _init_mqtt(self):
        """Initialise la connexion MQTT"""
        try:
            client_id = f"dashboard-{int(time.time())}"
            self.mqtt_client = mqtt.Client(client_id=client_id, transport="websockets")
            self.mqtt_client.tls_set(cert_reqs=ssl.CERT_REQUIRED)
            self.mqtt_client.username_pw_set(
                self.mqtt_config.get("username"), 
                self.mqtt_config.get("password")
            )
            self.mqtt_client.ws_set_options(path="/")
            self.mqtt_client.on_connect = self._on_connect
            self.mqtt_client.on_message = self._on_message
            self.mqtt_client.connect(
                self.mqtt_config.get("broker"), 
                self.mqtt_config.get("port", 443), 
                60
            )
            self.mqtt_client.loop_start()
            print("MQTT connecté.")
        except Exception as e:
            print(f"Erreur MQTT: {str(e)}")

    def _keyboard_listener(self):
        """Écoute les entrées clavier pour contrôler les LEDs"""
        print("\nContrôles clavier:")
        print("  1-4: Basculer les LED 1-4")
        print("  q: Quitter")
        print("  ?: Afficher cette aide\n")
        
        while self.running:
            try:
                # Utiliser input() avec timeout pour ne pas bloquer indéfiniment
                # Cette approche fonctionne dans la plupart des terminaux
                import select
                if select.select([sys.stdin], [], [], 0.1)[0]:
                    key = sys.stdin.read(1)
                    if key == 'q':
                        self.running = False
                        break
                    elif key in ['1', '2', '3', '4']:
                        led_num = int(key)
                        with self.led_lock:
                            new_state = not self.led_states[led_num]
                            self.led_states[led_num] = new_state
                            self._publish_led(led_num, new_state)
                            state_str = "ON" if new_state else "OFF"
                            print(f"LED {led_num} basculée -> {state_str}")
                    elif key == '?':
                        print("\nContrôles clavier:")
                        print("  1-4: Basculer les LED 1-4")
                        print("  q: Quitter")
                        print("  ?: Afficher cette aide\n")
            except (KeyboardInterrupt, EOFError):
                self.running = False
                break
            except Exception:
                # Ignorer les erreurs d'entrée dans certains environnements
                time.sleep(0.1)

    
    def _on_connect(self, client, userdata, flags, rc):
        print(f"[MQTT] Callback de connexion appelé avec code: {rc}")
        if rc == 0:
            self.mqtt_connected = True
            device_id = self.mqtt_config.get("device_id", "")
            print(f"[MQTT] Device ID lu depuis la config: '{device_id}'")
            # S'assure que le device_id se termine par '/'
            if not device_id.endswith('/'):
                root = f"{device_id}/"
            else:
                root = device_id
            print(f"[MQTT] Root utilisé pour les topics: '{root}'")
            
            # Abonnement aux topics des capteurs (copié exactement depuis interface_mqtt.py)
            self.sensors_buttons_topic = f"{root}sensors/buttons"
            self.sensors_pots_topic = f"{root}sensors/pots"
            self.sensors_accel_topic = f"{root}sensors/accel"
            self.status_topic = f"{root}status"  # ajout pour le status
            
            # Topics pour les LEDs (actuators)
            self.led1_topic = f"{root}actuators/led1"
            self.led2_topic = f"{root}actuators/led2"
            self.led3_topic = f"{root}actuators/led3"
            self.led4_topic = f"{root}actuators/led4"
            
            # Tous les topics à surveiller
            topics = [
                ("Boutons", self.sensors_buttons_topic),
                ("Pots", self.sensors_pots_topic),
                ("Accel", self.sensors_accel_topic),
                ("Status", self.status_topic),
                ("LED1", self.led1_topic),
                ("LED2", self.led2_topic),
                ("LED3", self.led3_topic),
                ("LED4", self.led4_topic)
            ]
            
            for name, topic in topics:
                print(f"[MQTT] Abonnement au topic {name}: {topic}")
                result, mid = self.mqtt_client.subscribe(topic)
                print(f"[MQTT] Résultat de l'abonnement {name}: {result}, MID: {mid}")
            
            print("[MQTT] Connexion établie et abonnements effectués")
        else:
            print(f"[MQTT] Échec de connexion avec le code: {rc}")
            self.mqtt_connected = False

    def _on_message(self, client, userdata, msg):
        """Traite les messages reçus"""
        try:
            payload = json.loads(msg.payload.decode('utf-8'))
            topic = msg.topic
            print(f"[MQTT] Message reçu - Topic: {topic}, Payload: {payload}")
            
            if topic == self.sensors_accel_topic:
                self.accel = payload
                print(f"[MQTT] Données d'accélération mises à jour: {self.accel}")
            elif topic == self.sensors_pots_topic:
                self.pot1 = payload.get("pot1", 0)
                self.pot2 = payload.get("pot2", 0)
                print(f"[MQTT] Données des potentiomètres mises à jour: pot1={self.pot1}, pot2={self.pot2}")
            elif topic == self.sensors_buttons_topic:
                # Un seul bouton : btn1
                self.button1 = payload.get("btn1", False)
                print(f"[MQTT] État du bouton mis à jour: btn1={self.button1}")
            elif topic == self.status_topic:
                self.status = payload
                print(f"[MQTT] Statut mis à jour: {self.status}")
            elif topic.endswith("led1"):
                led_state = payload.get("state", "off") == "on"
                self.led_states[1] = led_state
                print(f"[MQTT] État LED1 mis à jour: {'ON' if led_state else 'OFF'}")
            elif topic.endswith("led2"):
                led_state = payload.get("state", "off") == "on"
                self.led_states[2] = led_state
                print(f"[MQTT] État LED2 mis à jour: {'ON' if led_state else 'OFF'}")
            elif topic.endswith("led3"):
                led_state = payload.get("state", "off") == "on"
                self.led_states[3] = led_state
                print(f"[MQTT] État LED3 mis à jour: {'ON' if led_state else 'OFF'}")
            elif topic.endswith("led4"):
                led_state = payload.get("state", "off") == "on"
                self.led_states[4] = led_state
                print(f"[MQTT] État LED4 mis à jour: {'ON' if led_state else 'OFF'}")
                 
        except Exception as e:
            print(f"[MQTT] Erreur de traitement du message: {e}")
            print(f"[MQTT] Topic: {msg.topic}, Payload brut: {msg.payload}")
    
    def _clear_screen(self):
        """Efface l'écran"""
        print("\033[2J\033[H", end="")
    
    def _draw_dashboard(self):
        """Affiche le tableau de bord avec représentation visuelle des LEDs"""
        self._clear_screen()
        
        print("╔" + "═" * 68 + "╗")
        print("║" + " "*20 + "TABLEAU DE BORD DES CAPTEURS MQTT" + " "*21 + "║")
        print("╠" + "═" * 68 + "╣")
        print("║" + f"Statut connexion: {'\033[92mCONNECTÉ\033[0m' if self.mqtt_connected else '\033[91mDÉCONNECTÉ\033[0m'}" + " "*29 + "║")
        print("║" + f"Dernière mise à jour: {time.strftime('%H:%M:%S')}" + " "*38 + "║")
        print("╠" + "═" * 68 + "╣")
        
        # Affichage des potentiomètres
        print("║ POTENTIOMÈTRES:" + " "*51 + "║")
        print(f"║  Potentiomètre 1: {self.pot1:4d} / 4095" + " "*29 + "║")
        print(f"║  Potentiomètre 2: {self.pot2:4d} / 4095" + " "*29 + "║")
        print()
        
        # Barres de progression simples pour les pots
        bar_width = 25
        pot1_bar = int((self.pot1 / 4095) * bar_width)
        pot2_bar = int((self.pot2 / 4095) * bar_width)
        print(f"║  Pot1: [{'█' * pot1_bar}{'░' * (bar_width - pot1_bar)}] {self.pot1/4095*100:5.1f}%" + " "*11 + "║")
        print(f"║  Pot2: [{'█' * pot2_bar}{'░' * (bar_width - pot2_bar)}] {self.pot2/4095*100:5.1f}%" + " "*11 + "║")
        print()
        
        # Affichage de l'accéléromètre
        print("║ ACCELÉROMÈTRE:" + " "*50 + "║")
        print(f"║  Accélération X: {self.accel.get('x', 0):7.3f} g" + " "*25 + "║")
        print(f"║  Accélération Y: {self.accel.get('y', 0):7.3f} g" + " "*25 + "║")
        print(f"║  Accélération Z: {self.accel.get('z', 0):7.3f} g" + " "*25 + "║")
        print(f"║  Roll:           {self.accel.get('roll', 0):7.2f}°" + " "*28 + "║")
        print(f"║  Pitch:          {self.accel.get('pitch', 0):7.2f}°" + " "*28 + "║")
        print()
        
        # Affichage du bouton
        print("║ BOUTON:" + " "*59 + "║")
        btn_status = '\033[93mAPPUYÉ\033[0m' if self.button1 else '\033[94mRELÂCHÉ\033[0m'
        print(f"║  Bouton 1: {btn_status}" + " "*47 + "║")
        print()
        
        # Affichage des LEDs avec représentation visuelle
        print("║ LEDs:" + " "*61 + "║")
        led_display = ""
        for i in range(1, 5):
            if self.led_states[i]:
                # LED allumée - cercle vert rempli
                led_display += f"\033[92m●\033[0m "
            else:
                # LED éteinte - cercle rouge vide
                led_display += f"\033[91m○\033[0m "
            led_display += f"LED{i} "
        
        print(f"║  {led_display}" + " "*31 + "║")
        print("║  (Appuyez sur 1-4 pour basculer les LEDs)" + " "*20 + "║")
        print()
        
        # Affichage du statut
        print("║ STATUT ESP32:" + " "*51 + "║")
        print(f"║  Uptime:  {self.status.get('uptime', 0):4d} secondes" + " "*28 + "║")
        print(f"║  RSSI:    {self.status.get('rssi', 0):4d} dBm" + " "*31 + "║")
        print()
        
        print("╠" + "═" * 68 + "╣")
        print("║" + "Appuyez sur 'q' pour quitter, '?' pour l'aide" + " "*22 + "║")
        print("╚" + "═" * 68 + "╝")
    
    def run(self):
        """Boucle principale du tableau de bord avec gestion tactile"""
        try:
            print("\n" + "="*50)
            print("TABLEAU DE BORD INTERACTIF DES CAPTEURS MQTT")
            print("="*50)
            print("Utilisez l'écran tactile pour contrôler les LEDs")
            print("Utilisez 'q' pour quitter via clavier")
            print("="*50 + "\n")
            
            while self.running:
                self._draw_dashboard()
                
                # Vérifier les événements tactiles
                try:
                    # Timeout court pour ne pas bloquer l'affichage
                    event_type, x, y = self.touch_event_queue.get_nowait()
                    if event_type == "tap":
                        self._handle_touch(x, y)
                except Empty:
                    pass  # Pas d'événement tactile, continuer
                except Exception as e:
                    print(f"[WARN] Erreur traitement tactile: {e}")
                
                time.sleep(0.1)  # Rafraîchissement plus fréquent pour la réactivité tactile
        except KeyboardInterrupt:
            print("\nArrêt du tableau de bord...")
        finally:
            self.running = False
            if self.mqtt_client:
                self.mqtt_client.loop_stop()
                self.mqtt_client.disconnect()

    def _handle_touch(self, x, y):
        """Gère les événements tactiles pour contrôler les LEDs"""
        # Définir les zones tactiles pour chaque LED (à ajuster selon votre écran)
        # Format: (x_min, x_max, y_min, y_max, led_number)
        touch_zones = [
            (50, 150, 100, 150, 1),   # Zone pour LED 1
            (160, 260, 100, 150, 2),  # Zone pour LED 2
            (270, 370, 100, 150, 3),  # Zone pour LED 3
            (380, 480, 100, 150, 4)   # Zone pour LED 4
        ]
        
        # Ajuster les coordonnées si nécessaire (calibrage de base)
        # Normaliser les coordonnées brutes de l'écran tactile
        norm_x = int((x - self.touch_reader.min_x) / 
                    (self.touch_reader.max_x - self.touch_reader.min_x) * 500) if self.touch_reader.max_x > self.touch_reader.min_x else x
        norm_y = int((y - self.touch_reader.min_y) / 
                    (self.touch_reader.max_y - self.touch_reader.min_y) * 200) if self.touch_reader.max_y > self.touch_reader.min_y else y
        
        print(f"[TOUCH] Coordonnées brutes: ({x}, {y}) -> Normalisées: ({norm_x}, {norm_y})")
        
        # Vérifier dans quelle zone le tap s'est produit
        for zone_x_min, zone_x_max, zone_y_min, zone_y_max, led_num in touch_zones:
            if zone_x_min <= norm_x <= zone_x_max and zone_y_min <= norm_y <= zone_y_max:
                print(f"[TOUCH] Tap détecté dans la zone LED{led_num}")
                # Basculer l'état de la LED
                with self.led_lock:
                    new_state = not self.led_states[led_num]
                    self.led_states[led_num] = new_state
                    self._publish_led(led_num, new_state)
                    state_str = "ON" if new_state else "OFF"
                    print(f"LED {led_num} basculée via tactile -> {state_str}")
                break
        else:
            print(f"[TOUCH] Tap en dehors des zones LED: ({norm_x}, {norm_y})")


    def _publish_led(self, led_index, state):
        """Publie une commande pour contrôler une LED"""
        led_topics = {
            1: self.led1_topic,
            2: self.led2_topic,
            3: self.led3_topic,
            4: self.led4_topic
        }
        
        topic = led_topics.get(led_index)
        if topic:
            payload = json.dumps({"state": "on" if state else "off"})
            self.mqtt_client.publish(topic, payload)
            print(f"[MQTT] Commande envoyée vers {topic}: {payload}")


def main():
    dashboard = SensorDashboard()
    dashboard.run()

if __name__ == "__main__":
    main()