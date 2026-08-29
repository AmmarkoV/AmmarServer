#!/bin/bash
# benchmark_vs_apache.sh
#
# AmmarServer vs. a real, already-running Apache instance : static file throughput AND compute-bound dynamic
# content ( C callback vs. PHP - see scripts/benchmark_primes.php ). Unlike scripts/benchmark_vs_nginx.sh, this
# script does NOT start/stop/reconfigure Apache itself - Apache is assumed to already be running as a normal
# system service ( `systemctl status apache2` ), same as any other site it happens to be hosting, and this
# script never touches that service. All it does is:
#   1. Stage two distinctively-named files ( ammarserver_bench_logo.png , ammarserver_bench_primes.php ) into
#      Apache's docroot - refuses to run if either name already exists there, so it can never overwrite real
#      content.
#   2. Run the same wrk matrix scripts/benchmark_ammarserver.sh already knows how to run, pointed at both
#      servers via --compare-url ( with --compare-static-resource/--compare-dynamic-resource , since Apache's
#      docroot is real site content and the staged files can't reuse AmmarServer's own /logo.png / /primes.html
#      paths ).
#   3. Removes the staged files again on exit, however the script exits.
#
# Usage:
#   scripts/benchmark_vs_apache.sh                                   # auto-detects Apache's docroot, benchmarks both
#   scripts/benchmark_vs_apache.sh --apache-url http://127.0.0.1:80 --apache-root /var/www/html
#   scripts/benchmark_vs_apache.sh --static-only                     # skip the PHP compute comparison
#   scripts/benchmark_vs_apache.sh --duration 10 --concurrency "20 100 200"
#
# Anything not recognized here is passed straight through to benchmark_ammarserver.sh.
#
# Requires: Apache already running and reachable at --apache-url ; PHP + mod_php (or php-fpm wired up as the
# handler for .php) enabled in Apache, unless --static-only is passed.

set -u

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"

APACHE_URL="http://127.0.0.1:80"
APACHE_ROOT=""
WEBROOT="$REPO_ROOT/public_html/"
STATIC_FILE="logo.png"
PRIMES_PHP="$REPO_ROOT/scripts/benchmark_primes.php"
STAGED_STATIC_NAME="ammarserver_bench_logo.png"
STAGED_PHP_NAME="ammarserver_bench_primes.php"
STATIC_ONLY_REQUESTED=0
PASSTHROUGH_ARGS=()

while [ $# -gt 0 ]; do
  case "$1" in
    --apache-url) APACHE_URL="$2"; shift 2 ;;
    --apache-root) APACHE_ROOT="$2"; shift 2 ;;
    --root) WEBROOT="$2"; shift 2 ;;
    --static-only) STATIC_ONLY_REQUESTED=1; shift 1 ;;
    -h|--help)
      grep '^#' "$0" | sed 's/^# \{0,1\}//'
      exit 0
      ;;
    *) PASSTHROUGH_ARGS+=("$1"); shift 1 ;;
  esac
done

if ! curl -s -o /dev/null "$APACHE_URL/"; then
  echo "Apache doesn't seem to be reachable at $APACHE_URL - this script does not start/stop Apache itself." >&2
  echo "Start it yourself first ( e.g. 'sudo systemctl start apache2' ), or pass --apache-url if it's elsewhere." >&2
  exit 1
fi

if [ -z "$APACHE_ROOT" ]; then
  APACHE_ROOT="$(grep -rhoP '(?<=DocumentRoot\s)\S+' /etc/apache2/sites-enabled/*.conf 2>/dev/null | head -1 | tr -d '"')"
  if [ -z "$APACHE_ROOT" ]; then
    echo "Couldn't auto-detect Apache's DocumentRoot from /etc/apache2/sites-enabled/*.conf - pass --apache-root explicitly." >&2
    exit 1
  fi
  echo "Auto-detected Apache DocumentRoot: $APACHE_ROOT" >&2
fi

if [ ! -w "$APACHE_ROOT" ]; then
  echo "$APACHE_ROOT is not writable by this user - either fix permissions or point --apache-root at a writable docroot." >&2
  exit 1
fi

STAGED_STATIC_PATH="$APACHE_ROOT/$STAGED_STATIC_NAME"
STAGED_PHP_PATH="$APACHE_ROOT/$STAGED_PHP_NAME"

if [ -e "$STAGED_STATIC_PATH" ] || [ -e "$STAGED_PHP_PATH" ]; then
  echo "$STAGED_STATIC_PATH or $STAGED_PHP_PATH already exists - refusing to touch it. Remove it yourself if it's" >&2
  echo "leftover from a previous interrupted run of this script, otherwise it's someone else's real content." >&2
  exit 1
fi

cleanup() {
  rm -f "$STAGED_STATIC_PATH" "$STAGED_PHP_PATH"
}
trap cleanup EXIT

echo "Staging $STATIC_FILE -> $STAGED_STATIC_PATH ..." >&2
cp "$WEBROOT$STATIC_FILE" "$STAGED_STATIC_PATH" || { echo "Failed to stage static file" >&2; exit 1; }

DO_PHP=0
if [ "$STATIC_ONLY_REQUESTED" -eq 0 ]; then
  DO_PHP=1
  echo "Staging benchmark_primes.php -> $STAGED_PHP_PATH ..." >&2
  cp "$PRIMES_PHP" "$STAGED_PHP_PATH" || { echo "Failed to stage benchmark_primes.php" >&2; exit 1; }
fi

if ! curl -s -o /dev/null -w '' --fail "$APACHE_URL/$STAGED_STATIC_NAME"; then
  echo "Apache isn't serving the staged static file correctly at $APACHE_URL/$STAGED_STATIC_NAME" >&2
  exit 1
fi

if [ "$DO_PHP" -eq 1 ]; then
  PHP_OUTPUT="$(curl -s "$APACHE_URL/$STAGED_PHP_NAME")"
  if ! echo "$PHP_OUTPUT" | grep -q "primes_below_n="; then
    echo "Apache isn't executing the staged PHP file correctly at $APACHE_URL/$STAGED_PHP_NAME - is mod_php enabled?" >&2
    echo "Got: $PHP_OUTPUT" >&2
    echo "Falling back to --static-only for this run." >&2
    DO_PHP=0
  else
    echo "PHP side OK: $PHP_OUTPUT" >&2
  fi
fi

AMMARSERVER_ARGS=(
  --compare-url "$APACHE_URL"
  --compare-label "Apache"
  --root "$WEBROOT"
  --static-resource "/$STATIC_FILE"
  --compare-static-resource "/$STAGED_STATIC_NAME"
)
if [ "$DO_PHP" -eq 1 ]; then
  AMMARSERVER_ARGS+=(--dynamic-resource "/primes.html" --compare-dynamic-resource "/$STAGED_PHP_NAME")
else
  AMMARSERVER_ARGS+=(--static-only)
fi
AMMARSERVER_ARGS+=("${PASSTHROUGH_ARGS[@]}")

"$SCRIPT_DIR/benchmark_ammarserver.sh" "${AMMARSERVER_ARGS[@]}"
