#include <arpa/inet.h>
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

void handle_client(int client_socket, struct sockaddr_in client_addr) {
  char buffer[BUFFER_SIZE];
  memset(buffer, 0, BUFFER_SIZE);

  int bytes_received = recv(client_socket, buffer, BUFFER_SIZE, 0);
  if (bytes_received <= 0) {
    close(client_socket);
    return;
  }

  string username(buffer);
  string client_ip = inet_ntoa(client_addr.sin_addr);
  int client_port = ntohs(client_addr.sin_port);

  clients_mutex.lock();
  clients[username] = {client_ip, client_port};
  clients_mutex.unlock();

  // Print received username and client details
  cout << "Received from " << username << " at " << client_ip << ":"
       << client_port << endl;
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

    } else if (command.find("connect") == 0) {
      string target_username = command.substr(8);
      target_username.erase(
          target_username.find_first_not_of(" \t")); // Trim spaces

      clients_mutex.lock();

      // Check if the target user exists and isn't already connected
      if (clients.find(target_username) != clients.end() &&
          user_sessions.find(target_username) == user_sessions.end()) {
        // Start a session between the two clients
        user_sessions[username] = target_username;
        user_sessions[target_username] = username;

        // Notify both users about the connection
        string message = "You are now connected to " + target_username;
        send(client_socket, message.c_str(), message.length(), 0);
        send(clients[target_username].first, message.c_str(), message.length(),
             0);

        cout << username << " connected to " << target_username << endl;

        // Handle the communication between the two clients
        while (!stop_server.load()) {
          memset(buffer, 0, BUFFER_SIZE);
          bytes_received = recv(client_socket, buffer, BUFFER_SIZE, 0);
          if (bytes_received <= 0) {
            break;
          }

          // Forward the message to the target user
          send(clients[target_username].first, buffer, bytes_received, 0);
        }

        // Clean up the session after communication ends
        user_sessions.erase(username);
        user_sessions.erase(target_username);

        cout << "Connection between " << username << " and " << target_username
             << " ended." << endl;
      } else {
        string error_message = "User not found or already connected.";
        send(client_socket, error_message.c_str(), error_message.length(), 0);
      }

      clients_mutex.unlock();
    } else {
      string message = "Type correct Command";
      send(client_socket, message.c_str(), message.length(), 0);
    }
  }

  // Close client connection
  close(client_socket);
}

int main() {
  // Handle outside signal to end the program
  signal(SIGINT, signal_handler);

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
    if (errno == EADDRINUSE) {
      cerr << "Error: Address already in use (EADDRINUSE)" << endl;
    } else if (errno == EACCES) {
      cerr << "Error: Permission denied (EACCES). Try running with elevated "
              "privileges."
           << endl;
    } else if (errno == ENOTSOCK) {
      cerr << "Error: The file descriptor is not a socket (ENOTSOCK)." << endl;
    } else if (errno == EADDRNOTAVAIL) {
      cerr << "Error: The requested address is not available (EADDRNOTAVAIL)."
           << endl;
    } else {
      cerr << "Error binding socket: " << strerror(errno) << endl;
    }
    return 1; // Exit the program on bind failure
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
