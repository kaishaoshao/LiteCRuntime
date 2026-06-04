#!/usr/bin/env bash

set -u
set -o pipefail

ROOT_DIR=$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)
OUT_DIR=${1:-"$ROOT_DIR/build/printf-compare"}
LOG_DIR="$OUT_DIR/logs"
CSV_PATH="$OUT_DIR/newlib-nano.csv"

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
  API_SCOPE["newlib-nano"]="printf fprintf snprintf vsnprintf sprintf vfprintf vprintf plus iprintf asprintf dprintf and wide APIs; nano-vfprintf/nano-printf family selected by specs"
  FORMAT_SCOPE["newlib-nano"]="--enable-newlib-nano-formatted-io builds lc_nano with C89-style non-wide formatted I/O, split-out float support via _printf_float, and no wide-char formatted I/O"
}

build_newlib() {
  local src="$ROOT_DIR/third_party/clib/newlib/newlib"
  local build_dir="$OUT_DIR/newlib-nano"
  local log_prefix="newlib_nano"
  local jobs
  jobs=$(jobs_default)

  mkdir -p "$build_dir"

  if ! run_shell_logged "${log_prefix}_config" "cd '$build_dir' && '$src/configure' --enable-newlib-nano-formatted-io"; then
    BUILD_STATE["newlib-nano"]="fail"
    VERIFY_STATE["newlib-nano"]="configure failed"
    BUILD_NOTE["newlib-nano"]="see $(basename "$LOG_DIR")/${log_prefix}_config.log"
    return
  fi

  if ! run_logged "${log_prefix}_build" make -C "$build_dir" -j"$jobs"; then
    BUILD_STATE["newlib-nano"]="fail"
    VERIFY_STATE["newlib-nano"]="build failed"
    BUILD_NOTE["newlib-nano"]="see $(basename "$LOG_DIR")/${log_prefix}_build.log"
    return
  fi

  BUILD_STATE["newlib-nano"]="ok"
  VERIFY_STATE["newlib-nano"]="build completed"
  BUILD_NOTE["newlib-nano"]="built from newlib/newlib subproject with nano formatted I/O"

  sum_sizes_from_expr newlib-nano "printf '%s\n' '$build_dir/libc.a'"
  SIZE_NOTE["newlib-nano"]="size of libc.a static library"
}

setup_metadata
build_newlib
write_single_csv_report newlib-nano "$CSV_PATH"

printf 'Wrote %s\n' "$CSV_PATH"
