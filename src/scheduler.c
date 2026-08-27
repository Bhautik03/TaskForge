#define _GNU_SOURCE
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <time.h>
#include <unistd.h>
#include <sys/wait.h>
#include <signal.h>
#include "scheduler.h"
#include "process.h"
#include "signals.h"

void scheduler_init(Scheduler *sched)
{
    if (!sched) return;
    sched->job_count = 0;
    sched->next_job_id = 1;
    sched->active_workers = 0;

    for (int i = 0; i < MAX_WORKERS; i++) {
        worker_init(&sched->workers[i], i);
    }

    signals_init();
}

void scheduler_reap_workers(Scheduler *sched)
{
    if (!sched) return;

    /* Clear pending SIGCHLD flag before harvesting */
    signals_clear_pending_chld();

    int status = 0;
    pid_t wpid;

    /*
     * Non-blocking waitpid loop:
     * Crucial for signal coalescing! Multiple child processes terminating simultaneously
     * may trigger only ONE SIGCHLD signal. Looping waitpid(-1, &status, WNOHANG) until
     * it returns <= 0 guarantees ALL terminated child processes are reaped.
     */
    while ((wpid = waitpid(-1, &status, WNOHANG)) > 0) {
        Job *j = NULL;
        for (int i = 0; i < sched->job_count; i++) {
            if (sched->jobs[i].pid == wpid) {
                j = &sched->jobs[i];
                break;
            }
        }

        Worker *w = NULL;
        for (int i = 0; i < MAX_WORKERS; i++) {
            if (sched->workers[i].pid == wpid) {
                w = &sched->workers[i];
                break;
            }
        }

        if (j && j->state != JOB_STATE_CANCELLED) {
            process_evaluate_exit_status(j, status);
        }

        int worker_slot = -1;
        if (w) {
            worker_slot = w->worker_id + 1;
            w->active = 0;
            w->current_job_id = -1;
            w->pid = -1;
            if (sched->active_workers > 0) {
                sched->active_workers--;
            }
        }

        if (j && j->state != JOB_STATE_CANCELLED) {
            printf("\n[SIGCHLD REAPER] Worker Slot %d (PID %d) finished Job ID %d ('%s') -> State: %s (Exit Code: %d, Duration: %.2fs)\n",
                   worker_slot > 0 ? worker_slot : 0,
                   wpid,
                   j->job_id,
                   j->full_command,
                   job_state_to_string(j->state),
                   j->exit_code,
                   j->duration);
        }
    }
}

void scheduler_dispatch(Scheduler *sched)
{
    if (!sched) return;

    /* First, reap any finished worker processes notified via SIGCHLD or timer */
    scheduler_reap_workers(sched);

    /* Fill available worker slots with highest-priority WAITING jobs */
    while (sched->active_workers < MAX_WORKERS) {
        Job *best_job = NULL;
        for (int i = 0; i < sched->job_count; i++) {
            if (sched->jobs[i].state == JOB_STATE_WAITING) {
                if (best_job == NULL) {
                    best_job = &sched->jobs[i];
                } else if (sched->jobs[i].priority < best_job->priority) {
                    best_job = &sched->jobs[i];
                } else if (sched->jobs[i].priority == best_job->priority && sched->jobs[i].job_id < best_job->job_id) {
                    best_job = &sched->jobs[i];
                }
            }
        }

        if (!best_job) {
            break; /* No waiting jobs in queue */
        }

        /* Find an idle worker slot */
        Worker *free_worker = NULL;
        for (int i = 0; i < MAX_WORKERS; i++) {
            if (!sched->workers[i].active) {
                free_worker = &sched->workers[i];
                break;
            }
        }

        if (!free_worker) {
            break; /* No worker slots available */
        }

        /* Launch process asynchronously */
        pid_t pid = process_spawn_job_async(best_job);

        if (pid > 0) {
            free_worker->active = 1;
            free_worker->current_job_id = best_job->job_id;
            free_worker->pid = pid;
            sched->active_workers++;

            printf("[SCHEDULER] Dispatched Job ID %d ('%s', Priority %d) -> Worker Slot %d (PID %d)\n",
                   best_job->job_id,
                   best_job->full_command,
                   best_job->priority,
                   free_worker->worker_id + 1,
                   pid);
        }
    }
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

    /* Clamp priority to valid range 1..10 if out of range */
    if (priority < HIGHEST_PRIORITY || priority > LOWEST_PRIORITY) {
        priority = DEFAULT_PRIORITY;
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
    j->term_sig = 0;
    j->created_at = time(NULL);
    j->started_at = 0;
    j->completed_at = 0;
    j->duration = 0.0;

    sched->job_count++;

    printf("\nJob Submitted:\n");
    printf("  Job ID:   %d\n", j->job_id);
    printf("  Priority: %d (1 = Highest, 10 = Lowest)\n", j->priority);
    printf("  Status:   WAITING\n");

    /* Try dispatching jobs immediately */
    scheduler_dispatch(sched);

    return j->job_id;
}

int scheduler_cancel_job(Scheduler *sched, int job_id)
{
    if (!sched) return -1;

    Job *target = NULL;
    for (int i = 0; i < sched->job_count; i++) {
        if (sched->jobs[i].job_id == job_id) {
            target = &sched->jobs[i];
            break;
        }
    }

    if (!target) {
        printf("\nError: Job ID %d not found.\n\n", job_id);
        return -1;
    }

    if (target->state == JOB_STATE_COMPLETED || target->state == JOB_STATE_FAILED || target->state == JOB_STATE_CANCELLED) {
        printf("\nError: Cannot cancel Job ID %d (Current state: %s).\n\n",
               job_id, job_state_to_string(target->state));
        return -1;
    }

    if (target->state == JOB_STATE_WAITING) {
        target->state = JOB_STATE_CANCELLED;
        target->completed_at = time(NULL);
        printf("\n[SCHEDULER] Job ID %d ('%s') cancelled from waiting queue.\n\n",
               job_id, target->full_command);
        return 0;
    }

    /* Target is RUNNING or STOPPED */
    if (target->pid > 0) {
        if (target->state == JOB_STATE_STOPPED) {
            /* Wake up process with SIGCONT so it handles SIGTERM */
            signals_send(target->pid, SIGCONT);
        }

        signals_send(target->pid, SIGTERM);

        int status = 0;
        waitpid(target->pid, &status, 0);

        target->state = JOB_STATE_CANCELLED;
        target->term_sig = SIGTERM;
        target->exit_code = 128 + SIGTERM;
        target->completed_at = time(NULL);
        if (target->started_at > 0) {
            target->duration = difftime(target->completed_at, target->started_at);
        }

        /* Free worker slot */
        for (int k = 0; k < MAX_WORKERS; k++) {
            if (sched->workers[k].current_job_id == job_id) {
                sched->workers[k].active = 0;
                sched->workers[k].current_job_id = -1;
                sched->workers[k].pid = -1;
                if (sched->active_workers > 0) sched->active_workers--;
                break;
            }
        }

        printf("\n[SCHEDULER] Job ID %d (PID %d) cancelled via SIGTERM.\n\n", job_id, target->pid);

        /* Try dispatching next waiting job into freed worker slot */
        scheduler_dispatch(sched);
        return 0;
    }

    return -1;
}

int scheduler_pause_job(Scheduler *sched, int job_id)
{
    if (!sched) return -1;

    Job *target = NULL;
    for (int i = 0; i < sched->job_count; i++) {
        if (sched->jobs[i].job_id == job_id) {
            target = &sched->jobs[i];
            break;
        }
    }

    if (!target) {
        printf("\nError: Job ID %d not found.\n\n", job_id);
        return -1;
    }

    if (target->state != JOB_STATE_RUNNING) {
        printf("\nError: Cannot pause Job ID %d (Current state: %s). Job must be RUNNING.\n\n",
               job_id, job_state_to_string(target->state));
        return -1;
    }

    if (target->pid > 0) {
        if (signals_send(target->pid, SIGSTOP) == 0) {
            target->state = JOB_STATE_STOPPED;
            printf("\n[SCHEDULER] Job ID %d (PID %d) paused (SIGSTOP sent).\n\n", job_id, target->pid);
            return 0;
        } else {
            perror("Failed to send SIGSTOP");
            return -1;
        }
    }

    return -1;
}

int scheduler_resume_job(Scheduler *sched, int job_id)
{
    if (!sched) return -1;

    Job *target = NULL;
    for (int i = 0; i < sched->job_count; i++) {
        if (sched->jobs[i].job_id == job_id) {
            target = &sched->jobs[i];
            break;
        }
    }

    if (!target) {
        printf("\nError: Job ID %d not found.\n\n", job_id);
        return -1;
    }

    if (target->state != JOB_STATE_STOPPED) {
        printf("\nError: Cannot resume Job ID %d (Current state: %s). Job must be STOPPED.\n\n",
               job_id, job_state_to_string(target->state));
        return -1;
    }

    if (target->pid > 0) {
        if (signals_send(target->pid, SIGCONT) == 0) {
            target->state = JOB_STATE_RUNNING;
            printf("\n[SCHEDULER] Job ID %d (PID %d) resumed (SIGCONT sent).\n\n", job_id, target->pid);
            return 0;
        } else {
            perror("Failed to send SIGCONT");
            return -1;
        }
    }

    return -1;
}

void scheduler_wait_all(Scheduler *sched)
{
    if (!sched) return;

    printf("\n[SCHEDULER] Waiting for all running and queued jobs to complete...\n");

    while (1) {
        scheduler_dispatch(sched);

        int pending = 0;
        for (int i = 0; i < sched->job_count; i++) {
            if (sched->jobs[i].state == JOB_STATE_WAITING || sched->jobs[i].state == JOB_STATE_RUNNING || sched->jobs[i].state == JOB_STATE_STOPPED) {
                pending++;
            }
        }

        if (pending == 0 && sched->active_workers == 0) {
            break;
        }

        usleep(50000); /* Sleep 50ms to prevent CPU spinning */
    }

    printf("[SCHEDULER] All jobs have completed execution.\n\n");
}

void scheduler_list_jobs(const Scheduler *sched)
{
    if (!sched) return;

    /* Harvest any finished jobs first */
    scheduler_reap_workers((Scheduler *)sched);

    printf("\nID    PID    PRIORITY    STATE       WORKER    DURATION\n");
    printf("----------------------------------------------------------\n");

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

        char worker_str[16] = "-";
        for (int k = 0; k < MAX_WORKERS; k++) {
            if (sched->workers[k].current_job_id == j->job_id) {
                snprintf(worker_str, sizeof(worker_str), "W%d", sched->workers[k].worker_id + 1);
                break;
            }
        }

        char dur_str[32];
        if (j->state == JOB_STATE_COMPLETED || j->state == JOB_STATE_FAILED || j->state == JOB_STATE_CANCELLED) {
            snprintf(dur_str, sizeof(dur_str), "%.2fs", j->duration);
        } else {
            snprintf(dur_str, sizeof(dur_str), "-");
        }

        printf("%-5d %-6s %-11d %-11s %-9s %-10s\n",
               j->job_id,
               pid_str,
               j->priority,
               job_state_to_string(j->state),
               worker_str,
               dur_str);
    }
    printf("\n");
}

static void format_timestamp(time_t t, char *buf, size_t size)
{
    if (t <= 0) {
        snprintf(buf, size, "N/A");
        return;
    }
    struct tm *tm_info = localtime(&t);
    if (tm_info) {
        strftime(buf, size, "%Y-%m-%d %H:%M:%S", tm_info);
    } else {
        snprintf(buf, size, "N/A");
    }
}

void scheduler_job_status(const Scheduler *sched, int job_id)
{
    if (!sched) return;

    /* Harvest any finished jobs first */
    scheduler_reap_workers((Scheduler *)sched);

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

    char created_str[64], started_str[64], completed_str[64];
    format_timestamp(found->created_at, created_str, sizeof(created_str));
    format_timestamp(found->started_at, started_str, sizeof(started_str));
    format_timestamp(found->completed_at, completed_str, sizeof(completed_str));

    char pid_str[16];
    if (found->pid <= 0) {
        snprintf(pid_str, sizeof(pid_str), "-");
    } else {
        snprintf(pid_str, sizeof(pid_str), "%d", found->pid);
    }

    char worker_str[32] = "-";
    for (int k = 0; k < MAX_WORKERS; k++) {
        if (sched->workers[k].current_job_id == found->job_id) {
            snprintf(worker_str, sizeof(worker_str), "Worker Slot %d", sched->workers[k].worker_id + 1);
            break;
        }
    }

    printf("\nJob Details:\n");
    printf("  Job ID:      %d\n", found->job_id);
    printf("  Command:     %s\n", found->full_command);
    printf("  Priority:    %d (1 = Highest, 10 = Lowest)\n", found->priority);
    printf("  State:       %s\n", job_state_to_string(found->state));
    printf("  Worker:      %s\n", worker_str);
    printf("  PID:         %s\n", pid_str);
    printf("  Exit Code:   %d\n", found->exit_code);

    if (found->term_sig > 0) {
        printf("  Term Signal: %d (%s)\n", found->term_sig, strsignal(found->term_sig));
    }

    printf("  Created At:  %s\n", created_str);
    printf("  Started At:  %s\n", started_str);
    printf("  Finished At: %s\n", completed_str);
    printf("  Duration:    %.2f seconds\n\n", found->duration);
}
