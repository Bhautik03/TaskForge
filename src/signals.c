#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <errno.h>
#include <string.h>
#include "signals.h"

volatile sig_atomic_t g_sigchld_received = 0;

static void handle_sigchld(int sig)
{
    (void)sig;
    /* Async-Signal-Safe Operation: Setting a volatile sig_atomic_t flag */
    g_sigchld_received = 1;
}

void signals_init(void)
{
    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));

    /* Ignore SIGPIPE using sigaction() so broken pipes don't crash scheduler */
    sa.sa_handler = SIG_IGN;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    sigaction(SIGPIPE, &sa, NULL);

    /* Setup SIGCHLD handler for asynchronous child termination notification */
    struct sigaction sa_chld;
    memset(&sa_chld, 0, sizeof(sa_chld));
    sa_chld.sa_handler = handle_sigchld;
    sigemptyset(&sa_chld.sa_mask);
    sa_chld.sa_flags = SA_RESTART; /* Restart interrupted system calls */
    sigaction(SIGCHLD, &sa_chld, NULL);
}

int signals_send(pid_t pid, int sig)
{
    if (pid <= 0) {
        errno = EINVAL;
        return -1;
    }
    return kill(pid, sig);
}

int signals_has_pending_chld(void)
{
    return g_sigchld_received != 0;
}

void signals_clear_pending_chld(void)
{
    g_sigchld_received = 0;
}
