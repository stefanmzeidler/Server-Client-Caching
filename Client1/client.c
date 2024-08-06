#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/socket.h>

// #define PORT 8080
#define BUFFER_SIZE 1024
static char *command = NULL;
int main(int argc, char *argv[])
{
    int sockfd = 0;
    struct sockaddr_in serv_addr;
    uint16_t port;
    if (argc != 2)
    {
        printf("Incorrect number of arguments\n");
    }
    if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
    {
        printf("\n Socket creation error \n");
        return -1;
    }
    if ((port = atoi(argv[1])) == 0)
    {
        printf("Argument conversion error\n");
        return -1;
    }
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(port);
    serv_addr.sin_addr.s_addr = inet_addr("127.0.0.1");
    if (connect(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
    {
        printf("\nConnection Failed \n");
        return -1;
    }

    command = (char *)malloc(BUFFER_SIZE);
    printf("Enter command (R for read, W for write, Q for quit)\n");
    if (fgets(command, BUFFER_SIZE, stdin) == NULL)
    {
        printf("Problem getting command, stopping.");
        return -1;
    }
    command[strcspn(command, "\n")] = '\0';
    if (strcmp(command, "R") == 0)
    {
        printf("Read command received\n");
    }
    else if (strcmp(command, "W") == 0)
    {
        printf("Write command received\n");
    }
    else if (strcmp(command, "Q") == 0)
    {
        return -1;
    }
    else
    {
        printf("Invalid command\n");
    }
    char greeting[BUFFER_SIZE] = {0};
    strcpy(greeting, "Hello from client!");
    write(sockfd, greeting, BUFFER_SIZE);

    char response[BUFFER_SIZE] = {0};
    read(sockfd, response, BUFFER_SIZE);
    printf("%s\n", response);

    close(sockfd);
    return 0;
}
