#include "common.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <poll.h>

int main() {
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock == -1) {
        perror("Socket failed");
        exit(1);
    }

    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(PORT);
    inet_pton(AF_INET, "127.0.0.1", &addr.sin_addr);

    if (connect(sock, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        perror("Connection failed");
        exit(1);
    }

    printf("Connected to server\n\n");

    // Disable stdout buffering for immediate output
    setvbuf(stdout, NULL, _IONBF, 0);

    char buffer[BUFFER_SIZE];
    char input[BUFFER_SIZE];
    struct pollfd fds[2];

    // Monitor both socket (server) and stdin (user input)
    fds[0].fd = sock;
    fds[0].events = POLLIN;
    fds[1].fd = STDIN_FILENO;
    fds[1].events = POLLIN;

    while (1) {
        // Poll with 100ms timeout
        int ret = poll(fds, 2, 100);

        if (ret > 0) {
            // Check if server sent data
            if (fds[0].revents & POLLIN) {
                memset(buffer, 0, BUFFER_SIZE);
                int n = read(sock, buffer, BUFFER_SIZE - 1);

                if (n > 0) {
                    buffer[n] = '\0';
                    printf("%s", buffer);

                    // Server explicitly closed connection (only disconnect, not logout)
                    // Connection stays alive during logout and returns to main menu

                } else if (n == 0) {
                    // Server closed connection
                    printf("\nServer disconnected\n");
                    break;
                } else {
                    // Read error
                    perror("Read error");
                    break;
                }
            }

            // Check if user typed input
            if (fds[1].revents & POLLIN) {
                if (fgets(input, BUFFER_SIZE, stdin) == NULL) {
                    break;
                }

                // Remove trailing newline
                input[strcspn(input, "\n")] = 0;

                // Send input to server
                if (write(sock, input, strlen(input)) <= 0) {
                    printf("\nFailed to send data\n");
                    break;
                }
            }
        } else if (ret < 0) {
            // Poll error
            perror("Poll error");
            break;
        }
        // ret == 0 means timeout, continue loop
    }

    close(sock);
    printf("Disconnected from server\n");
    return 0;
}
