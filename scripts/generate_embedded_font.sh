#!/bin/sh
# Regenerate include/insightdock/embedded_font.h from a system TTF (ASCII subset).
set -e
cd "$(dirname "$0")/.."
FONT="${1:-/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf}"
python3 - "$FONT" << 'PY'
import sys, os
font_path = sys.argv[1] if len(sys.argv) > 1 else "/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf"
with open(font_path, "rb") as f:
    data = f.read()
try:
    from fontTools.subset import Subsetter, Options
    from fontTools.ttLib import TTFont
    import io
    font = TTFont(io.BytesIO(data))
    opts = Options()
    subsetter = Subsetter(options=opts)
    subsetter.populate(text="ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789 .,:-/%<>()&+")
    subsetter.subset(font)
    out = io.BytesIO()
    font.save(out)
    data = out.getvalue()
except Exception as e:
    print("Warning: subset failed:", e, file=sys.stderr)
out_path = "include/insightdock/embedded_font.h"
with open(out_path, "w") as h:
    h.write("#ifndef INSIGHTDOCK_CHARTS_EMBEDDED_FONT_H\n#define INSIGHTDOCK_CHARTS_EMBEDDED_FONT_H\n\n")
    h.write("#include <cstddef>\n\nnamespace insightdock::charts {\nnamespace embedded_font {\n\n")
    h.write("static unsigned char const data[] = {\n")
    for i in range(0, len(data), 16):
        chunk = data[i:i+16]
        h.write("  " + ", ".join(f"0x{b:02x}" for b in chunk) + ",\n")
    h.write("};\n\n")
    h.write(f"static std::size_t const size = {len(data)};\n\n")
    h.write("} // namespace embedded_font\n} // namespace insightdock::charts\n\n#endif\n")
print(f"Wrote {out_path} ({len(data)} bytes)")
PY
