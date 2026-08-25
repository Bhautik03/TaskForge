#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <time.h>
#include "process.h"

int process_execute_job_dummy(Job *job)
{
    if (!job) return -1;

    pid_t pid = fork();

    if (pid < 0) {
        perror("fork failed");
        job->state = JOB_STATE_FAILED;
        return -1;
    } 
    else if (pid == 0) {
        /* Child Process */
        printf("\n[CHILD  PID %d] (Parent PPID %d) Executing test action for Job ID %d ('%s')...\n",
               getpid(), getppid(), job->job_id, job->full_command);
        
        sleep(1); /* Simulate work */
        
        printf("[CHILD  PID %d] Test action complete. Exiting child process with status 0.\n\n", getpid());
        exit(0); /* Terminate child process cleanly */
    } 
    else {
        /* Parent Process */
        job->pid = pid;
        job->state = JOB_STATE_RUNNING;
        job->started_at = time(NULL);

        printf("\n[PARENT PID %d] Forked child PID %d for Job ID %d. Waiting via waitpid()...\n",
               getpid(), pid, job->job_id);

        int status = 0;
        pid_t wpid = waitpid(pid, &status, 0);

        if (wpid > 0) {
            if (WIFEXITED(status)) {
                job->exit_code = WEXITSTATUS(status);
                job->state = JOB_STATE_COMPLETED;
            } else {
                job->exit_code = -1;
                job->state = JOB_STATE_FAILED;
            }
        } else {
            perror("waitpid failed");
            job->state = JOB_STATE_FAILED;
        }

        job->completed_at = time(NULL);

        printf("[PARENT PID %d] Child PID %d harvested. Job ID %d state updated to %s.\n\n",
               getpid(), pid, job->job_id, job_state_to_string(job->state));
        return 0;
    }
}
