#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#define PORT 8080

int main(int argc, char const* argv[]){
    int server_fd,new_socket;
    ssize_t valread;
    struct sockaddr_in address;
    int opt = 1 ;
    socklen_t addrlen = sizeof(address);
    char buffer[1024]= { 0 };
    char *hello = "Hello From Server";

    // Creating socket File descriptor

    if((server_fd = socket(AF_INET,SOCK_STREAM,0))<0){
        perror("Socket Failed");
        exit(EXIT_FAILURE);
    }

    // attaching socket to the port 

    int socket_option = setsockopt(server_fd, SOL_SOCKET,SO_REUSEADDR , &opt ,sizeof(opt));

    if(socket_option){
        perror("setsockopt");
        exit(EXIT_FAILURE);
    }

    // Configure address struct 

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY ;
    address.sin_port = htons(PORT);

    // Binding IP and PORT

    int ip_port = bind(server_fd,(struct sockaddr*)&address,sizeof(address));

    if(ip_port<0){
        perror("Bind Failed");
        exit(EXIT_FAILURE);
    }

    printf("The connection is in port: %d",PORT);

    if(listen(server_fd,3)<0){
        perror("Listen");
        exit(EXIT_FAILURE);
    }

    //Accepting a client connection 
    if ((new_socket
         = accept(server_fd, (struct sockaddr*)&address,
                  &addrlen))
        < 0) {
        perror("accept");
        exit(EXIT_FAILURE);
    }
    valread = read(new_socket,buffer,1024-1);
    printf("%s\n",buffer);
    send(new_socket,hello,strlen(hello),0);
    printf("Hello message sent\n");
    close(new_socket);
    close(server_fd);
    return 0;



}