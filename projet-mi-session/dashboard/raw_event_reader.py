#!/usr/bin/env python3
"""
Read raw input events from /dev/input/event1 and parse them manually
"""

import os
import struct
import time

def main():
    device_path = '/dev/input/event1'
    print(f"Opening {device_path}")
    
    try:
        fd = os.open(device_path, os.O_RDONLY | os.O_NONBLOCK)
        print(f"Opened successfully (fd={fd})")
    except Exception as e:
        print(f"Failed to open {device_path}: {e}")
        return 1
    
    # Format for struct input_event: llHHI
    # ll: two longs (sec, usec) - 8 bytes
    # H: unsigned short (type) - 2 bytes
    # H: unsigned short (code) - 2 bytes
    # I: unsigned int (value) - 4 bytes
    # Total: 16 bytes
    FORMAT = 'llHHI'
    EVENT_SIZE = struct.calcsize(FORMAT)
    
    print(f"Event size: {EVENT_SIZE} bytes")
    print("Waiting for touch events...")
    print("Touch the screen now...")
    print("-" * 50)
    
    try:
        while True:
            # Try to read one event
            try:
                data = os.read(fd, EVENT_SIZE)
                if len(data) == 0:
                    # No data available, sleep a bit
                    time.sleep(0.001)
                    continue
                elif len(data) != EVENT_SIZE:
                    print(f"Warning: read {len(data)} bytes, expected {EVENT_SIZE}")
                    continue
                
                # Parse the event
                sec, usec, ev_type, ev_code, ev_value = struct.unpack(FORMAT, data)
                
                # Only print certain event types to reduce noise
                if ev_type == 3:  # EV_ABS
                    if ev_code == 0:  # ABS_X
                        print(f"[{sec}.{usec:06d}] ABS_X: {ev_value}")
                    elif ev_code == 1:  # ABS_Y
                        print(f"[{sec}.{usec:06d}] ABS_Y: {ev_value}")
                    elif ev_code == 24:  # ABS_MT_TRACKING_ID
                        status = "ACTIVE" if ev_value != -1 else "INACTIVE"
                        print(f"[{sec}.{usec:06d}] ABS_MT_TRACKING_ID: {ev_value} ({status})")
                elif ev_type == 1:  # EV_KEY
                    if ev_code == 330:  # BTN_TOUCH (common value, might vary)
                        state = "DOWN" if ev_value else "UP"
                        print(f"[{sec}.{usec:06d}] BTN_TOUCH: {state}")
                # Uncomment the following lines to see all events (very noisy)
                # else:
                #     print(f"[{sec}.{usec:06d}] type={ev_type} code={ev_code} value={ev_value}")
                    
            except BlockingIOError:
                # No data available, this is expected with O_NONBLOCK
                time.sleep(0.001)
                continue
            except Exception as e:
                print(f"Error reading/parsing event: {e}")
                break
                
    except KeyboardInterrupt:
        print("\nStopped by user")
    finally:
        os.close(fd)
    
    return 0

if __name__ == "__main__":
    exit(main())