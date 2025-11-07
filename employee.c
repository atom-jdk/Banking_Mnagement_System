#include "employee.h"
#include "common.h"
#include "utils.h"
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

static void add_customer(int sock) {
    char buffer[BUFFER_SIZE];
    Customer c = {0};

    char path[100];
    snprintf(path, 100, "%s/customers.csv", DATA_DIR);

    c.id = get_next_id_csv(path);
    c.active = 1;

    send_message(sock, "Enter username: ");
    if (!read_input(sock, buffer, BUFFER_SIZE)) return;
    strcpy(c.username, buffer);

    send_message(sock, "Enter password: ");
    if (!read_input(sock, buffer, BUFFER_SIZE)) return;
    strcpy(c.password, buffer);

    send_message(sock, "Initial balance: ");
    if (!read_input(sock, buffer, BUFFER_SIZE)) return;
    c.balance = atof(buffer);

    FILE *fp = fopen(path, "a");
    if (!fp) return;

    fprintf(fp, "%d,%s,%s,%.2f,%d\n", c.id, c.username, c.password, c.balance, c.active);
    fclose(fp);

    snprintf(buffer, BUFFER_SIZE, "Customer created! ID: %d\n", c.id);
    send_message(sock, buffer);
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

            fprintf(tmp, "%d,%s,%s,%.2f,%d\n", c.id, c.username, c.password, c.balance, c.active);
        }
    }

    fclose(fp);
    fclose(tmp);
    rename(temp_path, path);
    send_message(sock, found ? "Updated\n" : "Not found\n");
}

static void view_assigned_loans(int sock, int eid) {
    char path[100];
    snprintf(path, 100, "%s/loans.csv", DATA_DIR);

    FILE *fp = fopen(path, "r");
    if (!fp) {
        send_message(sock, "No loans\n");
        return;
    }

    char line[256];
    char buffer[200];
    send_message(sock, "\n=== Assigned Loans ===\n");

    fgets(line, sizeof(line), fp);
    int count = 0;

    while (fgets(line, sizeof(line), fp)) {
        Loan loan;
        if (sscanf(line, "%d,%d,%lf,%19[^,],%d",
                   &loan.loan_id, &loan.customer_id, &loan.amount, loan.status, &loan.assigned_employee) == 5) {
            if (loan.assigned_employee == eid) {
                snprintf(buffer, 200, "ID:%d Customer:%d Amount:%.2f Status:%s\n",
                         loan.loan_id, loan.customer_id, loan.amount, loan.status);
                send_message(sock, buffer);
                count++;
            }
        }
    }

    if (count == 0) send_message(sock, "No loans assigned to you\n");

    fclose(fp);
}

static void process_loan(int sock, int eid) {
    char buffer[BUFFER_SIZE];

    send_message(sock, "Enter loan ID: ");
    if (!read_input(sock, buffer, BUFFER_SIZE)) return;
    int lid = atoi(buffer);

    send_message(sock, "Approve (1) or Reject (0): ");
    if (!read_input(sock, buffer, BUFFER_SIZE)) return;
    int approve = atoi(buffer);

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

    fgets(line, sizeof(line), fp);
    fprintf(tmp, "%s", line);

    while (fgets(line, sizeof(line), fp)) {
        Loan loan;
        if (sscanf(line, "%d,%d,%lf,%19[^,],%d",
                   &loan.loan_id, &loan.customer_id, &loan.amount, loan.status, &loan.assigned_employee) == 5) {
            if (loan.loan_id == lid && loan.assigned_employee == eid) {
                strcpy(loan.status, approve ? "approved" : "rejected");
                found = 1;
            }

            fprintf(tmp, "%d,%d,%.2f,%s,%d\n", loan.loan_id, loan.customer_id,
                    loan.amount, loan.status, loan.assigned_employee);
        }
    }

    fclose(fp);
    fclose(tmp);
    rename(temp_path, path);
    send_message(sock, found ? "Processed\n" : "Not found\n");
}

void handle_employee(int sock, int eid) {
    char buffer[BUFFER_SIZE];

    while (1) {
        send_message(sock,
            "\n=== Employee Menu ===\n"
            "1. Add Customer\n"
            "2. Modify Customer\n"
            "3. View Assigned Loans\n"
            "4. Process Loan\n"
            "5. Logout\n"
            "Choice: ");

        if (!read_input(sock, buffer, BUFFER_SIZE)) break;
        int choice = atoi(buffer);

        switch (choice) {
            case 1: add_customer(sock); break;
            case 2: modify_customer(sock); break;
            case 3: view_assigned_loans(sock, eid); break;
            case 4: process_loan(sock, eid); break;
            case 5: return;
            default: send_message(sock, "Invalid choice\n");
        }
    }
}

