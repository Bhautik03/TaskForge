#ifndef SCHEDULER_H
#define SCHEDULER_H

#include "job.h"
#include "worker.h"
#include "ipc.h"

typedef struct {
    Job jobs[MAX_JOBS];
    int job_count;
    int next_job_id;
    Worker workers[MAX_WORKERS];
    int active_workers;
    int shmid;                       /* System V Shared Memory Identifier */
    SharedSchedulerState *shm;      /* Pointer to attached Shared Memory */
    int semid;                       /* System V Semaphore Identifier */
} Scheduler;

void scheduler_init(Scheduler *sched);
void scheduler_cleanup(Scheduler *sched);

/*
 * Phase 16: Robust Shutdown Sequence
 *
 * Performs ordered teardown:
 *   1. Stop accepting new jobs (caller responsibility: exit main loop)
 *   2. Send SIGTERM to all running workers (grace period)
 *   3. Wait for workers to exit (blocking waitpid with timeout)
 *   4. Send SIGKILL to any remaining workers
 *   5. Reap all children (waitpid loop)
 *   6. Close all open pipe file descriptors
 *   7. Detach shared memory region
 *   8. Remove (destroy) shared memory segment (IPC_RMID)
 *   9. Remove semaphore set (IPC_RMID)
 */
void scheduler_shutdown(Scheduler *sched);
void scheduler_sync_shm(Scheduler *sched);
int scheduler_submit_job(Scheduler *sched, const char *raw_cmd, int priority);
void scheduler_dispatch(Scheduler *sched);
void scheduler_reap_workers(Scheduler *sched);
void scheduler_wait_all(Scheduler *sched);
void scheduler_list_jobs(const Scheduler *sched);
void scheduler_job_status(const Scheduler *sched, int job_id);

/* Phase 8: Signal control functions */
int scheduler_cancel_job(Scheduler *sched, int job_id);
int scheduler_pause_job(Scheduler *sched, int job_id);
int scheduler_resume_job(Scheduler *sched, int job_id);

#endif /* SCHEDULER_H */
