import socket
import threading
import time
import os
import mysql.connector
from datetime import datetime

# --- Configuration from Environment Variables ---
SERVER_PORT = int(os.environ.get("SERVER_PORT", 5004))
DB_HOST = os.environ.get("DB_HOST", "database")
DB_USER = os.environ.get("DB_USER", "root")
DB_PASS = os.environ.get("DB_PASS", "root@12345")
DB_NAME = os.environ.get("DB_NAME", "project_db")
SERVER_USER = "python_user-hasif"
UPDATE_INTERVAL = 30 # seconds

# --- Database Connection Management ---
def get_db_connection():
    MAX_RETRIES = 10
    RETRY_DELAY = 3 # seconds
    
    for attempt in range(MAX_RETRIES):
        try:
            # Attempt to connect
            db = mysql.connector.connect(
                host=DB_HOST,
                user=DB_USER,
                password=DB_PASS,
                database=DB_NAME
            )
            print(f"[{SERVER_USER} Server] Database connection established successfully.")
            return db
            
        except mysql.connector.Error as e:
            if attempt < MAX_RETRIES - 1:
                # Log the retry attempt to the Docker console
                print(f"[{SERVER_USER} Server] DB connection failed (Attempt {attempt+1}/{MAX_RETRIES}): {e}")
                print(f"[{SERVER_USER} Server] Retrying in {RETRY_DELAY} seconds...")
                time.sleep(RETRY_DELAY)
            else:
                print(f"[{SERVER_USER} Server] DB connection failed after {MAX_RETRIES} attempts. CRITICAL FAILURE.")
                raise # Re-raise the exception after exhausting retries

# --- Background Task: 30-Second Update ---
def database_update_task():
    print(f"[{SERVER_USER} Server] Background update task started...")
    while True:
        try:
            db = get_db_connection()
            cursor = db.cursor()
            
            # SQL to increment points and update timestamp
            sql = f"""
                UPDATE users 
                SET points = points + 1, datetime_stamp = NOW() 
                WHERE user = %s
            """
            cursor.execute(sql, (SERVER_USER,))
            db.commit()
            
            print(f"[{SERVER_USER} Server] DB updated: Points incremented.")
            cursor.close()
            db.close()
            
        except Exception as e:
            print(f"[{SERVER_USER} Server] DB Error during update: {e}")
            
        time.sleep(UPDATE_INTERVAL) # Wait 30 seconds

# --- Main Thread: Socket Server Logic ---
def run_socket_server():
    HOST = '0.0.0.0' # Bind to all interfaces for Docker networking

    try:
        s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        s.bind((HOST, SERVER_PORT))
        s.listen(5)
        print(f"[{SERVER_USER} Server] Listening on {HOST}:{SERVER_PORT}...")
    except Exception as e:
        print(f"[{SERVER_USER} Server] Socket Error: {e}")
        return

    while True:
        conn, addr = s.accept()
        print(f"[{SERVER_USER} Server] Connection established from {addr[0]}:{addr[1]}")
        
        try:
            # 1. Receive client request (simple placeholder read)
            data = conn.recv(1024)
            print(f"[{SERVER_USER} Server] Received: {data.decode().strip()}")
            
            # 2. Access database (get latest points)
            db = get_db_connection()
            cursor = db.cursor()
            sql = "SELECT points, datetime_stamp FROM users WHERE user = %s"
            cursor.execute(sql, (SERVER_USER,))
            result = cursor.fetchone()
            
            if result:
                points, timestamp = result
                response_msg = f"User: {SERVER_USER}, Points: {points}, Last Update: {timestamp.strftime('%Y-%m-%d %H:%M:%S')}\n"
            else:
                response_msg = f"User {SERVER_USER} not found.\n"
                
            cursor.close()
            db.close()
            
            # 3. Send data back to client
            conn.sendall(response_msg.encode())
            print(f"[{SERVER_USER} Server] Sent: {response_msg.strip()}")
            
        except Exception as e:
            print(f"[{SERVER_USER} Server] Communication Error: {e}")
            
        finally:
            conn.close()

if __name__ == "__main__":
    # Start the background update thread
    update_thread = threading.Thread(target=database_update_task, daemon=True)
    update_thread.start()
    
    # Run the main socket server loop
    run_socket_server()