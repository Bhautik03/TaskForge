#ifndef WORKER_H
#define WORKER_H

#include <sys/types.h>
#include "common.h"

typedef struct {
    int worker_id;      /* Slot index (0, 1, 2) */
    int active;         /* 1 if slot is active, 0 if idle */
    int current_job_id; /* ID of assigned Job (-1 if idle) */
    pid_t pid;          /* PID of child process (-1 if idle) */
} Worker;

void worker_init(Worker *w, int id);

#endif /* WORKER_H */
