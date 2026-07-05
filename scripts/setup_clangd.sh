#!/usr/bin/env bash
# Generate compile_commands.json for clangd / IntelliSense.
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
cd "$ROOT"

PODOFO_DIR="${INSIGHTDOCK_PODOFO_DIR:-../podofo}"

echo "==> Core build (compile_commands.json)"
cmake -B build \
  -DCMAKE_EXPORT_COMPILE_COMMANDS=ON \
  -DINSIGHTDOCK_BUILD_EXAMPLES=ON \
  -DINSIGHTDOCK_BUILD_TESTS=ON

MERGED="$ROOT/build/compile_commands.json"
cp "$MERGED" "${MERGED}.core"

if [[ -f "$PODOFO_DIR/CMakeLists.txt" ]]; then
  echo "==> PoDoFo build (merge into compile_commands.json)"
  cmake -B build-podofo \
    -DCMAKE_EXPORT_COMPILE_COMMANDS=ON \
    -DINSIGHTDOCK_WITH_PODOFO=ON \
    -DINSIGHTDOCK_PODOFO_DIR="$PODOFO_DIR"

  python3 << 'PY'
import json
from pathlib import Path

root = Path(".")
core = json.loads((root / "build/compile_commands.json").read_text())
podofo = json.loads((root / "build-podofo/compile_commands.json").read_text())
seen = {e["file"] for e in core}
merged = list(core)
for e in podofo:
    if e["file"] not in seen:
        merged.append(e)
        seen.add(e["file"])
(root / "build/compile_commands.json").write_text(json.dumps(merged, indent=2))
print(f"Merged {len(merged)} compile commands (core + PoDoFo)")
PY
else
  echo "==> PoDoFo not found at $PODOFO_DIR — skipping PoDoFo merge"
  echo "    (PoDoFo examples will show clang errors until build-podofo is configured)"
fi

# Optional root symlink (gitignored) — some tools look here first
ln -sf build/compile_commands.json compile_commands.json 2>/dev/null || true

echo "==> Done. Restart clangd / reload the IDE window."
echo "    compile_commands: build/compile_commands.json"
