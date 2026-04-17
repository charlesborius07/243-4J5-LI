#!/usr/bin/env python3
"""
Ultra-simple touch test - just prints when any touch event occurs
"""

import sys
import os
import time

# Add interface to path
sys.path.append(os.path.abspath(os.path.join(os.path.dirname(__file__), '..', 'interface')))

try:
    from evdev import list_devices, InputDevice, ecodes
    from select import select
    print("Imports successful")
except ImportError as e:
    print(f"Import error: {e}")
    sys.exit(1)

def main():
    print("=== Ultra Simple Touch Test ===")
    
    # Find touch device
    device = None
    for path in list_devices():
        try:
            dev = InputDevice(path)
            name = dev.name.lower()
            if "touch" in name or "ft5406" in name or "goodix" in name:
                print(f"Found touch device: {dev.name} at {path}")
                device = dev
                break
        except:
            continue
    
    if not device:
        print("ERROR: No touch device found")
        return 1
    
    print(f"Using device: {device.name}")
    print("Make sure to touch the screen firmly and hold for a moment")
    print("Waiting for touch events...")
    print("-" * 50)
    
    try:
        # Make device non-blocking
        fd = device.fileno()
        # Note: evdev devices might not support O_NONBLOCK the same way
        
        print("Listening for events (touch the screen now)...")
        start = time.time()
        last_report = start
        
        while time.time() - start < 20:  # 20 second timeout
            try:
                # Try to read events - this will block until one is available
                for event in device.read():
                    if event.type == ecodes.EV_ABS:
                        if event.code == ecodes.ABS_MT_POSITION_X:
                            x = event.value
                        elif event.code == ecodes.ABS_MT_POSITION_Y:
                            y = event.value
                    elif event.type == ecodes.EV_KEY and event.code == ecodes.BTN_TOUCH:
                        if event.value == 1:  # Touch down
                            print(f"[TOUCH DOWN] at ({x}, {y})")
                            # Also show where this would map on a typical 800x480 screen
                            # These are rough estimates - adjust based on actual calibration
                            screen_x = int((x / 4095) * 800) if 4095 > 0 else 0
                            screen_y = int((y / 4095) * 480) if 4095 > 0 else 0
                            print(f"         -> Screen approx: ({screen_x}, {screen_y})")
                        elif event.value == 0:  # Touch up
                            print(f"[TOUCH UP] at ({x}, {y})")
                    
                    last_report = time.time()
            except Exception as e:
                print(f"Error reading events: {e}")
                break
            
            # Report if no events for a while
            if time.time() - last_report > 5.0:
                print(f"No touch events for {time.time()-last_report:.0f} seconds...")
                print("Please try touching the screen more firmly")
                print("-" * 30)
                last_report = time.time()
                
    except KeyboardInterrupt:
        print("\nTest interrupted by user")
    except Exception as e:
        print(f"Error: {e}")
        import traceback
        traceback.print_exc()
    
    print("Test ended")
    return 0

if __name__ == "__main__":
    sys.exit(main())