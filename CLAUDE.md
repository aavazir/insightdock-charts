# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Build commands

### Make (core only — OpenWrt / minimal)
```bash
make                          # build library + generic examples
make test                     # build + run example_main, check chart_output.png exists
make clean
```

Three variables control what gets compiled into the library:

| Variable | Default | Purpose |
|---|---|---|
| `CHART_TYPES` | `pie bar line area scatter` | Space-separated subset; omit types to shrink the archive (`combo` is built whenever both `bar` and `line` are) |
| `EXPORT_FORMATS` | `png jpeg` | Omit `jpeg` to drop ~15 KB of JPEG codec |
| `FONT_MODE` | `embedded` | `embedded-latin` bakes in a Latin-1/Ext-A font; `system` drops the baked-in TTF, so set `ChartOptions::font` on every chart |

```bash
# Minimal OpenWrt build — pie only, PNG only, font loaded from /www/assets/ at runtime
make CHART_TYPES="pie" EXPORT_FORMATS="png" FONT_MODE=system \
     CXXFLAGS="-std=c++17 -Os -ffunction-sections -fdata-sections -Wall"

# Cross-compile minimal build
make CXX=mips-openwrt-linux-g++ AR=mips-openwrt-linux-gcc-ar \
     CHART_TYPES="pie" EXPORT_FORMATS="png" FONT_MODE=system
```

### CMake (recommended)
```bash
cmake -B build -DINSIGHTDOCK_BUILD_EXAMPLES=ON -DINSIGHTDOCK_BUILD_TESTS=ON
cmake --build build
ctest --test-dir build                                   # run all tests
./build/tests/insightdock_smoke_tests /tmp/out.png       # run single test directly
```

### CMake with PoDoFo PDF support
```bash
cmake -B build-podofo \
  -DINSIGHTDOCK_WITH_PODOFO=ON \
  -DINSIGHTDOCK_BUILD_EXAMPLES=ON \
  -DINSIGHTDOCK_PODOFO_DIR=../podofo
cmake --build build-podofo
export LD_LIBRARY_PATH="$(pwd)/build-podofo/target:${LD_LIBRARY_PATH}"
./build-podofo/podofo_report_example
```
PoDoFo requires CMake 3.23+ (`pip install --user 'cmake>=3.23'` if needed).

### Installing for out-of-tree consumers (`cmake --install`)

**Always run `cmake --build <dir>` immediately before `cmake --install <dir>`.** `install(DIRECTORY include/insightdock ...)` copies headers straight from the live source tree regardless of whether the library binary in `<dir>` is current — if you `cmake --install` a build directory that wasn't just rebuilt, you silently get fresh headers paired with a stale `.a`/`.so`. If a public struct passed by value (e.g. `ExportOptions`) changed size between the two, every call through the mismatched ABI boundary reads garbage from the wrong registers/stack slots — no compiler warning, no link error, just intermittent heap corruption at runtime (see `CHANGELOG.md` [Unreleased] "Diagnosed intermittent `bad_alloc`/`length_error`..."). Re-running just the install step after a `git pull`, or testing against a long-lived build dir you forgot to rebuild, are the two ways this bites.

### Formatting
```bash
clang-format -i src/*.cc include/insightdock/*.h
```
Style: LLVM base, 2-space indent, 100-column limit (see `.clang-format`).

### Documentation site (MkDocs Material + MrDocs API reference)
```bash
python3 -m venv .venv-docs && .venv-docs/bin/pip install -r requirements-docs.txt
cd docs && mrdocs mrdocs.yml && cd ..      # generate the API reference first
./scripts/build_docs_site.sh               # assemble docs-site/ + mkdocs build -> site/
.venv-docs/bin/mkdocs serve                 # live preview at 127.0.0.1:8000
```
`docs-site/` is the MkDocs markdown source; `mkdocs.yml` lives at the repo root. Built and published to GitHub Pages via `.github/workflows/docs.yml` — see `docs/DOCUMENTATION.md`.

## Architecture

### Class hierarchy
```
Chart (abstract)
├── PieChart          — pie slices, Tableau-10/Chart.js palette, raster + vector PDF + native vector SVG (own pipeline)
├── ScatterChart      — scatter/bubble; continuous numeric x/y Scales, not CartesianChart's category axis
└── CartesianChart (abstract) — shared axes, grid, legend, title rendering
    ├── BarChart      — grouped or stacked vertical bars
    ├── LineChart     — multi-series lines with optional area fill and point markers
    ├── AreaChart     — filled-area polygons, optional stacking
    └── ComboChart    — grouped bars + line overlay; each series opts into the shared left axis or an independent right axis (sessions pattern)
```

`Chart::render()` returns an `ImageBuffer` (RGBA, row-major). `ChartExporter` is the only output path: `save()`, `encode()`, `render()`, and `draw_pdf()` (PoDoFo only).

### Key types
- **`ImageBuffer`** (`include/insightdock/image_buffer.h`, `src/image_buffer.cc`) — RGBA raster buffer with drawing primitives (lines, polylines, polygons, sectors, circles, `fill_rect`). All chart rendering writes here. `downsample()` implements supersampling AA.
- **`ChartExporter`** (`include/insightdock/export.h`, `src/export.cc`) — static-only export API. `format_from_path()` infers format from extension. PDF methods are guarded by `#ifdef INSIGHTDOCK_HAS_PODOFO`. `ExportOptions::svg_backend = SvgBackend::Vector` dispatches to `src/svg_pie.cc` for `PieChart` and to `SvgTarget` (`src/svg_target.cc`) for Cartesian charts (native shapes + `<text>`); scatter raster-falls back, same `Raster`/`Vector` shape as `PdfBackend`.
- **`DrawTarget`** (`include/insightdock/draw_target.h`) — pixel-space drawing interface Cartesian charts render into via `Chart::draw(DrawTarget&)`: `RasterTarget` (wraps `ImageBuffer`, byte-identical PNGs), `SvgTarget` (`detail/svg_target.h`), `PdfTarget` (`pdf/podofo_target.h`). Text is always *measured* with the embedded `Font` so layout is the same on every target.
- **`CartesianChart`** (`include/insightdock/cartesian_chart.h`, `src/cartesian_chart.cc`) — handles background, title, axes, grid, and legend; calls `draw_series(PlotContext&)` for the concrete geometry (`PlotContext::target` is a `DrawTarget&`).
- **`Scale`** (`include/insightdock/scale.h`, `src/scale.cc`) — linear mapping between data values and pixel coordinates; used by `CartesianChart` for both axes.
- **`Font`** (`include/insightdock/font.h`, `src/font.cc`) — embedded ASCII-subset TTF (~20 KB) compiled in via `include/insightdock/embedded_font.h`; text is decoded as UTF-8 (missing glyphs draw as `?`); inject a full TTF with `ChartOptions::font = load_font_data(path)`; `INSIGHTDOCK_FONT_LATIN` / `FONT_MODE=embedded-latin` swaps in `embedded_font_latin.h`.
- **`Palette`** (`include/insightdock/palette.h`, `src/palette.cc`) — `PaletteKind::ChartJs12` (bar/line default) and `PaletteKind::Tableau10` (pie default).

### PDF layer (optional)
`src/podofo_adapter.cc` — raster embed: renders chart to PNG bytes and injects into a PoDoFo page.  
`src/podofo_vector.cc` — vector embed: `PieChart` uses its own pipeline (`draw_pie_impl`); Cartesian charts draw through `PdfTarget` (`src/podofo_target.cc`), a `DrawTarget` that emits native PDF paths/text; scatter falls back to raster.  
Both compiled only when `INSIGHTDOCK_WITH_PODOFO=ON`; guarded by `INSIGHTDOCK_HAS_PODOFO` macro.

### Third-party
`third_party/` — stb headers (`stb_image.h`, `stb_image_write.h`, `stb_truetype.h`) compiled into the library. Zero link-time external dependencies in the core.

### Directory conventions
- `include/insightdock/` — public headers; umbrella is `charts.h`
- `include/insightdock/pdf/` — PDF headers (podofo.h, podofo_adapter.h, podofo_vector.h)
- `src/` — implementation; PoDoFo sources compiled conditionally
- `tests/` — single CTest executable (`insightdock_smoke_tests`)
- `cmake/` — `InsightDockConfig.cmake.in` for `find_package(InsightDock)` downstream
- `docs-site/` — MkDocs Material source (`mkdocs.yml` at repo root); `api/` and `img/gallery/` inside it are generated by `scripts/build_docs_site.sh`, not hand-maintained — see `docs/DOCUMENTATION.md`

### Namespace
All public API is under `insightdock::charts`. PDF helpers are under `insightdock::charts::podofo`.

## Adding a new chart type

1. Derive from `Chart` (freeform) or `CartesianChart` (axis-based).
2. Implement `render()` (freeform) or `draw_series(PlotContext&)` (Cartesian).
3. Add the `.cc` file to `INSIGHTDOCK_SOURCES` in `CMakeLists.txt` and to `SRCS` in `Makefile`.
4. Include the new header in `include/insightdock/charts.h`.
5. Add an entry to `CHANGELOG.md` and update `ROADMAP.md`.
