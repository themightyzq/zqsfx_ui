#!/bin/bash
# FAST tier — every commit. Builds the test programs CTest knows about (into build-check/,
# arm64 Release, never installed) and runs CTest. The program list comes from CTest itself
# (`ctest -N -V`), so a newly registered test is built and run without editing this file.
# Before a `dv commit` that changes this folder, the JUCE repo's .dvhooks/pre-commit runs it (through
# the JUCE root's tools/check.sh --hook); zq-commit-gate runs it before Claude's dv and git commits.
# Template: ~/Projects/_policy/check-sh-template.sh. Full tier: CLAUDE.md, "Verification tiers".
set -u
cd "$(dirname "$0")/.." || exit 2
# Diversion runs the hook once per chunk of a large commit, in parallel, and two builds into
# one build-check/ tree corrupt each other. Take turns; a lock left by a killed run is cleared
# after 10 minutes. (build-check.lock/ sits beside build-check/ and is ignored with it.)
lock=build-check.lock
until mkdir "$lock" 2>/dev/null; do
  [ -n "$(find "$lock" -maxdepth 0 -mmin +10 2>/dev/null)" ] && rmdir "$lock" 2>/dev/null
  sleep 1
done
trap 'rmdir "$lock" 2>/dev/null' EXIT
fail=0
step() {
  local name="$1"; shift
  local log; log="$(mktemp -t fastcheck)"; local t0=$SECONDS
  if "$@" >"$log" 2>&1; then
    printf '  ok    %-44s %3ss\n' "$name" "$((SECONDS - t0))"
  else
    local rc=$?
    printf '  FAIL  %-44s %3ss (exit %s)\n' "$name" "$((SECONDS - t0))" "$rc"
    tail -20 "$log" | sed 's/^/        /'
    fail=1
  fi
  rm -f "$log"
}
echo "fast check: $(basename "$PWD")"
# ---- project steps ----
configure() {
  [ -f build-check/CMakeCache.txt ] && return 0
  cmake -S . -B build-check -DCMAKE_BUILD_TYPE=Release -DCMAKE_OSX_ARCHITECTURES=arm64
}
# The CMake target behind each CTest entry, read from its command line. Test names are not
# target names here: Catch2 lists one entry per test case (or <target>_NOT_BUILT before the
# first build), and a JUCE console app lives at <target>_artefacts/<config>/<product name>.
# Commands outside build-check/ (cmake -P, python) need no build and are skipped.
test_targets() {
  local bdir; bdir="$(cd build-check && pwd)"
  # A built program is listed as "Test command: <path> <args>", an unbuilt one as
  # "Could not find executable <path>".
  ctest --test-dir build-check -N -V |
    sed -n -e 's/^.*Test command: //p' -e 's/^Could not find executable //p' | awk '{print $1}' |
    while read -r exe; do
      case "$exe" in
        *_NOT_BUILT*) basename "${exe%%_NOT_BUILT*}" ;;
        "$bdir"/*_artefacts/*) local t="${exe#"$bdir"/}"; basename "${t%%_artefacts/*}" ;;
        "$bdir"/*) basename "$exe" ;;
      esac
    done | sort -u
}
build_tests() {
  local targets; targets=$(test_targets)
  [ -n "$targets" ] || { echo "CTest lists no tests in build-check"; return 1; }
  echo "targets:" $targets
  # shellcheck disable=SC2086
  cmake --build build-check --config Release --parallel 10 --target $targets
}
step "configure build-check (first run only)" configure
step "build the CTest programs" build_tests
step "ctest" ctest --test-dir build-check -C Release --output-on-failure --no-tests=error
# ----------------------------------------------------------
if [ "$fail" -ne 0 ]; then echo "FAST CHECK FAILED"; exit 1; fi
echo "FAST CHECK PASSED"
