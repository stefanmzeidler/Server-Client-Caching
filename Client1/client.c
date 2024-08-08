/**
 * Author: Stefan Zeidler
 *
 * This program simulates a client asking a server to execute remote commands.
 *
 * Resources used:
 * CS351 Homework Assignment for powers of two bitwise comparison.
 *
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <time.h>

// #define PORT 8080
#define BUFFER_SIZE 1024
static char *command = NULL;
static int port;
// void *read_from_cache(char *filename)
// {
// }

int openSocket(uint16_t port)
{
    int sockfd = 0;
    struct sockaddr_in serv_addr;
    if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
    {
        printf("\n Socket creation error \n");
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
    return sockfd;
}

void update_cache(char *filename, int sockfd)
{
    int fd;
    if ((fd = open(filename, O_CREAT | O_WRONLY | O_TRUNC, 0666)) == -1)
    {
        printf("file open failed\n");
        exit(-1);
    }
    char protocol[BUFFER_SIZE] = {0};
    strcpy(protocol, "R ");
    strcat(protocol, filename);
    write(sockfd, protocol, BUFFER_SIZE);
    char *response = malloc(BUFFER_SIZE);
    // read(sockfd, response, BUFFER_SIZE);
    // write(fd, response, strlen(response));
    int bytes;
    while ((bytes = read(sockfd, response, BUFFER_SIZE)) > 0)
    {
        // printf("%s\n", response);
        write(fd, response, bytes);
    }
    free(response);
    close(fd);
}

char *readfile(char *filename)
{

    struct stat *data = malloc(sizeof(struct stat));
    if (stat(filename, data) == -1 || (time(NULL) - data->st_mtime) > 1)
    {
        int sockfd = openSocket(port);
        update_cache(filename, sockfd);
        // close(sockfd);
    }
    else{
        printf("file up to date\n");
    }
    free(data);
    return NULL;
}

int main(int argc, char *argv[])
{
    if (argc != 2)
    {
        printf("Incorrect number of arguments\n");
        return -1;
    }
    if ((port = atoi(argv[1])) == 0)
    {
        printf("Argument conversion error\n");
        return -1;
    }
    // int sockfd = 0;
    // struct sockaddr_in serv_addr;
    // if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
    // {
    //     printf("\n Socket creation error \n");
    //     return -1;
    // }
    // serv_addr.sin_family = AF_INET;
    // serv_addr.sin_port = htons(port);
    // serv_addr.sin_addr.s_addr = inet_addr("127.0.0.1");
    // if (connect(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
    // {
    //     printf("\nConnection Failed \n");
    //     return -1;
    // }
    int sockfd = openSocket(port);
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
            readfile("foo.txt");
            // printf("%s\n", readfile("foo.txt"));
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
