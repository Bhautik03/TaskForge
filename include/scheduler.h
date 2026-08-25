#ifndef SCHEDULER_H
#define SCHEDULER_H

#include "job.h"

typedef struct {
    Job jobs[MAX_JOBS];
    int job_count;
    int next_job_id;
} Scheduler;

void scheduler_init(Scheduler *sched);
int scheduler_submit_job(Scheduler *sched, const char *raw_cmd, int priority);
void scheduler_list_jobs(const Scheduler *sched);
void scheduler_job_status(const Scheduler *sched, int job_id);

#endif /* SCHEDULER_H */
