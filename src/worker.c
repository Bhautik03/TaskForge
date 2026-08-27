#include "worker.h"

void worker_init(Worker *w, int id)
{
    if (!w) return;
    w->worker_id = id;
    w->active = 0;
    w->current_job_id = -1;
    w->pid = -1;
    w->pipe_read_fd = -1;
}
