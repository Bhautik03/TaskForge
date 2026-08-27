#ifndef SIGNALS_H
#define SIGNALS_H

#include <signal.h>
#include <sys/types.h>

/* Async-signal-safe flag set when SIGCHLD is caught */
extern volatile sig_atomic_t g_sigchld_received;

/*
 * Phase 16: Robust Shutdown Flag
 *
 * Set to 1 when SIGINT (Ctrl+C) or SIGTERM is received.
 * The main REPL loop checks this flag each iteration and
 * initiates a graceful shutdown sequence when it is set.
 * This flag is declared volatile sig_atomic_t to be safely
 * readable from both signal context and the main loop.
 */
extern volatile sig_atomic_t g_shutdown_requested;

/* Setup sigaction for SIGCHLD, SIGINT, SIGTERM, and SIGPIPE */
void signals_init(void);

/* Sends target signal to specified PID using kill() */
int signals_send(pid_t pid, int sig);

/* Checks if SIGCHLD flag is set */
int signals_has_pending_chld(void);

/* Resets SIGCHLD flag */
void signals_clear_pending_chld(void);

#endif /* SIGNALS_H */
