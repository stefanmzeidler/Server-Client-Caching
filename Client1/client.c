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
#include <dirent.h>

// #define PORT 8080
#define BUFFER_SIZE 1024
#define MAX_FILENAME 256
#define MAX_DATA BUFFER_SIZE - sizeof(char) - MAX_FILENAME
#define CACHE "./cache"
const char READ = 'R';
const char WRITE = 'W';
const char QUERY = 'Q';
const char CONTINUE = 'C';
const char END = 'E';
const char *ERR = "ERR";
// #define WRITE 'W'
#define EMPTY ""
#define TIMEOUT 60
const char EOT = 0x04;
static char *command = NULL;
static int port;
const char *ACK = "ACK";
int mode = 1;
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
unsigned int checksum(char *filepath)
{
    unsigned int sum = 0;
    int fd;
    if ((fd = open(filepath, O_RDONLY, 0666)) == -1)
    {
        return -1;
    }
    char *buffer = malloc(BUFFER_SIZE);
    char *ptr = buffer;
    while ((read(fd, buffer, BUFFER_SIZE)) > 0)
    {
        while (*buffer)
        {
            sum += *buffer;
            buffer++;
        }
        buffer = ptr;
    }
    printf("%u\n", sum);
    return sum;
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

int validateCache(char *filename)
{
    chdir(CACHE);
    struct stat *data = malloc(sizeof(struct stat));
    if (stat(filename, data) == -1)
    {
        return 0;
    }
    if (mode == 1)
    {
        if ((time(NULL) - data->st_mtime) > TIMEOUT)
        {
            printf("Cache invalid\n");
            return 0;
        }
        else
        {
            return 1;
        }
    }
    else
    {
        int sockfd = openSocket(port);

        Package _message;
        Package *message = &_message;
        message->command = QUERY;
        strcpy(message->filename, filename);
        printf("Sending query command\n");
        char buffer[BUFFER_SIZE];
        pack(message, buffer);
        write(sockfd, buffer, BUFFER_SIZE);
        char *response = malloc(BUFFER_SIZE);
        read(sockfd, response, BUFFER_SIZE);
        char *endptr;
        unsigned int s_checksum = strtoul(response, &endptr, 10);
        unsigned int c_checksum = checksum(filename);
        if (s_checksum != c_checksum)
        {
            return 0;
        }
        return 1;
    }
}
int update_cache(char *filename)
{
    int sockfd = openSocket(port);
    chdir(CACHE);
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
    printf("Sending read command\n");
    char buffer[BUFFER_SIZE];
    pack(message, buffer);
    write(sockfd, buffer, BUFFER_SIZE);

    // write(sockfd, READ, strlen(READ));
    // write(sockfd, filename, strlen(filename));

    char *response = malloc(BUFFER_SIZE);
    int bytes;
    bytes = read(sockfd, response, BUFFER_SIZE);
    if (strcmp(response, ERR) == 0)
    {
        unlink(filename);
        return -1;
    }
    while ((bytes = read(sockfd, response, BUFFER_SIZE)) > 0)
    {
        write(fd, response, bytes);
    }
    write(sockfd, EMPTY, 0);
    free(response);
    close(fd);
    close(sockfd);
    return 0;
}

int readfile(char *filename)
{
    chdir(CACHE);
    char *source = malloc(BUFFER_SIZE);
    struct stat *data = malloc(sizeof(struct stat));
    if (!validateCache(filename))
    {
        if (update_cache(filename) == -1)
        {
            printf("Cache update failed\n");
            return -1;
        }
        source = "Reading content from server: ";
    }
    else
    {
        source = "Reading content from cache: ";
    }
    int fd;
    if ((fd = open(filename, O_RDONLY, 0666)) == -1)
    {
        return -1;
    }
    printf("%s", source);
    char *buffer = malloc(BUFFER_SIZE);
    while ((read(fd, buffer, BUFFER_SIZE)) > 0)
    {
        printf("%s", buffer);
    }
    printf("\n");
    stat(filename, data);
    if (mode == 1)
    {
        printf("\nCache expires in %ld seconds\n", TIMEOUT - (time(NULL) - data->st_mtime));
    }
    free(buffer);
    free(data);
    return 0;
}

char *getInput(char *prompt)
{
    char *fileName = malloc(BUFFER_SIZE);
    printf("%s", prompt);
    if ((fgets(fileName, BUFFER_SIZE, stdin)) == NULL)
    {
        printf("Failed to get input, aborting\n");
        exit(-1);
    }
    fileName[strcspn(fileName, "\n")] = '\0';
    return fileName;
}

int writeFile(char *filename, char *data)
{
    int sockfd = openSocket(port);
    Package _message;
    Package *message = &_message;
    message->command = WRITE;
    strcpy(message->filename, filename);
    strcpy(message->data, data);
    printf("Sending write command\n");
    char buffer[BUFFER_SIZE];
    pack(message, buffer);
    write(sockfd, buffer, BUFFER_SIZE);
    char *response = malloc(BUFFER_SIZE);
    read(sockfd, response, BUFFER_SIZE);
    if (strcmp(response, ERR) == 0)
    {
        close(sockfd);
        return -1;
    }

    // write(sockfd, filename, strlen(filename));
    // while(write(sockfd, data, BUFFER_SIZE)>0){
    //     data+=BUFFER_SIZE;
    // }
    // write(sockfd, &EOT, sizeof(char));
    close(sockfd);
    return 0;
}

int clearCache()
{
    int retval = 0;
    chdir(CACHE);
    DIR *dp = opendir(".");
    if (dp == NULL)
    {
        return -1;
    }
    struct dirent *d;
    while ((d = readdir(dp)) != NULL)
    {
        printf("%s\n", d->d_name);
        if ((unlink(d->d_name) == -1))
        {
            retval = -1;
        }
    }
    closedir(dp);
    return retval;
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
    mkdir(CACHE, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
    command = (char *)malloc(BUFFER_SIZE);
    char *modeS = getInput("Choose cache validation method:\n1. Timeout-based validation\n2. Attribute-based validation\nEnter choice (1 or 2): ");
    if ((mode = atoi(modeS)) == 0)
    {
        printf("Input conversion error\n");
        return -1;
    }
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
            if ((readfile(getInput("Enter filename: "))) == -1)
            {
                printf("Failed to read file\n");
            }
        }
        else if (*command == WRITE)
        {
            char *filename = getInput("Enter filename: ");
            while (strlen(filename) > MAX_FILENAME)
            {
                printf("Filename max 255 characters");
                filename = getInput("Enter filename: ");
            }
            char *data = getInput("Enter data: ");
            if (writeFile(filename, data) != 0)
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
            clearCache();
            break;
        }
        else
        {
            printf("Invalid command\n");
        }
    }
    free(command);

    return 0;
}