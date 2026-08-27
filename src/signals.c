#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <errno.h>
#include <string.h>
#include "signals.h"

volatile sig_atomic_t g_sigchld_received  = 0;
volatile sig_atomic_t g_shutdown_requested = 0;

static void handle_sigchld(int sig)
{
    (void)sig;
    /* Async-Signal-Safe: only set a flag */
    g_sigchld_received = 1;
}

/*
 * Phase 16: SIGINT / SIGTERM handler.
 *
 * When the user presses Ctrl+C or the OS sends SIGTERM, we cannot
 * perform complex cleanup directly here (malloc, printf, and most
 * library functions are NOT async-signal-safe).
 *
 * Instead we set the atomic flag and return immediately.
 * The main scheduler loop detects the flag on its next iteration
 * and performs the full ordered shutdown from normal (safe) context.
 */
static void handle_shutdown_signal(int sig)
{
    (void)sig;
    g_shutdown_requested = 1;
}

void signals_init(void)
{
    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));

    /* Ignore SIGPIPE: broken pipe from a closed worker does not crash scheduler */
    sa.sa_handler = SIG_IGN;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    sigaction(SIGPIPE, &sa, NULL);

    /* SIGCHLD: async notification of child termination */
    struct sigaction sa_chld;
    memset(&sa_chld, 0, sizeof(sa_chld));
    sa_chld.sa_handler = handle_sigchld;
    sigemptyset(&sa_chld.sa_mask);
    sa_chld.sa_flags = SA_RESTART; /* Restart interrupted system calls */
    sigaction(SIGCHLD, &sa_chld, NULL);

    /*
     * Phase 16: SIGINT (Ctrl+C) and SIGTERM shutdown signals.
     * SA_RESTART is intentionally NOT set so that fgets() in main
     * returns EINTR, which causes the loop to re-check g_shutdown_requested.
     */
    struct sigaction sa_shutdown;
    memset(&sa_shutdown, 0, sizeof(sa_shutdown));
    sa_shutdown.sa_handler = handle_shutdown_signal;
    sigemptyset(&sa_shutdown.sa_mask);
    sa_shutdown.sa_flags = 0; /* No SA_RESTART: interrupt fgets() */
    sigaction(SIGINT,  &sa_shutdown, NULL);
    sigaction(SIGTERM, &sa_shutdown, NULL);
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
