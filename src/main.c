#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "common.h"
#include "scheduler.h"

static void trim_whitespace(char *str)
{
    if (!str) return;

    /* Trim leading whitespace */
    char *start = str;
    while (isspace((unsigned char)*start)) {
        start++;
    }

    if (start != str) {
        memmove(str, start, strlen(start) + 1);
    }

    /* Trim trailing whitespace */
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
                /* 'submit sleep 5' -> 5 is duration argument, priority defaults to 5 */
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
    printf("                               Example: submit sleep 2 1\n");
    printf("  jobs                         List all jobs, priorities, and active worker slots\n");
    printf("  status <job_id>              Display detailed status for a specific job\n");
    printf("  wait                         Wait for all running and queued jobs to complete\n");
    printf("  help                         Display this help menu\n");
    printf("  exit                         Exit the scheduler program\n\n");
}

int main(void)
{
    Scheduler sched;
    scheduler_init(&sched);

    char line[MAX_COMMAND_LEN * 2];

    printf("=========================================================\n");
    printf(" Multi-Process Job Scheduler (Phase 7: Priority Scheduling) \n");
    printf(" Priority: 1 = Highest, 10 = Lowest                     \n");
    printf(" Type 'help' for available commands or 'exit' to quit.   \n");
    printf("=========================================================\n\n");

    while (1) {
        /* Periodically harvest finished workers and dispatch waiting jobs */
        scheduler_dispatch(&sched);

        printf("scheduler> ");
        fflush(stdout);

        if (fgets(line, sizeof(line), stdin) == NULL) {
            printf("\nExiting scheduler.\n");
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
        } else if (strcmp(command, "wait") == 0) {
            scheduler_wait_all(&sched);
        } else if (strcmp(command, "help") == 0) {
            print_help();
        } else if (strcmp(command, "exit") == 0) {
            printf("Exiting scheduler.\n");
            break;
        } else {
            printf("Unknown command '%s'. Type 'help' for available commands.\n\n", command);
        }
    }

    return 0;
}
