#ifndef IPC_H
#define IPC_H

#include <stddef.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <sys/select.h>
#include "job.h"

/* Shared Memory Structure storing real-time scheduler state */
typedef struct {
    int total_jobs;
    int running_jobs;
    int completed_jobs;
    int failed_jobs;
    int cancelled_jobs;
    int active_workers;
    time_t scheduler_start_time;
    Job jobs[MAX_JOBS];
} SharedSchedulerState;

/* Creates a unidirectional anonymous pipe */
int ipc_create_pipe(int pipefd[2]);

/* Sends a string message over specified pipe write file descriptor */
int ipc_send_message(int write_fd, const char *msg);

/* Non-blocking read of messages from pipe read file descriptor */
int ipc_read_message(int read_fd, char *buf, size_t max_len);

/* Monitor array of pipe read descriptors using select() */
int ipc_select_pipes(const int *read_fds, int count, int *ready_flags, int timeout_ms);

/* System V Shared Memory Management Functions */
key_t ipc_get_key(const char *path, int proj_id);
int ipc_shm_create(key_t key, size_t size, int *out_shmid);
void *ipc_shm_attach(int shmid);
int ipc_shm_detach(const void *shmaddr);
int ipc_shm_remove(int shmid);

/* System V Semaphore Synchronization Functions */
int ipc_sem_create(key_t key, int *out_semid);
int ipc_sem_init(int semid, int val);
int ipc_sem_lock(int semid);   /* P() Operation: Wait / Decrement */
int ipc_sem_unlock(int semid); /* V() Operation: Signal / Increment */
int ipc_sem_remove(int semid);

#endif /* IPC_H */
