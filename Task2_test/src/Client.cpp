#include <arpa/inet.h>
#include <cstring>
#include <iostream>
#include <netinet/in.h>
#include <thread>
#include <unistd.h>
#include <vector>

#define SERVER_IP "127.0.0.1"
#define SERVER_PORT 8080
#define BUFFER_SIZE 1024

using namespace std;
bool in_p2p_chat = false;
// Function to receive messages from a peer
void receive_messages(int peer_socket) {
  char buffer[BUFFER_SIZE];
  while (true) {
    memset(buffer, 0, BUFFER_SIZE);
    int bytes_received = recv(peer_socket, buffer, BUFFER_SIZE, 0);
    if (bytes_received <= 0) {
      cout << "[Peer Disconnected]\n";
      break;
    }
    // Clear current input line
    cout << "\33[2K\r";
    cout.flush();

    // Print received message
    cout << "[Received]: " << buffer << endl;

    // Reprint [Send]: for user input
    cout << "[Send]: ";
    cout.flush();
  }
  close(peer_socket);
}

// Function to handle a connected peer (both sending & receiving)
void handle_peer(int peer_socket) {

  in_p2p_chat = true;
  cout << "Peer connected!\n";

  thread receive_thread(receive_messages, peer_socket);

  // Send messages to peer
  string message;
  while (true) {
    cout << "[Send]:";
    getline(cin, message);
    if (send(peer_socket, message.c_str(), message.length(), 0) == -1) {
      cerr << "Error sending message\n";
      break;
    }
    if (message == "EOM")
      break;
  }

  close(peer_socket);
  receive_thread.join(); // Ensure receiving thread finishes before exiting
  in_p2p_chat = false;
}

// Function to listen for incoming peer connections
void listen_for_clients(int listen_socket) {
  while (true) {
    struct sockaddr_in client_addr;
    socklen_t addr_size = sizeof(client_addr);

    int peer_socket =
        accept(listen_socket, (struct sockaddr *)&client_addr, &addr_size);
    if (peer_socket == -1) {
      cerr << "Error accepting peer connection!\n";
      continue;
    }

    thread peer_thread(handle_peer, peer_socket);
    peer_thread.detach();
  }
}

// Function to initiate a P2P chat with another client
void start_p2p_chat(int peer_port, int client_socket) {
  string command = "connect " + to_string(peer_port);
  send(client_socket, command.c_str(), command.length(), 0);

  // Wait for the server to respond with connection details
  char buffer[BUFFER_SIZE];
  memset(buffer, 0, BUFFER_SIZE);
  int bytes_received = recv(client_socket, buffer, BUFFER_SIZE, 0);
  if (bytes_received > 0) {
    string response(buffer);
    if (response == "Client not found.\n") {
      cout << "No peer found at that port.\n";
    } else {
      string peer_ip = "127.0.0.1";
      int peer_socket = socket(AF_INET, SOCK_STREAM, 0);
      if (peer_socket == -1) {
        cerr << "Failed to create P2P socket!\n";
        return;
      }

      struct sockaddr_in peer_addr;
      peer_addr.sin_family = AF_INET;
      peer_addr.sin_port = htons(peer_port);
      inet_pton(AF_INET, peer_ip.c_str(), &peer_addr.sin_addr);

      if (connect(peer_socket, (struct sockaddr *)&peer_addr,
                  sizeof(peer_addr)) == -1) {
        cerr << "Failed to connect to peer!\n";
        close(peer_socket);
        return;
      }

      cout << "Connected to peer at " << peer_ip << ":" << peer_port << "\n";
      handle_peer(peer_socket); // Start two-way communication    }
    }
  }
}
int main() {
  string username;
  cout << "Enter username: ";
  cin >> username;

  // Client socket to connect to server
  int clientSocket = socket(AF_INET, SOCK_STREAM, 0);
  if (clientSocket == -1) {
    cerr << "Socket creation failed!" << endl;
    return 1;
  }

  sockaddr_in serverAddress;
  serverAddress.sin_family = AF_INET;
  serverAddress.sin_port = htons(SERVER_PORT);
  serverAddress.sin_addr.s_addr = inet_addr(SERVER_IP);

  // Connect to server
  if (connect(clientSocket, (struct sockaddr *)&serverAddress,
              sizeof(serverAddress)) == -1) {
    cerr << "Connection failed!" << endl;
    return 1;
  }

  // P2P listening socket
  int listen_socket = socket(AF_INET, SOCK_STREAM, 0);
  if (listen_socket == -1) {
    cerr << "Failed to create listening socket!\n";
    return 1;
  }

  struct sockaddr_in listen_addr;
  listen_addr.sin_family = AF_INET;
  listen_addr.sin_port = htons(0); // Bind to any available port
  listen_addr.sin_addr.s_addr = INADDR_ANY;

  if (::bind(listen_socket, (struct sockaddr *)&listen_addr,
             sizeof(listen_addr)) == -1) {
    cerr << "Bind failed!\n";
    return 1;
  }

  if (listen(listen_socket, 5) == -1) {
    cerr << "Listen failed!\n";
    return 1;
  }

  // Retrieve assigned port
  struct sockaddr_in bound_addr;
  socklen_t bound_len = sizeof(bound_addr);
  getsockname(listen_socket, (struct sockaddr *)&bound_addr, &bound_len);
  int client_port = ntohs(bound_addr.sin_port);

  cout << "Listening for P2P connections on port: " << client_port << endl;

  // Send username and port to server
  string user_info = username + ":" + to_string(client_port);
  send(clientSocket, user_info.c_str(), user_info.length(), 0);

  thread listener_thread(listen_for_clients, listen_socket);
  listener_thread.detach(); // Run in background

  while (true) {
    if (in_p2p_chat) {
      sleep(5); // Pause for 5 seconds
      continue; // Skip this iteration and wait for the user to finish
    }
    cout << "Enter command (type 'exit' to quit): ";
    string command;
    cin >> command;

    if (command == "exit") {
      cout << "Closing connection..." << endl;
      break;
    }

    if (command == "connect") {
      int peer_port;
      cout << "Enter Peer Port: ";
      cin >> peer_port;
      cin.ignore();
      start_p2p_chat(peer_port, clientSocket);
      continue;
    }

    // Send command to server
    send(clientSocket, command.c_str(), command.length(), 0);

    // Receive server response
    char buffer[BUFFER_SIZE];
    memset(buffer, 0, BUFFER_SIZE);
    int bytes_received = recv(clientSocket, buffer, BUFFER_SIZE, 0);

    if (bytes_received == -1) {
      cerr << "Error receiving data!" << endl;
      break;
    } else if (bytes_received == 0) {
      cout << "Server closed the connection." << endl;
      break;
    } else {
      cout << "Server response: " << buffer << endl;
    }
  }

  // Close sockets
  close(clientSocket);
  close(listen_socket);
  return 0;
}
