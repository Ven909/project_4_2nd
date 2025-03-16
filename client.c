#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <netdb.h>
#include <unistd.h>
#include <string.h>

#define PORT 4000  // Match server port
#define BUFFER_SIZE 1024

void send_request(int fd) {
    char filename[BUFFER_SIZE];
    char buffer[BUFFER_SIZE];

    while (1) {
        printf("Enter filename to request (or 'exit' to quit): ");
        if (fgets(filename, sizeof(filename), stdin) == NULL) break;
        
        // Remove newline character
        filename[strcspn(filename, "\n")] = 0;
        if (strcmp(filename, "exit") == 0) break;

        // Send GET request
        dprintf(fd, "GET %s\n", filename);

        // Read server response
        ssize_t received;
        printf("Server response:\n");
        while ((received = read(fd, buffer, sizeof(buffer) - 1)) > 0) {
            buffer[received] = '\0';  // Null-terminate the received data
            printf("%s", buffer);
        }

        printf("\n");
    }
}

int connect_to_server(struct hostent *host_entry) {
    int fd;
    struct sockaddr_in their_addr;

    if ((fd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        perror("socket");
        return -1;
    }

    their_addr.sin_family = AF_INET;
    their_addr.sin_port = htons(PORT);
    their_addr.sin_addr = *((struct in_addr *)host_entry->h_addr);

    if (connect(fd, (struct sockaddr *)&their_addr, sizeof(struct sockaddr)) == -1) {
        perror("connect");
        close(fd);
        return -1;
    }

    return fd;
}

struct hostent *gethost(char *hostname) {
    struct hostent *he;
    if ((he = gethostbyname(hostname)) == NULL) {
        herror(hostname);
    }
    return he;
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <server address>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    struct hostent *host_entry = gethost(argv[1]);
    if (!host_entry) exit(EXIT_FAILURE);

    int fd = connect_to_server(host_entry);
    if (fd != -1) {
        send_request(fd);
        close(fd);
    }

    return 0;
}

