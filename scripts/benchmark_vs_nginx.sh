#!/bin/bash
# benchmark_vs_nginx.sh
#
# Static-file throughput comparison: AmmarServer vs. the nginx reference build at 3dparty/nginx (see
# knowledge/ammarserver.md §11). Builds nginx if it hasn't been built yet ( scripts/build_nginx.sh ), starts a
# throwaway, self-contained nginx instance ( its own temp prefix dir, daemon off, foreground child process -
# nothing written outside /tmp and nothing touching any system nginx install ) serving the same public_html/
# webroot AmmarServer serves, then delegates the actual wrk matrix to scripts/benchmark_ammarserver.sh via
# --compare-url --static-only ( nginx has no equivalent to AmmarServer's dynamic SAME_PAGE resource without a
# whole separate FastCGI/PHP-FPM pipeline, so this is a static-file-only comparison - the fair, standard
# nginx-vs-something benchmark ).
#
# Usage:
#   scripts/benchmark_vs_nginx.sh                                  # build+benchmark both, static file only
#   scripts/benchmark_vs_nginx.sh --duration 10 --concurrency "10 100 200"
#   scripts/benchmark_vs_nginx.sh --static-resource /logo.png
#   scripts/benchmark_vs_nginx.sh --ammarserver-url http://127.0.0.1:8080   # benchmark an already-running AmmarServer
#
# Anything not recognized here is passed straight through to benchmark_ammarserver.sh ( --threads, --skip-build, etc ).

set -u

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"
NGINX_DIR="$REPO_ROOT/3dparty/nginx"
NGINX_BIN="$NGINX_DIR/objs/nginx"

NGINX_PORT=18100
WEBROOT="$REPO_ROOT/public_html/"
STATIC_RESOURCE="/logo.png"
AMMARSERVER_URL=""
PASSTHROUGH_ARGS=()

while [ $# -gt 0 ]; do
  case "$1" in
    --nginx-port) NGINX_PORT="$2"; shift 2 ;;
    --root) WEBROOT="$2"; PASSTHROUGH_ARGS+=(--root "$2"); shift 2 ;;
    --static-resource) STATIC_RESOURCE="$2"; PASSTHROUGH_ARGS+=(--static-resource "$2"); shift 2 ;;
    --ammarserver-url) AMMARSERVER_URL="$2"; shift 2 ;;
    -h|--help)
      grep '^#' "$0" | sed 's/^# \{0,1\}//'
      exit 0
      ;;
    *) PASSTHROUGH_ARGS+=("$1"); shift 1 ;;
  esac
done

if [ ! -x "$NGINX_BIN" ]; then
  echo "nginx not built yet - building it now (one-time, see scripts/build_nginx.sh) ..." >&2
  "$SCRIPT_DIR/build_nginx.sh" || { echo "Failed to build nginx" >&2; exit 1; }
fi

if [ ! -d "$WEBROOT" ]; then
  echo "Webroot not found: $WEBROOT" >&2
  exit 1
fi

NGINX_RUN_DIR="$(mktemp -d /tmp/ammarserver_vs_nginx.XXXXXX)"
mkdir -p "$NGINX_RUN_DIR/logs"

cat > "$NGINX_RUN_DIR/nginx.conf" <<EOF
daemon off;
worker_processes auto;
pid $NGINX_RUN_DIR/nginx.pid;
error_log $NGINX_RUN_DIR/logs/error.log warn;

events {
    worker_connections 4096;
}

http {
    access_log $NGINX_RUN_DIR/logs/access.log;
    sendfile on;
    tcp_nopush on;
    tcp_nodelay on;
    keepalive_timeout 65;

    server {
        listen $NGINX_PORT;
        server_name _;
        root $WEBROOT;
    }
}
EOF

NGINX_PID=""
cleanup() {
  if [ -n "$NGINX_PID" ]; then
    kill "$NGINX_PID" >/dev/null 2>&1
    wait "$NGINX_PID" 2>/dev/null
  fi
  rm -rf "$NGINX_RUN_DIR"
}
trap cleanup EXIT

echo "Starting throwaway nginx on port $NGINX_PORT (root: $WEBROOT, config: $NGINX_RUN_DIR/nginx.conf) ..." >&2
"$NGINX_BIN" -p "$NGINX_RUN_DIR/" -c "$NGINX_RUN_DIR/nginx.conf" &
NGINX_PID=$!
sleep 1
if ! kill -0 "$NGINX_PID" 2>/dev/null; then
  echo "nginx failed to start - see $NGINX_RUN_DIR/logs/error.log" >&2
  cat "$NGINX_RUN_DIR/logs/error.log" >&2 2>/dev/null
  exit 1
fi

if ! curl -s -o /dev/null "http://127.0.0.1:$NGINX_PORT$STATIC_RESOURCE"; then
  echo "nginx started but isn't answering requests on $STATIC_RESOURCE - see $NGINX_RUN_DIR/logs/error.log" >&2
  exit 1
fi
echo "nginx is up." >&2

AMMARSERVER_ARGS=(--compare-url "http://127.0.0.1:$NGINX_PORT" --compare-label "nginx" --static-only "${PASSTHROUGH_ARGS[@]}")
if [ -n "$AMMARSERVER_URL" ]; then
  AMMARSERVER_ARGS=(--url "$AMMARSERVER_URL" "${AMMARSERVER_ARGS[@]}")
fi

"$SCRIPT_DIR/benchmark_ammarserver.sh" "${AMMARSERVER_ARGS[@]}"
