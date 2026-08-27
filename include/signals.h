#ifndef SIGNALS_H
#define SIGNALS_H

#include <signal.h>
#include <sys/types.h>

/* Global async-signal-safe flag set when SIGCHLD is caught */
extern volatile sig_atomic_t g_sigchld_received;

/* Setup sigaction configuration for process signals including SIGCHLD */
void signals_init(void);

/* Sends target signal to specified PID using kill() system call */
int signals_send(pid_t pid, int sig);

/* Checks if SIGCHLD flag is set */
int signals_has_pending_chld(void);

/* Resets SIGCHLD flag */
void signals_clear_pending_chld(void);

#endif /* SIGNALS_H */
