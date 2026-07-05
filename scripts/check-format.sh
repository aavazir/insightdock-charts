#!/usr/bin/env bash
# Match CI lint job (clang-format 18).
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
cd "$ROOT"

FMT=""
for candidate in clang-format-18 clang-format; do
  if command -v "$candidate" >/dev/null 2>&1; then
    FMT="$candidate"
    break
  fi
done

if [[ -z "$FMT" ]]; then
  echo "error: clang-format not found (install clang-format-18 or clang-format)" >&2
  exit 1
fi

echo "Using $FMT ($($FMT --version | head -1))"
if [[ "$($FMT --version)" != *"version 18"* ]]; then
  echo "warning: CI uses clang-format 18; use --fix with clang-format-18 or the Docker one-liner in CONTRIBUTING.md" >&2
fi

mapfile -d '' FILES < <(
  find src include tests -type f \( -name '*.cc' -o -name '*.h' \) -print0
)

if [[ ${#FILES[@]} -eq 0 ]]; then
  echo "No C++ sources under src/, include/, or tests/"
  exit 0
fi

if [[ "${1:-}" == "--fix" ]]; then
  xargs -0 "$FMT" -i < <(printf '%s\0' "${FILES[@]}")
  echo "Formatted ${#FILES[@]} file(s)"
  exit 0
fi

xargs -0 "$FMT" --dry-run --Werror < <(printf '%s\0' "${FILES[@]}")
echo "clang-format OK (${#FILES[@]} files)"
