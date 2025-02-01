#include <iostream>
#include <map>
#include <cstring>
#include <unistd.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <thread>
#include <mutex>
#include <string>

#define SERVER_PORT 8080
#define BUFFER_SIZE 1024

using namespace std;

map<string, pair<string, int>> clients; // Stores client data
mutex clients_mutex;

void handle_client(int client_socket, struct sockaddr_in client_addr)
{
    char buffer[BUFFER_SIZE];
    memset(buffer, 0, BUFFER_SIZE);

    int bytes_received = recv(client_socket, buffer, BUFFER_SIZE, 0);
    if (bytes_received <= 0)
    {
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
    cout << "Received from " << username << " at " << client_ip << ":" << client_port << endl;
    while (true)
    {
        memset(buffer, 0, BUFFER_SIZE);
        int bytes_received = recv(client_socket, buffer, BUFFER_SIZE, 0);
        if (bytes_received == 0)
        {
            cout << "Client disconnected!" << endl;
            break; // Exit the loop if the client has disconnected
        }

        // Check for errors in recv()
        if (bytes_received < 0)
        {
            cerr << "Error receiving data!" << endl;
            break;
        }

        // Print the buffer contents if data was received
        cout << "This is buffer: " << buffer << endl;
        string command(buffer);
        command.erase(0, command.find_first_not_of(" \t")); // Remove leading spaces
        command.erase(command.find_last_not_of(" \t") + 1);
        if (command == "list")
        {

            string user_list;
            {
                std::lock_guard<std::mutex> lock(clients_mutex);
                for (const auto &entry : clients)
                {
                    user_list += entry.first + " " + entry.second.first + " " + std::to_string(entry.second.second) + "\n";
                }
            }
            send(client_socket, user_list.c_str(), user_list.length(), 0);
        }
        else
        {
            string message = "Type correct Command";
            send(client_socket, message.c_str(), message.length(), 0);
        }
    }

    // Close client connection
    close(client_socket);
}

int main()
{
    int server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket == -1)
    {
        cerr << "Socket creation failed!" << endl;
        return 1;
    }

    sockaddr_in server_addr = {};
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(SERVER_PORT);

    if (bind(server_socket, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0)
    {
        cerr << "Binding failed!" << endl;
        return 1;
    }

    if (listen(server_socket, 5) < 0)
    {
        cerr << "Listening failed!" << endl;
        return 1;
    }

    cout << "Server running on port " << SERVER_PORT << endl;

    while (true)
    {
        struct sockaddr_in client_addr;
        socklen_t addr_size = sizeof(client_addr);
        int client_socket = accept(server_socket, (struct sockaddr *)&client_addr, &addr_size);

        // Handle each client connection in a separate thread
        thread(handle_client, client_socket, client_addr).detach();
    }

    close(server_socket);
    return 0;
}
