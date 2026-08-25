#include "job.h"

const char* job_state_to_string(JobState state)
{
    switch (state) {
        case JOB_STATE_WAITING:   return "WAITING";
        case JOB_STATE_RUNNING:   return "RUNNING";
        case JOB_STATE_COMPLETED: return "COMPLETED";
        case JOB_STATE_FAILED:    return "FAILED";
        case JOB_STATE_CANCELLED: return "CANCELLED";
        case JOB_STATE_STOPPED:   return "STOPPED";
        default:                  return "UNKNOWN";
    }
}
