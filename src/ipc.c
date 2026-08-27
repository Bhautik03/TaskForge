#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/select.h>
#include <sys/ipc.h>
#include <sys/shm.h>
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

int ipc_select_pipes(const int *read_fds, int count, int *ready_flags, int timeout_ms)
{
    if (!read_fds || !ready_flags || count <= 0) return -1;

    fd_set readfds;
    FD_ZERO(&readfds);
    int max_fd = -1;

    for (int i = 0; i < count; i++) {
        ready_flags[i] = 0;
        if (read_fds[i] >= 0) {
            FD_SET(read_fds[i], &readfds);
            if (read_fds[i] > max_fd) {
                max_fd = read_fds[i];
            }
        }
    }

    if (max_fd < 0) return 0;

    struct timeval tv;
    tv.tv_sec = timeout_ms / 1000;
    tv.tv_usec = (timeout_ms % 1000) * 1000;

    int ret = select(max_fd + 1, &readfds, NULL, NULL, &tv);

    if (ret < 0) {
        if (errno == EINTR) {
            /* Interrupted by signal (e.g. SIGCHLD) */
            return 0;
        }
        perror("select failed");
        return -1;
    }

    if (ret > 0) {
        for (int i = 0; i < count; i++) {
            if (read_fds[i] >= 0 && FD_ISSET(read_fds[i], &readfds)) {
                ready_flags[i] = 1;
            }
        }
    }

    return ret;
}

key_t ipc_get_key(const char *path, int proj_id)
{
    key_t key = ftok(path ? path : ".", proj_id);
    if (key == -1) {
        perror("ftok failed");
    }
    return key;
}

int ipc_shm_create(key_t key, size_t size, int *out_shmid)
{
    if (key == -1 || size == 0 || !out_shmid) return -1;

    /* Allocate shared memory segment with read/write permissions for owner */
    int shmid = shmget(key, size, IPC_CREAT | IPC_EXCL | 0666);
    if (shmid < 0) {
        if (errno == EEXIST) {
            /* Segment exists, connect to existing segment */
            shmid = shmget(key, size, 0666);
        }
    }

    if (shmid < 0) {
        perror("shmget failed");
        return -1;
    }

    *out_shmid = shmid;
    return 0;
}

void *ipc_shm_attach(int shmid)
{
    if (shmid < 0) return NULL;
    void *shmaddr = shmat(shmid, NULL, 0);
    if (shmaddr == (void *)-1) {
        perror("shmat failed");
        return NULL;
    }
    return shmaddr;
}

int ipc_shm_detach(const void *shmaddr)
{
    if (!shmaddr) return -1;
    if (shmdt(shmaddr) < 0) {
        perror("shmdt failed");
        return -1;
    }
    return 0;
}

int ipc_shm_remove(int shmid)
{
    if (shmid < 0) return -1;
    if (shmctl(shmid, IPC_RMID, NULL) < 0) {
        perror("shmctl IPC_RMID failed");
        return -1;
    }
    return 0;
}
