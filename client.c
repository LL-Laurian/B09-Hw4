#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#define MAX_NAME_LENGTH 30
#define MAX_REPLY_LENGTH 11

int main(int argc, char **argv)
{
    if (argc < 3) {
        fprintf(stderr, "Usage: %s <server_address> <server_port>\n", argv[0]);
        return 1;
    }

    int server_port = atoi(argv[2]);

    int cfd;
    struct sockaddr_in a;

    memset(&a, 0, sizeof(a));
    a.sin_family = AF_INET;
    a.sin_port = htons(server_port);
    if (inet_pton(AF_INET, argv[1], &a.sin_addr) <= 0) {
        fprintf(stderr, "Invalid IPv4 address.\n");
        return 1;
    }

    cfd = socket(AF_INET, SOCK_STREAM, 0);
    if (cfd == -1) {
        perror("socket");
        return 1;
    }

    if (connect(cfd, (struct sockaddr *)&a, sizeof(a)) == -1) {
        perror("connect");
        close(cfd);
        return 1;
    }

    printf("Ready.\n");

    char name[MAX_NAME_LENGTH + 1];
    char reply[MAX_REPLY_LENGTH + 1];

    while (fgets(name, sizeof(name), stdin)) {
        size_t name_len = strlen(name);

        if (name_len == 1 && name[0] == '\n') {
            close(cfd);
            return 0;
        }

        // Remove trailing newline if present

        name[name_len - 1] = '\0';
        //fprintf(stdout, "name: %s\n", name);

        if (write(cfd, name, MAX_NAME_LENGTH) == -1) {
            perror("write");
            close(cfd);
            return 1;
        }

        ssize_t bytes_read = read(cfd, reply, MAX_REPLY_LENGTH);
        //fprintf(stdout,"bytes-read %ld\n", bytes_read);
        if (bytes_read == -1) {
            perror("read error");
            close(cfd);
            return 1;
        } else if (bytes_read == 0) {
            fprintf(stderr, "Server closed connection unexpectedly.\n");
            close(cfd);
            return 1;
        }  //else if (reply[bytes_read -1] != '\n' ) {
            //fprintf(stderr, "Server bug.\n");
            //close(cfd);
            //return 1;
        //}
        reply[bytes_read-1] = '\0';

        printf("%s\n", reply);
    }

    close(cfd);
    return 0;
}
