import json
import time
import sys
import os
import paho.mqtt.client as mqtt
import ssl

# Import de la config
sys.path.append(os.path.abspath(os.path.join(os.path.dirname(__file__), '..', 'interface')))
from mqtt_config import MQTT_CONFIG

def on_connect(client, userdata, flags, rc):
    if rc == 0:
        print("Connecté au broker.")
        # Abonnement large pour voir tout le trafic du device
        device_id = MQTT_CONFIG.get("device_id", "")
        root = device_id if device_id.endswith('/') else f"{device_id}/"
        topic = f"{root}#"
        print(f"Abonnement à: {topic}")
        client.subscribe(topic)
    else:
        print(f"Échec de connexion, code: {rc}")

def on_message(client, userdata, msg):
    print(f"--- MESSAGE REÇU ---")
    print(f"Topic: {msg.topic}")
    print(f"Payload: {msg.payload.decode()}")

print("Démarrage du test de diagnostic MQTT...")
client = mqtt.Client(transport="websockets")
client.tls_set(cert_reqs=ssl.CERT_REQUIRED)
client.username_pw_set(MQTT_CONFIG.get("username"), MQTT_CONFIG.get("password"))
client.on_connect = on_connect
client.on_message = on_message

try:
    client.connect(MQTT_CONFIG.get("broker"), MQTT_CONFIG.get("port", 443), 60)
    client.loop_start()
    print("En attente de messages (30s)...")
    time.sleep(30)
    client.loop_stop()
except Exception as e:
    print(f"Erreur: {e}")
