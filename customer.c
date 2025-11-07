#include "customer.h"
#include "common.h"
#include "utils.h"
#include "session_manager.h"
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <time.h>
#include <stdlib.h>

static void view_my_loans(int sock, int cid) {
    char path[100];
    snprintf(path, 100, "%s/loans.csv", DATA_DIR);

    FILE *fp = fopen(path, "r");
    if (!fp) {
        send_message(sock, "No loans found\n");
        return;
    }

    char line[256], buffer[200];
    fgets(line, sizeof(line), fp);

    int found = 0;
    while (fgets(line, sizeof(line), fp)) {
        Loan loan;
        if (sscanf(line, "%d,%d,%lf,%19[^,],%d",
                   &loan.loan_id, &loan.customer_id, &loan.amount, loan.status, &loan.assigned_employee) == 5) {

            if (loan.customer_id == cid) {
                snprintf(buffer, 200, "Loan ID:%d Amount:%.2f Status:%s\n",
                         loan.loan_id, loan.amount, loan.status);
                send_message(sock, buffer);
                found = 1;
            }
        }
    }

    if (!found) send_message(sock, "No loans for your account\n");
    fclose(fp);
}


static void add_transaction(int cid, const char *type, double amt, double bal) {
    char path[100];
    snprintf(path, 100, "%s/transactions.csv", DATA_DIR);

    FILE *fp = fopen(path, "a");
    if (!fp) return;

    time_t now = time(NULL);
    char timestamp[50];
    strftime(timestamp, 50, "%Y-%m-%d %H:%M:%S", localtime(&now));

    fprintf(fp, "%d,%s,%.2f,%.2f,%s\n", cid, type, amt, bal, timestamp);
    fclose(fp);
}

static void view_balance(int sock, int cid) {
    char path[100];
    snprintf(path, 100, "%s/customers.csv", DATA_DIR);

    FILE *fp = fopen(path, "r");
    if (!fp) {
        send_message(sock, "Error opening file\n");
        return;
    }

    char line[256];
    char msg[100];
    int found = 0;

    fgets(line, sizeof(line), fp);
    while (fgets(line, sizeof(line), fp)) {
        Customer c;
        if (sscanf(line, "%d,%49[^,],%49[^,],%lf,%d", 
            &c.id, c.username, c.password, &c.balance, &c.active) == 5) {
            
            if (c.id == cid) {
                snprintf(msg, 100, "Balance: %.2f\n", c.balance);
                found = 1;
                break;
            }
        }
    }

    fclose(fp);
    send_message(sock, found ? msg : "Account not found\n");
}

static void deposit_money(int sock, int cid) {
    char buffer[BUFFER_SIZE];

    send_message(sock, "Enter amount: ");
    if (!read_input(sock, buffer, BUFFER_SIZE)) return;

    double amount = atof(buffer);
    if (amount <= 0) {
        send_message(sock, "Invalid amount\n");
        return;
    }

    char path[100], temp_path[100];
    snprintf(path, 100, "%s/customers.csv", DATA_DIR);
    snprintf(temp_path, 100, "%s/customers.tmp", DATA_DIR);

    lock_balance_operation();

    FILE *fp = fopen(path, "r");
    FILE *tmp = fopen(temp_path, "w");

    if (!fp || !tmp) {
        if (fp) fclose(fp);
        if (tmp) fclose(tmp);
        unlock_balance_operation();
        send_message(sock, "Error opening file\n");
        return;
    }

    char line[256];
    int found = 0;
    double new_balance = 0;

    fgets(line, sizeof(line), fp);
    fprintf(tmp, "%s", line);

    while (fgets(line, sizeof(line), fp)) {
        Customer c;
        if (sscanf(line, "%d,%49[^,],%49[^,],%lf,%d", 
            &c.id, c.username, c.password, &c.balance, &c.active) == 5) {
            
            if (c.id == cid) {
                c.balance += amount;
                new_balance = c.balance;
                found = 1;
            }
            fprintf(tmp, "%d,%s,%s,%.2f,%d\n",
                c.id, c.username, c.password, c.balance, c.active);
        }
    }

    fclose(fp);
    fclose(tmp);
    rename(temp_path, path);
    unlock_balance_operation();

    if (found) {
        add_transaction(cid, "DEPOSIT", amount, new_balance);
        snprintf(buffer, BUFFER_SIZE, "Success! New balance: %.2f\n", new_balance);
        send_message(sock, buffer);
    } else {
        send_message(sock, "Account not found\n");
    }
}

static void withdraw_money(int sock, int cid) {
    char buffer[BUFFER_SIZE];

    send_message(sock, "Enter amount: ");
    if (!read_input(sock, buffer, BUFFER_SIZE)) return;

    double amount = atof(buffer);
    if (amount <= 0) {
        send_message(sock, "Invalid amount\n");
        return;
    }

    char path[100], temp_path[100];
    snprintf(path, 100, "%s/customers.csv", DATA_DIR);
    snprintf(temp_path, 100, "%s/customers.tmp", DATA_DIR);

    lock_balance_operation();

    FILE *fp = fopen(path, "r");
    FILE *tmp = fopen(temp_path, "w");

    if (!fp || !tmp) {
        if (fp) fclose(fp);
        if (tmp) fclose(tmp);
        unlock_balance_operation();
        send_message(sock, "Error opening file\n");
        return;
    }

    char line[256];
    int found = 0;
    double new_balance = 0;
    int insufficient = 0;

    fgets(line, sizeof(line), fp);
    fprintf(tmp, "%s", line);

    while (fgets(line, sizeof(line), fp)) {
        Customer c;
        if (sscanf(line, "%d,%49[^,],%49[^,],%lf,%d",
            &c.id, c.username, c.password, &c.balance, &c.active) == 5) {
            
            if (c.id == cid) {
                if (c.balance < amount) {
                    insufficient = 1;
                } else {
                    c.balance -= amount;
                    new_balance = c.balance;
                    found = 1;
                }
            }
            fprintf(tmp, "%d,%s,%s,%.2f,%d\n",
                c.id, c.username, c.password, c.balance, c.active);
        }
    }

    fclose(fp);
    fclose(tmp);

    if (!insufficient) {
        rename(temp_path, path);
    } else {
        remove(temp_path);
    }

    unlock_balance_operation();

    if (insufficient) {
        send_message(sock, "Insufficient balance\n");
    } else if (found) {
        add_transaction(cid, "WITHDRAWAL", amount, new_balance);
        snprintf(buffer, BUFFER_SIZE, "Success! New balance: %.2f\n", new_balance);
        send_message(sock, buffer);
    } else {
        send_message(sock, "Account not found\n");
    }
}

static void transfer_funds(int sock, int cid) {
    char buffer[BUFFER_SIZE];

    send_message(sock, "Enter recipient ID: ");
    if (!read_input(sock, buffer, BUFFER_SIZE)) return;
    int recipient_id = atoi(buffer);

    send_message(sock, "Enter amount: ");
    if (!read_input(sock, buffer, BUFFER_SIZE)) return;
    double amount = atof(buffer);

    if (amount <= 0 || recipient_id == cid) {
        send_message(sock, "Invalid transfer\n");
        return;
    }

    char path[100], temp_path[100];
    snprintf(path, 100, "%s/customers.csv", DATA_DIR);
    snprintf(temp_path, 100, "%s/customers.tmp", DATA_DIR);

    lock_balance_operation();

    FILE *fp = fopen(path, "r");
    FILE *tmp = fopen(temp_path, "w");

    if (!fp || !tmp) {
        if (fp) fclose(fp);
        if (tmp) fclose(tmp);
        unlock_balance_operation();
        send_message(sock, "Error opening file\n");
        return;
    }

    char line[256];
    int sender_found = 0, recipient_found = 0;
    double sender_balance = 0, recipient_balance = 0;
    int insufficient = 0;

    fgets(line, sizeof(line), fp);
    fprintf(tmp, "%s", line);

    while (fgets(line, sizeof(line), fp)) {
        Customer c;
        if (sscanf(line, "%d,%49[^,],%49[^,],%lf,%d",
            &c.id, c.username, c.password, &c.balance, &c.active) == 5) {
            
            if (c.id == cid) {
                sender_found = 1;
                if (c.balance < amount) {
                    insufficient = 1;
                } else {
                    c.balance -= amount;
                    sender_balance = c.balance;
                }
            } else if (c.id == recipient_id) {
                recipient_found = 1;
                c.balance += amount;
                recipient_balance = c.balance;
            }
            fprintf(tmp, "%d,%s,%s,%.2f,%d\n",
                c.id, c.username, c.password, c.balance, c.active);
        }
    }

    fclose(fp);
    fclose(tmp);

    if (!insufficient && sender_found && recipient_found) {
        rename(temp_path, path);
        unlock_balance_operation();

        add_transaction(cid, "TRANSFER_OUT", amount, sender_balance);
        add_transaction(recipient_id, "TRANSFER_IN", amount, recipient_balance);

        snprintf(buffer, BUFFER_SIZE, "Transfer successful! Balance: %.2f\n", sender_balance);
        send_message(sock, buffer);
    } else {
        remove(temp_path);
        unlock_balance_operation();

        if (insufficient) {
            send_message(sock, "Insufficient balance\n");
        } else {
            send_message(sock, "Invalid accounts\n");
        }
    }
}

static void apply_loan(int sock, int cid) {
    char buffer[BUFFER_SIZE];

    send_message(sock, "Enter loan amount: ");
    if (!read_input(sock, buffer, BUFFER_SIZE)) return;

    double amount = atof(buffer);
    if (amount <= 0) {
        send_message(sock, "Invalid amount\n");
        return;
    }

    char path[100];
    snprintf(path, 100, "%s/loans.csv", DATA_DIR);

    int loan_id = get_next_id_csv(path);
    FILE *fp = fopen(path, "a");
    if (!fp) return;

    fprintf(fp, "%d,%d,%.2f,%s,%d\n", loan_id, cid, amount, "pending", -1);
    fclose(fp);
    send_message(sock, "Loan application submitted\n");
}
static void add_feedback(int sock, int cid) {
    char buffer[BUFFER_SIZE];

    send_message(sock, "Enter your feedback: ");
    if (!read_input(sock, buffer, BUFFER_SIZE)) return;

    // Trim new line if any
    buffer[strcspn(buffer, "\n")] = 0;

    char path[100];
    snprintf(path, 100, "%s/feedback.csv", DATA_DIR);

    // Get next feedback ID
    int feedback_id = get_next_id_csv(path);

    FILE *fp = fopen(path, "a");
    if (!fp) {
        send_message(sock, "Error saving feedback\n");
        return;
    }

    fprintf(fp, "%d,%d,%s,%d\n", 
            feedback_id, 
            cid, 
            buffer, 
            0 // reviewed flag = 0 (pending)
    );

    fclose(fp);
    send_message(sock, "✅ Thank you! Your feedback has been submitted.\n");
}


 static void change_password(int sock, int cid) {
    char buffer[BUFFER_SIZE];

    send_message(sock, "Enter current password: ");
    if (!read_input(sock, buffer, BUFFER_SIZE)) return;
    char current_pass[50];
    strcpy(current_pass, buffer);

    send_message(sock, "Enter new password: ");
    if (!read_input(sock, buffer, BUFFER_SIZE)) return;
    char new_pass[50];
    strcpy(new_pass, buffer);

    char path[100], temp_path[100];
    snprintf(path, 100, "%s/customers.csv", DATA_DIR);
    snprintf(temp_path, 100, "%s/customers.tmp", DATA_DIR);

    FILE *fp = fopen(path, "r");
    FILE *tmp = fopen(temp_path, "w");

    if (!fp || !tmp) {
        if(fp) fclose(fp);
        if(tmp) fclose(tmp);
        send_message(sock, "Error opening file\n");
        return;
    }

    char line[256];
    int updated = 0;

    fgets(line, sizeof(line), fp);
    fprintf(tmp, "%s", line);

    while (fgets(line, sizeof(line), fp)) {
        Customer c;
        if (sscanf(line, "%d,%49[^,],%49[^,],%lf,%d",
            &c.id, c.username, c.password, &c.balance, &c.active) == 5) {
            
            if (c.id == cid && strcmp(c.password, current_pass) == 0) {
                strcpy(c.password, new_pass);
                updated = 1;
            }

            fprintf(tmp, "%d,%s,%s,%.2f,%d\n",
                c.id, c.username, c.password, c.balance, c.active);
        }
    }

    fclose(fp);
    fclose(tmp);

    if (updated) {
        rename(temp_path, path);
        send_message(sock, "Password changed successfully!\n");
    } else {
        remove(temp_path);
        send_message(sock, "Current password incorrect!\n");
    }
}



static void view_transactions(int sock, int cid) {
    char path[100];
    snprintf(path, 100, "%s/transactions.csv", DATA_DIR);

    FILE *fp = fopen(path, "r");
    if (!fp) {
        send_message(sock, "No transactions\n");
        return;
    }

    char line[256];
    char buffer[200];

    send_message(sock, "\n=== Transaction History ===\n");
    fgets(line, sizeof(line), fp);

    int count = 0;
    while (fgets(line, sizeof(line), fp)) {
        Transaction t;
        if (sscanf(line, "%d,%19[^,],%lf,%lf,%49[^\n]",
            &t.customer_id, t.type, &t.amount, &t.balance_after, t.timestamp) == 5) {
            
            if (t.customer_id == cid) {
                snprintf(buffer, 200, "%s: %.2f | Balance: %.2f | %s\n",
                    t.type, t.amount, t.balance_after, t.timestamp);
                
                send_message(sock, buffer);
                count++;
            }
        }
    }

    if (count == 0) {
        send_message(sock, "No transactions found\n");
    }

    fclose(fp);
}

void handle_customer(int sock, int cid) {
    char buffer[BUFFER_SIZE];

    while (1) {
        send_message(sock,
            "\n=== Customer Menu ===\n"
            "1. View Balance\n"
            "2. Deposit\n"
            "3. Withdraw\n"
            "4. Transfer\n"
            "5. Apply Loan\n"
            "6. View Loan Status\n"
            "7. View Transactions\n"
            "8. Change Password\n"
            "9. Add Feedback\n"
            "10. Logout\n"
            "Choice: "
        );

        if (!read_input(sock, buffer, BUFFER_SIZE)) break;
        int choice = atoi(buffer);

        switch (choice) {
            case 1: view_balance(sock, cid); break;
            case 2: deposit_money(sock, cid); break;
            case 3: withdraw_money(sock, cid); break;
            case 4: transfer_funds(sock, cid); break;
            case 5: apply_loan(sock, cid); break;
            case 6: view_my_loans(sock, cid); break;   // ✅ New function call
            case 7: view_transactions(sock, cid); break;
            case 8: change_password(sock, cid); break;
            case 9: add_feedback(sock, cid); break;
            case 10: return;
            default: send_message(sock, "Invalid choice\n");
        }
    }
}

