# /python-client/client.py

import socket
import os
import sys
import time # Import time for sleep

# --- Configuration from Environment Variables ---
TARGET_HOST = os.environ.get("TARGET_HOST", "python-server")
TARGET_PORT = int(os.environ.get("TARGET_PORT", 5001))

def run_client():
    MAX_RETRIES = 5
    RETRY_DELAY = 5 # Wait 5 seconds between attempts
    
    for attempt in range(MAX_RETRIES):
        s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        
        try:
            print(f"[Python Client] Attempt {attempt + 1}/{MAX_RETRIES}: Connecting to {TARGET_HOST}:{TARGET_PORT}...")
            
            # Connect to the target server using its Docker service name
            s.connect((TARGET_HOST, TARGET_PORT))
            print("[Python Client] Connection successful.")
            
            # 1. Send request
            message = "REQUEST_LATEST_POINTS\n"
            s.sendall(message.encode())
            print(f"[Python Client] Sent: {message.strip()}")
            
            # 2. Receive response
            response = s.recv(1024).decode()
            print("\n--- SERVER RESPONSE ---")
            print(response.strip())
            print("-----------------------\n")
            
            s.close()
            sys.exit(0) # Exit successfully after connection
            
        except socket.error as e:
            print(f"[Python Client] Connection failed: {e}")
            s.close()
            
            if attempt < MAX_RETRIES - 1:
                print(f"[Python Client] Retrying in {RETRY_DELAY} seconds...")
                time.sleep(RETRY_DELAY)
            else:
                print("[Python Client] Failed to connect after all retries. Exiting.")
                sys.exit(1) # Exit with error code

if __name__ == "__main__":
    run_client()