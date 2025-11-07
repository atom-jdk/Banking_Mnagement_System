

// auth.h
#ifndef AUTH_H
#define AUTH_H

int authenticate(int role, const char *username, const char *password, int *user_id);
void logout_user(int role, int user_id);

#endif
