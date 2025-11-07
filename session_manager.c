#include "session_manager.h"
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static SessionManager *session_mgr = NULL;
static int sem_sessions;
static int sem_balance;

#if defined(__linux__)
union semun {
    int val;
    struct semid_ds *buf;
    unsigned short *array;
};
#endif

static void sem_wait(int sem_id) {
    struct sembuf op = {0, -1, SEM_UNDO};
    semop(sem_id, &op, 1);
}

static void sem_signal(int sem_id) {
    struct sembuf op = {0, 1, SEM_UNDO};
    semop(sem_id, &op, 1);
}

static int create_semaphore(key_t key) {
    int sem_id = semget(key, 1, IPC_CREAT | 0666);
    if (sem_id == -1) {
        perror("semget failed");
        exit(1);
    }
    union semun arg;
    arg.val = 1;
    semctl(sem_id, 0, SETVAL, arg);
    return sem_id;
}

void init_session_manager() {
    // Create shared memory segment
    int shm_id = shmget(SHM_KEY_SESSIONS, sizeof(SessionManager), IPC_CREAT | 0666);
    if (shm_id == -1) {
        perror("shmget failed");
        exit(1);
    }

    // Attach shared memory
    session_mgr = (SessionManager*)shmat(shm_id, NULL, 0);
    if (session_mgr == (void*)-1) {
        perror("shmat failed");
        exit(1);
    }

    // Initialize session data
    session_mgr->session_count = 0;
    memset(session_mgr->sessions, 0, sizeof(session_mgr->sessions));

    // Create semaphores
    sem_sessions = create_semaphore(SHM_KEY_SESSIONS + 1);
    sem_balance = create_semaphore(SHM_KEY_SESSIONS + 2);

    printf("✓ Session manager initialized (shared memory + semaphores)\n");
}

int is_user_logged_in(int role, int user_id) {
    if (!session_mgr) return 0;

    sem_wait(sem_sessions);

    int logged_in = 0;
    for (int i = 0; i < session_mgr->session_count; i++) {
        if (session_mgr->sessions[i].role == role &&
            session_mgr->sessions[i].user_id == user_id &&
            session_mgr->sessions[i].active) {
            logged_in = 1;
            break;
        }
    }

    sem_signal(sem_sessions);
    return logged_in;
}

int register_login(int role, int user_id) {
    if (!session_mgr) return 0;

    sem_wait(sem_sessions);

    // Check if already logged in
    for (int i = 0; i < session_mgr->session_count; i++) {
        if (session_mgr->sessions[i].role == role &&
            session_mgr->sessions[i].user_id == user_id &&
            session_mgr->sessions[i].active) {
            sem_signal(sem_sessions);
            return 0;  // Already logged in
        }
    }

    // Find empty slot or create new
    int slot = -1;
    for (int i = 0; i < session_mgr->session_count; i++) {
        if (!session_mgr->sessions[i].active) {
            slot = i;
            break;
        }
    }

    if (slot == -1 && session_mgr->session_count < MAX_SESSIONS) {
        slot = session_mgr->session_count++;
    }

    if (slot != -1) {
        session_mgr->sessions[slot].role = role;
        session_mgr->sessions[slot].user_id = user_id;
        session_mgr->sessions[slot].active = 1;
    }

    sem_signal(sem_sessions);
    return slot != -1;
}

void register_logout(int role, int user_id) {
    if (!session_mgr) return;

    sem_wait(sem_sessions);

    for (int i = 0; i < session_mgr->session_count; i++) {
        if (session_mgr->sessions[i].role == role &&
            session_mgr->sessions[i].user_id == user_id) {
            session_mgr->sessions[i].active = 0;
            break;
        }
    }

    sem_signal(sem_sessions);
}

void lock_balance_operation() {
    sem_wait(sem_balance);
}

void unlock_balance_operation() {
    sem_signal(sem_balance);
}
