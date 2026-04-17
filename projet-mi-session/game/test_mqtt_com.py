import json
import time
import sys
import os
sys.path.append(os.path.abspath(os.path.join(os.path.dirname(__file__), '..', 'interface')))
from mqtt_config import MQTT_CONFIG
import paho.mqtt.client as mqtt

def on_connect(client, userdata, flags, rc):
    print(f"Connecté avec code {rc}")
    device_id = MQTT_CONFIG.get("device_id", "")
    root = device_id if device_id.endswith('/') else f"{device_id}/"
    topic = f"{root}sensors/#"
    client.subscribe(topic)
    print(f"Abonné à: {topic}")

def on_message(client, userdata, msg):
    print(f"Reçu sur {msg.topic}: {msg.payload.decode()}")

client = mqtt.Client(transport="websockets")
# Ajout de la configuration TLS requise pour le broker websockets
import ssl
client.tls_set(cert_reqs=ssl.CERT_REQUIRED)
client.username_pw_set(MQTT_CONFIG.get("username"), MQTT_CONFIG.get("password"))
client.on_connect = on_connect
client.on_message = on_message
client.connect(MQTT_CONFIG.get("broker"), MQTT_CONFIG.get("port", 443), 60)
client.loop_forever()
