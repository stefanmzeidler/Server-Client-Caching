/**
 * Author: Stefan Zeidler
 * August 6, 2024
 *
 * This program simulates a client asking a server to execute remote commands.
 * This program uses two different methods of cache validation. First, it uses a timeout method so that the cache expires after a set period of time. Second,
 * before reading from the cached file (if any), the program calculates a simple checksum of the file and asks the server for the checksum of the server version.
 * If the checksums do not match, the client requests the server version and updates its cache. If they do match, it simply reads from the cache.
 *
 * The checksum validation addresses the issue where client A updates the server version of a file before client B's cached version expires.
 * This means that client B will read from the cache and have an outdated version. With checksum validation, all clients will be significantly more likely to have
 * the real-time version of the file. However, this comes at the cost of overhead. Each time a client wants to read a file it needs to send and receive data from the
 * server as well as compute its own checksum. As the checksum algorithm gets more complex and the files get larger, this will increase the time to validate the cache.
 * This also does not address the issue if the server version is updated after the file checksum is sent but before the client reads the file from its cache.
 * Additionally, since my checksum algorith is very simple, I think it is possible that two files with same exact characters but in a different order would
 * have the same checksum.
 *
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
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <fcntl.h>
#include <time.h>
#include <dirent.h>

#define BUFFER_SIZE 1024
#define MAX_FILENAME 256
#define MAX_DATA BUFFER_SIZE - sizeof(char) - MAX_FILENAME
#define CACHE "./cache"
const char READ = 'R';
const char WRITE = 'W';
const char QUIT = 'Q';
const char GET = 'G';
const char *ERR = "ERR";
#define EMPTY ""
#define TIMEOUT 60
static int sockfd;
int mode = 1;
typedef struct _package
{
    char command;
    char filename[MAX_FILENAME];
    char data[MAX_DATA];
} Package;

/**
 * Helper method to clear the cache upon program exit.
 * @return returns 0 if all files were successfully deleted or -1 if one or more files failed to be deleted.
 */
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
        if ((unlink(d->d_name) == -1))
        {
            retval = -1;
        }
    }
    closedir(dp);
    return retval;
}

/**
 * Helper method to open a UDP socket for communication with a server.
 * @param port The port number to use for connection.
 * @return The global variable sockfd, referring to the socket descriptor, has been set.
 */
void openSocket(uint16_t port)
{

    struct sockaddr_in serv_addr;
    if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
    {
        printf("\n Socket creation error \n");
        clearCache();
        exit(EXIT_FAILURE);
    }
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(port);
    serv_addr.sin_addr.s_addr = inet_addr("127.0.0.1");
    if (connect(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
    {
        printf("\nConnection Failed \n");
        clearCache();
        exit(EXIT_FAILURE);
    }
}

/**
 * Helper method to reduce code duplication when reading from the server.
 * @param response String where to store the response from the server.
 * @return Returns the number of bytes read.
 */
int safeRead(char *response)
{
    int retval = 0;
    if ((retval = read(sockfd, response, BUFFER_SIZE)) == -1)
    {
        printf("Server read failure, aborting.\n");
        clearCache();
        exit(EXIT_FAILURE);
    }
    return retval;
}

/**
 * Helper method to reduce code duplication when writing to the server.
 * @param buffer String where to write to the server.
 * @return Returns the number of bytes written.
 */
int safeWrite(char *buffer)
{
    int retval = 0;
    if ((retval = write(sockfd, buffer, BUFFER_SIZE)) <= 0)
    {
        printf("Write failure, aborting.\n");
        clearCache();
        exit(EXIT_FAILURE);
    }
    return retval;
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
    free(buffer);
    return sum;
}

/**
 * Method to serialize the Package struct to send to the server.
 * @param p Package to serialize.
 * @param buffer Char* buffer to place serialized structure.
 * @return The buffer contains the serialized structure.
 */
void pack(Package *p, char *buffer)
{
    size_t offset = 0;
    memcpy(buffer + offset, &(p->command), sizeof(char));
    offset += sizeof(char);
    memcpy(buffer + offset, p->filename, sizeof(p->filename));
    offset += MAX_FILENAME;
    memcpy(buffer + offset, p->data, sizeof(p->data));
}

/**
 * Validates the cache based on the client-selected method. Can either use a timeout or checksum comparision with the server version.
 * Requires that client and server use the same checksum algorithm.
 * @param filename Cached version of this file will be validated.
 * @return Returns 0 If file does not exist in cache or the cached version is invalid. Returns 1 if cached version exists and is valid. 
 */
int validateCache(char *filename)
{
    chdir(CACHE);
    struct stat *data = malloc(sizeof(struct stat));
    if (stat(filename, data) == -1)
    {
        free(data);
        return 0;
    }
    if (mode == 1)
    {
        if ((time(NULL) - data->st_mtime) > TIMEOUT)
        {
            free(data);
            return 0;
        }
        else
        {
            free(data);
            return 1;
        }
    }
    else
    {

        Package _message;
        Package *message = &_message;
        message->command = GET;
        strcpy(message->filename, filename);
        printf("Sending get command\n");
        char buffer[BUFFER_SIZE];
        pack(message, buffer);
        safeWrite(buffer);
        char *response = malloc(BUFFER_SIZE);
        safeRead(response);
        char *endptr;
        unsigned int s_checksum = strtoul(response, &endptr, 10);
        unsigned int c_checksum = checksum(filename);
        if (s_checksum != c_checksum)
        {
            free(data);
            free(response);
            return 0;
        }
        free(data);
        free(response);
        return 1;
    }
}

/**
 * Helper method to update cached version from server. Requests file from server and writes to cache.
 * @param filename File to update.
 * @return Returns 0 if operation was successful or -1 of operation failed.
 */
int update_cache(char *filename)
{

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
    char buffer[BUFFER_SIZE] = EMPTY;
    pack(message, buffer);
    safeWrite(buffer);
    char *response = malloc(BUFFER_SIZE);
    int bytes;
    bytes = safeRead(response);
    if (strcmp(response, ERR) == 0)
    {
        unlink(filename);
        free(response);

        return -1;
    }
    while ((bytes = safeRead(response)) > 0)
    {
        write(fd, response, bytes);
    }
    safeWrite(EMPTY);
    free(response);
    close(fd);
    return 0;
}

/**
 * Reads selected file and prints to stdout. Will either read from cache or server depending cache validity. File must be present on server.
 * @param filename File to read. 
 * @return Returns 0 if read was successful or -1 if read was not successful.
 */
int readfile(char *filename)
{
    chdir(CACHE);
    struct stat *data = malloc(sizeof(struct stat));
    if (!validateCache(filename))
    {
        if (update_cache(filename) == -1)
        {
            printf("Cache update failed\n");
            free(data);
            return -1;
        }
        printf("Reading content from server: ");
    }
    else
    {
        printf("Reading content from cache: ");
    }
    int fd;
    if ((fd = open(filename, O_RDONLY, 0666)) == -1)
    {
        free(data);
        return -1;
    }

    char buffer[BUFFER_SIZE] = EMPTY;
    while ((read(fd, buffer, BUFFER_SIZE)) > 0)
    {
        printf("%s", buffer);
    }
    if (mode == 2)
    {
        printf("\n");
    }
    stat(filename, data);
    if (mode == 1)
    {
        printf("\nCache expires in %ld seconds\n", TIMEOUT - (time(NULL) - data->st_mtime));
    }
    free(data);
    return 0;
}

/**
 * Helper method to get user input from stdin.
 * @param prompt Prompt to ask user.
 * @return Character pointer containing use response.
 */
char *getInput(char *prompt)
{
    char *retval = malloc(BUFFER_SIZE);
    printf("%s", prompt);
    if ((fgets(retval, BUFFER_SIZE, stdin)) == NULL)
    {
        printf("Failed to get input, aborting\n");
        clearCache();
        exit(EXIT_FAILURE);
    }
    retval[strcspn(retval, "\n")] = '\0';
    return retval;
}

/**
 * Writes user-specified text to server file. If file does not exist on server, new file is created.
 * @param filename Filename of file to write to or create.
 * @param data Data to write to file.
 * @return Returns 0 if write was successful or -1 if write failed.
 */
int writeFile(char *filename, char *data)
{

    Package _message;
    Package *message = &_message;
    message->command = WRITE;
    strcpy(message->filename, filename);
    strcpy(message->data, data);
    printf("Sending write command\n");
    char buffer[BUFFER_SIZE];
    pack(message, buffer);
    safeWrite(buffer);
    char response[BUFFER_SIZE] = EMPTY;
    read(sockfd, response, BUFFER_SIZE);
    if (strcmp(response, ERR) == 0)
    {
        return -1;
    }
    if (mode == 1)
    {
        update_cache(filename);
    }
    return 0;
}

int main(int argc, char *argv[])
{
    if (argc != 2)
    {
        printf("Incorrect number of arguments\n");
        return -1;
    }
    int port;
    if ((port = atoi(argv[1])) == 0)
    {
        printf("Argument conversion error\n");
        return -1;
    }
    openSocket(port);
    mkdir(CACHE, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
    char *modeS = getInput("Choose cache validation method:\n1. Timeout-based validation\n2. Checksum-based validation\nEnter choice (1 or 2): ");
    while (strcmp(modeS, "1") && strcmp(modeS, "2"))
    {
        printf("Invalid input. Please enter 1 or 2.\n");
        free(modeS);
        modeS = getInput("Choose cache validation method:\n1. Timeout-based validation\n2. Attribute-based validation\nEnter choice (1 or 2): ");
    }
    if ((mode = atoi(modeS)) == 0)
    {
        printf("Input conversion error\n");
        return -1;
    }
    free(modeS);
    while (1)
    {
        char *command = getInput("Enter command (R for read, W for write, Q for quit)\n");
        command[strcspn(command, "\n")] = '\0';
        while ((*command != READ && *command != WRITE && *command != QUIT) || strlen(command) > 1)
        {
            printf("Invalid command\n");
            command = getInput("Enter command (R for read, W for write, Q for quit)\n");
        }
        if (*command == QUIT)
        {
            free(command);
            break;
        }
        char *filename = getInput("Enter filename: ");
        if (*command == READ)
        {
            if ((readfile(filename)) == -1)
            {
                printf("Failed to read file\n");
            }
        }
        else if (*command == WRITE)
        {

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
            free(data);
        }

        free(command);
        free(filename);
    }

    clearCache();
    close(sockfd);
    return 0;
}