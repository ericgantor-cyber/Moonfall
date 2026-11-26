#!/usr/bin/env bash
# One strict compiler configuration for all standalone Unreal C++ policy tests.
set -euo pipefail
if [[ $# -ne 1 ]]; then
  echo 'Usage: bash tools/run_unreal_sanitizer.sh tools/test_unreal_example.cpp' >&2
  exit 2
fi
cd "$(dirname -- "$0")/.."
source_file="$1"
case "$source_file" in
  tools/test_unreal_*.cpp|tools/test_v27_*.cpp) ;;
  *) echo "Unsupported policy test: $source_file" >&2; exit 2 ;;
esac
[[ "$source_file" != *"/../"* && -f "$source_file" ]]
workdir="$(mktemp -d)"
trap 'rm -rf -- "$workdir"' EXIT
clang++ -std=c++17 -O1 -g -Wall -Wextra -Werror -pedantic \
  -fno-omit-frame-pointer -fsanitize=address,undefined -fno-sanitize-recover=all \
  -I MoonfallUnreal/Source/MoonfallUnreal "$source_file" -o "$workdir/policy"
ASAN_OPTIONS=detect_leaks=1:halt_on_error=1 \
UBSAN_OPTIONS=halt_on_error=1:print_stacktrace=1 "$workdir/policy"
