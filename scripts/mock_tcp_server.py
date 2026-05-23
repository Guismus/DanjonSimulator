#!/usr/bin/env python3
import socket
import json
import random
import sys

def main():
    port = 8080
    if len(sys.argv) > 1:
        port = int(sys.argv[1])
        
    server = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    server.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
    server.bind(('127.0.0.1', port))
    server.listen(1)
    print(f"Mock TCP Server listening on 127.0.0.1:{port}...")
    
    try:
        while True:
            conn, addr = server.accept()
            print(f"Connection from {addr}")
            
            # Read until newline
            data = b""
            while b"\n" not in data:
                chunk = conn.recv(1024)
                if not chunk:
                    break
                data += chunk
                
            if not data:
                conn.close()
                continue
                
            try:
                line = data.decode('utf-8').strip()
                state = json.loads(line)
                print(f"Received state for character: {state['active_character']['name']}")
                
                # Alternate or choose actions:
                # We can respond with "Attaquer", "Parer", "Esquiver", or "Passer"
                queued = state['active_character']['queued_actions']
                if len(queued) < 2:
                    action = random.choice(["Attaquer", "Parer", "Esquiver"])
                else:
                    action = "Passer"
                    
                print(f"Responding with action: {action}")
                response = json.dumps({"action": action}) + "\n"
                conn.sendall(response.encode('utf-8'))
            except Exception as e:
                print(f"Error handling request: {e}")
            finally:
                conn.close()
    except KeyboardInterrupt:
        print("\nStopping Mock TCP Server.")
    finally:
        server.close()

if __name__ == '__main__':
    main()
