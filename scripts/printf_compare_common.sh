#!/usr/bin/env bash

set -u
set -o pipefail

ROOT_DIR=$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)
ZCC_BIN=${ZCC_BIN:-/home/shaokai/Terapines/ZCC/4.1.9/bin/zcc}

jobs_default() {
  getconf _NPROCESSORS_ONLN 2>/dev/null || echo 4
}

run_logged() {
  local name="$1"
  shift
  local log_path="$LOG_DIR/$name.log"
  local status

  printf '$ %s\n' "$*" >"$log_path"
  "$@" >>"$log_path" 2>&1
  status=$?
  return "$status"
}

run_shell_logged() {
  local name="$1"
  local script="$2"
  local log_path="$LOG_DIR/$name.log"
  local status

  printf '$ %s\n' "$script" >"$log_path"
  bash -lc "$script" >>"$log_path" 2>&1
  status=$?
  return "$status"
}

sum_sizes_from_expr() {
  local impl="$1"
  local expr="$2"
  local list_path="$OUT_DIR/$impl.size-files.txt"
  local size_path="$OUT_DIR/$impl.size.txt"

  bash -lc "$expr" >"$list_path"

  local count
  count=$(wc -l <"$list_path")
  if [[ "$count" -eq 0 ]]; then
    SIZE_FILES["$impl"]=0
    SIZE_TEXT["$impl"]=0
    SIZE_DATA["$impl"]=0
    SIZE_BSS["$impl"]=0
    SIZE_TOTAL["$impl"]=0
    return 1
  fi

  xargs -r size <"$list_path" >"$size_path"

  read -r text data bss total <<EOF_SUM
$(awk 'NR > 1 { text += $1; data += $2; bss += $3 } END { printf "%d %d %d %d\n", text, data, bss, text + data + bss }' "$size_path")
EOF_SUM

  SIZE_FILES["$impl"]=$count
  SIZE_TEXT["$impl"]=$text
  SIZE_DATA["$impl"]=$data
  SIZE_BSS["$impl"]=$bss
  SIZE_TOTAL["$impl"]=$total
  return 0
}

csv_escape() {
  local value="${1:-}"
  value=${value//\"/\"\"}
  printf '"%s"' "$value"
}

write_single_csv_report() {
  local impl="$1"
  local csv_path="$2"
  local generated_at
  generated_at=$(date '+%Y-%m-%d %H:%M:%S %z')

  {
    printf '%s\n' 'generated_at,impl,build_state,verification,build_note,public_apis,format_scope,objects_counted,text,data,bss,total,size_note'

    printf '%s,' "$(csv_escape "$generated_at")"
    printf '%s,' "$(csv_escape "$impl")"
    printf '%s,' "$(csv_escape "${BUILD_STATE[$impl]:-n/a}")"
    printf '%s,' "$(csv_escape "${VERIFY_STATE[$impl]:-n/a}")"
    printf '%s,' "$(csv_escape "${BUILD_NOTE[$impl]:-}")"
    printf '%s,' "$(csv_escape "${API_SCOPE[$impl]:-}")"
    printf '%s,' "$(csv_escape "${FORMAT_SCOPE[$impl]:-}")"
    printf '%s,' "$(csv_escape "${SIZE_FILES[$impl]:-0}")"
    printf '%s,' "$(csv_escape "${SIZE_TEXT[$impl]:-0}")"
    printf '%s,' "$(csv_escape "${SIZE_DATA[$impl]:-0}")"
    printf '%s,' "$(csv_escape "${SIZE_BSS[$impl]:-0}")"
    printf '%s,' "$(csv_escape "${SIZE_TOTAL[$impl]:-0}")"
    printf '%s\n' "$(csv_escape "${SIZE_NOTE[$impl]:-}")"
  } >"$csv_path"
}
