# Getting started

## Host (Make) — core only

Best for OpenWrt and minimal embedded builds:

```bash
make
./bar_line_example
./example_main
```

Three variables let you trim the library for size-constrained targets:

| Variable | Default | Options | Effect |
|---|---|---|---|
| `CHART_TYPES` | `pie bar line area scatter` | any subset (`bar`+`line` also builds combo) | Omitted types are not compiled |
| `EXPORT_FORMATS` | `png jpeg` | `png` or `png jpeg` | Omitting `jpeg` drops ~15 KB |
| `FONT_MODE` | `embedded` | `embedded` \| `embedded-latin` \| `system` | `embedded-latin` adds Latin-1/Latin-Ext glyphs (~22 KB); `system` drops the baked-in font |

**Minimal OpenWrt build** (pie only, PNG only, system font from `/www/assets/`):

```bash
make CXX=mips-openwrt-linux-musl-g++ AR=mips-openwrt-linux-musl-gcc-ar \
     CHART_TYPES="pie" EXPORT_FORMATS="png" FONT_MODE=system \
     CXXFLAGS="-std=c++17 -Os -ffunction-sections -fdata-sections -Wall"
```

When `FONT_MODE=system`, no font is baked in, so supply one on every chart:

```cpp
static auto font = insightdock::charts::load_font_data("/www/assets/fonts/OpenSans-Regular.ttf");
chart.options().font = font;   // shared_ptr — reuse it across charts
```

| Configuration | Code section (unstripped) |
|---|---|
| Full default (all types, embedded font, JPEG) | ~287 KB |
| Minimal (pie + PNG + system font) | ~90 KB |
| Minimal with `-Os -s` strip (estimated) | ~45-55 KB |

## CMake (recommended)

```bash
cmake -B build \
  -DINSIGHTDOCK_BUILD_EXAMPLES=ON \
  -DINSIGHTDOCK_BUILD_TESTS=ON
cmake --build build
ctest --test-dir build
```

| Option | Default | Description |
|--------|---------|-------------|
| `INSIGHTDOCK_BUILD_EXAMPLES` | ON | Build generic examples |
| `INSIGHTDOCK_WITH_PODOFO` | OFF | PDF adapter + PDF examples |
| `INSIGHTDOCK_BUILD_TESTS` | ON | CTest smoke tests |
| `INSIGHTDOCK_WERROR` | OFF | `-Werror` (keep OFF for cross-toolchains) |
| `INSIGHTDOCK_PODOFO_DIR` | `../podofo` | PoDoFo source tree (subproject) or install |

## CMake + PoDoFo (PDF)

PoDoFo requires **CMake 3.23+**. On older systems:

```bash
pip install --user 'cmake>=3.23'
export PATH="$HOME/.local/bin:$PATH"
```

```bash
cmake -B build-podofo \
  -DINSIGHTDOCK_WITH_PODOFO=ON \
  -DINSIGHTDOCK_BUILD_EXAMPLES=ON \
  -DINSIGHTDOCK_PODOFO_DIR=/path/to/podofo
cmake --build build-podofo
export LD_LIBRARY_PATH="$(pwd)/build-podofo/podofo/target:${LD_LIBRARY_PATH}"
./build-podofo/podofo_report_example
```

## Install and consume

```bash
cmake --install build --prefix /opt/insightdock
```

```cmake
find_package(InsightDock CONFIG REQUIRED)
target_link_libraries(myapp PRIVATE InsightDock::Charts)
```

```cpp
#include "insightdock/charts.h"
```

## Platform support

| Platform | Core (PNG/JPEG) | PDF (PoDoFo) |
|----------|-----------------|--------------|
| Linux distros | Yes (CMake + Make) | Yes (optional) |
| OpenWrt / cross | Yes (Make, no GUI deps) | Yes — verified on OpenWrt x86_64/musl, `BUILD_SHARED_LIBS=ON` with PoDoFo packaged separately |
| Windows | CMake + MSVC/MinGW (feasibility target) | Optional |

## Next steps

- [Concepts](concepts.md) — the class hierarchy and render pipeline, if you're extending a chart type
- [Chart types](chart-types/pie.md) — full options for each chart type, with rendered examples
- [Usage guide](guides/usage.md) — error handling, theming, tick formatting
- Hit a build or install error not covered above? See [Troubleshooting](guides/troubleshooting.md)
