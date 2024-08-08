#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include "client.h"

#define PORT 8080
#define BUFFER_SIZE 1024

int main(int argc, char *argv[]) {
    int sockfd = 0;
    struct sockaddr_in serv_addr;

    if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        printf("\n Socket creation error \n");
        return -1;
    }

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(PORT);
    serv_addr.sin_addr.s_addr = inet_addr("127.0.0.1");

    if (connect(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        printf("\nConnection Failed \n");
        return -1;
    }

    send_read(sockfd);

    close(sockfd);
    return 0;
}

void send_read(int sockfd)
{
    char greeting[BUFFER_SIZE] = {0};
    strcpy(greeting, "Hello from client!");
    write(sockfd, greeting, BUFFER_SIZE);

    char response[BUFFER_SIZE] = {0};
    read(sockfd, response, BUFFER_SIZE);
    printf("%s\n", response);
}
