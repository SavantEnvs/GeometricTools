#!/usr/bin/env bash
#
# mayhem/build.sh — build the GTE fuzz harnesses (sanitized libFuzzer + standalone reproducers) and the
# known-answer probe test.sh runs. GTE is a header-only C++ library (GTE/Mathematics/*.h), so there is no
# project build step: each harness translation unit instantiates the templates it fuzzes, and is compiled
# with $SANITIZER_FLAGS and $DEBUG_FLAGS, which instruments the library code along with the harness.
set -euo pipefail

[ -n "${SOURCE_DATE_EPOCH:-}" ] || unset SOURCE_DATE_EPOCH

: "${SANITIZER_FLAGS=-fsanitize=address,undefined -fno-sanitize-recover=all -fno-omit-frame-pointer}"
: "${DEBUG_FLAGS:=-g -gdwarf-3}"
: "${CC:=clang}" ; : "${CXX:=clang++}" ; : "${LIB_FUZZING_ENGINE:=-fsanitize=fuzzer}"
: "${MAYHEM_JOBS:=$(nproc)}"
: "${COVERAGE_FLAGS=}"
export SANITIZER_FLAGS DEBUG_FLAGS CC CXX LIB_FUZZING_ENGINE MAYHEM_JOBS COVERAGE_FLAGS

SRC="${SRC:-/mayhem}"
cd "$SRC"

OUT="$SRC"
BUILD="$SRC/mayhem/build"
mkdir -p "$BUILD"

TARGETS="fuzz_stl_binary fuzz_bsnumber_string fuzz_convex_hull2 fuzz_delaunay2 fuzz_natural_spline"
CXXSTD="-std=c++17 -O1 -I$SRC/GTE"

# The standalone driver is a C file: compile it as C so its LLVMFuzzerTestOneInput reference is not
# mangled when it is linked with a C++ harness.
# shellcheck disable=SC2086
$CC $SANITIZER_FLAGS $DEBUG_FLAGS -c "$STANDALONE_FUZZ_MAIN" -o "$BUILD/standalone_main.o"
# Disable LeakSanitizer at build time for every binary; ASan and UBSan stay on.
# shellcheck disable=SC2086
$CXX $SANITIZER_FLAGS $DEBUG_FLAGS -c "$SRC/mayhem/lsan_off.cc" -o "$BUILD/lsan_off.o"

pids=()
slots=0
for t in $TARGETS; do
  # shellcheck disable=SC2086
  ( $CXX $SANITIZER_FLAGS $DEBUG_FLAGS $CXXSTD -fsanitize=fuzzer-no-link -c "$SRC/mayhem/harnesses/$t.cpp" -o "$BUILD/$t.fuzz.o" ) &
  pids+=($!)
  # shellcheck disable=SC2086
  ( $CXX $SANITIZER_FLAGS $DEBUG_FLAGS $CXXSTD -c "$SRC/mayhem/harnesses/$t.cpp" -o "$BUILD/$t.standalone.o" ) &
  pids+=($!)
  slots=$((slots + 2))
  if [ "$slots" -ge "$MAYHEM_JOBS" ]; then
    for p in "${pids[@]}"; do wait "$p"; done
    pids=(); slots=0
  fi
done
for p in "${pids[@]:-}"; do [ -z "$p" ] || wait "$p"; done

for t in $TARGETS; do
  # shellcheck disable=SC2086
  $CXX $SANITIZER_FLAGS $DEBUG_FLAGS $LIB_FUZZING_ENGINE "$BUILD/$t.fuzz.o" "$BUILD/lsan_off.o" -o "$OUT/$t"
  # shellcheck disable=SC2086
  $CXX $SANITIZER_FLAGS $DEBUG_FLAGS "$BUILD/$t.standalone.o" "$BUILD/standalone_main.o" "$BUILD/lsan_off.o" -o "$OUT/$t-standalone"
done

# Known-answer probe for mayhem/test.sh: the project's NORMAL flags (no sanitizer, no -gdwarf-3), dynamically
# linked, so the oracle stays an honest functional check.
# shellcheck disable=SC2086
$CXX -std=c++17 -O2 $COVERAGE_FLAGS -I"$SRC/GTE" "$SRC/mayhem/kat/gte_kat.cpp" -o "$OUT/gte_kat"
