#!/usr/bin/env python3
import time
from touch_handler import TouchHandler

def main():
    touch_handler = TouchHandler(80, 24)
    if not touch_handler.start():
        print("Failed to start touch handler")
        return

    print("Touch handler started. Waiting for touch events...")
    try:
        while True:
            touch_pos = touch_handler.get_touch_position()
            if touch_pos:
                print(f"Touch at {touch_pos}")
            time.sleep(0.1)
    except KeyboardInterrupt:
        print("Stopping...")
    finally:
        touch_handler.stop()

if __name__ == "__main__":
    main()
