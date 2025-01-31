#include <stdlib.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#define PORT 8080

int main(int argc,char const* argv[]){
    int status ,valread, client_fd;
    struct sockaddr_in serv_addr;
    char *hello = "Hello from client";
    char buffer[1024] ={0};
    if((client_fd = socket(AF_INET, SOCK_STREAM,0))<0){
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(PORT);
    serv_addr.sin_addr.s_addr = inet_addr("127.0.0.1");

    if(connect(client_fd, (struct sockaddr *)&serv_addr,sizeof(serv_addr))<0){
        perror("Connection failed");
        exit(EXIT_FAILURE);
    }
    printf("Connected to the server \n");

    send(client_fd,hello,strlen(hello),0);
    printf("Hello message sent \n");
    valread = read(client_fd,buffer,1024-1);
    printf("%s\n",buffer);
    close(client_fd);
    return 0;
}