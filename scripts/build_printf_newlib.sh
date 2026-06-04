#!/usr/bin/env bash

set -u
set -o pipefail

ROOT_DIR=$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)
OUT_DIR=${1:-"$ROOT_DIR/build/printf-compare"}
LOG_DIR="$OUT_DIR/logs"
CSV_PATH="$OUT_DIR/newlib-normal.csv"

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
  API_SCOPE["newlib-normal"]="printf fprintf snprintf vsnprintf sprintf vfprintf vprintf plus iprintf asprintf dprintf and wide APIs; nano-formatted variants are excluded"
  FORMAT_SCOPE["newlib-normal"]="default newlib.h build with normal formatted I/O; float and wide APIs enabled; _WANT_IO_C99_FORMATS/_WANT_IO_LONG_LONG/_WANT_IO_POS_ARGS are unset in this build"
}

build_newlib() {
  local src="$ROOT_DIR/third_party/clib/newlib/newlib"
  local build_dir="$OUT_DIR/newlib-normal"
  local log_prefix="newlib_normal"
  local jobs
  jobs=$(jobs_default)

  mkdir -p "$build_dir"

  if ! run_shell_logged "${log_prefix}_config" "cd '$build_dir' && '$src/configure' --disable-newlib-nano-formatted-io"; then
    BUILD_STATE["newlib-normal"]="fail"
    VERIFY_STATE["newlib-normal"]="configure failed"
    BUILD_NOTE["newlib-normal"]="see $(basename "$LOG_DIR")/${log_prefix}_config.log"
    return
  fi

  if ! run_logged "${log_prefix}_build" make -C "$build_dir" -j"$jobs"; then
    BUILD_STATE["newlib-normal"]="fail"
    VERIFY_STATE["newlib-normal"]="build failed"
    BUILD_NOTE["newlib-normal"]="see $(basename "$LOG_DIR")/${log_prefix}_build.log"
    return
  fi

  BUILD_STATE["newlib-normal"]="ok"
  VERIFY_STATE["newlib-normal"]="build completed"
  BUILD_NOTE["newlib-normal"]="built from newlib/newlib subproject"

  sum_sizes_from_expr newlib-normal "printf '%s\n' '$build_dir/libc.a'"
  SIZE_NOTE["newlib-normal"]="size of libc.a static library"
}

setup_metadata
build_newlib
write_single_csv_report newlib-normal "$CSV_PATH"

printf 'Wrote %s\n' "$CSV_PATH"
