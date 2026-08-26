#ifndef JOB_H
#define JOB_H

#include <sys/types.h>
#include <time.h>
#include "common.h"

typedef enum {
    JOB_STATE_WAITING,
    JOB_STATE_RUNNING,
    JOB_STATE_COMPLETED,
    JOB_STATE_FAILED,
    JOB_STATE_CANCELLED,
    JOB_STATE_STOPPED
} JobState;

typedef struct {
    int job_id;                                    /* Unique auto-incrementing integer key */
    char full_command[MAX_COMMAND_LEN];             /* Raw command string */
    char args_storage[MAX_ARGS][MAX_COMMAND_LEN];  /* Static argument buffers */
    char *args[MAX_ARGS + 1];                      /* NULL-terminated pointer vector for execvp */
    int arg_count;                                 /* Number of arguments */
    int priority;                                  /* Priority integer level */
    JobState state;                                /* Lifecycle state enum */
    pid_t pid;                                     /* Operating system Process ID */
    int exit_code;                                 /* Exit status code */
    int term_sig;                                  /* Termination signal number if signaled */
    time_t created_at;                             /* Submission timestamp */
    time_t started_at;                             /* Start timestamp */
    time_t completed_at;                           /* Finish timestamp */
    double duration;                               /* Execution duration in seconds */
} Job;

/* Helper to convert JobState enum to readable string */
const char *job_state_to_string(JobState state);

#endif /* JOB_H */
