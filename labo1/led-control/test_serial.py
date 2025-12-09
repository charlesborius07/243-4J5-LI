#!/usr/bin/env python3
"""
Script de test pour la communication série avec le LilyGO
Permet de tester le contrôle des LEDs sans l'interface tactile
"""

import serial
import time
import sys

def test_serial_connection():
    """
    Teste la connexion série et envoie des commandes de test
    """
    port = '/dev/ttyACM0'
    baudrate = 115200

    print("========================================")
    print("Test de communication série")
    print("========================================")
    print(f"Port: {port}")
    print(f"Baudrate: {baudrate}")
    print()

    try:
        # Ouvrir le port série
        ser = serial.Serial(port, baudrate, timeout=1)
        print("✓ Port série ouvert avec succès")
        time.sleep(2)  # Attendre que la connexion soit établie

        # Lire les messages d'initialisation du LilyGO
        print("\n--- Messages d'initialisation ---")
        time.sleep(0.5)
        while ser.in_waiting > 0:
            line = ser.readline().decode('utf-8', errors='ignore').strip()
            if line:
                print(f"  {line}")

        print("\n--- Début des tests ---")

        # Test 1: LED rouge
        print("\n1. Test LED ROUGE")
        ser.write(b"rouge\n")
        time.sleep(0.5)
        response = read_response(ser)
        print(f"   Réponse: {response}")
        time.sleep(2)

        # Test 2: LED verte
        print("\n2. Test LED VERTE")
        ser.write(b"vert\n")
        time.sleep(0.5)
        response = read_response(ser)
        print(f"   Réponse: {response}")
        time.sleep(2)

        # Test 3: Éteindre
        print("\n3. Test ÉTEINDRE")
        ser.write(b"off\n")
        time.sleep(0.5)
        response = read_response(ser)
        print(f"   Réponse: {response}")
        time.sleep(1)

        # Test 4: Commande invalide
        print("\n4. Test commande INVALIDE")
        ser.write(b"bleu\n")
        time.sleep(0.5)
        response = read_response(ser)
        print(f"   Réponse: {response}")
        time.sleep(1)

        # Test 5: Séquence automatique
        print("\n5. Séquence automatique (rouge → vert → off)")
        for i in range(3):
            ser.write(b"rouge\n")
            time.sleep(1)
            ser.write(b"vert\n")
            time.sleep(1)
        ser.write(b"off\n")
        print("   Séquence terminée")

        print("\n========================================")
        print("✓ Tests terminés avec succès!")
        print("========================================")

        # Fermer le port
        ser.close()

    except serial.SerialException as e:
        print(f"❌ Erreur de connexion série: {e}")
        print("\nVérifications:")
        print("  • Le LilyGO est-il connecté?")
        print("  • Le port /dev/ttyUSB0 existe-t-il?")
        print("  • Un autre programme utilise-t-il le port série?")
        sys.exit(1)
    except KeyboardInterrupt:
        print("\n\nInterruption par l'utilisateur")
        if 'ser' in locals() and ser.is_open:
            ser.close()
        sys.exit(0)
    except Exception as e:
        print(f"❌ Erreur inattendue: {e}")
        sys.exit(1)

def read_response(ser, timeout=1):
    """
    Lit la réponse du LilyGO
    """
    responses = []
    start_time = time.time()

    while time.time() - start_time < timeout:
        if ser.in_waiting > 0:
            line = ser.readline().decode('utf-8', errors='ignore').strip()
            if line:
                responses.append(line)
        time.sleep(0.01)

    return '\n   '.join(responses) if responses else "(aucune réponse)"

def interactive_mode():
    """
    Mode interactif pour envoyer des commandes manuellement
    """
    port = '/dev/ttyACM0'
    baudrate = 115200

    print("========================================")
    print("Mode interactif")
    print("========================================")
    print("Commandes disponibles:")
    print("  rouge  - Allume LED rouge")
    print("  vert   - Allume LED verte")
    print("  off    - Éteint toutes les LEDs")
    print("  quit   - Quitter")
    print("========================================")
    print()

    try:
        ser = serial.Serial(port, baudrate, timeout=1)
        print("✓ Connecté au LilyGO")
        time.sleep(2)

        # Vider le buffer initial
        while ser.in_waiting > 0:
            ser.readline()

        while True:
            cmd = input("\nCommande> ").strip().lower()

            if cmd == 'quit':
                break
            elif cmd in ['rouge', 'vert', 'off']:
                ser.write(f"{cmd}\n".encode())
                time.sleep(0.2)
                response = read_response(ser, 0.5)
                if response != "(aucune réponse)":
                    print(f"Réponse: {response}")
            else:
                print("❌ Commande non reconnue")

        ser.close()
        print("\nAu revoir!")

    except serial.SerialException as e:
        print(f"❌ Erreur: {e}")
        sys.exit(1)
    except KeyboardInterrupt:
        print("\n\nInterruption")
        if 'ser' in locals() and ser.is_open:
            ser.close()
        sys.exit(0)

if __name__ == "__main__":
    if len(sys.argv) > 1 and sys.argv[1] == "-i":
        interactive_mode()
    else:
        test_serial_connection()
