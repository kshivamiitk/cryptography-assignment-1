#!/usr/bin/env bash
set -euo pipefail

usage() {
  cat <<EOF
Usage: $0 <m> <n> <k> <q>

EOF
  exit 1
}

if [[ "${1:-}" == "" || "${2:-}" == "" || "${3:-}" == "" || "${4:-}" == "" ]]; then
  usage
fi

M="$1"
N="$2" 
K="$3"
Q="$4"

export M N K Q

echo "final_run.sh: mapped args -> M=$M N=$N K=$K Q=$Q"
echo "Using 'docker-compose'."

if command -v docker >/dev/null 2>&1; then
  echo "Stopping any previous composition (if exists)..."
  docker compose down --remove-orphans || true
fi

echo "Building image and services..."
docker compose build --no-cache

echo "Starting services (this will stream logs)..."

docker compose up --remove-orphans --force-recreate
