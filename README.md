# InsightDock Charts for C++

[![CI](https://github.com/aavazir/insightdock-charts/actions/workflows/ci.yml/badge.svg)](https://github.com/aavazir/insightdock-charts/actions/workflows/ci.yml)

> InsightDock is an MIT-licensed C++ charting engine for developers who need high-quality charts in reports and applications, with vector PDF output and an extensible architecture from day one.

Production-oriented C++17 charting for PDF reports, PNG/JPEG export, and embedded devices. Designed for easy extension (new chart types plug into shared bases) and OpenWrt cross-compilation (gcc 10+, zero link-time dependencies in the core).

## Features

- **Chart types:** pie/doughnut, bar (grouped, stacked or 100%-stacked, vertical or horizontal), multi-series line (optional area fill, step lines), area (stacked or 100%), combo (bar + line, dual axis), scatter and bubble, histogram
- **Axes:** linear, logarithmic (Bar/Line/Area/Combo y-axis), and continuous DateTime x-axis positioned by real timestamp (Line/Area)
- **Easy export:** one API — `ChartExporter::save(chart, "output.png")` for PNG, JPEG, SVG, and PDF
- **Output:** RGBA `ImageBuffer`, PNG/JPEG, SVG and PDF as native **vector** output for pie, bar, line, area and combo (selectable text, crisp at any zoom; scatter embeds a raster) or as a raster embed for every chart type
- **Styling:** Chart.js 12-color, Tableau-10 and colorblind-safe palettes; per-series dash, marker, width and opacity; gap handling for missing values; reference lines; multi-line titles
- **Text:** UTF-8 labels; embedded ASCII font (~20 KB) or an optional Latin-1/Latin-Ext font (`INSIGHTDOCK_FONT_LATIN`); bring any TTF via `ChartOptions::font`
- **Core deps:** C++17 stdlib only (stb headers compiled into the library)
- **Optional PDF:** PoDoFo adapter via `insightdock/pdf/podofo.h`

## Quick start

```cpp
#include "insightdock/charts.h"

using namespace insightdock::charts;

PieChart pie;
pie.add_slice("Online", 72);
pie.add_slice("Offline", 28);
ChartExporter::save(pie, "status.png");
```

```cpp
#include "insightdock/charts.h"
#include "insightdock/pdf/podofo.h"

ExportOptions opts;
opts.pdf_backend = PdfBackend::Vector;
ChartExporter::save(pie, "status.pdf", opts);  // requires INSIGHTDOCK_WITH_PODOFO
```

See [examples/](examples/) for bar, line, and PDF samples.

## Gallery

| Pie | Bar (grouped) | Bar (stacked) |
|---|---|---|
| ![pie](docs/gallery/pie_chart.png) | ![bar grouped](docs/gallery/bar_grouped.png) | ![bar stacked](docs/gallery/bar_stacked.png) |

| Line | Area (stacked) |
|---|---|
| ![line](docs/gallery/line_chart.png) | ![area stacked](docs/gallery/area_stacked.png) |

| Combo (bar + line, dual axis) |
|---|
| ![combo](docs/gallery/combo_chart.png) |

More samples, including PDF report pages, in [docs/gallery/](docs/gallery/).

## Project layout

```
├── include/insightdock/       Public headers (charts.h umbrella)
│   └── pdf/                   PoDoFo PDF helpers (podofo.h)
├── src/                       Library implementation
├── examples/                  Generic sample programs
├── tests/                     CTest smoke tests
├── third_party/               stb headers (header-only)
├── docs/                      API, release, gallery, benchmarks
└── cmake/                     Package config for install
```

## Building

### Host (Make) — core only

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
| Minimal with `-Os -s` strip (estimated) | ~45–55 KB |

Cross-compile (full build):

```bash
make CXX=mips-openwrt-linux-musl-g++ AR=mips-openwrt-linux-musl-gcc-ar
```

Packaging this into an OpenWrt feed: [packaging/openwrt/](packaging/openwrt/) has a standard
`package.mk`-based feed `Makefile` sample wired to these same `CHART_TYPES`/`EXPORT_FORMATS`/
`FONT_MODE` knobs.

### CMake (recommended)

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

### CMake + PoDoFo (PDF)

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

### Install and consume

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

A `pkg-config` file (`insightdock.pc`) is also installed to `lib/pkgconfig/`, for Meson or
plain-Makefile consumers:

```bash
export PKG_CONFIG_PATH="/opt/insightdock/lib/pkgconfig:${PKG_CONFIG_PATH}"
g++ -std=c++17 $(pkg-config --cflags --libs insightdock) myapp.cc -o myapp
```

Note: pass `-DCMAKE_INSTALL_PREFIX=/opt/insightdock` at the `cmake -B build` configure step (not
just `cmake --install build --prefix ...`) so the generated `.pc` file's paths match where the
library actually ends up — `configure_file()` bakes `CMAKE_INSTALL_PREFIX` at configure time.

## Platform support

| Platform | Core (PNG/JPEG) | PDF (PoDoFo) |
|----------|-----------------|--------------|
| Linux distros | Yes (CMake + Make) | Yes (optional) |
| OpenWrt / cross | Yes (Make, no GUI deps) | Yes — verified on OpenWrt x86_64/musl, `BUILD_SHARED_LIBS=ON` with PoDoFo packaged separately |
| macOS | CMake (CI) | Optional |
| Windows | CMake + MSVC (CI; static and DLL) | Optional |

## How it compares

InsightDock is for **native C++, headless report output** (PNG, JPEG, SVG, PDF) under an MIT license. It is not an interactive plotting toolkit. For zoom/pan in a desktop app use QCustomPlot, ImPlot or Qt Charts; for scientific plotting use Matplot++ or PLplot; for the widest chart catalogue use ChartDirector.

| | InsightDock | QCustomPlot | Matplot++ | ImPlot |
|---|---|---|---|---|
| License | MIT | GPL-3 / commercial | MIT | MIT |
| Needs | C++17 only | Qt | Gnuplot at runtime | Dear ImGui |
| Interactive | No | Yes | Limited | Yes |
| Vector PDF / SVG | Yes, pie/bar/line/area/combo | PDF | Yes | No |
| Embedded / OpenWrt | Yes | No | No | No |

Full comparison, including limits: [docs/COMPARISON.md](docs/COMPARISON.md).

Contributions for new chart types are welcome — see [CONTRIBUTING.md](CONTRIBUTING.md).

## Measurement criteria

Track progress objectively (see [docs/BENCHMARKS.md](docs/BENCHMARKS.md)):

- Chart and export coverage
- Render quality (labels, legends, deterministic output)
- Performance at 1k / 10k / 100k points
- API simplicity (lines of code for common tasks)
- Portability (OpenWrt, Linux, Windows feasibility)

## Documentation

The MkDocs Material site (`mkdocs.yml` + `docs-site/`, built via the `docs` GitHub Actions
workflow — see [docs/DOCUMENTATION.md](docs/DOCUMENTATION.md)) is the full documentation set:
Getting started, Concepts (architecture, extension points), Guides (usage, PDF reports,
troubleshooting), one page per chart type with every option, Examples, API Reference, Comparison,
and Changelog. The GitHub-readable sources for that site live under `docs/` and `docs-site/`:

- [examples/README.md](examples/README.md) — run generic examples
- [docs/DOCUMENTATION.md](docs/DOCUMENTATION.md) — doc stack; how to build the MkDocs Material site with the MrDocs API reference embedded
- [docs/API.md](docs/API.md) — usage guide (error handling, theming, tick formatting, extending); published as the site's Usage guide page
- [docs-site/concepts.md](docs-site/concepts.md) — class hierarchy, render pipeline, how to add a new chart type
- [docs-site/chart-types/](docs-site/chart-types/) — per-chart-type option reference (pie, bar, line, area, combo, scatter/bubble)
- [docs-site/guides/pdf-reports.md](docs-site/guides/pdf-reports.md) — `PdfReportBuilder` walkthrough
- [docs-site/guides/troubleshooting.md](docs-site/guides/troubleshooting.md) — real incidents: ABI/install hazards, clangd setup, cross-compile issues
- [docs/RELEASE.md](docs/RELEASE.md) — versioning and release checklist
- [docs/gallery/](docs/gallery/) — generated sample outputs
- [docs/COMPARISON.md](docs/COMPARISON.md) — proprietary and open-source comparison (website source)

## License

MIT — see [LICENSE](LICENSE) and [NOTICE](NOTICE).
