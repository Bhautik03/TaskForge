#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <sys/wait.h>
#include <time.h>
#include "process.h"

pid_t process_spawn_job_async(Job *job)
{
    if (!job || job->arg_count == 0 || job->args[0] == NULL) {
        printf("Error: No command provided for execution.\n");
        return -1;
    }

    job->started_at = time(NULL);

    pid_t pid = fork();

    if (pid < 0) {
        perror("fork failed");
        job->state = JOB_STATE_FAILED;
        job->completed_at = time(NULL);
        job->duration = difftime(job->completed_at, job->started_at);
        return -1;
    } 
    else if (pid == 0) {
        /* Child Process: Replace memory image with binary */
        execvp(job->args[0], job->args);

        /* Reached only on execvp failure */
        fprintf(stderr, "\n[WORKER CHILD PID %d] execvp failed for '%s': %s\n",
                getpid(), job->args[0], strerror(errno));
        
        /* POSIX standard exit code 127 for execution failure */
        exit(127);
    } 
    else {
        /* Parent Process */
        job->pid = pid;
        job->state = JOB_STATE_RUNNING;
        return pid;
    }
}

void process_evaluate_exit_status(Job *job, int status)
{
    if (!job) return;

    job->completed_at = time(NULL);
    job->duration = difftime(job->completed_at, job->started_at);

    if (WIFEXITED(status)) {
        job->exit_code = WEXITSTATUS(status);
        job->term_sig = 0;
        if (job->exit_code == 0) {
            job->state = JOB_STATE_COMPLETED;
        } else {
            job->state = JOB_STATE_FAILED;
        }
    } 
    else if (WIFSIGNALED(status)) {
        job->term_sig = WTERMSIG(status);
        job->exit_code = 128 + job->term_sig;
        job->state = JOB_STATE_FAILED;
    } 
    else {
        job->exit_code = -1;
        job->state = JOB_STATE_FAILED;
    }
}
