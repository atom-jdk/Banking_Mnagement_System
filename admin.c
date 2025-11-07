#include "admin.h"
#include "common.h"
#include "utils.h"
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

static void trim(char *str) {
    int len = strlen(str);
    while (len > 0 && (str[len-1] == '\n' || str[len-1] == '\r' || str[len-1] == ' '))
        str[--len] = '\0';
}

static void modify_customer(int sock) {
    char buffer[BUFFER_SIZE];
    send_message(sock, "Enter customer ID: ");
    if (!read_input(sock, buffer, BUFFER_SIZE)) return;
    int cid = atoi(buffer);

    char path[100], temp_path[100];
    snprintf(path, 100, "%s/customers.csv", DATA_DIR);
    snprintf(temp_path, 100, "%s/customers.tmp", DATA_DIR);

    FILE *fp = fopen(path, "r");
    FILE *tmp = fopen(temp_path, "w");

    if (!fp || !tmp) {
        if (fp) fclose(fp);
        if (tmp) fclose(tmp);
        send_message(sock, "Error opening file\n");
        return;
    }

    char line[256];
    int found = 0;

    fgets(line, sizeof(line), fp);
    fprintf(tmp, "%s", line);

    while (fgets(line, sizeof(line), fp)) {
        Customer c;
        if (sscanf(line, "%d,%49[^,],%49[^,],%lf,%d",
                   &c.id, c.username, c.password, &c.balance, &c.active) == 5) {

            if (c.id == cid && !found) {
                found = 1;

                send_message(sock, "New username (- to skip): ");
                if (read_input(sock, buffer, BUFFER_SIZE) && buffer[0] != '-')
                    strcpy(c.username, buffer);

                send_message(sock, "New password (- to skip): ");
                if (read_input(sock, buffer, BUFFER_SIZE) && buffer[0] != '-')
                    strcpy(c.password, buffer);
            }

            fprintf(tmp, "%d,%s,%s,%.2f,%d\n",
                    c.id, c.username, c.password, c.balance, c.active);
        }
    }

    fclose(fp);
    fclose(tmp);
    rename(temp_path, path);

    send_message(sock, found ? "Updated\n" : "Customer not found\n");
}


static void admin_change_password(int sock, int aid) {
    char buffer[BUFFER_SIZE], pwd[50];
    char path[100], temp_path[100];
    snprintf(path, 100, "%s/admins.csv", DATA_DIR);
    snprintf(temp_path, 100, "%s/admins.tmp", DATA_DIR);

    FILE *fp = fopen(path, "r");
    FILE *tmp = fopen(temp_path, "w");
    if (!fp || !tmp) {
        if (fp) fclose(fp);
        if (tmp) fclose(tmp);
        send_message(sock, "Error opening file\n");
        return;
    }

    char line[256];
    Admin a;
    int found = 0;

    fgets(line, sizeof(line), fp);
    fprintf(tmp, "%s", line);

    while (fgets(line, sizeof(line), fp)) {
        if (sscanf(line, "%d,%49[^,],%49[^\n]",
                   &a.id, a.username, a.password) == 3) {

            trim(a.password);

            if (a.id == aid) {
                found = 1;

                send_message(sock, "Enter current password: ");
                if (!read_input(sock, buffer, BUFFER_SIZE)) break;
                trim(buffer);

                if (strcmp(buffer, a.password) != 0) {
                    send_message(sock, "✗ Incorrect password\n");
                    fclose(fp);
                    fclose(tmp);
                    remove(temp_path);
                    return;
                }

                send_message(sock, "Enter new password: ");
                if (!read_input(sock, buffer, BUFFER_SIZE)) break;
                trim(buffer);
                strcpy(a.password, buffer);
            }

            fprintf(tmp, "%d,%s,%s\n", a.id, a.username, a.password);
        }
    }

    fclose(fp);
    fclose(tmp);
    rename(temp_path, path);

    send_message(sock, found ? "✓ Password Updated\n" : "✗ Admin Not Found\n");
}

static void add_employee(int sock) {
    char buffer[BUFFER_SIZE];
    Employee e = {0};

    char path[100];
    snprintf(path, 100, "%s/employees.csv", DATA_DIR);
    e.id = get_next_id_csv(path);

    send_message(sock, "Enter username: ");
    if (!read_input(sock, buffer, BUFFER_SIZE)) return;
    strcpy(e.username, buffer);

    send_message(sock, "Enter password: ");
    if (!read_input(sock, buffer, BUFFER_SIZE)) return;
    strcpy(e.password, buffer);

    FILE *fp = fopen(path, "a");
    if (!fp) return;

    fprintf(fp, "%d,%s,%s\n", e.id, e.username, e.password);
    fclose(fp);

    snprintf(buffer, BUFFER_SIZE, "Employee created! ID: %d\n", e.id);
    send_message(sock, buffer);
}

static void modify_employee(int sock) {
    char buffer[BUFFER_SIZE];
    send_message(sock, "Enter employee ID: ");
    if (!read_input(sock, buffer, BUFFER_SIZE)) return;
    int eid = atoi(buffer);

    char path[100], temp_path[100];
    snprintf(path, 100, "%s/employees.csv", DATA_DIR);
    snprintf(temp_path, 100, "%s/employees.tmp", DATA_DIR);

    FILE *fp = fopen(path, "r");
    FILE *tmp = fopen(temp_path, "w");

    if (!fp || !tmp) {
        if (fp) fclose(fp);
        if (tmp) fclose(tmp);
        send_message(sock, "Error opening file\n");
        return;
    }

    char line[256];
    int found = 0;

    // Copy header
    fgets(line, sizeof(line), fp);
    fprintf(tmp, "%s", line);

    while (fgets(line, sizeof(line), fp)) {
        Employee e;
        if (sscanf(line, "%d,%49[^,],%49[^\n]",
                   &e.id, e.username, e.password) == 3) {

            if (e.id == eid && !found) {
                found = 1;

                send_message(sock, "New username (- to skip): ");
                if (read_input(sock, buffer, BUFFER_SIZE) && buffer[0] != '-')
                    strcpy(e.username, buffer);

                send_message(sock, "New password (- to skip): ");
                if (read_input(sock, buffer, BUFFER_SIZE) && buffer[0] != '-')
                    strcpy(e.password, buffer);
            }

            fprintf(tmp, "%d,%s,%s\n", e.id, e.username, e.password);
        }
    }

    fclose(fp);
    fclose(tmp);

    rename(temp_path, path);

    send_message(sock, found ? "Updated\n" : "Not found\n");
}

void handle_admin(int sock, int aid) {
    char buffer[BUFFER_SIZE];

    while (1) {
        send_message(sock,
            "\n=== Admin Menu ===\n"
            "1. Add Employee\n"
            "2. Modify Employee\n"
            "3. Modify Customer\n"
            "4. Change Password\n"
            "5. Logout\n"
            "Choice: ");

        if (!read_input(sock, buffer, BUFFER_SIZE)) break;
        int choice = atoi(buffer);

        switch (choice) {
            case 1:
                add_employee(sock);
                break;

            case 2:
                modify_employee(sock);
                break;

            case 3:
                modify_customer(sock);
                break;

            case 4:
                admin_change_password(sock, aid);
                break;

            case 5:
                return;

            default:
                send_message(sock, "Invalid choice\n");
        }
    }
}

