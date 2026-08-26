#ifndef PROCESS_H
#define PROCESS_H

#include <sys/types.h>
#include "job.h"

/* Spawns child process asynchronously using fork() + execvp() */
pid_t process_spawn_job_async(Job *job);

/* Analyzes status integer from waitpid() and updates job state, exit code, and duration */
void process_evaluate_exit_status(Job *job, int status);

#endif /* PROCESS_H */
