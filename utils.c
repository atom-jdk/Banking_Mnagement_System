#include "utils.h"
#include "common.h"
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>

void lock_read(int fd) {
    struct flock lock;
    lock.l_type = F_RDLCK;
    lock.l_whence = SEEK_SET;
    lock.l_start = 0;
    lock.l_len = 0;
    fcntl(fd, F_SETLKW, &lock);
}

void lock_write(int fd) {
    struct flock lock;
    lock.l_type = F_WRLCK;
    lock.l_whence = SEEK_SET;
    lock.l_start = 0;
    lock.l_len = 0;
    fcntl(fd, F_SETLKW, &lock);
}

void unlock_file(int fd) {
    struct flock lock;
    lock.l_type = F_UNLCK;
    lock.l_whence = SEEK_SET;
    lock.l_start = 0;
    lock.l_len = 0;
    fcntl(fd, F_SETLK, &lock);
}

void send_message(int sock, const char *msg) {
    write(sock, msg, strlen(msg));
}

int read_input(int sock, char *buffer, int size) {
    memset(buffer, 0, size);
    int n = read(sock, buffer, size);
    if (n > 0) buffer[strcspn(buffer, "\n")] = 0;
    return n > 0;
}

int get_next_id(const char *filename, int record_size) {
    int fd = open(filename, O_RDONLY);
    if (fd == -1) return 1;

    lock_read(fd);
    int max_id = 0;
    char buffer[record_size];

    while (read(fd, buffer, record_size) == record_size) {
        int id = *(int *)buffer;
        if (id > max_id) max_id = id;
    }

    unlock_file(fd);
    close(fd);
    return max_id + 1;
}

// NEW: Get next ID from CSV file
int get_next_id_csv(const char *csv_path) {
    FILE *fp = fopen(csv_path, "r");
    if (!fp) return 1;

    char line[512];
    int max_id = 0;

    // Skip header
    fgets(line, sizeof(line), fp);

    // Read all lines and find max ID
    while (fgets(line, sizeof(line), fp)) {
        int id;
        if (sscanf(line, "%d,", &id) == 1) {
            if (id > max_id) max_id = id;
        }
    }

    fclose(fp);
    return max_id + 1;
}

// NEW: Remove trailing newline
void trim_newline(char *str) {
    str[strcspn(str, "\n")] = 0;
}
