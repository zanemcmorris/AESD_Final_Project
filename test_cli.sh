#!/bin/sh
set -eu

HOST="${1:-127.0.0.1}"
PORT="${2:-9000}"
LOG="${LOG:-nc_test_$(date +%Y%m%d_%H%M%S).log}"

# Tunables for destructive/test commands.
PART_SIZE="${PART_SIZE:-2048}"
PART_NUM="${PART_NUM:-5}"
WRITE_DATA="${WRITE_DATA:-hello_test}"
WRITE_OFFSET="${WRITE_OFFSET:-0}"
READ_SECTORS="${READ_SECTORS:-1}"
SECTOR_START="${SECTOR_START:-0}"
SECTOR_END="${SECTOR_END:-1024}"
DURATION_MS="${DURATION_MS:-2000}"
DELAY="${DELAY:-3}"

send_cmd() {
    cmd="$1"
    printf '\n>>> %s\n' "$cmd" >&2
    printf '%s\n' "$cmd"
    sleep "$DELAY"
}

run_commands() {
    sleep "$DELAY"

    # Usage / informational commands
    send_cmd "help"
    send_cmd "status"
    send_cmd "listparts"

    # Partition + basic I/O test
    send_cmd "mkpart $PART_SIZE"
    send_cmd "listparts"
    send_cmd "write $PART_NUM $WRITE_DATA $WRITE_OFFSET"
    send_cmd "read $PART_NUM $WRITE_OFFSET $READ_SECTORS"

    # Performance command aliases
    send_cmd "sw $PART_NUM $SECTOR_START $SECTOR_END $DURATION_MS"
    send_cmd "sr $PART_NUM $SECTOR_START $SECTOR_END $DURATION_MS"
    send_cmd "rw $PART_NUM $SECTOR_START $SECTOR_END $DURATION_MS"
    send_cmd "rr $PART_NUM $SECTOR_START $SECTOR_END $DURATION_MS"

    send_cmd "quit"
    # Uncomment these if you want the script to remove the test partition.
    # send_cmd "rmpart $PART_NUM"
    # send_cmd "listparts"
}

echo "Connecting to ${HOST}:${PORT}" >&2

NC_ARGS=""
if nc -h 2>&1 | grep -q ' -q '; then
    NC_ARGS="-q 1"
elif nc -h 2>&1 | grep -q ' -N '; then
    NC_ARGS="-N"
fi

if [ -n "$NC_ARGS" ]; then
    run_commands | sh -c "nc $NC_ARGS \"$HOST\" \"$PORT\"" | tee "$LOG"
else
    run_commands | telnet "$HOST" "$PORT" | tee "$LOG"
fi

echo >&2
echo "Done. Server output saved to: $LOG" >&2