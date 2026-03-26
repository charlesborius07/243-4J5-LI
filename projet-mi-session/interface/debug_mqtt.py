import paho.mqtt.client as mqtt
import ssl
import time

# Configuration
BROKER = "mqtt.charlesborius07.com"
PORT = 443
USERNAME = "telecom"
PASSWORD = "Teladmin1$"
DEVICE_ID = "etudiant/charlesboris-feugangfoteu"
TOPICS = [
    f"{DEVICE_ID}/sensors/buttons",
    f"{DEVICE_ID}/sensors/pots",
    f"{DEVICE_ID}/sensors/accel"
]

def on_connect(client, userdata, flags, rc):
    if rc == 0:
        print("Connected to MQTT Broker!")
        for topic in TOPICS:
            client.subscribe(topic)
            print(f"Subscribed to: {topic}")
    else:
        print(f"Failed to connect, return code {rc}")

def on_message(client, userdata, msg):
    print(f"Received message on {msg.topic}: {msg.payload.decode()}")

client = mqtt.Client(transport="websockets")
client.tls_set(cert_reqs=ssl.CERT_REQUIRED)
client.username_pw_set(USERNAME, PASSWORD)
client.on_connect = on_connect
client.on_message = on_message

print(f"Connecting to {BROKER}:{PORT}...")
client.connect(BROKER, PORT, 60)

client.loop_start()

print("Waiting for messages for 20 seconds...")
time.sleep(20)

client.loop_stop()
client.disconnect()
print("Done.")
