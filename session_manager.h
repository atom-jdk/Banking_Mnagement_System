#ifndef SESSION_MANAGER_H
#define SESSION_MANAGER_H

#include "common.h"

// Initialize shared memory session manager
void init_session_manager();

// Session management
int is_user_logged_in(int role, int user_id);
int register_login(int role, int user_id);
void register_logout(int role, int user_id);

// Critical section locks for balance operations
void lock_balance_operation();
void unlock_balance_operation();

#endif
