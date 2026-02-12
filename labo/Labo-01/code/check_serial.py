import sys
import time
import termios
import os

# Function to read from serial without pyserial (using os.read)
def read_serial(port, baudrate=115200, timeout=2):
    fd = os.open(port, os.O_RDWR | os.O_NOCTTY | os.O_NONBLOCK)
    
    # Set baudrate (simplified, assumes 115200 is default or already set)
    # Ideally we'd use termios here to set baudrate properly
    # commands: stty -F /dev/ttyACM0 115200
    
    print(f"Listening on {port}...")
    start = time.time()
    buffer = b""
    while time.time() - start < timeout:
        try:
            chunk = os.read(fd, 1024)
            if chunk:
                buffer += chunk
                # Reset timeout on data
                # start = time.time() 
        except OSError:
            pass
        time.sleep(0.1)
    
    os.close(fd)
    return buffer

if __name__ == "__main__":
    try:
        # Configure stty first just in case
        os.system("stty -F /dev/ttyACM0 115200 raw -echo")
        data = read_serial("/dev/ttyACM0")
        print(f"Received: {data}")
        if b">>>" in data:
            print("Detected MicroPython REPL")
    except Exception as e:
        print(f"Error: {e}")
