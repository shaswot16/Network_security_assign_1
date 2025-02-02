import socket
import threading

clients = {}  # Store clients as {username: (IP, Port)}

def handle_client(client_socket, addr):
    try:
        username = client_socket.recv(1024).decode().strip()
        if not username:
            client_socket.close()
            return
        
        clients[username] = addr  # Store client details
        print(f"{username} connected from {addr}")

        while True:
            request = client_socket.recv(1024).decode().strip()
            if request == "LIST":
                client_list = "\n".join([f"{user} {ip}:{port}" for user, (ip, port) in clients.items()])
                client_socket.send(client_list.encode() if client_list else b"No users connected")
            elif request.startswith("EXIT"):
                break
        
    except Exception as e:
        print(f"Error handling client: {e}")
    finally:
        if username in clients:
            del clients[username]
        client_socket.close()

def start_server():
    server_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    server_socket.bind(("0.0.0.0", 8080))
    server_socket.listen(5)
    print("Server is listening on port 8080...")

    while True:
        client_socket, addr = server_socket.accept()
        threading.Thread(target=handle_client, args=(client_socket, addr)).start()

if __name__ == "__main__":
    start_server()
