import json
import sys
import os
sys.path.append(os.path.abspath(os.path.join(os.path.dirname(__file__), '..', 'interface')))
from mqtt_config import MQTT_CONFIG
import paho.mqtt.client as mqtt
import time

def on_message(client, userdata, msg):
    print(f"Message reçu sur {msg.topic}: {msg.payload.decode()}")

client = mqtt.Client(transport="websockets")
client.username_pw_set(MQTT_CONFIG.get("username"), MQTT_CONFIG.get("password"))
client.on_message = on_message
client.connect(MQTT_CONFIG.get("broker"), MQTT_CONFIG.get("port", 443), 60)
client.loop_start()

device_id = MQTT_CONFIG.get("device_id", "etudiant/charlesboris-feugangfoteu/")
if not device_id.endswith('/'): root = f"{device_id}/"
else: root = device_id

client.subscribe(f"{root}sensors/accel")
print(f"Abonné à {root}sensors/accel...")

try:
    time.sleep(10)
except KeyboardInterrupt:
    pass
client.loop_stop()
