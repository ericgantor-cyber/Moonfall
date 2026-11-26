#!/usr/bin/env bash
# Run every non-equipment portable C++ suite with strict GCC release optimization.
set -euo pipefail
cd "$(dirname -- "$0")/.."
shopt -s nullglob
tests=(tools/test_unreal_*.cpp tools/test_v27_*.cpp)
workdir="$(mktemp -d)"
trap 'rm -rf -- "$workdir"' EXIT
total=0
passed=0
rows=()
for test in "${tests[@]}"; do
  [[ "$test" == tools/test_unreal_gear_advisor_policy.cpp ]] && continue
  total=$((total + 1))
  echo "=== $test ==="
  exe="$workdir/$(basename "$test" .cpp)"
  if g++ -std=c++17 -O2 -Wall -Wextra -Werror -pedantic \
      -I MoonfallUnreal/Source/MoonfallUnreal "$test" -o "$exe"; then
    if "$exe"; then
      result=Passed
      passed=$((passed + 1))
    else
      result="Test failed ($?)"
    fi
  else
    result="Compile failed ($?)"
  fi
  rows+=("| $test | $result |")
done
{
  echo '### Additional GCC portable policy tests'
  echo
  echo "Passed: $passed/$total"
  echo
  echo '| Suite | Result |'
  echo '| --- | --- |'
  printf '%s\n' "${rows[@]}"
} | tee -a "${GITHUB_STEP_SUMMARY:-/dev/null}"
[[ "$total" -gt 0 && "$passed" -eq "$total" ]]
