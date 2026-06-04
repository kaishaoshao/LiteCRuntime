#!/usr/bin/env bash

set -u
set -o pipefail

ROOT_DIR=$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)
OUT_DIR=${1:-"$ROOT_DIR/build/printf-compare"}
LOG_DIR="$OUT_DIR/logs"
CSV_PATH="$OUT_DIR/emrun.csv"

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
  API_SCOPE["emrun"]="printf/fprintf/vprintf/vfprintf/sprintf/snprintf plus asprintf/vasprintf and wide swprintf/vswprintf; internal __SEGGER_RTL_vfprintf_* specializations"
  FORMAT_SCOPE["emrun"]="multiple vfprintf variants: int, long, long long, float, short-float, wchar and nwp forms; narrow and wide paths support integers, strings, pointers, width/precision/star/flags, long/long long, float, hex-float, locale variants, asprintf; no positional args, no %n"
}

build_emrun() {
  local src="$ROOT_DIR/third_party/clib/emrun/emRun"
  local build_dir="$OUT_DIR/emrun"
  local obj_dir="$build_dir/obj"
  local lib_path="$build_dir/libemrun.a"
  local build_log="$LOG_DIR/emrun_build.log"
  local cc="$ZCC_BIN"
  local cflags=(
    -Os
    -D__SEGGER_RTL_VA_LIST=__builtin_va_list
    -D__SEGGER_RTL_TYPESET=64
    -I"$ROOT_DIR/third_party/clib/emrun/include"
  )
  local sources=(
    codesets.c
    config.c
    convops.c
    errno.c
    execops.c
    fileops.c
    fenvops.c
    floatops.c
    locales.c
    mbops.c
    prinops.c
    utilops.c
    wprinops.c
  )
  local failed=""

  mkdir -p "$obj_dir"
  {
    printf '$ %s\n' "$cc ${cflags[*]} -c <emrun source files>"
    printf 'emRun printf slice source list:\n'
    printf '  %s\n' "${sources[@]}"
  } >"$build_log"

  for src_file in "${sources[@]}"; do
    local obj_file="$obj_dir/${src_file%.c}.o"
    if ! run_logged "emrun_$(basename "${src_file%.c}")" \
      "$cc" "${cflags[@]}" -c "$src/$src_file" -o "$obj_file"; then
      failed="$src_file"
      printf 'FAILED: %s\n' "$src_file" >>"$build_log"
      break
    fi
    printf 'OK: %s\n' "$src_file" >>"$build_log"
  done

  if [[ -n "$failed" ]]; then
    BUILD_STATE["emrun"]="fail"
    VERIFY_STATE["emrun"]="build failed"
    BUILD_NOTE["emrun"]="failed while compiling $failed with zcc; see $(basename "$LOG_DIR")/emrun_build.log and emrun_*.log"
    return
  fi

  BUILD_STATE["emrun"]="ok"
  VERIFY_STATE["emrun"]="zcc-compiled printf slice"
  BUILD_NOTE["emrun"]="compiled emRun printf-related objects with zcc and archived them into libemrun.a"

  if ! run_logged emrun_archive ar rcs "$lib_path" "$obj_dir"/*.o; then
    BUILD_STATE["emrun"]="fail"
    VERIFY_STATE["emrun"]="archive failed"
    BUILD_NOTE["emrun"]="built objects successfully, but archiving into libemrun.a failed; see $(basename "$LOG_DIR")/emrun_archive.log"
    return
  fi

  sum_sizes_from_expr emrun "printf '%s\n' '$lib_path'"
  SIZE_NOTE["emrun"]="zcc-compiled emRun static library"
}

setup_metadata
build_emrun
write_single_csv_report emrun "$CSV_PATH"

printf 'Wrote %s\n' "$CSV_PATH"
