# Concepts

How InsightDock is put together, for anyone extending a chart type, embedding a custom overlay,
or just wanting to know why a call is shaped the way it is. If you only need to *use* existing
chart types, the [Usage guide](guides/usage.md) and [Chart types](chart-types/pie.md) pages are a
faster path — come back here when you need the "why."

## Class hierarchy

```
Chart (abstract)
├── PieChart          — pie slices, Tableau-10/Chart.js palette, raster + vector PDF + native vector SVG
├── ScatterChart      — scatter/bubble; continuous numeric x/y Scales, not a category axis
└── CartesianChart (abstract) — shared axes, grid, legend, title rendering
    ├── BarChart      — grouped or stacked vertical bars
    ├── LineChart     — multi-series lines with optional area fill and point markers
    ├── AreaChart     — filled-area polygons, optional stacking
    └── ComboChart    — grouped bars + line overlay; series opt into a shared or independent axis
```

Two base classes, not one, because the axis model genuinely differs:

- **`Chart`** is the root: it owns `ChartOptions` (title, size, background, margins, font) and
  requires only `render() -> ImageBuffer`. `PieChart` and `ScatterChart` derive from it directly.
- **`CartesianChart`** adds everything charts-with-a-category-axis share: category labels, one or
  more `DataSeries`, axis/grid/legend rendering, and the `draw_series(PlotContext&)` extension
  point. `BarChart`, `LineChart`, `AreaChart`, and `ComboChart` derive from it.

`ScatterChart` deliberately does **not** derive from `CartesianChart` even though it also has axes
and a legend — its x-axis is a continuous numeric `Scale` over arbitrary (possibly unsorted,
unevenly-spaced) point data, not a fixed list of category labels. Reusing `CartesianChart` would
have meant bolting a second, incompatible axis model onto it. When you're deciding which base to
derive a new chart type from, this is the test to apply: *is the x-axis a category list, or a
continuous numeric domain?*

## The render pipeline

Every chart's public entry point is `render() -> ImageBuffer` (RGBA, row-major). For
`CartesianChart` subclasses, `render()` itself is not virtual — it creates the image, wraps it in a
`RasterTarget` and calls `Chart::draw(DrawTarget&)`. That same `draw()` is what the SVG and PDF
exporters call with an `SvgTarget` / `PdfTarget`, which is how Cartesian charts get native vector
output (see [Vector export](guides/vector-export.md)). `draw()` runs, in order:

1. `draw_background()` — fills the canvas with `ChartOptions::background`.
2. `draw_title()` — chart title, if set.
3. `compute_plot_rect()` — works out the plot rectangle inside the margins, leaving room for axis
   labels/ticks and the legend.
4. `draw_axes_and_grid()` — builds `PlotContext::x_scale` / `y_scale` (and `y_scale2` if a
   secondary axis is enabled), then draws ticks, gridlines, and axis labels.
5. **`draw_series(PlotContext&)`** — pure virtual; this is what each subclass implements. `BarChart`
   draws grouped/stacked rectangles, `LineChart` draws polylines and point markers, `AreaChart`
   draws filled polygons, `ComboChart` draws bars and dispatches line-series drawing to the same
   overlay hook it also uses for itself (see below).
6. **`draw_overlay(PlotContext&)`** — virtual, default no-op; called *after* `draw_series`. This is
   the supported extension point for adding things without touching the render pipeline (see
   below).
7. `draw_legend()` — one entry per series, using each `DataSeries::color`.

`PieChart` and `ScatterChart` don't go through this pipeline — they override `render()` directly
since they don't share the category/series axis model. (`PieChart` has its own vector SVG/PDF
writers; `ScatterChart` embeds a raster image in vector formats.)

## `PlotContext`

`draw_series` and `draw_overlay` receive a `CartesianChart::PlotContext&`, a bundle of everything
needed to draw geometry without recomputing it:

| Field | Purpose |
|---|---|
| `target` | `DrawTarget&` — draw onto this (raster, SVG or PDF). Use its methods (`fill_rect`, `draw_dashed_polyline`, `draw_text(font, ...)`, …) rather than `ImageBuffer` ones. |
| `plot` | Plot area `Rect`, already inset for margins/labels. |
| `x_scale` | Category index → pixel X. |
| `y_scale` | Data value → pixel Y (primary, left axis). |
| `y_scale2` / `has_y2` | Secondary (right) axis scale, populated only when `has_secondary_y_axis()` returns true. |
| `font` | For any text the overlay draws. |
| `opts` / `cart_opts` | Common and Cartesian-specific options. |
| `categories` / `series` | The chart's data. |

Because `PlotContext` carries the already-computed scales, an overlay or a new chart type never
needs to re-derive pixel mapping — call `ctx.y_scale.to_pixel(value)` and draw.

## `Scale`

`Scale` (`include/insightdock/scale.h`) is the one class doing the linear-mapping work behind every
axis, in both directions:

- `to_pixel(value)` — data value → pixel coordinate, for drawing.
- `from_pixel(px)` — pixel coordinate → data value, for hit-testing or overlays anchored to a
  screen position.
- `linear_nice(min, max, tick_hint)` — expands a raw data range to "nice" round numbers before
  computing ticks (this is what backs `CartesianOptions::min_y_labels` / `max_y_labels`).
- `nice_ticks()` — generates the actual tick positions/labels for an axis.

`ScaleType` (`Linear`, `Log`, `DateTime`) controls how `nice_ticks` buckets and formats values —
`DateTime` combined with `TickFormat::DateTime` is what a time-series x or y axis uses.

## Extending: the `draw_overlay` hook

`draw_overlay(PlotContext&)` is the supported way to add custom drawing to an existing
`CartesianChart` subclass **without** subclassing the render pipeline itself. It runs once, after
`draw_series`, so anything it draws sits on top of bars/lines/areas but under the legend.

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

This is also the mechanism `ComboChart` itself is built on: bars are drawn in `draw_series`, and
the line-series pass happens in its own `draw_overlay` override, reusing the same hook rather than
inventing a second combo-specific pipeline.

## Adding a new chart type

1. Derive from `Chart` (freeform axes/layout) or `CartesianChart` (category axis, shared
   axes/grid/legend) — see the test above for which one fits.
2. Implement `render()` (freeform) or `draw_series(PlotContext&)` (Cartesian). Override
   `draw_overlay` too if the chart needs a second drawing pass (dual-axis, annotations).
3. If the chart needs its own axis behavior — a secondary y-axis, a non-default y-domain — override
   `has_secondary_y_axis()` / `compute_y_min()` / `compute_y_max()` / `compute_y2_min()` /
   `compute_y2_max()` as needed. `ComboChart`'s dual-axis support is the reference implementation
   for this.
4. Add the new `.cc` file to `INSIGHTDOCK_SOURCES` in `CMakeLists.txt` and to `SRCS` in `Makefile`.
5. Include the new header in `include/insightdock/charts.h` (the umbrella include).
6. Add a CTest smoke test and a `gallery_regression` reference PNG so the new type has a committed
   visual baseline (see [Troubleshooting → updating the visual regression baseline](guides/troubleshooting.md#updating-the-visual-regression-baseline)).
7. Add an entry to `CHANGELOG.md` and update `ROADMAP.md`.

Chart-type-specific integration code (application/report samples) belongs in `examples/`, never in
`include/insightdock/` or `src/`.

## Determinism

Rendering is deterministic given the same inputs — no randomness, no wall-clock or locale
dependence in geometry or color selection. This is what makes `gallery_regression` (pixel-diff
comparison against `docs/gallery/`, tolerant of ≤0.1% differing pixels for cross-platform font
rasterization variance) a meaningful CI gate rather than a fuzzy visual diff.
