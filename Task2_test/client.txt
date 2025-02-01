#include <iostream>
#include <vector>
#include <cstring>
#include <unistd.h>
#include <arpa/inet.h>
#include <netinet/in.h>

#define SERVER_IP "127.0.0.1"
#define SERVER_PORT 8080
#define BUFFER_SIZE 1024

using namespace std;

int main()
{
    string username;
    cout << "Enter username: ";
    cin >> username;

    int clientSocket = socket(AF_INET, SOCK_STREAM, 0); // Create socket
    if (clientSocket == -1)
    {
        cerr << "Socket creation failed!" << endl;
        return 1;
    }

    sockaddr_in serverAddress;
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_port = htons(SERVER_PORT);
    serverAddress.sin_addr.s_addr = inet_addr(SERVER_IP);

    // Connect to server
    if (connect(clientSocket, (struct sockaddr *)&serverAddress, sizeof(serverAddress)) == -1)
    {
        cerr << "Connection failed!" << endl;
        return 1;
    }

    // Send username to server
    send(clientSocket, username.c_str(), username.length(), 0);

    while (true) {
        cout << "Enter command (type 'exit' to quit): ";
        string command;
        cin >> command;

        if (command == "exit") {
            cout << "Closing connection..." << endl;
            break;
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

    close(clientSocket);
    return 0;
}
