/**
 * Author: Stefan Zeidler
 * August 6, 2024
 *
 * This program simulates a server responding to clients read and write requests. It is also able to use provide file checksums for clients' case validation.
 * Please see comments in client file for more explanation.
 *
 * Resources used:
 * For serializing and deserializing of structures:
 * https://stackoverflow.com/questions/15707933/how-to-serialize-a-struct-in-c
 *
 * For basic checksum algorithm:
 * https://www.tutorialspoint.com/c-program-to-implement-checksum
 *
 */
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

/**
 * Helper method to deserialize message from client.
 * @param p The Package in which to place the de-serialized data.
 * @param buffer The serialized message from the client.
 * @return P now contains the deserialized data from the client.
 */
void unpack(Package *p, char *buffer)
{
    size_t offset = 0;
    memcpy(&(p->command), buffer, sizeof(char));
    offset += sizeof(char);
    memcpy(p->filename, buffer + offset, MAX_FILENAME);
    offset += MAX_FILENAME;
    memcpy(p->data, buffer + offset, sizeof(p->data));
}

/**
 * Calculates a file checksum based on the sum of all strings in the file.
 * @param filepath Path to the file.
 * @return Returns the calculated file checksum.
 */
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

/**
 * Reads from client-specified file and sents to client. Extracted method.
 * @param p Package to serialize file data.
 * @param sockfd Socket to use.
 * @param client_ptr Client to sent data to.
 * @param addr_len Address length.
 * @return File has been serialized and sent to client.
 */
void read_file(Package *p, int sockfd, struct sockaddr_in *client_ptr, socklen_t addr_len)
{
    printf("Read command received\n");
    char *response = malloc(BUFFER_SIZE);
    int fd;
    if ((fd = open(p->filename, O_RDONLY)) == -1)
    {
        char *error = "Error opening file";
        printf("%s: %s\n", error, p->filename);
        sendto(sockfd, ERR, strlen(ERR), 0, (struct sockaddr *)client_ptr, addr_len);
    }
    else
    {
        printf("Reading from file: %s\n", p->filename);
        while (read(fd, response, BUFFER_SIZE))
        {
            sendto(sockfd, response, strlen(response), 0, (struct sockaddr *)client_ptr, addr_len);
        }
        sendto(sockfd, response, 0, 0, (struct sockaddr *)client_ptr, addr_len);
        printf("Read complete\n");
    }
    free(response);
    close(fd);
}

/**
 * Writes data from client to client-specified file. Extracted method.
 * @param p Package from client with serialized data.
 * @param sockfd Socket to use to communicate with client.
 * @param client_ptr Client that sent data.
 * @param addr_len Address length.
 * @return Data has been deserialized and written to file.
 */
void write_file(Package *p, int sockfd, struct sockaddr_in *client_ptr, socklen_t addr_len)
{
    printf("Write command received\n");
    char *response = malloc(BUFFER_SIZE);
    int fd;
    if ((fd = open(p->filename, O_CREAT | O_WRONLY | O_TRUNC, 0666)) == -1)
    {
        char *error = "Error opening file";
        printf("%s: %s\n", error, p->filename);
        sendto(sockfd, error, strlen(error), 0, (struct sockaddr *)client_ptr, addr_len);
    }
    else
    {

        printf("Writing to file: %s\n", p->filename);
        if ((write(fd, p->data, strlen(p->data))) == -1)
        {
            char *error = "Error writing to file";
            printf("%s: %s\n", error, p->filename);
            sendto(sockfd, ERR, strlen(ERR), 0, (struct sockaddr *)client_ptr, addr_len);
        }
        sendto(sockfd, response, 0, 0, (struct sockaddr *)client_ptr, addr_len);
        printf("Write complete\n");
    }
    free(response);
    close(fd);
}

/**
 * Calculates checksum for client-specified file and sends to client. Extracted method.
 * @param p Package from client with serialized data.
 * @param sockfd Socket to use to communicate with client.
 * @param client_ptr Client that sent data.
 * @param addr_len Address length.
 * @return Checksum calculated and sent to client.
 */
void get_checksum(Package *p, int sockfd, struct sockaddr_in *client_ptr, socklen_t addr_len)
{
    printf("Get command received\n");
    char *response = malloc(BUFFER_SIZE);
    unsigned int sum = checksum(p->filename);
    sprintf(response, "%u", sum);
    sendto(sockfd, response, strlen(response), 0, (struct sockaddr *)client_ptr, addr_len);
    free(response);
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
    struct sockaddr_in *client_ptr = &client_addr;
    socklen_t *addr_len_ptr = &addr_len;
    while (1)
    {
        int n = recvfrom(sockfd, buffer, BUFFER_SIZE, 0, (struct sockaddr *)client_ptr, addr_len_ptr);
        buffer[n] = '\0';
        Package *p = malloc(sizeof(buffer));
        unpack(p, buffer);
        if (p->command == READ)
        {
            read_file(p, sockfd, client_ptr, addr_len);
        }
        else if (p->command == WRITE)
        {
            write_file(p, sockfd, client_ptr, addr_len);
        }
        else if (p->command == GET)
        {
            get_checksum(p, sockfd, client_ptr, addr_len);
        }
    }
    close(sockfd);
    return 0;
}
