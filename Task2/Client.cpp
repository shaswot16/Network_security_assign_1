#include <cstring>
#include <iostream>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>
#include <string>
#include <string.h>

using namespace std;
int main()
{

    int clientSocket = socket(AF_INET, SOCK_STREAM, 0);

    sockaddr_in serverAddress;
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_port = htons(8080);
    serverAddress.sin_addr.s_addr = INADDR_ANY;

    connect(clientSocket, (struct sockaddr *)&serverAddress,
            sizeof(serverAddress));

    while (1)
    {
        string message;
        cout << "Enter string : ";
        getline(cin, message);
        send(clientSocket, message.c_str(), message.length(), 0);
        if (strcmp(message.c_str(), "exit") == 0)
        {
            break;
        }
        // char buffer[1024] = {0};
        // int valread = read(clientSocket, buffer, 1024);
        // if (valread > 0)
        // {
        //     buffer[valread] = '\0'; 
        //     cout << "Result from server: " << buffer << endl;
        // }
        // else
        // {
        //     cout << "No data received from server. Continuing..." << endl;
        // }
    }
    close(clientSocket);

    return 0;
}