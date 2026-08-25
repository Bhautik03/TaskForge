#ifndef PROCESS_H
#define PROCESS_H

#include <sys/types.h>
#include "job.h"

/* Spawns a child process using fork() to execute a dummy test action for the job */
int process_execute_job_dummy(Job *job);

#endif /* PROCESS_H */
