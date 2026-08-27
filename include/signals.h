#ifndef SIGNALS_H
#define SIGNALS_H

#include <signal.h>

/* Setup sigaction configuration for process signals */
void signals_init(void);

/* Sends target signal to specified PID using kill() system call */
int signals_send(pid_t pid, int sig);

#endif /* SIGNALS_H */
