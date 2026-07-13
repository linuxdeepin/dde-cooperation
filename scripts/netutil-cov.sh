#!/usr/bin/env bash
# Quick netutil-only coverage measurement for fast iteration.
# Usage: ./scripts/netutil-cov.sh [build_dir]
set -uo pipefail
PROJECT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
BUILD_DIR="${1:-${BUILD_DIR:-build-full}}"
COV_DIR="$BUILD_DIR/cov-netutil"
NETUTIL_TESTS_DIR="$BUILD_DIR/src/infrastructure/netutil/tests"

cd "$PROJECT_DIR"

# Rebuild the netutil test binaries (incremental)
step() { printf "\n\033[0;36m===== %s =====\033[0m\n" "$1"; }
step "Build netutil tests"
cmake --build "$BUILD_DIR" --target netutil_http_tests netutil_logic_tests -j"$(nproc)" 2>&1 | tail -5

mkdir -p "$COV_DIR"

step "Clean gcda & run netutil test binaries"
find "$BUILD_DIR" -name '*.gcda' -delete 2>/dev/null

declare -A BINS=(
  [netutil_http_tests]="$NETUTIL_TESTS_DIR/netutil_http_tests"
  [netutil_logic_tests]="$NETUTIL_TESTS_DIR/netutil_logic_tests"
)
for name in "${!BINS[@]}"; do
  bin="${BINS[$name]}"
  [[ -x "$bin" ]] || { echo "  $name: binary not found"; continue; }
  printf "  -> %-22s ... " "$name"
  if timeout 120 "$bin" > "/tmp/${name}.log" 2>&1; then
    echo -e "\033[0;32mPASS\033[0m"
  else
    code=$?
    [[ $code -eq 124 ]] && echo -e "\033[1;33mTIMEOUT(kill)\033[0m" || echo -e "\033[1;33mexit=$code\033[0m"
  fi
done

step "Capture & filter netutil coverage"
lcov --initial -c -d "$BUILD_DIR" -o "$COV_DIR/base.info" --rc lcov_branch_coverage=0 -q 2>/dev/null
lcov -c -d "$BUILD_DIR" -o "$COV_DIR/run.info" --rc lcov_branch_coverage=0 -q 2>/dev/null
lcov -a "$COV_DIR/base.info" -a "$COV_DIR/run.info" -o "$COV_DIR/comb.info" --rc lcov_branch_coverage=0 -q 2>/dev/null
lcov -e "$COV_DIR/comb.info" '*/src/infrastructure/netutil/*' -o "$COV_DIR/netutil.info" --rc lcov_branch_coverage=0 -q 2>/dev/null
lcov -r "$COV_DIR/netutil.info" '*/tests/*' -o "$COV_DIR/final.info" --rc lcov_branch_coverage=0 -q 2>/dev/null

step "Per-file coverage"
python3 - "$COV_DIR/final.info" <<'PY'
import sys
files=[]; cur=None
with open(sys.argv[1]) as f:
    for line in f:
        line=line.strip()
        if line.startswith('SF:'): cur={'sf':line[3:],'lf':0,'lh':0}
        elif line.startswith('LF:'): cur['lf']=int(line[3:])
        elif line.startswith('LH:'): cur['lh']=int(line[3:])
        elif line=='end_of_record' and cur: files.append(cur); cur=None
tl=th=0
for fr in sorted(files,key=lambda x:x['sf']):
    p=100.0*fr['lh']/fr['lf'] if fr['lf'] else 0
    tl+=fr['lf']; th+=fr['lh']
    print(f"{p:6.2f}%  LH={fr['lh']:4d} LF={fr['lf']:4d}  {fr['sf'].split('netutil/')[-1]}")
p=100.0*th/tl if tl else 0
print(f"\nTOTAL: {p:.2f}%  LH={th} LF={tl}  (need 70% = {int(tl*0.7)})")
PY
