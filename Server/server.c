#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <sys/stat.h>

#define BUFFER_SIZE 1024
#define MAX_FILENAME 256
const char *WHITESPACE = " \t\r\v\f";
const char *ACK = "ACK";
const char *ERR = "ERR";
const char READ = 'R';
const char WRITE = 'W';
const char GET = 'G';
static int port;

typedef struct _package
{
    char command;
    char filename[MAX_FILENAME];
    char data[BUFFER_SIZE - sizeof(char) - MAX_FILENAME];
} Package;

void unpack(Package *p, char *buffer)
{
    size_t offset = 0;
    memcpy(&(p->command), buffer, sizeof(char));
    offset += sizeof(char);
    memcpy(p->filename, buffer + offset, MAX_FILENAME);
    offset += MAX_FILENAME;
    memcpy(p->data, buffer + offset, sizeof(p->data));
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
    return sum;
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
    int sockfd;
    struct sockaddr_in serv_addr, client_addr;
    char buffer[BUFFER_SIZE];
    socklen_t addr_len = sizeof(client_addr);

    if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
    {
        perror("Socket creation error");
        exit(EXIT_FAILURE);
    }

    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    serv_addr.sin_port = htons(port);

    if (bind(sockfd, (const struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
    {
        perror("Bind failed");
        close(sockfd);
        exit(EXIT_FAILURE);
    }

    printf("Server is listening on port %d...\n", port);

    while (1)
    {
        int n = recvfrom(sockfd, buffer, BUFFER_SIZE, 0, (struct sockaddr *)&client_addr, &addr_len);
        buffer[n] = '\0';
        Package *p = malloc(sizeof(buffer));
        unpack(p, buffer);
        char *response = malloc(BUFFER_SIZE);
        if (p->command == READ)
        {
            printf("Read command received\n");
            int fd;
            if ((fd = open(p->filename, O_RDONLY)) == -1)
            {
                char *error = "Error opening file";
                printf("%s: %s\n", error, p->filename);
                sendto(sockfd, ERR, strlen(ERR), 0, (struct sockaddr *)&client_addr, addr_len);
            }
            else
            {
                sendto(sockfd, ACK, strlen(ACK), 0, (struct sockaddr *)&client_addr, addr_len);
                printf("Reading from file: %s\n", p->filename);
                while (read(fd, response, BUFFER_SIZE))
                {
                    sendto(sockfd, response, strlen(response), 0, (struct sockaddr *)&client_addr, addr_len);
                }
                sendto(sockfd, response, 0, 0, (struct sockaddr *)&client_addr, addr_len);
                printf("Read complete\n");
            }
            close(fd);
        }
        else if (p->command == WRITE)
        {
            printf("Write command received\n");
            int fd;
            if ((fd = open(p->filename, O_CREAT | O_WRONLY | O_TRUNC, 0666)) == -1)
            {
                char *error = "Error opening file";
                printf("%s: %s\n", error, p->filename);
                sendto(sockfd, error, strlen(error), 0, (struct sockaddr *)&client_addr, addr_len);
            }
            else
            {

                printf("Writing to file: %s\n", p->filename);
                if ((write(fd, p->data, strlen(p->data))) == -1)
                {
                    char *error = "Error writing to file";
                    printf("%s: %s\n", error, p->filename);
                    sendto(sockfd, ERR, strlen(ERR), 0, (struct sockaddr *)&client_addr, addr_len);
                }
                sendto(sockfd, response, 0, 0, (struct sockaddr *)&client_addr, addr_len);
                printf("Write complete\n");
            }
            close(fd);
        }
        else if (p->command == GET)
        {
            printf("Get command received\n");
            char *response = malloc(BUFFER_SIZE);
            unsigned int sum = checksum(p->filename);
            sprintf(response, "%u", sum);
            sendto(sockfd, response, strlen(response), 0, (struct sockaddr *)&client_addr, addr_len);
        }
    }
    close(sockfd);
    return 0;
}