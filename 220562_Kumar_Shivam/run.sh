
set -euo pipefail

M=${1:-6}
N=${2:-8}
K=${3:-5}
Q=${4:-4}
PORT=9000
HOST=127.0.0.1

echo "Parameters: M=$M N=$N K=$K Q=$Q"

# ensure rebuild
rm -f gen_queries server client check_results || true

echo "Compiling..."
g++ -std=c++17 -O2 gen_queries.cpp -o gen_queries
g++ -std=c++17 -O2 server.cpp -o server
g++ -std=c++17 -O2 client.cpp -o client
g++ -std=c++17 -O2 check_results.cpp -o check_results

echo "Generating shares and queries..."
./gen_queries "$M" "$N" "$K" "$Q"

cp -v party0_shares.txt party0_shares.orig
cp -v party1_shares.txt party1_shares.orig
cp -v queries_i.txt queries_i.orig

echo "Starting server (logs -> server.log)"
./server "$PORT" > server.log 2>&1 &
SERVER_PID=$!
sleep 1

echo "Starting client 0 (logs -> client0.log)"
./client 0 "$HOST" "$PORT" > client0.log 2>&1 &
CLIENT0_PID=$!

echo "Starting client 1 (logs -> client1.log)"
./client 1 "$HOST" "$PORT" > client1.log 2>&1 &
CLIENT1_PID=$!

echo "Waiting for clients..."
wait "$CLIENT0_PID"
echo "Client 0 finished."
wait "$CLIENT1_PID"
echo "Client 1 finished."

sleep 1

echo "Running checker..."
./check_results || CHECK_STATUS=$? || true

if ps -p "$SERVER_PID" > /dev/null 2>&1; then
  kill "$SERVER_PID" || true
fi

echo ""
echo "=== server.log (tail) ==="
tail -n 200 server.log || true
echo ""
echo "=== client0.log (tail) ==="
tail -n 200 client0.log || true
echo ""
echo "=== client1.log (tail) ==="
tail -n 200 client1.log || true
echo ""
echo "====P0===="
tail -n 200 party0_shares.txt || true
echo ""
echo "====P1===="
tail -n 200 party1_shares.txt || true
echo ""
echo "====final reconstructed U and V ===="
tail -n 200 "final_reconstructed_UV.txt" || true
echo ""
if [[ "${CHECK_STATUS:-0}" == "0" ]]; then
  echo ""
  echo "======================================"
  echo "RESULT: PASS — final_reconstructed_UV.txt created."
  echo "======================================"
  exit 0
else
  echo ""
  echo "======================================"
  echo "RESULT: FAIL — mismatches detected. See logs and final_reconstructed_UV.txt"
  echo "======================================"
  exit 2
fi
