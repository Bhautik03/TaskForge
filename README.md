# Multi-Process Job Scheduler

A C17 Linux multi-process job scheduler and process manager built incrementally to explore low-level Linux systems programming concepts.

## Project Status
**Project skeleton only — scheduler functionality has not yet been implemented.**

## Build Instructions

Compilation requires `gcc` supporting standard C17.

```bash
# Build the project executable
make

# Build with debug flags enabled
make debug

# Clean build artifacts
make clean
```

## Run Instructions

```bash
./scheduler
```

## Current Limitations
* CLI command submission interface is not yet implemented.
* Process spawning (`fork`), execution (`exec`), and process waiting (`waitpid`) are not yet enabled.
* IPC (Pipes, Shared Memory, Semaphores) and Signal Handling logic are not active.
* Process Monitoring functionality (`/proc`) is deferred to a future phase.

## Planned Future Phases
1. **CLI & Job Table**: Basic command-line parser and in-memory job table.
2. **Process Spawning**: `fork()` integration for worker creation.
3. **Program Execution**: `exec()` integration to run user binaries.
4. **Lifecycle & Harvesting**: `waitpid()` and child termination tracking.
5. **Worker Pool & Priority Scheduling**: Multi-worker queue dispatcher.
6. **Signal Control & IPC**: `SIGCHLD`, `SIGTERM`, `SIGSTOP`, `SIGCONT`, Pipes, `select()`, Shared Memory, Semaphores.
7. **Logging & Failure Recovery**: Low-level audit logs and worker failure recovery.
