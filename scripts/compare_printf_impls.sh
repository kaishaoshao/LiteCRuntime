#!/usr/bin/env bash

set -u
set -o pipefail

ROOT_DIR=$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)
OUT_DIR=${1:-"$ROOT_DIR/build/printf-compare"}
REPORT_PATH="$OUT_DIR/printf-impl-report.csv"

mkdir -p "$OUT_DIR"

for script in \
  build_printf_emrun.sh \
  build_printf_microcrt.sh \
  build_printf_picolibc.sh \
  build_printf_picolibc_nano.sh \
  build_printf_newlib.sh \
  build_printf_newlib_nano.sh
do
  "$ROOT_DIR/scripts/$script" "$OUT_DIR"
done

{
  head -n 1 "$OUT_DIR/emrun.csv"
  tail -n +2 "$OUT_DIR/emrun.csv"
  tail -n +2 "$OUT_DIR/microcrt.csv"
  tail -n +2 "$OUT_DIR/picolibc-normal.csv"
  tail -n +2 "$OUT_DIR/picolibc-nano.csv"
  tail -n +2 "$OUT_DIR/newlib-normal.csv"
  tail -n +2 "$OUT_DIR/newlib-nano.csv"
} >"$REPORT_PATH"

printf 'Report written to %s\n' "$REPORT_PATH"
