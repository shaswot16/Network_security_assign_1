import socket
import threading

def receive_messages(peer_socket):
    while True:
        try:
            msg = peer_socket.recv(1024).decode().strip()
            if msg == "EOM":
                print("[Chat Ended]")
                break
            print(f"[Received]: {msg}")
        except:
            break
    peer_socket.close()

def chat(peer_ip, peer_port):
    peer_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    peer_socket.connect((peer_ip, int(peer_port)))

    threading.Thread(target=receive_messages, args=(peer_socket,)).start()

    while True:
        msg = input("[Send]: ")
        peer_socket.send(msg.encode())
        if msg == "EOM":
            break
    peer_socket.close()

def client_program():
    client_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    client_socket.connect(("127.0.0.1", 8080))

    username = input("Enter your username: ")
    client_socket.send(username.encode())

    while True:
        print("\n1. List Users\n2. Connect to User\n3. Exit")
        choice = input("Enter choice: ")

        if choice == "1":
            client_socket.send("LIST".encode())
            print(client_socket.recv(1024).decode())

        elif choice == "2":
            user_ip, user_port = input("Enter peer IP and Port (e.g. 127.0.0.1 5000): ").split()
            chat(user_ip, user_port)

        elif choice == "3":
            client_socket.send("EXIT".encode())
            break

if __name__ == "__main__":
    client_program()
