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
#include <math.h>

// #define PORT 8080
#define BUFFER_SIZE 1024
static char *command = NULL;
// static const int INITIAL_CAPACITY = 8;
// static int used;
// static int elements;

// typedef struct _Node
// {
//     char *filename;
//     FILE *file;
//     int offset;
//     int count;
//     Node *left, *right, *parent;

// } Node;

// Node *root;
// Node _placeHolder = {.filename = NULL, .file = NULL, .offset = 0};
// Node *placeHolder = &_placeHolder;
// Node *cache[INITIAL_CAPACITY] = {NULL};

// int getSize()
// {
//     return sizeof(cache) / sizeof(Node);
// }

// int getCount(Node *r)
// {
//     if (r == NULL)
//     {
//         return 0;
//     }
//     return r->count;
// }

// void setCount(Node *r)
// {
//     if (r == NULL)
//         return;
//     r->count = getCount(r->left) + getCount(r->right);
// }

// unsigned long hash(unsigned char *str)
// {
//     unsigned long hash = 5381;
//     int c;

//     while (c = *str++)
//     {
//         hash = ((hash << 5) + hash) + c; /* hash * 33 + c */
//     }
//     hash = hash % getSize();
//     if (hash < 0)
//     {
//         hash += getSize();
//     }
//     return hash;
// }
// int isPowerOfTwo(int x)
// {
//     if (x <= 0)
//         return 0;
//     if ((x & (x - 1)) == 0)
//         return 1;
//     else
//         return 0;
// }

// int nextPower(int x)
// {
//     if (x <= 0)
//         return 1;
//     int sh = x >> 1;
//     while (sh != 0)
//     {
//         x |= sh;
//         sh >>= 1;
//     }
//     return x + 1;
// }

// /**
//  * Finds a value in the cache hash table and returns its index or the index of the first available location.
//  * Uses quadratic hashing.
//  * @param value Value to search for.
//  * @param skipPH Determines whether to skip the placeholder or not.
//  * @return index of place in hash table that matches the value or the next
//  * available index for this value.
//  */
// int find(char *value, int skipPH)
// {
//     int size = sizeof(cache) / sizeof(Node);
//     int base = hash(value);
//     int hash = base;
//     Node *result;

//     for (int i = 1;; i++)
//     {
//         result = cache[hash];
//         if (result == NULL)
//             break;
//         if (result == placeHolder)
//         {
//             if (!skipPH)
//                 break;
//         }
//         else if (strcmp(result->filename, value) == 0)
//         {
//             break;
//         }
//         hash = (base + (int)pow(i, 2) + i) % getSize();
//         if (hash < 0)
//         {
//             hash += getSize();
//         }
//     }
//     return hash;
// }

// void rehash()
// {
//     if (used > (getSize() / 2))
//     {
//         int newLength = nextPower(elements);
//         while (newLength < 4 * elements)
//         {
//             newLength = nextPower(newLength);
//         }
//         if (newLength < INITIAL_CAPACITY)
//         {
//             newLength
//         }
//     }
// }

// /**
//  * Return the k'th (1 based) node in the subtree.
//  * This helper method uses the count fields to avoid an
//  * in-order traversal.
//  * @param r subtree to examine, may be null
//  * @param k index (1 based)
//  * @return k'th node of the tree or null if none such
//  */
// Node *doGet(Node *r, int k)
// {
//     if (r == NULL)
//         return NULL;
//     int l = getCount(r->left);
//     if (k <= l)
//         return doGet(r->left, k);
//     if (k == l + 1)
//         return r;
//     return doGet(r->right, k - l - 1);
// }

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
