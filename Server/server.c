#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <sys/stat.h>

#define PORT 8080
#define BUFFER_SIZE 1024
const char *WHITESPACE = " \t\r\v\f";

int main()
{
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
    serv_addr.sin_port = htons(PORT);

    if (bind(sockfd, (const struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
    {
        perror("Bind failed");
        close(sockfd);
        exit(EXIT_FAILURE);
    }

    printf("Server is listening on port %d...\n", PORT);

    while (1)
    {
        int n = recvfrom(sockfd, buffer, BUFFER_SIZE, 0, (struct sockaddr *)&client_addr, &addr_len);
        buffer[n] = '\0';
        char *token = strtok(buffer, WHITESPACE);
        char *response = malloc(BUFFER_SIZE);
        if (strcmp(token, "R") == 0)
        {
            printf("Read command received\n");
            token = strtok(NULL, WHITESPACE);
            token[strcspn(token, "\n")] = '\0';
            int fd;

            if ((fd = open(token, O_RDONLY)) == -1)
            {
                char *error = "Error opening file";
                printf("%s: %s\n", error, token);
                sendto(sockfd, error, strlen(error), 0, (struct sockaddr *)&client_addr, addr_len);
                sendto(sockfd, response, 0, 0, (struct sockaddr *)&client_addr, addr_len);
            }
            else
            {
                while (read(fd, response, BUFFER_SIZE))
                {
                    sendto(sockfd, response, strlen(response), 0, (struct sockaddr *)&client_addr, addr_len);
                }
                sendto(sockfd, response, 0, 0, (struct sockaddr *)&client_addr, addr_len);
            }
        }

        // printf("Filename: %s\n", token);

        // printf("%s\n", buffer);

        // const char *response = "Hello from server. Got your message.";
        // sendto(sockfd, response, strlen(response), 0, (struct sockaddr *)&client_addr, addr_len);
    }

    close(sockfd);
    return 0;
}
