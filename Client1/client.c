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
static const int INITIAL_CAPACITY = 8;

typedef struct _cache_t
{
    FILE *file;
    int offset;

} cache_t;

cache_t cache[INITIAL_CAPACITY];

unsigned long hash(unsigned char *str)
{
    unsigned long hash = 5381;
    int cache_length = sizeof(cache) / sizeof(cache_t);
    int c;

    while (c = *str++)
    {
        hash = ((hash << 5) + hash) + c; /* hash * 33 + c */
    }
    hash = hash % cache_length;
    if (hash < 0)
    {
        hash += cache_length;
    }
    return hash;
}

char *send_read(int sockfd)
{
    char protocol[BUFFER_SIZE] = {0};
    strcpy(protocol, command);
    write(sockfd, protocol, BUFFER_SIZE);

    char response[BUFFER_SIZE] = {0};
    read(sockfd, response, BUFFER_SIZE);
    return response;
}
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
    while (1)
    {
        printf("Enter command (R for read, W for write, Q for quit)\n");
        if (fgets(command, BUFFER_SIZE, stdin) == NULL)
        {
            printf("Problem getting command, stopping.");
            return -1;
        }
        command[strcspn(command, "\n")] = '\0';
        if (strcmp(command, "R") == 0)
        {
            printf("Sending read command\n");
            printf(send_read(sockfd));
        }
        else if (strcmp(command, "W") == 0)
        {
            printf("Write command received\n");
        }
        else if (strcmp(command, "Q") == 0)
        {
            break;
        }
        else
        {
            printf("Invalid command\n");
        }
        // char greeting[BUFFER_SIZE] = {0};
        // strcpy(greeting, "Hello from client!");
        // write(sockfd, greeting, BUFFER_SIZE);

        // char response[BUFFER_SIZE] = {0};
        // read(sockfd, response, BUFFER_SIZE);
        // printf("%s\n", response);
    }
    free(command);
    close(sockfd);

    return 0;
}
