#!/usr/bin/env bash

set -u
set -o pipefail

ROOT_DIR=$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)
OUT_DIR=${1:-"$ROOT_DIR/build/printf-compare"}
LOG_DIR="$OUT_DIR/logs"
CSV_PATH="$OUT_DIR/picolibc-nano.csv"

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
  API_SCOPE["picolibc-nano"]="printf fprintf snprintf vsnprintf sprintf vfprintf vprintf plus asprintf dprintf wide and *_s helpers; aliases point to the minimal variant"
  FORMAT_SCOPE["picolibc-nano"]="format-default=m minimal printf/scanf: no width/precision/alternate/sign presentation in output, no positional args or %b; current build keeps wchar APIs and disables %n"
}

build_picolibc() {
  local src="$ROOT_DIR/third_party/clib/picolibc"
  local build_dir="$OUT_DIR/picolibc-nano"
  local log_prefix="picolibc_nano"
  local selected_member="libc_stdio_vfmprintf.c.o"
  local selected_obj="$build_dir/default-vfprintf.o"

  if [[ -d "$build_dir" && -f "$build_dir/build.ninja" ]]; then
    if ! run_logged "${log_prefix}_setup" \
      meson setup --reconfigure "$build_dir" "$src" \
        -Dtests=false \
        -Dmultilib=false \
        -Dsemihost=false \
        -Dpicocrt=false \
        -Dsysroot-install=false \
        -Dspecsdir=none \
        -Dformat-default=m \
        -Dc_args=-D_GNU_SOURCE; then
      BUILD_STATE["picolibc-nano"]="fail"
      VERIFY_STATE["picolibc-nano"]="configure failed"
      BUILD_NOTE["picolibc-nano"]="see $(basename "$LOG_DIR")/${log_prefix}_setup.log"
      return
    fi
  else
    if ! run_logged "${log_prefix}_setup" \
      meson setup "$build_dir" "$src" \
        -Dtests=false \
        -Dmultilib=false \
        -Dsemihost=false \
        -Dpicocrt=false \
        -Dsysroot-install=false \
        -Dspecsdir=none \
        -Dformat-default=m \
        -Dc_args=-D_GNU_SOURCE; then
      BUILD_STATE["picolibc-nano"]="fail"
      VERIFY_STATE["picolibc-nano"]="configure failed"
      BUILD_NOTE["picolibc-nano"]="see $(basename "$LOG_DIR")/${log_prefix}_setup.log"
      return
    fi
  fi

  if run_shell_logged "${log_prefix}_build" "CCACHE_DISABLE=1 ninja -C '$build_dir'"; then
    BUILD_STATE["picolibc-nano"]="ok"
    VERIFY_STATE["picolibc-nano"]="build completed"
    BUILD_NOTE["picolibc-nano"]="build completed"
  else
    if [[ -f "$build_dir/libc.a" ]]; then
      BUILD_STATE["picolibc-nano"]="partial"
      VERIFY_STATE["picolibc-nano"]="libc.a produced, final check failed"
      if [[ -s "$build_dir/libc_duplicates" ]]; then
        BUILD_NOTE["picolibc-nano"]="duplicate names reported in libc.a: $(tr '\n' ' ' <"$build_dir/libc_duplicates" | sed 's/[[:space:]]\+/ /g')"
      else
        BUILD_NOTE["picolibc-nano"]="see $(basename "$LOG_DIR")/${log_prefix}_build.log"
      fi
    else
      BUILD_STATE["picolibc-nano"]="fail"
      VERIFY_STATE["picolibc-nano"]="build failed"
      BUILD_NOTE["picolibc-nano"]="see $(basename "$LOG_DIR")/${log_prefix}_build.log"
      return
    fi
  fi

  if ! run_logged "${log_prefix}_extract" sh -c "ar p '$build_dir/libc.a' '$selected_member' > '$selected_obj'"; then
    BUILD_STATE["picolibc-nano"]="fail"
    VERIFY_STATE["picolibc-nano"]="extract failed"
    BUILD_NOTE["picolibc-nano"]="failed to extract $selected_member from libc.a; see $(basename "$LOG_DIR")/${log_prefix}_extract.log"
    return
  fi

  sum_sizes_from_expr picolibc-nano "printf '%s\n' '$selected_obj'"
  SIZE_NOTE["picolibc-nano"]="size of default printf member ($selected_member)"
}

setup_metadata
build_picolibc
write_single_csv_report picolibc-nano "$CSV_PATH"

printf 'Wrote %s\n' "$CSV_PATH"
