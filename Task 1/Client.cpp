#include <iostream>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <string.h>

using namespace std;

int main() {
    int sock = 0;
    struct sockaddr_in serv_addr;

    // Create socket
    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("Socket creation error");
        return -1;
    }

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(8080);

    // Convert IPv4 and IPv6 addresses from text to binary form
    if (inet_pton(AF_INET, "127.0.0.1", &serv_addr.sin_addr) <= 0) {
        perror("Invalid address");
        return -1;
    }

    // Connect to the server
    if (connect(sock, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) < 0) {
        perror("Connection failed");
        return -1;
    }

    string expression;
    while (true) {
        cout << "Enter a mathematical expression (or 'exit' to quit): ";
        getline(cin, expression);

        if (expression == "exit") break;

        send(sock, expression.c_str(), expression.length(), 0);

        char buffer[1024] = {0};
        int valread = read(sock, buffer, 1024);
        buffer[valread] = '\0';
        
        cout << "Result from server: " << buffer << endl;
    }

    close(sock);
    return 0;
}
