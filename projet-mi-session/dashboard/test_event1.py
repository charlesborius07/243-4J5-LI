#!/usr/bin/env python3
"""
Test reading from /dev/input/event1 which is identified as a touchscreen
"""

import sys
import os
import time

try:
    from evdev import InputDevice, ecodes
    print("evdev disponible")
except ImportError as e:
    print(f"ERREUR: evdev non disponible: {e}")
    sys.exit(1)

def test_event1():
    device_path = '/dev/input/event1'
    print(f"Tentative d'ouverture de {device_path}")
    
    try:
        dev = InputDevice(device_path)
        print(f"Appareil ouvert: {dev.name}")
        print(f"Physique: {dev.phys}")
        print(f"Uniq: {dev.uniq}")
        # firmware_version might not be available on all devices
        try:
            print(f"Firmware: {dev.firmware_version}")
        except AttributeError:
            print("Firmware: Non disponible")
        
        # Afficher les capacités
        print("\nCapacités:")
        for event_type, codes in dev.capabilities().items():
            print(f"  Type {event_type}: {codes}")
        
        # Afficher les informations d'axes si disponibles
        try:
            abs_x = dev.absinfo(ecodes.ABS_MT_POSITION_X)
            abs_y = dev.absinfo(ecodes.ABS_MT_POSITION_Y)
            print(f"Axes X: min={abs_x.min}, max={abs_x.max}, resolution={abs_x.resolution}")
            print(f"Axes Y: min={abs_y.min}, max={abs_y.max}, resolution={abs_y.resolution}")
        except Exception as e:
            print(f"Pas d'informations d'axes absolus MT: {e}")
        
        print("\nÉcoute des événements (appuyez sur Ctrl+C pour quitter)...")
        print("Touchez l'écran pour voir les événements...")
        
        for event in dev.read_loop():
            if event.type == ecodes.EV_ABS:
                if event.code == ecodes.ABS_MT_POSITION_X:
                    print(f"X: {event.value}")
                elif event.code == ecodes.ABS_MT_POSITION_Y:
                    print(f"Y: {event.value}")
                elif event.code == ecodes.ABS_MT_TRACKING_ID:
                    print(f"Tracking ID: {event.value} ({-1 if event.value == -1 else 'active'})")
            elif event.type == ecodes.EV_KEY and event.code == ecodes.BTN_TOUCH:
                if event.value == 1:
                    print("TOUCH DOWN")
                elif event.value == 0:
                    print("TOUCH UP")
            elif event.type == ecodes.EV_SYN:
                pass  # Ignorer les événements de synchronisation pour simplifier l'affichage
    except FileNotFoundError:
        print(f"ERREUR: Périphérique {device_path} non trouvé")
    except PermissionError:
        print(f"ERREUR: Permission refusée pour accéder à {device_path}. Essayez avec sudo ou en ajoutant votre utilisateur au groupe input.")
    except Exception as e:
        print(f"ERREUR lors de l'ouverture ou de la lecture de {device_path}: {e}")

if __name__ == "__main__":
    test_event1()