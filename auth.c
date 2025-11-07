#include "auth.h"
#include "common.h"
#include "utils.h"
#include "session_manager.h"
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>

static const char* get_file_path(int role) {
    static char path[100];
    switch (role) {
        case ROLE_CUSTOMER: snprintf(path, 100, "%s/customers.csv", DATA_DIR); break;
        case ROLE_EMPLOYEE: snprintf(path, 100, "%s/employees.csv", DATA_DIR); break;
        case ROLE_MANAGER: snprintf(path, 100, "%s/managers.csv", DATA_DIR); break;
        case ROLE_ADMIN: snprintf(path, 100, "%s/admins.csv", DATA_DIR); break;
        default: return NULL;
    }
    return path;
}

int authenticate(int role, const char *username, const char *password, int *user_id) {
    const char *filepath = get_file_path(role);
    if (!filepath) {
        printf("DEBUG: Invalid role %d\n", role);
        return 0;
    }

    printf("DEBUG: Opening file: %s\n", filepath);
    FILE *fp = fopen(filepath, "r");
    if (!fp) {
        printf("DEBUG: Cannot open file: %s\n", filepath);
        perror("fopen");
        return 0;
    }

    char line[256];
    int authenticated = 0;

    // Skip header
    if (!fgets(line, sizeof(line), fp)) {
        printf("DEBUG: Cannot read header\n");
        fclose(fp);
        return 0;
    }
    printf("DEBUG: Header: %s", line);

    if (role == ROLE_CUSTOMER) {
        printf("DEBUG: Searching for customer '%s' with password '%s'\n", username, password);
        int line_num = 1;

        while (fgets(line, sizeof(line), fp)) {
            line_num++;
            Customer user;
            int parsed = sscanf(line, "%d,%49[^,],%49[^,],%lf,%d",
                       &user.id, user.username, user.password, &user.balance, &user.active);

            printf("DEBUG: Line %d - Parsed %d fields: id=%d user='%s' pass='%s'\n",
                   line_num, parsed, user.id, user.username, user.password);

            if (parsed == 5) {
                if (strcmp(user.username, username) == 0 &&
                    strcmp(user.password, password) == 0) {

                    printf("DEBUG: Match found! Checking session...\n");

                    if (is_user_logged_in(role, user.id)) {
                        printf("DEBUG: User already logged in\n");
                        fclose(fp);
                        return 0;
                    }

                    *user_id = user.id;

                    if (register_login(role, user.id)) {
                        printf("DEBUG: Login successful!\n");
                        authenticated = 1;
                    } else {
                        printf("DEBUG: Failed to register login\n");
                    }
                    break;
                }
            } else {
                printf("DEBUG: Parse failed for line: %s", line);
            }
        }

        if (!authenticated) {
            printf("DEBUG: No match found for user '%s'\n", username);
        }
    } else {
        // Employee, Manager, Admin (same structure: id,username,password)
        while (fgets(line, sizeof(line), fp)) {
            int id;
            char uname[50], pwd[50];

            if (sscanf(line, "%d,%49[^,],%49[^\n]", &id, uname, pwd) == 3) {
                if (strcmp(uname, username) == 0 && strcmp(pwd, password) == 0) {

                    if (is_user_logged_in(role, id)) {
                        fclose(fp);
                        return 0;
                    }

                    *user_id = id;

                    if (register_login(role, id)) {
                        authenticated = 1;
                    }
                    break;
                }
            }
        }
    }

    fclose(fp);
    return authenticated;
}


void logout_user(int role, int user_id) {
    register_logout(role, user_id);
}
