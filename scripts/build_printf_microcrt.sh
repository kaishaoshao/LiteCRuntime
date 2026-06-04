#!/usr/bin/env bash

set -u
set -o pipefail

ROOT_DIR=$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)
OUT_DIR=${1:-"$ROOT_DIR/build/printf-compare"}
LOG_DIR="$OUT_DIR/logs"
CSV_PATH="$OUT_DIR/microcrt.csv"

mkdir -p "$OUT_DIR" "$LOG_DIR"
source "$ROOT_DIR/scripts/printf_compare_common.sh"

declare -A BUILD_STATE
declare -A VERIFY_STATE
declare -A BUILD_NOTE
declare -A API_SCOPE
declare -A FORMAT_SCOPE
declare -A SIZE_FILES
declare -A SIZE_TEXT
declare -A SIZE_DATA
declare -A SIZE_BSS
declare -A SIZE_TOTAL
declare -A SIZE_NOTE

setup_metadata() {
  API_SCOPE["microcrt"]="printf fprintf vfprintf snprintf vsnprintf sprintf vprintf plus iprintf and *_full variants"
  FORMAT_SCOPE["microcrt"]="default path: integer float(f/e/g) wchar(%lc/%ls) positional long long %n ; full path adds hex-float(%a/%A)"
}

build_microcrt() {
  local build_dir="$OUT_DIR/microcrt"

  if ! run_logged microcrt_config \
    cmake -S "$ROOT_DIR" -B "$build_dir" \
      -DMICROCRT_BUILD_STDIO_LEGACY=ON \
      -DMICROCRT_BUILD_TESTS=ON \
      -DCMAKE_C_COMPILER_LAUNCHER=; then
    BUILD_STATE["microcrt"]="fail"
    VERIFY_STATE["microcrt"]="configure failed"
    BUILD_NOTE["microcrt"]="see $(basename "$LOG_DIR")/microcrt_config.log"
    return
  fi

  if ! run_shell_logged microcrt_build "CCACHE_DISABLE=1 cmake --build '$build_dir'"; then
    BUILD_STATE["microcrt"]="fail"
    VERIFY_STATE["microcrt"]="build failed"
    BUILD_NOTE["microcrt"]="see $(basename "$LOG_DIR")/microcrt_build.log"
    return
  fi

  BUILD_STATE["microcrt"]="ok"
  if run_logged microcrt_test ctest --test-dir "$build_dir" --output-on-failure; then
    VERIFY_STATE["microcrt"]="ctest passed"
    BUILD_NOTE["microcrt"]="build and tests passed"
  else
    VERIFY_STATE["microcrt"]="ctest failed"
    BUILD_NOTE["microcrt"]="build ok, test failed"
  fi

  sum_sizes_from_expr microcrt "find '$build_dir/src/libc/CMakeFiles/microcrt_c.dir' -type f -name '*.o' | rg '/stdio/.*\\.o$' | rg -v '/legacy/' | sort -u"
  SIZE_NOTE["microcrt"]="sum of non-legacy stdio/printf objects in microcrt_c"
}

setup_metadata
build_microcrt
write_single_csv_report microcrt "$CSV_PATH"

printf 'Wrote %s\n' "$CSV_PATH"
