# Line chart

`LineChart` (`insightdock/line_chart.h`) draws multi-series lines with optional point markers and
area fill, built on the shared `CartesianChart` axis/grid/legend pipeline.

![Line chart](../img/gallery/line_chart.png)

## Minimal example

```cpp
#include "insightdock/charts.h"

using namespace insightdock::charts;

LineChart line;
line.set_categories({"Jan", "Feb", "Mar", "Apr", "May"});
line.add_series("Series A", {10, 25, 18, 32, 28});
line.add_series("Series B", {15, 12, 22, 20, 35});
line.line_options().fill_area = true;
ChartExporter::save(line, "line.png");
```

## `LineChartOptions`

Accessed via `line.line_options()` (mutable reference, so set fields directly):

| Option | Default | Purpose |
|---|---|---|
| `show_points` | `true` | Draw markers at each data point. |
| `fill_area` | `false` | Fill the area under the line (first series only). |
| `line_width` | `2.0` | Stroke width in pixels. |
| `point_radius` | `4.0` | Point marker radius in pixels. |
| `spline` | `false` | Catmull-Rom cubic interpolation between points instead of straight segments. |
| `dash` | empty (solid) | Alternating on/off stroke lengths in pixels, e.g. `{6, 4}`. Chart-wide, not per-series. |
| `point_shape` | `PointMarkerShape::Circle` | `Circle`, `Square`, `Triangle`, or `Cross`. Chart-wide, not per-series. |
| `data_labels` | disabled | `DataLabelOptions` — draws each point's formatted value above it. |

`fill_area` only fills under the *first* added series — if you need every series filled and
stacked, use [`AreaChart`](area.md) instead, which is built for exactly that.

```cpp
line.line_options().spline = true;
line.line_options().dash = {8.0f, 4.0f};
line.line_options().point_shape = PointMarkerShape::Triangle;
```

`spline`/`dash`/`point_shape` apply to every series in the chart uniformly — there's no per-series
override (e.g. a dashed "forecast" line next to a solid "actual" one in the same chart isn't
possible without two separate charts overlaid).

## Reference lines and bands

`cartesian_options().reference_lines`/`reference_bands` work the same way here as on `BarChart` —
see [Bar → Reference lines and bands](bar.md#reference-lines-and-bands).

## Dense / time-series data

For a line with many data points (daily data over months, for example), three tools keep the
chart readable without changing the underlying data:

- **`lttb_decimate_indexed()`** (`insightdock/decimate.h`) reduces point count before rendering,
  preserving the visual shape (peaks/valleys) using the largest-triangle-three-buckets algorithm.
- **`set_x_values()` + `cartesian_options().x_scale_type = ScaleType::DateTime`** positions each
  point by its real Unix timestamp instead of an evenly-spaced category slot — irregular gaps in
  the data render as irregular pixel spacing, and axis ticks are generated and formatted
  automatically from the numeric domain. See
  [Bar → Continuous DateTime x-axis](bar.md#continuous-datetime-x-axis) for the full explanation
  (only `LineChart`/`AreaChart` support it).
- **`CartesianOptions::max_x_labels`** caps the number of x-axis ticks/labels shown, regardless of
  chart width — works the same way in both category mode and continuous `DateTime` mode.

```cpp
auto [decimated_values, decimated_indices] = lttb_decimate_indexed(daily_values, /*target_points=*/25);
std::vector<double> decimated_timestamps;
for (std::size_t i : decimated_indices)
  decimated_timestamps.push_back(daily_timestamps[i]);

line.set_x_values(decimated_timestamps);
line.add_series("Bandwidth", decimated_values);
line.cartesian_options().x_scale_type = ScaleType::DateTime;
line.cartesian_options().max_x_labels = 12;   // ~12 readable date labels
```

See `examples/time_series_line_example.cc` (full source in [Examples](../examples.md)) for a
complete 100-point-to-25-point walkthrough, and
[Bar → Shared CartesianOptions](bar.md#shared-cartesianoptions) for the equivalent y-axis density
controls (`min_y_labels`/`max_y_labels`), which apply here identically.
