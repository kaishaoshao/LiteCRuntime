#!/usr/bin/env bash

set -u
set -o pipefail

ROOT_DIR=$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)
OUT_DIR=${1:-"$ROOT_DIR/build/printf-compare"}
LOG_DIR="$OUT_DIR/logs"
CSV_PATH="$OUT_DIR/picolibc-normal.csv"

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
  API_SCOPE["picolibc-normal"]="printf fprintf snprintf vsnprintf sprintf vfprintf vprintf plus asprintf dprintf wide and *_s helpers; printf alias families map to __m/__i/__l/__f/__d vfprintf variants"
  FORMAT_SCOPE["picolibc-normal"]="format-default=d full printf/scanf support with float/double, C99 extensions, positional args, and wide APIs; current build keeps wchar APIs, disables %n, and ends with duplicate-name check failure"
}

build_picolibc() {
  local src="$ROOT_DIR/third_party/clib/picolibc"
  local build_dir="$OUT_DIR/picolibc-normal"
  local log_prefix="picolibc_normal"
  local selected_member="libc_stdio_vfprintf.c.o"
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
        -Dc_args=-D_GNU_SOURCE; then
      BUILD_STATE["picolibc-normal"]="fail"
      VERIFY_STATE["picolibc-normal"]="configure failed"
      BUILD_NOTE["picolibc-normal"]="see $(basename "$LOG_DIR")/${log_prefix}_setup.log"
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
        -Dc_args=-D_GNU_SOURCE; then
      BUILD_STATE["picolibc-normal"]="fail"
      VERIFY_STATE["picolibc-normal"]="configure failed"
      BUILD_NOTE["picolibc-normal"]="see $(basename "$LOG_DIR")/${log_prefix}_setup.log"
      return
    fi
  fi

  if run_shell_logged "${log_prefix}_build" "CCACHE_DISABLE=1 ninja -C '$build_dir'"; then
    BUILD_STATE["picolibc-normal"]="ok"
    VERIFY_STATE["picolibc-normal"]="build completed"
    BUILD_NOTE["picolibc-normal"]="build completed"
  else
    if [[ -f "$build_dir/libc.a" ]]; then
      BUILD_STATE["picolibc-normal"]="partial"
      VERIFY_STATE["picolibc-normal"]="libc.a produced, final check failed"
      if [[ -s "$build_dir/libc_duplicates" ]]; then
        BUILD_NOTE["picolibc-normal"]="duplicate names reported in libc.a: $(tr '\n' ' ' <"$build_dir/libc_duplicates" | sed 's/[[:space:]]\+/ /g')"
      else
        BUILD_NOTE["picolibc-normal"]="see $(basename "$LOG_DIR")/${log_prefix}_build.log"
      fi
    else
      BUILD_STATE["picolibc-normal"]="fail"
      VERIFY_STATE["picolibc-normal"]="build failed"
      BUILD_NOTE["picolibc-normal"]="see $(basename "$LOG_DIR")/${log_prefix}_build.log"
      return
    fi
  fi

  if ! run_logged "${log_prefix}_extract" sh -c "ar p '$build_dir/libc.a' '$selected_member' > '$selected_obj'"; then
    BUILD_STATE["picolibc-normal"]="fail"
    VERIFY_STATE["picolibc-normal"]="extract failed"
    BUILD_NOTE["picolibc-normal"]="failed to extract $selected_member from libc.a; see $(basename "$LOG_DIR")/${log_prefix}_extract.log"
    return
  fi

  sum_sizes_from_expr picolibc-normal "printf '%s\n' '$selected_obj'"
  SIZE_NOTE["picolibc-normal"]="size of default printf member ($selected_member)"
}

setup_metadata
build_picolibc
write_single_csv_report picolibc-normal "$CSV_PATH"

printf 'Wrote %s\n' "$CSV_PATH"
