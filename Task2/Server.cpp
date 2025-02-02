#include <iostream>
#include <thread>
#include <cstring>
#include <unistd.h>
#include <arpa/inet.h>
#include <map>
#include <mutex>

#define PORT 8080
#define BUFFER_SIZE 1024
int global_id = 0;

using namespace std;
mutex client_mutex;

map<int, pair<string, int>> clients;
void threadFunction(int client_socket, struct sockaddr_in client_addr)
{
    char buffer[BUFFER_SIZE];
    memset(buffer, 0, BUFFER_SIZE);
    int incoming_bytes = recv(client_socket, buffer, BUFFER_SIZE, 0);
    if (incoming_bytes <= 0)
    {
        close(client_socket);
        return;
    }
    int client_id=++global_id;
    
    int client_port;

    recv(client_socket, &client_port, sizeof(client_port), 0);
    client_port = ntohs(client_port);
    string client_ip = inet_ntoa(client_addr.sin_addr);
    {
        lock_guard lock(client_mutex);
        clients[global_id] = {client_ip, client_port};
    }
    cout<<"Here "<< client_ip <<endl;
    string connection_message = "Client " + to_string(client_id) + " connected from " + client_ip + ":" + to_string(client_port);
    send(client_socket, connection_message.c_str(), connection_message.length(), 0);

    while (1)
    {
        cout<<"INside here"<<endl;
        memset(buffer, 0, sizeof(buffer));
        int incoming_bytes = recv(client_socket, buffer, BUFFER_SIZE, 0);
        if (incoming_bytes <= 0)
        {
            close(client_socket);
            return;
        }
        if (strcmp(buffer, "details") == 0)
        {
            string details = "Connected clients:\n";

            
            {
                lock_guard<mutex> lock(client_mutex);
                for (const auto &list : clients)
                {
                    details += "Client ID: " + to_string(list.first) + " connected from " + list.second.first + ":" + to_string(list.second.second) + "\n";
                }
            }
            cout<<details<<endl;

            send(client_socket, details.c_str(), details.length(), 0);
        }
    }
}

int main()
{
    int server_socket = socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(PORT);

    bind(server_socket, (struct sockaddr *)&server_addr, sizeof(server_addr));

    listen(server_socket, 5);

    cout << "Waiting for the connection....." << endl;
    while (true)
    {
        struct sockaddr_in client_addr;
        socklen_t addr_size = sizeof(client_addr);
        int client_socket = accept(server_socket, (struct sockaddr *)&client_addr, &addr_size);
        thread socketThread(threadFunction, client_socket, client_addr);
        socketThread.detach();
    }
}
