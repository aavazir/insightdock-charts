# Public API

InsightDock Charts for C++ exposes a small, easy-to-use surface. Include one header for most tasks:

```cpp
#include "insightdock/charts.h"
```

For PDF/PoDoFo integration (when built with `INSIGHTDOCK_WITH_PODOFO`):

```cpp
#include "insightdock/pdf/podofo.h"
```

This page is the general usage guide (also published as the doc site's "Usage guide" page). For
full per-option coverage of a specific chart type, see `docs-site/chart-types/`; for multi-page
PDF reports via `PdfReportBuilder`, see `docs-site/guides/pdf-reports.md`.

## Documentation approach (modern OSS)

Header comments are Doxygen-style `///`, `@brief`, `@param`, `@return` — portable across
IntelliSense/clangd, [MrDocs](https://mrdocs.com) (the primary generator, Clang-based), hdoc, and
Doxygen. If your IDE shows false-positive red squiggles (`file not found`, missing `std::`), run
`./scripts/setup_clangd.sh` and reload the editor window. Full doc-tooling strategy, MrDocs/Doxygen
generation commands, and the MkDocs site build: [DOCUMENTATION.md](https://github.com/aavazir/insightdock-charts/blob/main/docs/DOCUMENTATION.md).

## Documented headers

| Header | Contents |
|--------|----------|
| `insightdock/charts.h` | Umbrella include (pulls in everything below) |
| `insightdock/chart.h` | `Chart`, `ChartOptions`, `DataSeries`, `Chart::apply_theme` |
| `insightdock/error.h` | `ChartError` enum, `ChartResult<T>` result type |
| `insightdock/theme.h` | `ChartTheme` with `light()` / `dark()` presets |
| `insightdock/pie_chart.h` | `PieChart`, `PieOptions`, `PieSlice` |
| `insightdock/bar_chart.h` | `BarChart`, `BarMode` |
| `insightdock/line_chart.h` | `LineChart`, `LineChartOptions` |
| `insightdock/area_chart.h` | `AreaChart`, `AreaChartOptions` (area and stacked-area) |
| `insightdock/scatter_chart.h` | `ScatterChart`, `ScatterOptions`, `ScatterPoint`, `ScatterSeries` (scatter and bubble) |
| `insightdock/cartesian_chart.h` | `CartesianChart`, `CartesianOptions`, `TickFormat`, `PlotContext` |
| `insightdock/export.h` | `ChartExporter`, `ExportOptions`, `PdfBackend`, `SvgBackend` |
| `insightdock/scale.h` | `Scale`, `ScaleType` |
| `insightdock/decimate.h` | `lttb_decimate`, `lttb_decimate_indexed` — LTTB downsampling for large datasets |
| `insightdock/image_buffer.h` | `ImageBuffer` — RGBA raster buffer and draw primitives |
| `insightdock/color.h`, `palette.h`, `font.h` | Visual primitives |
| `insightdock/png_writer.h` | Low-level PNG/JPEG encode |
| `insightdock/svg_writer.h` | SVG encode — raster embed (PNG base64-inlined); native vector path for pie is `ExportOptions::svg_backend = SvgBackend::Vector` |
| `insightdock/pdf/podofo.h` | PDF umbrella (PoDoFo build) |
| `insightdock/pdf/report.h` | `PdfReportBuilder` — tables, sections, footers (PoDoFo build) |

## Core types

| Type | Purpose |
|------|---------|
| `PieChart`, `BarChart`, `LineChart`, `AreaChart`, `ComboChart`, `ScatterChart` | Chart implementations |
| `ChartOptions`, `PieOptions`, `ScatterOptions`, … | Styling and layout per chart type |
| `ChartTheme` | Reusable visual style — apply across multiple charts |
| `ChartResult<T>` | Structured result from `save()` / `encode()` — holds value or `ChartError` |
| `ChartError` | Enum of failure reasons: `BadPath`, `RenderFailure`, `EncodeFailure`, `PdfUnsupported`, `InvalidInput` |
| `TickFormat` | Y-axis label style: `Raw`, `SI`, `Currency`, `Percent`, `DateTime` |
| `ScaleType` | Axis scale mode: `Linear`, `Log`, `DateTime` |
| `ChartExporter` | Render and save PNG / JPEG / SVG / PDF |
| `SvgBackend` | `Raster` (default, all charts) or `Vector` (native paths for pie; others fall back to raster) |
| `lttb_decimate` / `lttb_decimate_indexed` | LTTB downsampling for large `LineChart`/`AreaChart` datasets |
| `PdfReportBuilder` | Multi-page PDF report layout (PoDoFo) |
| `ImageBuffer` | RGBA raster buffer |
| `Color`, `Palette`, `Font` | Visual primitives |

Namespace: `insightdock::charts`

PDF helpers: `insightdock::charts::podofo` (via `insightdock/pdf/podofo.h`)

## Design principles

1. **Defaults work** — charts should look good without manual tuning.
2. **One obvious export path** — `ChartExporter::save()`.
3. **Advanced options are optional** — grouped in `*Options` structs.
4. **Headless/server-safe** — no GUI toolkit in the core.

## Compatibility policy

- **v0.x:** API may evolve; breaking changes documented in CHANGELOG.
- **v1.0.0:** semver guarantees; deprecations before removal.
- **No `chartlib` aliases** — clean setup from scratch.

## Error handling

`ChartExporter::save` and `encode` return `ChartResult<T>` — never silently ignore failures:

```cpp
auto res = ChartExporter::save(chart, "out.png");
if (!res)
  fprintf(stderr, "save failed: %s\n", chart_error_message(res.error()));

auto enc = ChartExporter::encode(chart, opts);
if (enc)
  upload(enc.value.data(), enc.value.size());
```

## Theming

Apply a consistent style across multiple charts in a report:

```cpp
auto theme = ChartTheme::dark();
pie.apply_theme(theme);
bar.apply_theme(theme);
line.apply_theme(theme);
// Per-chart overrides still take effect after apply_theme:
pie.options().title = "Distribution";
```

## Tick formatting

```cpp
bar.cartesian_options().y_tick_format = TickFormat::SI;   // 1500000 → "1.5M"
line.cartesian_options().y_tick_format = TickFormat::Currency;

// Custom formatter:
bar.cartesian_options().custom_y_formatter = [](double v) {
  char buf[32];
  std::snprintf(buf, sizeof(buf), "%.1f kbps", v / 1000.0);
  return std::string(buf);
};
```

## Pie chart labels

Slice labels may contain `\n` for manual line breaks (long MAC addresses, etc.). The label and percentage lines are stacked vertically; the renderer adjusts percentage position when the label spans multiple lines (raster and PoDoFo vector).

## Visual regression

Reference chart PNGs live in `docs/gallery/`. CI runs `gallery_regression` via CTest. After intentional visual changes:

```bash
cmake --build build --target insightdock_gallery_regression
UPDATE_GALLERY=1 ./build/tests/insightdock_gallery_regression docs/gallery
```

See the Troubleshooting guide for when a baseline update is warranted vs. a sign of an unintended
regression.

## Extending

Subclass `Chart` or `CartesianChart`, implement `render()` or `draw_series()`, and use `ChartExporter` for output. For custom overlays on existing Cartesian charts, override `draw_overlay(PlotContext&)` — it runs after `draw_series()` without replacing the full render pipeline:

```cpp
class AnnotatedBar : public BarChart {
  void draw_overlay(PlotContext& ctx) const override {
    float py = ctx.y_scale.to_pixel(target_);
    ctx.buffer.draw_line(ctx.plot.left(), py, ctx.plot.right(), py,
                         1.5f, {255, 80, 80, 200});
  }
  double target_ = 100.0;
};
```

For the full class hierarchy, render pipeline order, `PlotContext`/`Scale` internals, and the
steps for adding a brand-new chart type, see the site's Concepts guide (`docs-site/concepts.md`).

## CMake

```cmake
find_package(InsightDock CONFIG REQUIRED)
target_link_libraries(app PRIVATE InsightDock::Charts)
```

Compile definition `INSIGHTDOCK_HAS_PODOFO=1` when PDF support is linked.
