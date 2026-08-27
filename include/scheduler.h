#ifndef SCHEDULER_H
#define SCHEDULER_H

#include "job.h"
#include "worker.h"

typedef struct {
    Job jobs[MAX_JOBS];
    int job_count;
    int next_job_id;
    Worker workers[MAX_WORKERS];
    int active_workers;
} Scheduler;

void scheduler_init(Scheduler *sched);
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
