#!/usr/bin/env bash
set -euo pipefail

# Usage: ./run.sh [m n k q]
# default values if not provided:
M=${1:-6}
N=${2:-8}
K=${3:-5}
Q=${4:-4}
PORT=9000

echo "Parameters: M=$M N=$N K=$K Q=$Q"

# compile programs
echo "Compiling..."
g++ -std=c++17 -O2 gen_queries.cpp -o gen_queries || { echo "gen_queries compile failed"; exit 1; }
g++ -std=c++17 -O2 server.cpp -o server || { echo "server compile failed"; exit 1; }
g++ -std=c++17 -O2 client.cpp -o client || { echo "client compile failed"; exit 1; }
g++ -std=c++17 -O2 check_results.cpp -o check_results || { echo "check_results compile failed"; exit 1; }

# generate queries & shares
echo "Generating shares and queries..."
./gen_queries "$M" "$N" "$K" "$Q"

# backup initial shares so checker can reconstruct original U/V
cp -v party0_shares.txt party0_shares.orig
cp -v party1_shares.txt party1_shares.orig
cp -v queries.txt queries.orig

# start server in background
echo "Starting server (port $PORT) ... (logs -> server.log)"
./server "$PORT" > server.log 2>&1 &
SERVER_PID=$!
sleep 1

# start two clients (role 0 and 1) in background, logs go to client0.log and client1.log
echo "Starting client 0 ..."
./client 0 127.0.0.1 "$PORT" > client0.log 2>&1 &
CLIENT0_PID=$!

echo "Starting client 1 ..."
./client 1 127.0.0.1 "$PORT" > client1.log 2>&1 &
CLIENT1_PID=$!

# wait for clients to finish
echo "Waiting for clients to finish..."
wait "$CLIENT0_PID"
echo "Client 0 finished."
wait "$CLIENT1_PID"
echo "Client 1 finished."

# give server a little time to finish any last I/O
sleep 1

# run checker
echo "Running checker..."
./check_results || CHECK_STATUS=$? || true

# kill server if still running
if ps -p "$SERVER_PID" > /dev/null 2>&1; then
  echo "Killing server (pid $SERVER_PID)"
  kill "$SERVER_PID" || true
fi

# show small logs for convenience
echo ""
echo "=== last 30 lines of server.log ==="
tail -n 30 server.log || true
echo ""
echo "=== last 30 lines of client0.log ==="
tail -n 30 client0.log || true
echo ""
echo "=== last 30 lines of client1.log ==="
tail -n 30 client1.log || true

# interpret checker exit code
if [[ "${CHECK_STATUS:-0}" == "0" ]]; then
  echo ""
  echo "======================================"
  echo "RESULT: PASS — reconstructed final U matches expected plaintext result."
  echo "======================================"
  exit 0
else
  echo ""
  echo "======================================"
  echo "RESULT: FAIL — mismatch between reconstructed shared result and expected plaintext."
  echo "Check 'server.log', 'client0.log', 'client1.log' and the checker output above for details."
  echo "======================================"
  exit 2
fi
