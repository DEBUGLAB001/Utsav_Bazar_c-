#!/usr/bin/env bash
set -e
cd "$(dirname "$0")"
if [ ! -d build ]; then cmake -B build -S . -DCMAKE_BUILD_TYPE=Release; fi
cmake --build build --target server -j2 1>&2
PORT="${PORT:-8080}" ./build/server &
PID=$!
trap 'kill -TERM "$PID" 2>/dev/null || true; wait "$PID" 2>/dev/null || true; exit 0' SIGTERM SIGINT EXIT
get_snap() { find src include -type f \( -name "*.cpp" -o -name "*.hpp" -o -name "*.h" \) -exec stat -c "%Y %n" {} + 2>/dev/null | sort; }
LAST_SNAP=$(get_snap)
while true; do
  sleep 2
  CUR_SNAP=$(get_snap)
  if [ "$CUR_SNAP" != "$LAST_SNAP" ]; then
    echo "[Engine] Source modified. Rebuilding C++ server..."
    if cmake --build build --target server -j2 1>&2; then
      kill -TERM "$PID" 2>/dev/null || true; wait "$PID" 2>/dev/null || true
      PORT="${PORT:-8080}" ./build/server & PID=$!
      echo "[Engine] Server reloaded successfully."
    fi
    LAST_SNAP="$CUR_SNAP"
  fi
done
