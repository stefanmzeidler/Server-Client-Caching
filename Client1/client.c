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
#define MAX_FILENAME 256
#define MAX_DATA BUFFER_SIZE - sizeof(char) - MAX_FILENAME
#define READ 'R'
#define TIMEOUT 60
static char *command = NULL;
static int port;
const char *ACK = "ACK";
typedef struct _package
{
    char command;
    char filename[MAX_FILENAME];
    char data[MAX_DATA];
} Package;

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

void pack(Package *p, char *buffer)
{
    size_t offset = 0;
    memcpy(buffer + offset, &(p->command), sizeof(char));
    offset += sizeof(char);
    memcpy(buffer + offset, p->filename, sizeof(p->filename));
    offset += MAX_FILENAME;
    memcpy(buffer + offset, p->data, sizeof(p->data));
}

int update_cache(char *filename)
{
    int sockfd = openSocket(port);
    int fd;
    if ((fd = open(filename, O_CREAT | O_WRONLY | O_TRUNC, 0666)) == -1)
    {
        printf("cache update failed\n");
        return -1;
    }
    Package _message;
    Package *message = &_message;
    message->command = READ;
    strcpy(message->filename, filename);
    printf("Filename: %s\n", message->filename);;
    printf("Sending read command\n");
    char buffer[BUFFER_SIZE];
    pack(message, buffer);
    write(sockfd, buffer, BUFFER_SIZE);
    char *response = malloc(BUFFER_SIZE);
    int bytes;
    bytes = read(sockfd, response, BUFFER_SIZE);
    if(strcmp(response, ACK) == 0){
        printf("Ack received\n");
    }
    else{
        printf("Cache update failed\n");
        unlink(filename);
        return -1;
    }
    while ((bytes = read(sockfd, response, BUFFER_SIZE)) > 0)
    {
        // printf("%s\n", response);
        write(fd, response, bytes);
    }

    free(response);
    close(fd);
    close(sockfd);
    return 0;
}

int readfile(char *filename)
{
    char *source = malloc(BUFFER_SIZE);
    struct stat *data = malloc(sizeof(struct stat));
    if (stat(filename, data) == -1 || (time(NULL) - data->st_mtime) > TIMEOUT)
    {
        if (update_cache(filename) == -1)
        {
            return -1;
        }
        source = "Reading content from server: ";
        // file_data temp = {.filename = filename, .offset = 0};
        // cache[find(filename, 0)] = &temp;
    }
    else
    {
        source = "Reading content from cache: ";
    }
    int fd;
    if ((fd = open(filename, O_RDONLY, 0666)) == -1)
    {
        // printf("file open failed\n");
        return -1;
    }
    printf("%s", source);
    char *buffer = malloc(BUFFER_SIZE);
    while ((read(fd, buffer, BUFFER_SIZE)) > 0)
    {
        printf("%s", buffer);
    }
    stat(filename, data);
    printf("\nCache expires in %ld seconds\n", TIMEOUT - (time(NULL) - data->st_mtime));
    free(buffer);
    free(data);
    return 0;
}

char *getFileName()
{
    char *fileName = malloc(MAX_FILENAME);
    printf("Enter filename: ");
    if ((fgets(fileName, MAX_FILENAME, stdin)) == NULL)
    {
        printf("Failed to get filename, aborting\n");
        exit(-1);
    }
    fileName[strcspn(fileName, "\n")] = '\0';
    return fileName;
}

int writeFile(char *filename)
{
    int sockfd = openSocket(port);
    char protocol[BUFFER_SIZE] = {0};
    strcpy(protocol, "W ");
    strcat(protocol, filename);
    write(sockfd, protocol, BUFFER_SIZE);
    char *response = malloc(BUFFER_SIZE);
    int bytes;
    if ((bytes = read(sockfd, response, BUFFER_SIZE)) < 1 || strcmp(response, ACK) != 0)
    {
        return -1;
    }
    update_cache(filename);
    return 0;
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
            if ((readfile(getFileName())) == -1)
            {
                printf("Failed to read file\n");
            }
        }
        else if (strcmp(command, "W") == 0)
        {
            if (writeFile(getFileName()) != 0)
            {
                printf("Write to file failed. \n");
            }
            else
            {
                printf("Write succeeded.\n");
            }
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
