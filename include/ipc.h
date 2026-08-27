#ifndef IPC_H
#define IPC_H

#include <stddef.h>
#include <sys/select.h>

/* Creates a unidirectional anonymous pipe */
int ipc_create_pipe(int pipefd[2]);

/* Sends a string message over specified pipe write file descriptor */
int ipc_send_message(int write_fd, const char *msg);

/* Non-blocking read of messages from pipe read file descriptor */
int ipc_read_message(int read_fd, char *buf, size_t max_len);

/* Monitor array of pipe read descriptors using select() */
int ipc_select_pipes(const int *read_fds, int count, int *ready_flags, int timeout_ms);

#endif /* IPC_H */
