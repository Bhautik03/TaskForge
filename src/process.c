#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <sys/wait.h>
#include <time.h>
#include "process.h"
#include "ipc.h"

pid_t process_spawn_job_async(Job *job, int out_pipefd[2])
{
    if (!job || job->arg_count == 0 || job->args[0] == NULL) {
        printf("Error: No command provided for execution.\n");
        return -1;
    }

    int pipefd[2];
    if (ipc_create_pipe(pipefd) < 0) {
        return -1;
    }

    job->started_at = time(NULL);

    pid_t pid = fork();

    if (pid < 0) {
        perror("fork failed");
        close(pipefd[0]);
        close(pipefd[1]);
        job->state = JOB_STATE_FAILED;
        job->completed_at = time(NULL);
        job->duration = difftime(job->completed_at, job->started_at);
        return -1;
    } 
    else if (pid == 0) {
        /* Child Process: Close unused read end of pipe */
        close(pipefd[0]);

        /* Send STARTED message over pipe to scheduler */
        char msg[128];
        snprintf(msg, sizeof(msg), "JOB %d STARTED\n", job->job_id);
        ipc_send_message(pipefd[1], msg);

        /* Replace child image with binary */
        execvp(job->args[0], job->args);

        /* Reached only if execvp fails */
        fprintf(stderr, "\n[WORKER CHILD PID %d] execvp failed for '%s': %s\n",
                getpid(), job->args[0], strerror(errno));

        snprintf(msg, sizeof(msg), "JOB %d FAILED\n", job->job_id);
        ipc_send_message(pipefd[1], msg);
        close(pipefd[1]);

        exit(127);
    } 
    else {
        /* Parent Process: Close unused write end of pipe */
        close(pipefd[1]);

        if (out_pipefd) {
            out_pipefd[0] = pipefd[0];
            out_pipefd[1] = -1;
        }

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
