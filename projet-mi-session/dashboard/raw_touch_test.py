#!/usr/bin/env python3
"""
Minimal test to read raw data from /dev/input/event1
"""

import os

def test_raw_read():
    device_path = '/dev/input/event1'
    print(f"Tentative de lecture brute de {device_path}")
    
    try:
        with open(device_path, 'rb') as f:
            print(f"Fichier ouvert avec succès")
            print("Lecture des 50 premiers événements (16 bytes chacun)...")
            print("Format: [time.tv_sec][time.tv_usec][type][code][value] (chaque champ: 4 bytes sauf valeur: 4 bytes)")
            print("(En fait: struct input_event: time.tv_sec, time.tv_usec, type, code, value)")
            print("-" * 60)
            
            for i in range(50):
                event_data = f.read(16)  # sizeof(struct input_event) = 16 bytes on most systems
                if len(event_data) < 16:
                    print(f"Fin prématurée après {i} événements")
                    break
                
                # Parse the event data manually
                # struct input_event {
                #     struct timeval time;
                #     unsigned short type;
                #     unsigned short code;
                #     unsigned int value;
                # };
                try:
                    tv_sec = int.from_bytes(event_data[0:4], byteorder='little')
                    tv_usec = int.from_bytes(event_data[4:8], byteorder='little')
                    event_type = int.from_bytes(event_data[8:10], byteorder='little')
                    event_code = int.from_bytes(event_data[10:12], byteorder='little')
                    value = int.from_bytes(event_data[12:16], byteorder='little', signed=True)
                    
                    print(f"[{i:2d}] sec={tv_sec:6d}, usec={tv_usec:6d}, type={event_type:3d}, code={event_code:3d}, value={value:6d}")
                    
                    # Interpreter quelques types connus
                    if event_type == 3:  # EV_ABS
                        if event_code == 0:  # ABS_X
                            print(f"      -> ABS_X: {value}")
                        elif event_code == 1:  # ABS_Y
                            print(f"      -> ABS_Y: {value}")
                    elif event_type == 1:  # EV_KEY
                        if event_code == 330:  # BTN_TOUCH (parfois)
                            print(f"      -> BTN_TOUCH: {'DOWN' if value else 'UP'}")
                            
                except Exception as e:
                    print(f"Erreur de parsing: {e}")
                    
    except PermissionError:
        print(f"ERREUR: Permission refusée pour accéder à {device_path}")
        print("Essayez: sudo adduser $USER input && newgrp input")
    except FileNotFoundError:
        print(f"ERREUR: Fichier {device_path} non trouvé")
    except Exception as e:
        print(f"ERREUR: {e}")

if __name__ == "__main__":
    test_raw_read()