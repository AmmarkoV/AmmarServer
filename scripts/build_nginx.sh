#!/bin/bash
# build_nginx.sh
#
# Configures and builds the nginx reference source vendored at 3dparty/nginx (see knowledge/ammarserver.md §11 -
# it's there purely as a comparison target for scripts/benchmark_vs_nginx.sh, not linked into AmmarServer itself).
# Produces 3dparty/nginx/objs/nginx. Never runs `make install` - this stays a throwaway, self-contained build,
# nothing is written outside 3dparty/nginx/.
#
# Usage:
#   scripts/build_nginx.sh                # configure + build (clones 3dparty/nginx first if it's missing)
#   scripts/build_nginx.sh --reconfigure   # force re-running ./configure even if already configured
#   scripts/build_nginx.sh --jobs 4        # override parallel job count (defaults to nproc)
#
# Requires a C compiler + make, and PCRE2 + zlib development headers (Debian/Ubuntu: libpcre2-dev zlib1g-dev) -
# the same class of packages scripts/get_dependencies.sh installs for AmmarServer's own build. The http_rewrite
# module is left disabled (--without-http_rewrite_module) specifically to avoid needing a PCRE dependency at
# all beyond what's already normally present - the benchmark this feeds only ever uses a plain `location /`
# static-file block, so regex location matching is never needed.

set -u

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"
NGINX_DIR="$REPO_ROOT/3dparty/nginx"

JOBS="$(nproc)"
RECONFIGURE=0

while [ $# -gt 0 ]; do
  case "$1" in
    --jobs) JOBS="$2"; shift 2 ;;
    --reconfigure) RECONFIGURE=1; shift 1 ;;
    -h|--help)
      grep '^#' "$0" | sed 's/^# \{0,1\}//'
      exit 0
      ;;
    *) echo "Unknown argument: $1" >&2; exit 1 ;;
  esac
done

if [ ! -d "$NGINX_DIR" ]; then
  echo "3dparty/nginx not found - cloning it (one-time)..." >&2
  mkdir -p "$REPO_ROOT/3dparty"
  git clone --depth 1 https://github.com/nginx/nginx.git "$NGINX_DIR" || { echo "Failed to clone nginx" >&2; exit 1; }
fi

if [ ! -f "$NGINX_DIR/auto/configure" ]; then
  echo "$NGINX_DIR doesn't look like an nginx source tree (no auto/configure found)" >&2
  exit 1
fi

cd "$NGINX_DIR" || exit 1

if [ "$RECONFIGURE" -eq 1 ] || [ ! -f objs/Makefile ]; then
  echo "Configuring nginx (prefix kept self-contained under 3dparty/nginx/objs/prefix) ..." >&2
  ./auto/configure \
    --prefix="$NGINX_DIR/objs/prefix" \
    --without-http_rewrite_module \
    || { echo "nginx ./configure failed - see output above (likely missing PCRE2/zlib dev headers, e.g. 'apt install libpcre2-dev zlib1g-dev')" >&2; exit 1; }
else
  echo "Already configured (objs/Makefile exists) - skipping ./configure. Use --reconfigure to force it." >&2
fi

echo "Building nginx (make -j$JOBS) ..." >&2
make -j"$JOBS" || { echo "nginx build failed" >&2; exit 1; }

if [ ! -x "$NGINX_DIR/objs/nginx" ]; then
  echo "Build finished but 3dparty/nginx/objs/nginx wasn't produced - something's wrong" >&2
  exit 1
fi

echo ""
echo "Built: $NGINX_DIR/objs/nginx"
"$NGINX_DIR/objs/nginx" -v
