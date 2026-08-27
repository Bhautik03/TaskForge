#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>
#include "common.h"
#include "scheduler.h"
#include "signals.h"

static void trim_whitespace(char *str)
{
    if (!str) return;

    char *start = str;
    while (isspace((unsigned char)*start)) {
        start++;
    }

    if (start != str) {
        memmove(str, start, strlen(start) + 1);
    }

    size_t len = strlen(str);
    while (len > 0 && isspace((unsigned char)str[len - 1])) {
        str[--len] = '\0';
    }
}

static int is_numeric_str(const char *str)
{
    if (!str || *str == '\0') return 0;
    while (*str) {
        if (!isdigit((unsigned char)*str)) return 0;
        str++;
    }
    return 1;
}

static void handle_submit(Scheduler *sched, char *args)
{
    trim_whitespace(args);
    if (strlen(args) == 0) {
        printf("Usage: submit <command> [priority (1=Highest, 10=Lowest)]\n");
        printf("Example: submit sleep 5 2\n\n");
        return;
    }

    char cmd_copy[MAX_COMMAND_LEN];
    strncpy(cmd_copy, args, MAX_COMMAND_LEN - 1);
    cmd_copy[MAX_COMMAND_LEN - 1] = '\0';

    int priority = DEFAULT_PRIORITY;

    int token_count = 0;
    char temp_tok[MAX_COMMAND_LEN];
    strncpy(temp_tok, args, MAX_COMMAND_LEN - 1);
    temp_tok[MAX_COMMAND_LEN - 1] = '\0';

    char *tok = strtok(temp_tok, " \t");
    while (tok != NULL) {
        token_count++;
        tok = strtok(NULL, " \t");
    }

    char *last_space = strrchr(cmd_copy, ' ');
    if (last_space != NULL && token_count >= 2) {
        char *last_token = last_space + 1;
        if (is_numeric_str(last_token)) {
            char first_token[64] = {0};
            sscanf(cmd_copy, "%63s", first_token);

            if (strcmp(first_token, "sleep") == 0 && token_count == 2) {
                priority = DEFAULT_PRIORITY;
            } else {
                priority = atoi(last_token);
                *last_space = '\0';
                trim_whitespace(cmd_copy);
            }
        }
    }

    scheduler_submit_job(sched, cmd_copy, priority);
}

static void print_help(void)
{
    printf("\nMulti-Process Job Scheduler CLI (MAX_WORKERS = %d)\n", MAX_WORKERS);
    printf("Available Commands:\n");
    printf("  submit <command> [priority]  Submit a job (priority 1=Highest to 10=Lowest, default 5)\n");
    printf("  jobs                         List all jobs, priorities, active workers & SHM state\n");
    printf("  status <job_id>              Display detailed status for a specific job\n");
    printf("  pause <job_id>               Pause a running job (sends SIGSTOP)\n");
    printf("  resume <job_id>              Resume a paused job (sends SIGCONT)\n");
    printf("  cancel <job_id>              Cancel a job (sends SIGTERM)\n");
    printf("  wait                         Wait for all running and queued jobs to complete\n");
    printf("  help                         Display this help menu\n");
    printf("  exit                         Gracefully shut down the scheduler\n\n");
}

int main(void)
{
    Scheduler sched;
    scheduler_init(&sched);

    char line[MAX_COMMAND_LEN * 2];

    printf("=========================================================\n");
    printf(" Multi-Process Job Scheduler (Phase 16: Robust Shutdown) \n");
    printf(" Priority: 1 = Highest, 10 = Lowest                     \n");
    printf(" Ctrl+C or 'exit' for graceful shutdown.                 \n");
    printf("=========================================================\n\n");

    while (1) {
        /*
         * Phase 16: Check shutdown flag before every iteration.
         * g_shutdown_requested is set by the SIGINT/SIGTERM handler.
         * Since SA_RESTART is NOT set, fgets() returns NULL with
         * errno == EINTR when interrupted, which also leads here.
         */
        if (g_shutdown_requested) {
            printf("\n[SHUTDOWN] Signal received (SIGINT/SIGTERM). Initiating graceful shutdown...\n");
            break;
        }

        /* Periodically harvest finished workers and dispatch waiting jobs */
        scheduler_dispatch(&sched);

        printf("scheduler> ");
        fflush(stdout);

        if (fgets(line, sizeof(line), stdin) == NULL) {
            /*
             * fgets returns NULL on:
             *   (a) EOF (Ctrl+D / end of pipe input)
             *   (b) EINTR (interrupted by SIGINT/SIGTERM)
             * In both cases, run the shutdown sequence.
             */
            if (g_shutdown_requested) {
                printf("\n[SHUTDOWN] Signal received. Initiating graceful shutdown...\n");
            } else {
                printf("\nEOF detected. Initiating graceful shutdown...\n");
            }
            break;
        }

        trim_whitespace(line);
        if (strlen(line) == 0) {
            continue;
        }

        char command[64] = {0};
        char *args = NULL;

        char *space_pos = strchr(line, ' ');
        if (space_pos != NULL) {
            size_t cmd_len = space_pos - line;
            if (cmd_len >= sizeof(command)) cmd_len = sizeof(command) - 1;
            strncpy(command, line, cmd_len);
            command[cmd_len] = '\0';
            args = space_pos + 1;
        } else {
            strncpy(command, line, sizeof(command) - 1);
            command[sizeof(command) - 1] = '\0';
            args = "";
        }

        if (strcmp(command, "submit") == 0) {
            handle_submit(&sched, args);
        } else if (strcmp(command, "jobs") == 0) {
            scheduler_list_jobs(&sched);
        } else if (strcmp(command, "status") == 0) {
            trim_whitespace(args);
            if (strlen(args) == 0 || !is_numeric_str(args)) {
                printf("Usage: status <job_id>\n\n");
            } else {
                int job_id = atoi(args);
                scheduler_job_status(&sched, job_id);
            }
        } else if (strcmp(command, "pause") == 0) {
            trim_whitespace(args);
            if (strlen(args) == 0 || !is_numeric_str(args)) {
                printf("Usage: pause <job_id>\n\n");
            } else {
                int job_id = atoi(args);
                scheduler_pause_job(&sched, job_id);
            }
        } else if (strcmp(command, "resume") == 0) {
            trim_whitespace(args);
            if (strlen(args) == 0 || !is_numeric_str(args)) {
                printf("Usage: resume <job_id>\n\n");
            } else {
                int job_id = atoi(args);
                scheduler_resume_job(&sched, job_id);
            }
        } else if (strcmp(command, "cancel") == 0) {
            trim_whitespace(args);
            if (strlen(args) == 0 || !is_numeric_str(args)) {
                printf("Usage: cancel <job_id>\n\n");
            } else {
                int job_id = atoi(args);
                scheduler_cancel_job(&sched, job_id);
            }
        } else if (strcmp(command, "wait") == 0) {
            scheduler_wait_all(&sched);
        } else if (strcmp(command, "help") == 0) {
            print_help();
        } else if (strcmp(command, "exit") == 0) {
            printf("[SHUTDOWN] 'exit' command received.\n");
            break;
        } else {
            printf("Unknown command '%s'. Type 'help' for available commands.\n\n", command);
        }
    }

    /* Phase 16: Always use scheduler_shutdown() for ordered cleanup */
    scheduler_shutdown(&sched);
    return 0;
}
