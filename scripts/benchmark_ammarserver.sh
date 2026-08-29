#!/bin/bash
# benchmark_ammarserver.sh
#
# Reusable wrk-based benchmark for AmmarServer. Starts a throwaway AmmarServer instance ( unless --url is
# given ), runs a small matrix of wrk tests against it ( static file + dynamic SAME_PAGE resource , at a few
# concurrency levels , always with explicit "Connection: keep-alive" - AmmarServer only treats a connection as
# keep-alive when a client says so explicitly, so omitting this header understates real throughput ), and
# cleans up after itself.
#
# Usage:
#   scripts/benchmark_ammarserver.sh                          # build+benchmark the local repo build
#   scripts/benchmark_ammarserver.sh --port 18099 --duration 10
#   scripts/benchmark_ammarserver.sh --url http://127.0.0.1:8080          # benchmark an already-running server instead
#   scripts/benchmark_ammarserver.sh --compare-url http://127.0.0.1:80    # also benchmark a second server ( e.g. Apache ) for comparison
#   scripts/benchmark_ammarserver.sh --compare-url http://127.0.0.1:80 --static-only  # skip the dynamic resource
#                                                                                       ( e.g. comparing against a
#                                                                                         plain static server like
#                                                                                         nginx with nothing dynamic
#                                                                                         registered at that path )
#
# Requires wrk. If it isn't already built at 3dparty/wrk/wrk or on PATH, this script builds it there
# ( git clone + make, same as this repo already does for reference sources under 3dparty/ ).

set -u

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"

PORT=18099
DURATION=8
THREADS=4
CONCURRENCY_LEVELS="10 100 200"
BUILD_DIR="$REPO_ROOT/build"
BINARY="$REPO_ROOT/src/Services/AmmarServer/ammarserver"
WEBROOT="$REPO_ROOT/public_html/"
STATIC_RESOURCE="/logo.png"
DYNAMIC_RESOURCE="/stats.html"
TARGET_URL=""
COMPARE_URL=""
COMPARE_LABEL="Comparison target"
SKIP_BUILD=0
STATIC_ONLY=0

while [ $# -gt 0 ]; do
  case "$1" in
    --port) PORT="$2"; shift 2 ;;
    --duration) DURATION="$2"; shift 2 ;;
    --threads) THREADS="$2"; shift 2 ;;
    --concurrency) CONCURRENCY_LEVELS="$2"; shift 2 ;;
    --root) WEBROOT="$2"; shift 2 ;;
    --static-resource) STATIC_RESOURCE="$2"; shift 2 ;;
    --dynamic-resource) DYNAMIC_RESOURCE="$2"; shift 2 ;;
    --url) TARGET_URL="$2"; shift 2 ;;
    --compare-url) COMPARE_URL="$2"; shift 2 ;;
    --compare-label) COMPARE_LABEL="$2"; shift 2 ;;
    --skip-build) SKIP_BUILD=1; shift 1 ;;
    --static-only) STATIC_ONLY=1; shift 1 ;;
    -h|--help)
      grep '^#' "$0" | sed 's/^# \{0,1\}//'
      exit 0
      ;;
    *) echo "Unknown argument: $1" >&2; exit 1 ;;
  esac
done

WRK="$REPO_ROOT/3dparty/wrk/wrk"
if [ ! -x "$WRK" ]; then
  if command -v wrk >/dev/null 2>&1; then
    WRK="$(command -v wrk)"
  else
    echo "wrk not found - building it into 3dparty/wrk (one-time)..." >&2
    mkdir -p "$REPO_ROOT/3dparty"
    if [ ! -d "$REPO_ROOT/3dparty/wrk" ]; then
      git clone --depth 1 https://github.com/wg/wrk.git "$REPO_ROOT/3dparty/wrk" || { echo "Failed to clone wrk" >&2; exit 1; }
    fi
    make -C "$REPO_ROOT/3dparty/wrk" -j"$(nproc)" || { echo "Failed to build wrk" >&2; exit 1; }
  fi
fi

SERVER_PID=""
cleanup() {
  if [ -n "$SERVER_PID" ]; then
    kill "$SERVER_PID" >/dev/null 2>&1
  fi
}
trap cleanup EXIT

if [ -z "$TARGET_URL" ]; then
  if [ "$SKIP_BUILD" -eq 0 ]; then
    if [ ! -d "$BUILD_DIR" ]; then
      echo "No build/ directory found at $BUILD_DIR - run cmake once first, or pass --url to benchmark an already-running server." >&2
      exit 1
    fi
    echo "Building ammarserver ..." >&2
    cmake --build "$BUILD_DIR" --target ammarserver -j"$(nproc)" >&2 || { echo "Build failed" >&2; exit 1; }
  fi

  if [ ! -x "$BINARY" ]; then
    echo "Binary not found at $BINARY" >&2
    exit 1
  fi

  echo "Starting test instance on port $PORT (root: $WEBROOT) ..." >&2
  ( cd "$(dirname "$BINARY")" && exec "$BINARY" -p "$PORT" --root "$WEBROOT" ) > /tmp/benchmark_ammarserver.$PORT.log 2>&1 &
  SERVER_PID=$!
  sleep 1
  if ! kill -0 "$SERVER_PID" 2>/dev/null; then
    echo "Server failed to start - see /tmp/benchmark_ammarserver.$PORT.log" >&2
    exit 1
  fi
  TARGET_URL="http://127.0.0.1:$PORT"
fi

run_matrix() {
  local label="$1" base_url="$2"
  echo ""
  echo "=== $label ($base_url) ==="
  local resource_pairs="static:$STATIC_RESOURCE"
  if [ "$STATIC_ONLY" -eq 0 ]; then
    resource_pairs="$resource_pairs dynamic:$DYNAMIC_RESOURCE"
  fi
  for resource_label_pair in $resource_pairs; do
    resource_label="${resource_label_pair%%:*}"
    resource="${resource_label_pair#*:}"
    for c in $CONCURRENCY_LEVELS; do
      echo "--- $resource_label $resource , concurrency=$c ---"
      "$WRK" -t"$THREADS" -c"$c" -d"${DURATION}s" -H "Connection: keep-alive" "$base_url$resource" \
        | grep -E "Requests/sec|Latency|Socket errors"
    done
  done
}

run_matrix "AmmarServer" "$TARGET_URL"

if [ -n "$COMPARE_URL" ]; then
  run_matrix "$COMPARE_LABEL" "$COMPARE_URL"
fi

echo ""
echo "Done."
