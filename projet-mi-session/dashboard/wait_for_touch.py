#!/usr/bin/env python3
"""
Wait for touch events with clear feedback
"""

import os
import sys
import time
import select

def wait_for_touch():
    device_path = '/dev/input/event1'
    print(f"Waiting for touch events on {device_path}")
    print("Please touch the screen now...")
    print("Touch events will be displayed below:")
    print("-" * 50)
    
    try:
        fd = os.open(device_path, os.O_RDONLY | os.O_NONBLOCK)
        print(f"Device opened successfully (fd={fd})")
        
        # Use select to wait for data with timeout
        while True:
            # Wait for up to 2 seconds for data
            ready, _, _ = select.select([fd], [], [], 2.0)
            if ready:
                try:
                    data = os.read(fd, 16)  # Read one input_event
                    if len(data) == 16:
                        # Parse the event
                        tv_sec = int.from_bytes(data[0:4], 'little', signed=False)
                        tv_usec = int.from_bytes(data[4:8], 'little', signed=False)
                        ev_type = int.from_bytes(data[8:10], 'little', signed=False)
                        ev_code = int.from_bytes(data[10:12], 'little', signed=False)
                        ev_value = int.from_bytes(data[12:16], 'little', signed=True)
                        
                        timestamp = tv_sec + tv_usec / 1000000.0
                        print(f"[{timestamp:9.6f}] type={ev_type:2d} code={ev_code:3d} value={ev_value:6d}", end='')
                        
                        # Interpret the event
                        if ev_type == 3:  # EV_ABS
                            if ev_code == 0:  # ABS_X
                                print(f"  -> X={ev_value}")
                            elif ev_code == 1:  # ABS_Y
                                print(f"  -> Y={ev_value}")
                            elif ev_code == 24:  # ABS_MT_TRACKING_ID
                                status = "ACTIVE" if ev_value != -1 else "INACTIVE"
                                print(f"  -> Tracking ID={ev_value} ({status})")
                        elif ev_type == 1:  # EV_KEY
                            if ev_code == 330:  # BTN_TOUCH
                                state = "DOWN" if ev_value else "UP"
                                print(f"  -> Touch {state}")
                        else:
                            print()  # Just newline
                    else:
                        print(f"Incomplete read: {len(data)} bytes")
                except OSError as e:
                    if e.errno == 11:  # EAGAIN
                        continue  # No data available
                    else:
                        print(f"OS error: {e}")
                        break
            else:
                print("No touch events received in the last 2 seconds...")
                print("(Try touching the screen more firmly or in different locations)")
                print("-" * 50)
                
    except PermissionError:
        print(f"ERROR: Permission denied accessing {device_path}")
        print("Even though you're in the 'input' group, sometimes you need to log out and back in")
        print("Or try running with sudo for testing purposes")
    except FileNotFoundError:
        print(f"ERROR: Device {device_path} not found")
    except Exception as e:
        print(f"ERROR: {e}")
    finally:
        try:
            os.close(fd)
        except:
            pass

if __name__ == "__main__":
    wait_for_touch()