#include <iostream>
#include <string>
#include <map>
#include <thread>
#include <mutex>
#include <arpa/inet.h>
#include <unistd.h>
#include <cstring>

#define SERVER_PORT 8080
#define BUFFER_SIZE 1024

std::map<std::string, std::pair<std::string, int>> clients; // username -> (IP, port)
std::mutex clients_mutex;

void handle_client(int client_socket, struct sockaddr_in client_addr) {
    char buffer[BUFFER_SIZE];
    memset(buffer, 0, BUFFER_SIZE);
    
    // Receive username
    int bytes_received = recv(client_socket, buffer, BUFFER_SIZE, 0);
    if (bytes_received <= 0) {
        close(client_socket);
        return;
    }
    std::string username(buffer);
    
    int client_port;
    recv(client_socket, &client_port, sizeof(client_port), 0);
    client_port = ntohs(client_port);
    
    std::string client_ip = inet_ntoa(client_addr.sin_addr);
    
    // Store client info
    {
        std::lock_guard<std::mutex> lock(clients_mutex);
        clients[username] = {client_ip, client_port};
    }
    
    std::cout << username << " connected from " << client_ip << ":" << client_port << std::endl;
    
    while (true) {
        memset(buffer, 0, BUFFER_SIZE);
        int bytes_received = recv(client_socket, buffer, BUFFER_SIZE, 0);
        if (bytes_received <= 0) break;
        
        std::string command(buffer);
        if (command == "t") {
            std::string user_list;
            {
                std::lock_guard<std::mutex> lock(clients_mutex);
                for (const auto& entry : clients) {
                    user_list += entry.first + " " + entry.second.first + " " + std::to_string(entry.second.second) + "\n";
                }
            }
            send(client_socket, user_list.c_str(), user_list.length(), 0);
        }
        else continue;
    }
    
    {
        std::lock_guard<std::mutex> lock(clients_mutex);
        clients.erase(username);
    }
    
    close(client_socket);
    std::cout << username << " disconnected." << std::endl;
}

int main() {
    int server_socket = socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(SERVER_PORT);
    
    bind(server_socket, (struct sockaddr*)&server_addr, sizeof(server_addr));
    listen(server_socket, 5);
    
    std::cout << "Server running on port " << SERVER_PORT << std::endl;
    
    while (true) {
        struct sockaddr_in client_addr;
        socklen_t addr_size = sizeof(client_addr);
        int client_socket = accept(server_socket, (struct sockaddr*)&client_addr, &addr_size);
        
        std::thread(handle_client, client_socket, client_addr).detach();
    }
    
    close(server_socket);
    return 0;
}
