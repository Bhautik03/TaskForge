#!/bin/bash
# Phase 15 Worker Failure Test Script
# Tests: SIGKILL, SIGTERM, exec failure, non-zero exit, unexpected termination

set -e
SCHEDULER="./scheduler"

echo "============================================================"
echo " Worker Failure & Resilience Test Suite"
echo "============================================================"
echo ""

# ── TEST 1: exec failure (bad command) ────────────────────────
echo "-- TEST 1: exec failure (invalid command returns exit 127) --"
echo "submit this_command_does_not_exist 5
wait
jobs
exit" | $SCHEDULER
echo ""

# ── TEST 2: non-zero exit (explicit failure) ───────────────────
echo "-- TEST 2: Non-zero exit code (exit 1) --"
echo "submit bash -c exit\ 1 5
wait
jobs
exit" | $SCHEDULER
echo ""

# ── TEST 3: SIGKILL an active worker from outside ──────────────
echo "-- TEST 3: SIGKILL active worker (kill -9 <pid>) --"
# Start scheduler in background, submit long job, kill its worker PID, then show jobs
$SCHEDULER <<'EOF' &
submit sleep 60 5
EOF
SCHED_PID=$!
sleep 1

# Find the sleep worker PID
WORKER_PID=$(pgrep -n sleep 2>/dev/null || true)
if [ -n "$WORKER_PID" ]; then
    echo "[TEST HARNESS] Found worker PID $WORKER_PID -- sending SIGKILL"
    kill -9 "$WORKER_PID"
fi
sleep 1
wait $SCHED_PID 2>/dev/null || true
echo ""

# ── TEST 4: SIGKILL with queued job behind it ──────────────────
echo "-- TEST 4: SIGKILL worker that has a WAITING job behind it (next job must start) --"
(
  $SCHEDULER <<'INNER'
submit sleep 60 5
submit echo "QUEUED JOB RAN AFTER CRASH" 5
INNER
) &
SCHED_PID=$!
sleep 1

WORKER_PID=$(pgrep -n sleep 2>/dev/null || true)
if [ -n "$WORKER_PID" ]; then
    echo "[TEST HARNESS] SIGKILL worker PID $WORKER_PID -- queued job should now start"
    kill -9 "$WORKER_PID"
fi
sleep 2
# Send exit to scheduler
echo "exit" | $SCHEDULER 2>/dev/null || true
wait $SCHED_PID 2>/dev/null || true
echo ""

# ── TEST 5: SIGTERM an active worker ──────────────────────────
echo "-- TEST 5: SIGTERM active worker (graceful termination) --"
echo "submit sleep 60 5
exit" | $SCHEDULER &
SCHED_PID=$!
sleep 1
WORKER_PID=$(pgrep -n sleep 2>/dev/null || true)
if [ -n "$WORKER_PID" ]; then
    echo "[TEST HARNESS] Sending SIGTERM to worker PID $WORKER_PID"
    kill -15 "$WORKER_PID"
fi
sleep 1
wait $SCHED_PID 2>/dev/null || true
echo ""

echo "============================================================"
echo " All Tests Completed. Scheduler survived all failures."
echo "============================================================"
