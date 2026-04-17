#!/usr/bin/env python3
"""
Simple touch test that prints coordinates when touched
Based on the working interface_mqtt.py approach
"""

import sys
import os
import time

# Add the interface directory to path
sys.path.append(os.path.abspath(os.path.join(os.path.dirname(__file__), '..', 'interface')))

try:
    from evdev import list_devices, InputDevice, ecodes
    print("evdev imported successfully")
except ImportError as e:
    print(f"Failed to import evdev: {e}")
    sys.exit(1)

def find_touch_device():
    """Find touch device like in interface_mqtt.py"""
    for path in list_devices():
        dev = InputDevice(path)
        name = dev.name.lower()
        if "touch" in name or "ft5406" in name:
            print(f"Using device: {dev.name} ({path})")
            return dev
    return None

def main():
    print("=== Simple Touch Test ===")
    
    # Find touch device
    device = find_touch_device()
    if not device:
        print("ERROR: No touch device found")
        return 1
    
    # Get axis info for calibration (like in interface_mqtt.py)
    abs_x = device.absinfo(ecodes.ABS_MT_POSITION_X)
    abs_y = device.absinfo(ecodes.ABS_MT_POSITION_Y)
    min_x, max_x = abs_x.min, abs_x.max
    min_y, max_y = abs_y.min, abs_y.max
    
    print(f"Device: {device.name}")
    print(f"X range: {min_x} to {max_x}")
    print(f"Y range: {min_y} to {max_y}")
    print(f"Center: X={(min_x+max_x)//2}, Y={(min_y+max_y)//2}")
    print("\nTouch the screen to see coordinates...")
    print("Press Ctrl+C to quit\n")
    
    try:
        for event in device.read_loop():
            if event.type == ecodes.EV_ABS:
                if event.code == ecodes.ABS_MT_POSITION_X:
                    current_x = event.value
                elif event.code == ecodes.ABS_MT_POSITION_Y:
                    current_y = event.value
                    
                    # When we have both coordinates, calculate and display
                    # Map to 0-100% for easier understanding
                    if max_x > min_x:
                        x_percent = (current_x - min_x) / (max_x - min_x) * 100
                    else:
                        x_percent = 0
                        
                    if max_y > min_y:
                        y_percent = (current_y - min_y) / (max_y - min_y) * 100
                    else:
                        y_percent = 0
                    
                    print(f"Raw: ({current_x:4d}, {current_y:4d}) -> "
                          f"Percent: ({x_percent:5.1f}%, {y_percent:5.1f}%)")
                          
            elif event.type == ecodes.EV_KEY and event.code == ecodes.BTN_TOUCH:
                if event.value == 1:
                    print("TOUCH DOWN")
                elif event.value == 0:
                    print("TOUCH UP")
                    
    except KeyboardInterrupt:
        print("\nTest ended.")
        return 0
    except Exception as e:
        print(f"Error: {e}")
        return 1

if __name__ == "__main__":
    sys.exit(main())