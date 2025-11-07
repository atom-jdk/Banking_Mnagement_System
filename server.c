#include "common.h"
#include "auth.h"
#include "customer.h"
#include "employee.h"
#include "manager.h"
#include "admin.h"
#include "utils.h"
#include "session_manager.h"
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/stat.h>

void* handle_client(void *arg) {
    int sock = *(int*)arg;
    free(arg);

    char buffer[BUFFER_SIZE];
    int role, user_id;

    // Main session loop - allows multiple logins/logouts
    while (1) {
        int authenticated = 0;

        // Authentication loop
        while (!authenticated) {
            send_message(sock,
                "\n=== Banking System ===\n"
                "1. Customer\n"
                "2. Employee\n"
                "3. Manager\n"
                "4. Admin\n"
                "5. Exit\n"
                "Select role: ");

            if (!read_input(sock, buffer, BUFFER_SIZE)) {
                close(sock);
                return NULL;
            }
            role = atoi(buffer);

            // Check if user wants to exit
            if (role == 5) {
                send_message(sock, "Goodbye!\n");
                close(sock);
                return NULL;
            }

            // Validate role
            if (role < 1 || role > 4) {
                send_message(sock, "Invalid role! Please try again.\n");
                continue;
            }

            send_message(sock, "Username: ");
            if (!read_input(sock, buffer, BUFFER_SIZE)) {
                close(sock);
                return NULL;
            }
            char username[50];
            strcpy(username, buffer);

            send_message(sock, "Password: ");
            if (!read_input(sock, buffer, BUFFER_SIZE)) {
                close(sock);
                return NULL;
            }
            char password[50];
            strcpy(password, buffer);

            // Try authentication
            if (authenticate(role, username, password, &user_id)) {
                authenticated = 1;
                send_message(sock, "\n✓ Login successful!\n");
            } else {
                send_message(sock, "\n✗ Authentication failed! Please try again.\n");
            }
        }

        // Successfully authenticated - handle the role menu
        switch (role) {
            case ROLE_CUSTOMER: handle_customer(sock, user_id); break;
            case ROLE_EMPLOYEE: handle_employee(sock, user_id); break;
            case ROLE_MANAGER: handle_manager(sock, user_id); break;
            case ROLE_ADMIN: handle_admin(sock, user_id); break;
        }

        // Logout - unregister session
        logout_user(role, user_id);

        // Loop continues - returns to main menu!
        send_message(sock, "\nLogged out. Returning to main menu...\n");
    }

    close(sock);
    return NULL;
}

int main() {
    mkdir(DATA_DIR, 0755);

    // Initialize shared memory session manager
    init_session_manager();

    int server_sock = socket(AF_INET, SOCK_STREAM, 0);
    if (server_sock == -1) {
        perror("Socket failed");
        exit(1);
    }

    int opt = 1;
    setsockopt(server_sock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(PORT);

    if (bind(server_sock, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        perror("Bind failed");
        exit(1);
    }

    if (listen(server_sock, 10) < 0) {
        perror("Listen failed");
        exit(1);
    }

    printf("Server listening on port %d\n", PORT);

    while (1) {
        struct sockaddr_in client_addr;
        socklen_t len = sizeof(client_addr);
        int client_sock = accept(server_sock, (struct sockaddr*)&client_addr, &len);

        if (client_sock < 0) continue;

        int *sock_ptr = malloc(sizeof(int));
        *sock_ptr = client_sock;

        pthread_t tid;
        pthread_create(&tid, NULL, handle_client, sock_ptr);
        pthread_detach(tid);
    }

    close(server_sock);
    return 0;
}
