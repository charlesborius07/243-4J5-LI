from machine import Pin
import sys
import select

# Configuration des LEDs
# Note: Sur ESP32 standard, GPIO 35 est INPUT ONLY. 
# Si cela ne fonctionne pas pour la LED Rouge, vérifiez votre modèle (S3/C3 supportent output sur 35)
# ou changez de pin (ex: 25, 27, 32, 33).
led_green = Pin(26, Pin.OUT)
led_red = Pin(35, Pin.OUT)

# État initial : éteint
led_green.value(0)
led_red.value(0)

print("LilyGO Ready. Waiting for commands (RED/GREEN)...")

def main():
    while True:
        # Lecture non-bloquante du port série (USB)
        if select.select([sys.stdin], [], [], 0)[0]:
            line = sys.stdin.readline().strip()
            
            if not line:
                continue
                
            print(f"Recu: {line}") # Echo pour debug
            
            cmd = line.upper()
            
            if cmd == "RED":
                led_red.value(1)
                led_green.value(0)
                print("LED ROUGE ON")
            elif cmd == "GREEN":
                led_green.value(1)
                led_red.value(0)
                print("LED VERTE ON")
            elif cmd == "OFF":
                led_green.value(0)
                led_red.value(0)
                print("LEDS OFF")
            else:
                print("Commande inconnue")

if __name__ == '__main__':
    try:
        main()
    except KeyboardInterrupt:
        print("Arrêt.")
