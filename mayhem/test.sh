#!/usr/bin/env bash
#
# mayhem/test.sh — RUN this repo's OWN functional test suite (already built by mayhem/build.sh).
# exit 0 = pass. EDIT per repo. PATCH-grade oracle: after an agent patches the source, the grader
# rebuilds (build.sh) then runs this. DELETE this file if the repo has no meaningful tests.
#
# IMPORTANT:
#  * Must assert BEHAVIOR/OUTPUT, not just exit status. The oracle has to check asserted values /
#    golden-output diffs / known-answer results — so a PATCH that "fixes" a bug by making the program
#    exit(0) (or any no-op) FAILS here. Running inputs and checking only "exit 0 / didn't crash" is
#    NOT a functional test (it's trivially reward-hackable) — use the project's real assertion suite.
#  * Do NOT build here — mayhem/build.sh already compiled the test suite (with the project's normal
#    flags). This script only RUNS the pre-built tests and reports counts. If the test runner is
#    missing, that's a build.sh bug — fail loudly rather than silently rebuilding.
#  * REQUIRED OUTPUT — a CTRF (https://ctrf.io) summary so Mayhem/the PATCH grader reads the counts:
#      - writes a CTRF JSON report to ${CTRF_REPORT:-$SRC/ctrf-report.json}, and
#      - prints a one-line `CTRF {...}` marker to stdout (same JSON, compact).
#    Only `results.summary` (with tests/passed/failed/pending/skipped/other) is required.
#    Use the emit_ctrf helper below; it computes tests = passed+failed+skipped and sets the exit
#    code (0 iff failed==0). Map your framework's output to passed/failed/skipped.
set -uo pipefail
[ -n "${SOURCE_DATE_EPOCH:-}" ] || unset SOURCE_DATE_EPOCH
: "${MAYHEM_JOBS:=$(nproc)}"   # build parallelism; env-overridable, falls back to nproc (use -j"$MAYHEM_JOBS")
cd "$SRC"

# emit_ctrf <tool> <passed> <failed> [skipped] [pending] [other]
# Writes a CTRF report (file + stdout `CTRF {...}` marker) and returns non-zero iff failed>0.
emit_ctrf() {
  local tool="$1" passed="$2" failed="$3" skipped="${4:-0}" pending="${5:-0}" other="${6:-0}"
  local tests=$(( passed + failed + skipped + pending + other ))
  cat > "${CTRF_REPORT:-$SRC/ctrf-report.json}" <<JSON
{
  "results": {
    "tool": { "name": "$tool" },
    "summary": {
      "tests": $tests,
      "passed": $passed,
      "failed": $failed,
      "pending": $pending,
      "skipped": $skipped,
      "other": $other
    }
  }
}
JSON
  printf 'CTRF {"results":{"tool":{"name":"%s"},"summary":{"tests":%d,"passed":%d,"failed":%d,"pending":%d,"skipped":%d,"other":%d}}}\n' \
    "$tool" "$tests" "$passed" "$failed" "$pending" "$skipped" "$other"
  [ "$failed" -eq 0 ]
}

# GTE ships no test suite of its own, so the oracle is a known-answer probe (mayhem/kat/gte_kat.cpp, built
# by build.sh with the project's normal flags, dynamically linked) over the same entry points the harnesses
# fuzz. Every expected value below was worked out independently of GTE (exact integer arithmetic in Python,
# the natural-spline equations solved by hand, convex hull / triangulation counted from the geometry, and
# the STL fixture written byte by byte). A missing probe or a missing output line is a failure.
PROBE="$SRC/gte_kat"
SEED="$SRC/mayhem/fuzz_stl_binary/testsuite/two_triangles.stl"
[ -x "$PROBE" ] || { echo "missing $PROBE: mayhem/build.sh did not build it" >&2; emit_ctrf "gte-kat" 0 1 0; exit 1; }
[ -f "$SEED" ] || { echo "missing $SEED" >&2; emit_ctrf "gte-kat" 0 1 0; exit 1; }

actual="$("$PROBE" "$SEED" 2>/dev/null || true)"

expected='bsnumber_product_double 1.2193263112482853e+38
bsnumber_product_equals 1
bsnumber_difference_sign -1
bsrational_sum_double 0.29999999999999999
bsrational_quarter_third 0.125
convex_hull2_dimension 2
convex_hull2_indices 5,0,1,3,
delaunay2_dimension 2
delaunay2_triangles 4
delaunay2_hull_size 8
spline_knot 1 2 3
spline_mid 0.6875 1.375 2.0625
stl_load 1
stl_triangles 2
stl_tri1_vertex1 1 1 0
stl_tri1_attr 7
stl_roundtrip 1 2'

passed=0 ; failed=0
while IFS= read -r line; do
  if printf '%s\n' "$actual" | grep -Fxq -- "$line"; then
    passed=$((passed + 1))
  else
    failed=$((failed + 1))
    echo "FAIL: expected '$line'; probe printed: $(printf '%s\n' "$actual" | grep -F -- "${line%% *} " || echo '<nothing>')" >&2
  fi
done <<< "$expected"

emit_ctrf "gte-kat" "$passed" "$failed" 0
