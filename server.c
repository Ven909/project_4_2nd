#define _GNU_SOURCE // keep this at the top
#include "net.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/wait.h>
#include <signal.h>
#include <fcntl.h>

#define PORT 4000  // Updated port
#define BUFFER_SIZE 1024

// Signal handler to clean up child processes
void sigchld_handler(int signo) {
    (void)signo;  // Suppress unused variable warning
    while (waitpid(-1, NULL, WNOHANG) > 0);
}

// Function to serve a client request
void handle_request(int client_fd) {
    char buffer[BUFFER_SIZE];
    ssize_t bytes_read;
    
    FILE *network = fdopen(client_fd, "r");
    if (!network) {
        perror("fdopen");
        close(client_fd);
        return;
    }

    // Read the client's request
    if (fgets(buffer, sizeof(buffer), network) == NULL) {
        fclose(network);
        return;
    }

    // Parse the request
    char method[4], filename[BUFFER_SIZE];
    if (sscanf(buffer, "%3s %1023s", method, filename) != 2 || strcmp(method, "GET") != 0) {
        write(client_fd, "ERROR: Invalid request\n", 23);
        fclose(network);
        return;
    }

    // Attempt to open the requested file
    FILE *file = fopen(filename, "r");
    if (!file) {
        write(client_fd, "ERROR: File not found\n", 22);
    } else {
        // Send file contents
        while ((bytes_read = fread(buffer, 1, sizeof(buffer), file)) > 0) {
            write(client_fd, buffer, bytes_read);
        }
        fclose(file);
    }

    fclose(network);
}

// Main function to run the server
void run_service(int server_fd) {
    struct sigaction sa;
    sa.sa_handler = sigchld_handler;
    sa.sa_flags = SA_RESTART | SA_NOCLDSTOP;
    sigemptyset(&sa.sa_mask);
    sigaction(SIGCHLD, &sa, NULL);

    while (1) {
        int client_fd = accept_connection(server_fd);
        if (client_fd == -1) continue;

        pid_t pid = fork();
        if (pid == 0) {
            // Child process
            close(server_fd);  // Child doesn't need to accept new connections
            handle_request(client_fd);
            close(client_fd);
            exit(0);
        } else if (pid > 0) {
            // Parent process
            close(client_fd);  // Parent doesn't need this socket
        } else {
            perror("fork");
        }
    }
}

int main(void) {
    int server_fd = create_service(PORT);
    if (server_fd == -1) {
        perror("Server setup failed");
        exit(1);
    }

    printf("Listening on port: %d\n", PORT);
    run_service(server_fd);
    close(server_fd);

    return 0;
}

