#include "manager.h"
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


static void activate_account(int sock) {
    char buffer[BUFFER_SIZE];

    send_message(sock, "Enter customer ID: ");
    if (!read_input(sock, buffer, BUFFER_SIZE)) return;
    int cid = atoi(buffer);

    send_message(sock, "Activate (1) or Deactivate (0): ");
    if (!read_input(sock, buffer, BUFFER_SIZE)) return;
    int activate = atoi(buffer);

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

    // Copy header
    fgets(line, sizeof(line), fp);
    fprintf(tmp, "%s", line);

    while (fgets(line, sizeof(line), fp)) {
        Customer c;
        if (sscanf(line, "%d,%49[^,],%49[^,],%lf,%d",
                   &c.id, c.username, c.password, &c.balance, &c.active) == 5) {
            if (c.id == cid) {
                c.active = activate;
                found = 1;
            }
            fprintf(tmp, "%d,%s,%s,%.2f,%d\n", c.id, c.username, c.password, c.balance, c.active);
        }
    }

    fclose(fp);
    fclose(tmp);
    rename(temp_path, path);

    send_message(sock, found ? "Updated\n" : "Not found\n");
}
static void change_password(int sock, int mid) {
    char buffer[BUFFER_SIZE], pwd[50];
    char path[100], temp_path[100];
    snprintf(path, 100, "%s/managers.csv", DATA_DIR);
    snprintf(temp_path, 100, "%s/managers.tmp", DATA_DIR);

    FILE *fp = fopen(path, "r");
    FILE *tmp = fopen(temp_path, "w");

    if (!fp || !tmp) {
        if (fp) fclose(fp);
        if (tmp) fclose(tmp);
        send_message(sock, "Error opening file\n");
        return;
    }

    char line[256];
    Manager m;
    int found = 0;

    fgets(line, sizeof(line), fp);
    fprintf(tmp, "%s", line);

    while (fgets(line, sizeof(line), fp)) {
        if (sscanf(line, "%d,%49[^,],%49[^,\n]",
                   &m.id, m.username, m.password) == 3) {

            // 🔧 Trim stored password (fix key bug)
            trim(m.password);

            if (m.id == mid) {
                found = 1;

                // Ask old password
                send_message(sock, "Enter current password: ");
                if (!read_input(sock, buffer, BUFFER_SIZE)) break;
                trim(buffer);

                if (strcmp(buffer, m.password) != 0) {
                    send_message(sock, "✗ Incorrect password\n");
                    fclose(fp);
                    fclose(tmp);
                    remove(temp_path);
                    return;
                }

                // Ask new password
                send_message(sock, "Enter new password: ");
                if (!read_input(sock, buffer, BUFFER_SIZE)) break;
                trim(buffer);

                strcpy(m.password, buffer);
            }

            fprintf(tmp, "%d,%s,%s\n", m.id, m.username, m.password);
        }
    }

    fclose(fp);
    fclose(tmp);
    rename(temp_path, path);
    send_message(sock, found ? "✓ Password changed successfully\n" : "✗ Manager not found\n");
}



static void assign_loan(int sock) {
    char buffer[BUFFER_SIZE];

    send_message(sock, "Enter loan ID: ");
    if (!read_input(sock, buffer, BUFFER_SIZE)) return;
    int lid = atoi(buffer);

    send_message(sock, "Enter employee ID: ");
    if (!read_input(sock, buffer, BUFFER_SIZE)) return;
    int eid = atoi(buffer);

    char path[100], temp_path[100];
    snprintf(path, 100, "%s/loans.csv", DATA_DIR);
    snprintf(temp_path, 100, "%s/loans.tmp", DATA_DIR);

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
        Loan loan;
        if (sscanf(line, "%d,%d,%lf,%19[^,],%d",
                   &loan.loan_id, &loan.customer_id, &loan.amount, loan.status, &loan.assigned_employee) == 5) {
            if (loan.loan_id == lid) {
                loan.assigned_employee = eid;
                found = 1;
            }
            fprintf(tmp, "%d,%d,%.2f,%s,%d\n",
                    loan.loan_id, loan.customer_id, loan.amount, loan.status, loan.assigned_employee);
        }
    }

    fclose(fp);
    fclose(tmp);
    rename(temp_path, path);

    send_message(sock, found ? "Assigned\n" : "Not found\n");
}

static void view_feedback(int sock) {
    char path[100];
    snprintf(path, 100, "%s/feedback.csv", DATA_DIR);

    FILE *fp = fopen(path, "r");
    if (!fp) {
        send_message(sock, "No feedback\n");
        return;
    }

    char line[300];
    char buffer[400];

    send_message(sock, "\n=== Feedback ===\n");

    // Skip header
    fgets(line, sizeof(line), fp);
    int count = 0;

    while (fgets(line, sizeof(line), fp)) {
        Feedback fb;
        if (sscanf(line, "%d,%d,%255[^,],%d",
                   &fb.feedback_id, &fb.customer_id, fb.message, &fb.reviewed) == 4) {
            snprintf(buffer, 400, "ID:%d Customer:%d\nMsg:%s\n\n",
                     fb.feedback_id, fb.customer_id, fb.message);
            send_message(sock, buffer);
            count++;
        }
    }

    if (count == 0) {
        send_message(sock, "No feedback available\n");
    }

    fclose(fp);
}

void handle_manager(int sock, int mid) {
    char buffer[BUFFER_SIZE];

    while (1) {
        send_message(sock,
            "\n=== Manager Menu ===\n"
            "1. Activate/Deactivate Account\n"
            "2. Assign Loan to Employee\n"
            "3. View Feedback\n"
            "4. Change Password\n"
            "5. Logout\n"
            "Choice: ");

        if (!read_input(sock, buffer, BUFFER_SIZE)) break;
        int choice = atoi(buffer);

        switch (choice) {
            case 1: activate_account(sock); break;
            case 2: assign_loan(sock); break;
            case 3: view_feedback(sock); break;
            case 4: change_password(sock, mid); break;
            case 5: return;
            default: send_message(sock, "Invalid choice\n");
        }
    }
}


