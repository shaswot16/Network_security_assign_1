#include <cstring>
#include <iostream>
#include <string>
#include <thread>
#include <vector>
#include <arpa/inet.h>
#include <unistd.h>

#define SERVER_PORT 8080
#define BUFFER_SIZE 1024

void receive_messages(int socket)
{
    char buffer[BUFFER_SIZE];
    while (true)
    {
        memset(buffer, 0, BUFFER_SIZE);
        int bytes_received = recv(socket, buffer, BUFFER_SIZE, 0);
        if (bytes_received <= 0)
            break;
        std::cout << "[Received]: " << buffer << std::endl;
    }
    close(socket);
}

void start_p2p_chat(const std::string &peer_ip, int peer_port)
{
    int peer_socket = socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in peer_addr;
    peer_addr.sin_family = AF_INET;
    peer_addr.sin_port = htons(peer_port);
    inet_pton(AF_INET, peer_ip.c_str(), &peer_addr.sin_addr);

    if (connect(peer_socket, (struct sockaddr *)&peer_addr, sizeof(peer_addr)) < 0)
    {
        std::cerr << "Failed to connect to peer!\n";
        return;
    }

    std::cout << "Connected to peer at " << peer_ip << ":" << peer_port << "\n";
    std::thread receive_thread(receive_messages, peer_socket);

    while (true)
    {
        std::string message;
        std::getline(std::cin, message);
        send(peer_socket, message.c_str(), message.length(), 0);
        if (message == "EOM")
            break;
    }

    close(peer_socket);
    receive_thread.join();
}

int main()
{
    std::string username, server_ip;
    std::cout << "Enter username: ";
    std::cin >> username;
    std::cout << "Enter server IP: ";
    std::cin >> server_ip;

    int server_socket = socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(SERVER_PORT);

    // Convert IPv4 address from text to binary form
    if (inet_pton(AF_INET, server_ip.c_str(), &server_addr.sin_addr) <= 0) {
        perror("Invalid address");
        return -1;
    }

    // Connect to the server
    if (connect(server_socket, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        perror("Connection failed");
        return -1;
    }

    // Send username to the server
    send(server_socket, username.c_str(), username.length(), 0);

    // Skip binding the client socket, let the OS handle it
    socklen_t len = sizeof(server_addr);
    int client_port;
    if (getsockname(server_socket, (struct sockaddr*)&server_addr, &len) == -1) {
        perror("getsockname failed");
        close(server_socket);
        return -1;
    }

    // client_port = ntohs(server_addr.sin_port);
    // send(server_socket, &client_port, sizeof(client_port), 0);

    std::cout << "Connected to server as " << username << "\n";

    while (true)
    {
        std::cout << "Enter command (/list to see users, /connect <user> to chat): ";
        std::string command;
        std::cin >> command;

        if (command == "/list")
        {
            send(server_socket, command.c_str(), command.length(), 0);

            char buffer[BUFFER_SIZE];
            memset(buffer, 0, BUFFER_SIZE);
            int bytes_received = recv(server_socket, buffer, BUFFER_SIZE, 0);

            if (bytes_received > 0)
            {
                std::cout << "Active users:\n" << buffer << std::endl;
            }
            else
            {
                std::cout << "No response from the server or no active users.\n";
            }
        }
        else if (command == "/connect")
        {
            // User is attempting to connect to another user
            std::string peer_ip;
            int peer_port;
            std::cout << "Enter peer IP: ";
            std::cin >> peer_ip;
            std::cout << "Enter peer port: ";
            std::cin >> peer_port;
            start_p2p_chat(peer_ip, peer_port);
        }
        else continue;  // Ignore invalid commands or do nothing
    }

    close(server_socket);
    return 0;
}
