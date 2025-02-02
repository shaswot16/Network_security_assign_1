#include <arpa/inet.h>
#include <cstring>
#include <iostream>
#include <map>
#include <mutex>
#include <netinet/in.h>
#include <string>
#include <sys/socket.h>
#include <thread>
#include <unistd.h>

#define SERVER_PORT 8080
#define BUFFER_SIZE 1024

using namespace std;

map<string, pair<string, int>> clients; // Stores client data
mutex clients_mutex;

int server_socket; // To store the server socket for cleanup

atomic<bool> stop_server(false);

void signal_handler(int signum) {
  switch (signum) {
  case SIGINT:
    cout << "\nReceived SIGINT (Ctrl+C). Shutting down..." << endl;
    break;
  case SIGTERM:
    cout << "\nReceived SIGTERM. Terminating gracefully..." << endl;
    break;
  case SIGQUIT:
    cout << "\nReceived SIGQUIT. Quitting..." << endl;
    break;
  case SIGABRT:
    cout << "\nReceived SIGABRT. Abort signal received." << endl;
    break;
  case SIGHUP:
    cout << "\nReceived SIGHUP. Hangup signal received." << endl;
    break;
  case SIGSEGV:
    cout << "\nReceived SIGSEGV. Segmentation fault occurred!" << endl;
    break;
  default:
    cout << "\nReceived signal " << signum << ". Shutting down..." << endl;
    break;
  }

  // Signal threads to stop
  stop_server.store(true);

  // Close the server socket to release the port
  if (server_socket != -1) {

    close(server_socket);
  }

  // Cleanup other resources if needed (such as terminating threads or clients)
  exit(signum); // Exit the program
}

void handle_connect_request(int client_socket, const string &command) {
  // Parse the connect command: "connect <username>"
  size_t delimiter_pos = command.find(' ');
  if (delimiter_pos == string::npos) {
    string message = "Invalid command format. Usage: connect <port>\n";
    send(client_socket, message.c_str(), message.length(), 0);
    return;
  }

  int target_port = stoi(command.substr(delimiter_pos + 1));

  pair<string, int> target_client;
  bool found = false;

  // Search for the client by port
  {
    lock_guard<mutex> lock(clients_mutex);
    for (const auto &entry : clients) {
      if (entry.second.second == target_port) { // Matching port
        target_client = entry.second;
        found = true;
        break;
      }
    }
  }

  if (!found) {
    string message = "Client not found.\n";
    send(client_socket, message.c_str(), message.length(), 0);
    return;
  }

  // Send the connection details to the requesting client
  string connect_message = "Connected to " + target_client.first + ":" +
                           to_string(target_client.second);
  send(client_socket, connect_message.c_str(), connect_message.length(), 0);
}
void handle_client(int client_socket, struct sockaddr_in client_addr) {
  char buffer[BUFFER_SIZE];
  memset(buffer, 0, BUFFER_SIZE);

  int bytes_received = recv(client_socket, buffer, BUFFER_SIZE, 0);
  if (bytes_received <= 0) {
    close(client_socket);
    return;
  }

  // The message is sent like shaswot:1234
  string received_data(buffer);
  string client_ip = inet_ntoa(client_addr.sin_addr);
  size_t delimiter_pos = received_data.find(':');
  if (delimiter_pos != string::npos) {
    string username = received_data.substr(0, delimiter_pos);
    int client_port = std::stoi(received_data.substr(delimiter_pos + 1));
    {
      std::lock_guard<std::mutex> lock(clients_mutex);
      clients[username] = {inet_ntoa(client_addr.sin_addr), client_port};
    }

    std::cout << username << " connected from "
              << inet_ntoa(client_addr.sin_addr) << ":" << client_port
              << std::endl;
  }

  while (!stop_server.load()) {
    memset(buffer, 0, BUFFER_SIZE);
    int bytes_received = recv(client_socket, buffer, BUFFER_SIZE, 0);
    if (bytes_received == 0) {
      cout << "Client disconnected!" << endl;
      break; // Exit the loop if the client has disconnected
    }
    // Check for errors in recv()
    if (bytes_received < 0) {
      cerr << "Error receiving data!" << endl;
      break;
    }

    // Print the buffer contents if data was received
    cout << "This is buffer: " << buffer << endl;
    string command(buffer);
    command.erase(0, command.find_first_not_of(" \t")); // Remove leading spaces
    command.erase(command.find_last_not_of(" \t") + 1);
    if (command == "list") {

      string user_list;
      {
        std::lock_guard<std::mutex> lock(clients_mutex);
        for (const auto &entry : clients) {
          user_list += entry.first + " " + entry.second.first + " " +
                       std::to_string(entry.second.second) + "\n";
        }
      }
      send(client_socket, user_list.c_str(), user_list.length(), 0);
    } else if (command.substr(0, 7) == "connect") {
      handle_connect_request(client_socket, command);
    } else {
      string message = "Type correct Command";
      send(client_socket, message.c_str(), message.length(), 0);
    }
  }

  close(client_socket);
}

int main() {
  signal(SIGINT, signal_handler);  // Ctrl+C
  signal(SIGTERM, signal_handler); // Termination signal
  signal(SIGQUIT, signal_handler); // Quit signal
  signal(SIGABRT, signal_handler); // Abort signal
  signal(SIGHUP, signal_handler);  // Hangup signal
  signal(SIGSEGV, signal_handler); // Segmentation fault signal
  int server_socket = socket(AF_INET, SOCK_STREAM, 0);
  if (server_socket == -1) {
    cerr << "Socket creation failed!" << endl;
    return 1;
  }

  sockaddr_in server_addr = {};
  server_addr.sin_family = AF_INET;
  server_addr.sin_addr.s_addr = INADDR_ANY;
  server_addr.sin_port = htons(SERVER_PORT);

  if (::bind(server_socket, (struct sockaddr *)&server_addr,
             sizeof(server_addr)) < 0) {
    cerr << "Binding failed!" << endl;
    return 1;
  }

  if (listen(server_socket, 5) < 0) {
    cerr << "Listening failed!" << endl;
    return 1;
  }

  cout << "Server running on port " << SERVER_PORT << endl;

  while (!stop_server.load()) {
    struct sockaddr_in client_addr;
    socklen_t addr_size = sizeof(client_addr);
    int client_socket =
        accept(server_socket, (struct sockaddr *)&client_addr, &addr_size);
    // Handle each client connection in a separate thread
    thread(handle_client, client_socket, client_addr).detach();
  }

  close(server_socket);
  return 0;
}
