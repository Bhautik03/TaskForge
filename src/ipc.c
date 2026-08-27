#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include "ipc.h"

int ipc_create_pipe(int pipefd[2])
{
    if (pipe(pipefd) < 0) {
        perror("pipe creation failed");
        return -1;
    }

    /* Set non-blocking flag on pipe read descriptor (pipefd[0]) */
    int flags = fcntl(pipefd[0], F_GETFL, 0);
    if (flags >= 0) {
        fcntl(pipefd[0], F_SETFL, flags | O_NONBLOCK);
    }

    return 0;
}

int ipc_send_message(int write_fd, const char *msg)
{
    if (write_fd < 0 || !msg) return -1;
    size_t len = strlen(msg);
    ssize_t bytes_written = write(write_fd, msg, len);
    if (bytes_written < 0) {
        return -1;
    }
    return (int)bytes_written;
}

int ipc_read_message(int read_fd, char *buf, size_t max_len)
{
    if (read_fd < 0 || !buf || max_len == 0) return -1;

    ssize_t bytes_read = read(read_fd, buf, max_len - 1);
    if (bytes_read > 0) {
        buf[bytes_read] = '\0';
        return (int)bytes_read;
    } else if (bytes_read == 0) {
        /* EOF reached: child process closed write end */
        buf[0] = '\0';
        return 0;
    } else {
        if (errno == EAGAIN || errno == EWOULDBLOCK) {
            /* Non-blocking read: no data currently available */
            buf[0] = '\0';
            return -1;
        }
        buf[0] = '\0';
        return -1;
    }
}
