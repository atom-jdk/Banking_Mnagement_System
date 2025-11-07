#ifndef UTILS_H
#define UTILS_H

void lock_read(int fd);
void lock_write(int fd);
void unlock_file(int fd);
void send_message(int sock, const char *msg);
int read_input(int sock, char *buffer, int size);
int get_next_id(const char *filename, int record_size);

// NEW: CSV utility functions
int get_next_id_csv(const char *csv_path);
void trim_newline(char *str);

#endif
