#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <unistd.h>
#include <signal.h>
#include "record.h"

volatile sig_atomic_t keep_running = 1;

int get_sunspots(FILE *f, const char *name, unsigned short *psunspots)
{
    fseek(f, 0, SEEK_SET);
    record rec;
    while(fread(&rec, sizeof(record), 1, f) == 1){
        if(strncmp(name, rec.name, rec.name_len) == 0){
            *psunspots = rec.sunspots;
            return 1;
        }
    }
    return 0;
}

int print_sunspot(const char *file_path, char* name){
    FILE *f = fopen(file_path, "r+b");
    if (f == NULL) {
        perror("fopen");
        return 1;
    }

    // Get Dennis Ritchie sunspots
    unsigned short y;
    if (get_sunspots(f, name, &y)) {
        //printf("%s has %hu sunspots\n",name, y);
        sprintf(name, "%u\n", y);
    } else {
        //printf("%s not found\n", name);
        sprintf(name, "none\n");
    }
    fclose(f);
    return 0;
}

void handle_sigterm(int sig) {
    (void)sig; // To avoid unused parameter warning
    keep_running = 0; // Set flag to exit the loop
}

void ignore_sigpipe(void) {
    struct sigaction myaction;
    myaction.sa_handler = SIG_IGN;
    sigemptyset(&myaction.sa_mask);
    myaction.sa_flags = 0;
    sigaction(SIGPIPE, &myaction, NULL);
}

void reap_zombies(int signum) {
    (void)signum; // To avoid unused parameter warning
    while (waitpid(-1, NULL, WNOHANG) > 0);
}

void handle_client(int cfd, const char *file_path) {
    char name[NAME_LEN_MAX + 1];
    unsigned short sunspots;

    int bytes_read = read(cfd, name, NAME_LEN_MAX);
    //printf( "name: %s\n", name);
    //name[bytes_read-1] = '\0'; // Null-terminate the name
    //if (bytes_read > 0) {
        name[bytes_read] = '\0';
        //FILE *file = fopen(file_path, "rb");
        //if (!file) {
            //strcpy(name, "Error opening");
        //}
        //else {
            //fprintf(stdout, "name %s\n", name);
            print_sunspot(file_path, name);

            //if (get_sunspots(file, name, &sunspots)) {
                //sprintf(name, "%u\n", sunspots); // Include newline for consistency
            //} else {
                //sprintf(name, "none\n");
            //}
            //fclose(file);
        //}

        // Send response to client
        write(cfd, name, strlen(name));
    //}
    close(cfd);
}


int main(int argc, char **argv) {
    if (argc != 3) {
        fprintf(stderr, "Usage: %s <port> <customer_file>\n", argv[0]);
        return 1;
    }

    //print_sunspot(argv[2], "Dennis Ritchie");
    int server_port = atoi(argv[1]);
    int sfd;
    struct sockaddr_in a;

    // Setup signal handlers
    struct sigaction sa;
    sa.sa_handler = handle_sigterm;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    sigaction(SIGTERM, &sa, NULL);
    sigaction(SIGINT, &sa, NULL); // Handle Ctrl+C

    // Ignore SIGPIPE
    ignore_sigpipe();

    sfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sfd == -1) {
        perror("socket");
        return 1;
    }

    memset(&a, 0, sizeof(a));
    a.sin_family = AF_INET;
    a.sin_port = htons(server_port);
    a.sin_addr.s_addr = htonl(INADDR_ANY);
    if (bind(sfd, (struct sockaddr *)&a, sizeof(a)) == -1) {
        perror("bind");
        close(sfd);
        return 1;
    }

    if (listen(sfd, 10) == -1) {
        perror("listen");
        close(sfd);
        return 1;
    }

    signal(SIGCHLD, reap_zombies);
    struct sockaddr_in ca;
    socklen_t sinlen = sizeof(ca);
    int cfd = accept(sfd, (struct sockaddr *)&ca, &sinlen);
    // Setup signal handler to reap zombies

    int count =0;

    while (keep_running) {

        if (cfd == -1) {
            if (!keep_running) {
                // Exit if server is shutting down
                break;
            }
            perror("accept");
            continue;
        }

        // Fork a child process to handle the client
        if (fork() == 0) {
            // Child process
            close(sfd); // Child doesn't need the listening socket
            handle_client(cfd, argv[2]);
            count++;
            //printf("the number of running time: %d\n", count);
            return 0;
        }

        // Parent process
        //close(cfd); // Parent doesn't need the client socket
        reap_zombies(0); // Reap any zombie processes
    }

    // Cleanup
    close(cfd);
    close(sfd);
    return 0;
}

