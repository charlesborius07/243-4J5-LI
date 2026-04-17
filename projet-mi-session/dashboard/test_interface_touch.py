#!/usr/bin/env python3
"""
Test the exact TouchReader class from interface_mqtt.py
"""

import sys
import os
import time
from queue import Queue

# Add the interface directory to path
sys.path.append(os.path.abspath(os.path.join(os.path.dirname(__file__), '..', 'interface')))

# Copy the exact TouchReader class from interface_mqtt.py
from evdev import list_devices, InputDevice, ecodes

class TouchReader:
    def __init__(self, event_queue: Queue):
        self.event_queue = event_queue
        self.device = self._find_touch_device()
        if not self.device:
            raise RuntimeError("Aucun périphérique touchscreen trouvé.")

        # On récupère les infos d'axes pour calibrer
        abs_x = self.device.absinfo(ecodes.ABS_MT_POSITION_X)
        abs_y = self.device.absinfo(ecodes.ABS_MT_POSITION_Y)

        self.min_x, self.max_x = abs_x.min, abs_x.max
        self.min_y, self.max_y = abs_y.min, abs_y.max

        self.current_x = (self.min_x + self.max_x) // 2
        self.current_y = (self.min_y + self.max_y) // 2

    def _find_touch_device(self):
        """
        Essaie de trouver un device dont le nom contient 'touch' ou 'ft5406'
        """
        for path in list_devices():
            dev = InputDevice(path)
            name = dev.name.lower()
            if "touch" in name or "ft5406" in name:
                print(f"[TouchReader] Using device: {dev.name} ({path})")
                return dev
        return None

    def run(self):
        for event in self.device.read_loop():
            if event.type == ecodes.EV_ABS:
                if event.code == ecodes.ABS_MT_POSITION_X:
                    self.current_x = event.value
                elif event.code == ecodes.ABS_MT_POSITION_Y:
                    self.current_y = event.value

            elif event.type == ecodes.EV_KEY and event.code == ecodes.BTN_TOUCH:
                # 1 = touch down, 0 = touch up
                if event.value == 1:
                    # On push un "tap" dans la queue avec les coordonnées brutes
                    self.event_queue.put(("tap", self.current_x, self.current_y))

def main():
    print("=== Testing TouchReader from interface_mqtt.py ===")
    
    event_queue = Queue()
    
    try:
        touch_reader = TouchReader(event_queue)
        print("TouchReader created successfully")
        print(f"Device: {touch_reader.device.name}")
        print(f"X range: {touch_reader.min_x} - {touch_reader.max_x}")
        print(f"Y range: {touch_reader.min_y} - {touch_reader.max_y}")
        print(f"Initial position: ({touch_reader.current_x}, {touch_reader.current_y})")
        print("\nWaiting for touch events... (touch the screen)")
        print("Press Ctrl+C to quit\n")
        
        start_time = time.time()
        last_event_time = start_time
        
        while time.time() - start_time < 30:  # Run for 30 seconds max
            try:
                # Try to get an event with timeout
                event_type, x, y = event_queue.get(timeout=2.0)
                if event_type == "tap":
                    last_event_time = time.time()
                    print(f"[{time.time()-start_time:6.2f}s] TOUCH EVENT: ({x:4d}, {y:4d})")
                    
                    # Also show the normalized coordinates (0-1 range)
                    norm_x = (x - touch_reader.min_x) / (touch_reader.max_x - touch_reader.min_x) if touch_reader.max_x > touch_reader.min_x else 0
                    norm_y = (y - touch_reader.min_y) / (touch_reader.max_y - touch_reader.min_y) if touch_reader.max_y > touch_reader.min_y else 0
                    print(f"         Normalized: ({norm_x:.3f}, {norm_y:.3f})")
                    
            except Exception as e:
                # Timeout or other error
                if time.time() - last_event_time > 5.0:
                    print(f"[{time.time()-start_time:6.2f}s] No touch events for 5+ seconds...")
                    last_event_time = time.time()  # Reset to avoid spamming
                continue
                
    except Exception as e:
        print(f"Error creating TouchReader: {e}")
        import traceback
        traceback.print_exc()
    finally:
        print("\nTest ended.")

if __name__ == "__main__":
    main()