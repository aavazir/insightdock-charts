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
| `insightdock/export.h` | `ChartExporter`, `ExportOptions`, `ExportCapabilities`, `VectorFallback`, `PdfBackend`, `SvgBackend` |
| `insightdock/scale.h` | `Scale`, `ScaleType` |
| `insightdock/decimate.h` | `lttb_decimate`, `lttb_decimate_indexed` — LTTB downsampling for large datasets |
| `insightdock/image_buffer.h` | `ImageBuffer` — RGBA raster buffer and draw primitives |
| `insightdock/color.h`, `palette.h`, `font.h` | Visual primitives |
| `insightdock/png_writer.h` | Low-level PNG/JPEG encode |
| `insightdock/svg_writer.h` | Low-level raster SVG encoding (PNG base64-inlined); use `ChartExporter` with `SvgBackend::Vector` for native vector charts |
| `insightdock/pdf/podofo.h` | PDF umbrella (PoDoFo build) |
| `insightdock/pdf/report.h` | `PdfReportBuilder` — tables, sections, footers (PoDoFo build) |

## Core types

| Type | Purpose |
|------|---------|
| `PieChart`, `BarChart`, `LineChart`, `AreaChart`, `ComboChart`, `ScatterChart` | Chart implementations |
| `ChartOptions`, `PieOptions`, `ScatterOptions`, … | Styling and layout per chart type |
| `ChartTheme` | Reusable visual style — apply across multiple charts |
| `ChartResult<T>` | Structured result from `save()` / `encode()` — holds value or `ChartError` |
| `ChartError` | Enum of failure reasons: `BadPath`, `RenderFailure`, `EncodeFailure`, `PdfUnsupported`, `InvalidInput`, `VectorUnsupported` |
| `TickFormat` | Y-axis label style: `Raw`, `SI`, `Currency`, `Percent`, `DateTime` |
| `ScaleType` | Axis scale mode: `Linear`, `Log`, `DateTime` |
| `ChartExporter` | Render and save PNG / JPEG / SVG / PDF |
| `SvgBackend` | `Raster` (default, all charts) or `Vector` (native output for all built-in charts) |
| `lttb_decimate` / `lttb_decimate_indexed` | LTTB downsampling for large `LineChart`/`AreaChart` datasets |
| `PdfReportBuilder` | Multi-page PDF report layout (PoDoFo) |
| `ImageBuffer` | RGBA raster buffer |
| `Color`, `Palette`, `Font` | Visual primitives |

DateTime tick labels use UTC by default. `CartesianOptions::custom_x_formatter` receives the
original timestamp for custom date formats or timezone presentation on continuous x axes.
Raw numeric labels preserve fractions; value-axis labels select decimal precision from tick spacing.

Legends wrap long labels and explicit newlines using the chart font. Top/bottom positions pack
centered rows; Right reserves measured space up to a third of the canvas width. Legends occupy
at most a third of the canvas height and show `+N more` for omitted entries without hiding data.
Automatic pie geometry reserves title, legend and measured callout space during rendering. Legacy PDF pie report
helpers retain their separate layout.

Namespace: `insightdock::charts`

PDF helpers: `insightdock::charts::podofo` (via `insightdock/pdf/podofo.h`)

Pie and Cartesian charts implement `Chart::draw(DrawTarget&)` with a shared raster/SVG/PDF chart
layout. `DrawTarget::fill_sector()` defaults to polygon tessellation, preserving source compatibility
for custom targets. `podofo::draw_chart_vector()` without `PieVectorOptions` uses this layout;
the overload with explicit `PieVectorOptions`, plus `draw_pie()` / `draw_pie_slices()`, retains the
separate PDF report layout.

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

Both functions accept const charts; mutable overloads use the same implementation.
`ChartExporter::capabilities(chart)` reports the chart's advertised `VectorCapability` and
compiled PDF availability without rendering. For native SVG/PDF requests,
`ExportOptions::vector_fallback = VectorFallback::Reject` returns `VectorUnsupported` when
`draw()` declines the target. The default `AllowRaster` embeds an image with normal raster
quality/size checks. Existing custom `draw()` implementations work in strict mode even if their
capability declaration remains `Unknown`.

Standard exceptions during rendering/cloning become `RenderFailure`; native drawing/encoding
exceptions become `EncodeFailure`; file I/O exceptions become `BadPath`. These APIs are not
`noexcept`: non-standard custom exceptions and allocations/path preparation outside guarded
operations can propagate. `render()` propagates exceptions. Void PDF embedding APIs propagate
rendering/PoDoFo errors and throw `std::runtime_error` on strict vector rejection; they cannot
roll back marks already written by custom drawing code.

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

Slice labels may contain `\n` for manual line breaks. Shared PNG/SVG/PDF chart exports wrap
labels using font measurements and place the label and optional percentage in a block outside
the pie. Blocks move vertically to avoid overlap; leader lines stay outside the circle. Title and
legend areas are reserved. When a column cannot fit every block, larger slices keep their
callouts first, with input-order ties. Blocks that cannot fit are omitted in full; slices and
legend entries remain independent. Explicit geometry can leave less room for callouts.
Legacy `PieVectorOptions` PDF report helpers retain their separate layout policy.

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

## Supersampling

Set `ExportOptions::supersample` to 2 or 4 to improve raster edge quality. Factors are clamped
into [1, 8]; pixel limits apply to the high-resolution allocation. Output dimensions stay at the
chart's configured size. Built-in charts compute layout once in logical pixels and scale every
primitive, including strokes, dashes, markers, corners, leaders and all text sizes. Wrapping,
legend overflow and callout selection therefore stay consistent across sampling factors.
Native SVG/PDF ignores this option.

Custom charts need `clone()` and should implement `draw(DrawTarget&)` for consistent scaling.
A `render()`-only clone receives scaled canvas, base font and margins through the legacy path;
its custom style sizes are its own responsibility. Exports never mutate the original chart.
