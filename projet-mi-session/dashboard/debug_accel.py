#!/usr/bin/env python3
"""
Outil de diagnostic pour vérifier les valeurs réelles publiées par l'accéléromètre
"""

import json
import time
import sys
import os
import paho.mqtt.client as mqtt
import ssl

# Import de la config MQTT
sys.path.append(os.path.abspath(os.path.join(os.path.dirname(__file__), '..', 'interface')))
from mqtt_config import MQTT_CONFIG

def on_connect(client, userdata, flags, rc):
    if rc == 0:
        print("Connecté au broker MQTT")
        device_id = MQTT_CONFIG.get("device_id", "")
        root = device_id if device_id.endswith('/') else f"{device_id}/"
        accel_topic = f"{root}sensors/accel"
        client.subscribe(accel_topic)
        print(f"Abonné au topic: {accel_topic}")
        print("En attente des données de l'accéléromètre...")
        print("-" * 50)
    else:
        print(f"Échec de connexion, code: {rc}")

def on_message(client, userdata, msg):
    try:
        payload = json.loads(msg.payload.decode('utf-8'))
        timestamp = time.strftime("%H:%M:%S")
        print(f"[{timestamp}] Accéléromètre: {payload}")
        
        # Vérifier si toutes les valeurs sont nulles
        if all(v == 0.0 for v in [payload.get('x', 0), payload.get('y', 0), payload.get('z', 0)]):
            print("  -> ATTENTION: Toutes les valeurs sont nulles!")
        else:
            print("  -> Valeurs détectées!")
            
    except Exception as e:
        print(f"Erreur de traitement: {e}")

def main():
    client = mqtt.Client(transport="websockets")
    client.tls_set(cert_reqs=ssl.CERT_REQUIRED)
    client.username_pw_set(MQTT_CONFIG.get("username"), MQTT_CONFIG.get("password"))
    client.on_connect = on_connect
    client.on_message = on_message
    
    try:
        client.connect(MQTT_CONFIG.get("broker"), MQTT_CONFIG.get("port", 443), 60)
        print("Démarrage du diagnostic de l'accéléromètre...")
        client.loop_forever()
    except KeyboardInterrupt:
        print("\nDiagnostic arrêté.")
    except Exception as e:
        print(f"Erreur: {e}")
    finally:
        client.loop_stop()
        client.disconnect()

if __name__ == "__main__":
    main()