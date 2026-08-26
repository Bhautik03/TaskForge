# TaskForge — Multi-Process Job Scheduler

A C17 Linux multi-process job scheduler built incrementally to master POSIX process management, job queues, worker process pools, system calls, and concurrency.

## Project Status
**Active Development — Phase 6 Complete (Multiple Concurrent Worker Processes)**

### Completed Features:
- **CLI REPL Interface**: Command parser supporting `submit`, `jobs`, `status`, `wait`, `help`, and `exit`.
- **In-Memory Job Table**: State tracking (`WAITING`, `RUNNING`, `COMPLETED`, `FAILED`, `CANCELLED`).
- **Process Creation & Execution Engine**: `fork()` process duplication combined with `execvp()` binary execution.
- **Advanced Lifecycle Management**: Precise exit status inspection using POSIX status macros (`WIFEXITED`, `WEXITSTATUS`, `WIFSIGNALED`, `WTERMSIG`), exact start/completed timestamps, and `difftime()` execution duration calculation.
- **Guaranteed Zombie Prevention**: Automatic process harvesting using synchronous and non-blocking asynchronous `waitpid(-1, &status, WNOHANG)`.
- **Concurrent Worker Process Pools**: Support for `MAX_WORKERS = 3` parallel execution slots (`W1`, `W2`, `W3`) with automated job dispatching from the waiting queue.

---

## Build Instructions

Compilation requires `gcc` supporting standard C17 on Linux.

```bash
# Build the project executable
make

# Force clean build from scratch
make clean && make

# Build with debug flags enabled
make debug

# Clean build artifacts
make clean
```

---

## Run Instructions

```bash
# Run the scheduler CLI
./scheduler

# Example CLI commands inside scheduler:
scheduler> submit sleep 3
scheduler> submit sleep 2
scheduler> submit echo "Hello World"
scheduler> jobs
scheduler> status 1
scheduler> wait
scheduler> exit
```

---

## Directory Structure

```text
job_scheduler/
├── Makefile
├── README.md
├── include/
│   ├── common.h
│   ├── job.h
│   ├── process.h
│   ├── scheduler.h
│   └── worker.h
├── src/
│   ├── main.c
│   ├── job.c
│   ├── process.c
│   ├── scheduler.c
│   └── worker.c
└── docs/
    └── revision/
        ├── Phase_02_Project_Skeleton.pdf
        ├── Phase_03_CLI_Job_Table.pdf
        ├── Phase_04_fork.pdf
        ├── Phase_05_exec.pdf
        ├── Phase_06_Job_Lifecycle.pdf
        └── Phase_07_Multiple_Workers.pdf
```
