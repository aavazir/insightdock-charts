# Benchmarks and measurement

InsightDock Charts does not publish performance marketing claims without measurements. Use this document to track objective progress against ChartDirector and StockChartX capabilities.

## Metrics

### Chart coverage

Count supported chart families: pie, bar (grouped/stacked), line, area, finance types, etc.

### Export coverage

| Format | Status |
|--------|--------|
| PNG | Supported |
| JPEG | Supported |
| PDF raster | Supported (PoDoFo) |
| PDF vector | Native pie, bar, line, area, combo, scatter/bubble |
| SVG raster embed | Supported |
| SVG native vector | Pie, bar, line, area, combo, scatter/bubble |

### Render quality

- Antialiased fills (`fill_sector`)
- Pie label placement and leader lines
- Legend layout
- Deterministic output (same input → same pixels)
- Visual regression: store reference PNGs in `docs/gallery/`

### Performance

Measure on a fixed machine (document CPU/RAM/compiler):

| Scenario | Data size | Metric |
|----------|-----------|--------|
| Line chart render | 1k, 10k, 100k points | ms, peak RSS |
| Bar chart render | 50 categories × 10 series | ms |
| Pie render | 20 slices | ms |
| PNG encode | 640×400 | ms |
| PDF export | single chart | ms |

Run benchmarks in release mode (`-O2` or `-O3`). Record results in this file when available.

#### Results (2026-06-21)

Machine: Intel Core i7-8650U @ 1.90 GHz (4.2 GHz boost), 8 threads (4 cores
HT), 16 GB RAM, Ubuntu 22.04, GCC 11.4.0, CMake 4.3.2.

```
cmake -B build-bench -DCMAKE_BUILD_TYPE=Release \
  -DINSIGHTDOCK_BUILD_EXAMPLES=OFF -DINSIGHTDOCK_BUILD_TESTS=OFF \
  -DINSIGHTDOCK_BUILD_BENCHMARKS=ON [-DINSIGHTDOCK_WITH_PODOFO=ON]
cmake --build build-bench -j8
./build-bench/insightdock_bench
```

| Scenario | Time | Peak RSS |
|----------|------|----------|
| Line render, 1,000 points | ~13 ms | |
| Line render, 10,000 points | ~105 ms | |
| Line render, 100,000 points | ~1015 ms | |
| Bar render, 50 categories × 10 series | ~3.7 ms | |
| Pie render, 20 slices | ~430 ms | |
| PNG encode, 640×400 | ~22 ms | |
| PDF raster export, single chart (PoDoFo) | ~100 ms | |
| — | — | ~9.2 MB (core), ~15.6 MB (+PoDoFo) |

Notes:
- Line/bar/pie renders scale roughly linearly with point/slice count, as
  expected from `ImageBuffer`'s per-primitive drawing — no quadratic blowups
  observed up to 100k points.
- Pie render is the outlier relative to its low element count (20 slices vs.
  100k line points): per-slice external-label wrapping and leader-line layout
  in `pie_chart.cc` dominates the per-render cost far more than bar/line's
  per-point cost. Not investigated further here — flagging for whoever picks
  up pie label layout next.
- PDF raster export time is on top of the underlying chart render (PoDoFo
  page setup + PNG re-embed), not a replacement for it.

### API simplicity

Target: common chart + export in **5–10 lines** without options.

Track examples per public feature in README and `examples/`.

### Portability

| Target | Build system | Status |
|--------|--------------|--------|
| Ubuntu/Debian | CMake + Make | CI |
| OpenWrt cross | Make | Manual |
| Windows MSVC | CMake | CI configured, static/shared |
| macOS | CMake | CI configured |
| Linux ARM64 | CMake | CI configured |

## Running smoke tests

```bash
cmake -B build -DINSIGHTDOCK_BUILD_TESTS=ON
cmake --build build
ctest --test-dir build --output-on-failure
```

## Future

`benchmarks/bench_render.cc` (std::chrono, no external dependency) covers the
scenarios above. Re-run and update the results table after any change to
`ImageBuffer`, `Font`, or per-chart `draw_series()`/`render()` hot paths, and
once finance/large-data chart types land.
