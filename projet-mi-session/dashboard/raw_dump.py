#!/usr/bin/env python3
"""
Just dump raw bytes from the touch device to see if we're getting anything
"""

import os
import time

def dump_raw_data():
    device_path = '/dev/input/event1'
    print(f"Dumping raw data from {device_path}")
    print("Appuyez sur l'écran tactile et observez si des données changent")
    print("Ctrl+C pour quitter\n")
    
    try:
        with open(device_path, 'rb') as f:
            while True:
                # Read in chunks
                data = f.read(32)
                if data:
                    # Show as hex and ASCII
                    hex_str = ' '.join(f'{b:02x}' for b in data)
                    ascii_str = ''.join(chr(b) if 32 <= b < 127 else '.' for b in data)
                    print(f"{hex_str:<48} {ascii_str}")
                time.sleep(0.01)
    except KeyboardInterrupt:
        print("\nArrêt du dump.")
    except Exception as e:
        print(f"Erreur: {e}")

if __name__ == "__main__":
    dump_raw_data()