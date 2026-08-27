#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <errno.h>
#include <string.h>
#include "signals.h"

void signals_init(void)
{
    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = SIG_IGN;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;

    /* Ignore SIGPIPE using sigaction() so broken pipes don't crash scheduler */
    sigaction(SIGPIPE, &sa, NULL);
}

int signals_send(pid_t pid, int sig)
{
    if (pid <= 0) {
        errno = EINVAL;
        return -1;
    }
    return kill(pid, sig);
}
