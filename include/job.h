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
    int job_id;
    char full_command[MAX_COMMAND_LEN];
    char args_storage[MAX_ARGS][MAX_COMMAND_LEN];
    char *args[MAX_ARGS + 1];
    int arg_count;
    int priority;
    JobState state;
    pid_t pid;
    int exit_code;
    time_t created_at;
    time_t started_at;
    time_t completed_at;
} Job;

/* Utility function to convert JobState enum to string representation */
const char* job_state_to_string(JobState state);

#endif /* JOB_H */
