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

int main()
{
    struct stat *data = malloc(sizeof(struct stat));
    stat("abc", data);
    time_t modified = data->st_mtime;
    time_t current = time(NULL);
    printf("%ld\n", modified);
    printf("%ld\n", current);
    printf("%ld\n", modified);
    printf("%ld\n", current - modified);
}