#ifndef COMMON_H
#define COMMON_H

#define PORT 8080
#define BUFFER_SIZE 1024
#define DATA_DIR "data"

// Shared memory configuration
#define MAX_SESSIONS 100
#define SHM_KEY_SESSIONS 9999

typedef struct {
    int id;
    char username[50];
    char password[50];
    double balance;
    int active;
    int logged_in;
} Customer;

typedef struct {
    int id;
    char username[50];
    char password[50];
    int logged_in;
} Employee;

typedef struct {
    int id;
    char username[50];
    char password[50];
    int logged_in;
} Manager;

typedef struct {
    int id;
    char username[50];
    char password[50];
    int logged_in;
} Admin;

typedef struct {
    int customer_id;
    char type[20];
    double amount;
    double balance_after;
    char timestamp[50];
} Transaction;

typedef struct {
    int loan_id;
    int customer_id;
    double amount;
    char status[20];
    int assigned_employee;
} Loan;

typedef struct {
    int feedback_id;
    int customer_id;
    char message[256];
    int reviewed;
} Feedback;

// Session tracking structure
typedef struct {
    int user_id;
    int role;
    int active;
} Session;

typedef struct {
    Session sessions[MAX_SESSIONS];
    int session_count;
} SessionManager;

#define ROLE_CUSTOMER 1
#define ROLE_EMPLOYEE 2
#define ROLE_MANAGER 3
#define ROLE_ADMIN 4

#endif
