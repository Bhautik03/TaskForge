#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <time.h>
#include "scheduler.h"
#include "process.h"

void scheduler_init(Scheduler *sched)
{
    if (!sched) return;
    sched->job_count = 0;
    sched->next_job_id = 1;
}

int scheduler_submit_job(Scheduler *sched, const char *raw_cmd, int priority)
{
    if (!sched || !raw_cmd || strlen(raw_cmd) == 0) {
        printf("Error: Invalid command for submission.\n");
        return -1;
    }

    if (sched->job_count >= MAX_JOBS) {
        printf("Error: Job table is full (max %d jobs).\n", MAX_JOBS);
        return -1;
    }

    Job *j = &sched->jobs[sched->job_count];
    j->job_id = sched->next_job_id++;
    strncpy(j->full_command, raw_cmd, MAX_COMMAND_LEN - 1);
    j->full_command[MAX_COMMAND_LEN - 1] = '\0';

    /* Tokenize command into args_storage and setup args pointers */
    char temp_cmd[MAX_COMMAND_LEN];
    strncpy(temp_cmd, raw_cmd, MAX_COMMAND_LEN - 1);
    temp_cmd[MAX_COMMAND_LEN - 1] = '\0';

    j->arg_count = 0;
    char *token = strtok(temp_cmd, " \t");
    while (token != NULL && j->arg_count < MAX_ARGS) {
        strncpy(j->args_storage[j->arg_count], token, MAX_COMMAND_LEN - 1);
        j->args_storage[j->arg_count][MAX_COMMAND_LEN - 1] = '\0';
        j->args[j->arg_count] = j->args_storage[j->arg_count];
        j->arg_count++;
        token = strtok(NULL, " \t");
    }
    j->args[j->arg_count] = NULL;

    j->priority = priority;
    j->state = JOB_STATE_WAITING;
    j->pid = -1;
    j->exit_code = 0;
    j->created_at = time(NULL);
    j->started_at = 0;
    j->completed_at = 0;

    sched->job_count++;

    printf("\nJob Submitted:\n");
    printf("  Job ID:   %d\n", j->job_id);
    printf("  Priority: %d\n", j->priority);

    /* Phase 3: Execute job test action using fork() and waitpid() */
    process_execute_job_dummy(j);

    return j->job_id;
}

void scheduler_list_jobs(const Scheduler *sched)
{
    if (!sched) return;

    printf("\nID    PID    PRIORITY    STATE\n");
    printf("--------------------------------\n");

    if (sched->job_count == 0) {
        printf("(No jobs submitted yet)\n\n");
        return;
    }

    for (int i = 0; i < sched->job_count; i++) {
        const Job *j = &sched->jobs[i];
        char pid_str[16];
        if (j->pid <= 0) {
            snprintf(pid_str, sizeof(pid_str), "-");
        } else {
            snprintf(pid_str, sizeof(pid_str), "%d", j->pid);
        }

        printf("%-5d %-6s %-11d %-10s\n",
               j->job_id,
               pid_str,
               j->priority,
               job_state_to_string(j->state));
    }
    printf("\n");
}

void scheduler_job_status(const Scheduler *sched, int job_id)
{
    if (!sched) return;

    const Job *found = NULL;
    for (int i = 0; i < sched->job_count; i++) {
        if (sched->jobs[i].job_id == job_id) {
            found = &sched->jobs[i];
            break;
        }
    }

    if (!found) {
        printf("\nError: Job ID %d not found.\n\n", job_id);
        return;
    }

    char created_str[64] = "N/A";
    if (found->created_at > 0) {
        struct tm *tm_info = localtime(&found->created_at);
        if (tm_info) {
            strftime(created_str, sizeof(created_str), "%Y-%m-%d %H:%M:%S", tm_info);
        }
    }

    char pid_str[16];
    if (found->pid <= 0) {
        snprintf(pid_str, sizeof(pid_str), "-");
    } else {
        snprintf(pid_str, sizeof(pid_str), "%d", found->pid);
    }

    printf("\nJob Details:\n");
    printf("  Job ID:      %d\n", found->job_id);
    printf("  Command:     %s\n", found->full_command);
    printf("  Priority:    %d\n", found->priority);
    printf("  State:       %s\n", job_state_to_string(found->state));
    printf("  PID:         %s\n", pid_str);
    printf("  Exit Code:   %d\n", found->exit_code);
    printf("  Created At:  %s\n\n", created_str);
}
