#!/usr/bin/env bash
set -euo pipefail

HOST="${1:-127.0.0.1}"
PORT="${2:-9000}"
LOG="${LOG:-nc_test_$(date +%Y%m%d_%H%M%S).log}"

# Tunables for destructive/test commands.
PART_SIZE=2048
PART_NUM=5
WRITE_DATA="${WRITE_DATA:-hello_test}"
WRITE_OFFSET="${WRITE_OFFSET:-0}"
READ_SECTORS="${READ_SECTORS:-1}"
SECTOR_START=0
SECTOR_END=1024
DURATION_MS=2000
DELAY=3

run_commands() {
  send() {
    local cmd="$1"
    printf '\n>>> %s\n' "$cmd" >&2
    printf '%s\n' "$cmd"
    sleep "$DELAY"
  }

  sleep "$DELAY"

  # Usage / informational commands
  send "help"
  send "status"
  send "listparts"

  # Partition + basic I/O test
  send "mkpart $PART_SIZE"
  send "listparts"
  send "write $PART_NUM $WRITE_DATA $WRITE_OFFSET"
  send "read $PART_NUM $WRITE_OFFSET $READ_SECTORS"

  # Performance command aliases
  send "sw $PART_NUM $SECTOR_START $SECTOR_END $DURATION_MS"
  send "sr $PART_NUM $SECTOR_START $SECTOR_END $DURATION_MS"
  send "rw $PART_NUM $SECTOR_START $SECTOR_END $DURATION_MS"
  send "rr $PART_NUM $SECTOR_START $SECTOR_END $DURATION_MS"

  send quit
  # Uncomment these if you want the script to remove the test partition.
  # send "rmpart $PART_NUM"
  # send "listparts"
}

echo "Connecting to ${HOST}:${PORT}" >&2

declare -a NC_ARGS=()
if nc -h 2>&1 | grep -q -- ' -q '; then
  NC_ARGS=(-q 1)
elif nc -h 2>&1 | grep -q -- ' -N '; then
  NC_ARGS=(-N)
fi

run_commands | nc "${NC_ARGS[@]}" "$HOST" "$PORT" | tee "$LOG"

echo >&2
echo "Done. Server output saved to: $LOG" >&2
