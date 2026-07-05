# Examples

Generic, synthetic examples for the reusable core library — source on the left logic, rendered output below.

## example_main

Minimal pie chart, exported to PNG and JPEG.

```bash
make example_main && ./example_main
# or
cmake --build build --target example_main && ./build/example_main
```

```cpp
--8<-- "examples/main.cc"
```

![Pie chart](img/gallery/pie_chart.png)

## bar_line_example

Grouped bar, stacked bar, line (with area fill), and pie in one run.

```bash
make bar_line_example && ./bar_line_example
```

```cpp
--8<-- "examples/bar_line_example.cc"
```

| Grouped bar | Stacked bar |
|---|---|
| ![Grouped bar](img/gallery/bar_grouped.png) | ![Stacked bar](img/gallery/bar_stacked.png) |

![Line chart](img/gallery/line_chart.png)

## combo_chart_example

Bar series on the left axis, line series on an independent right axis (different units/magnitude).

```bash
cmake --build build --target combo_chart_example && ./build/combo_chart_example
```

```cpp
--8<-- "examples/combo_chart_example.cc"
```

![Combo chart](img/gallery/combo_chart.png)

## time_series_line_example

Dense time-series `LineChart` with 100 daily data points: decimates to 25 representative points using LTTB and uses `max_x_labels = 12` to cap the x-axis to ~12 readable date labels regardless of chart width. For equivalent control over the y-axis, use `CartesianOptions::min_y_labels` / `max_y_labels` (e.g. set both to `12` to request ~12 evenly-spaced y-axis ticks).

```bash
cmake --build build --target time_series_line_example && ./build/time_series_line_example
```

```cpp
--8<-- "examples/time_series_line_example.cc"
```

![Time-series line chart](img/gallery/time_series_line.png)

## scatter_bubble_example

Plain scatter (continuous numeric x/y, two series) and a bubble chart (third value drives marker radius).

```bash
cmake --build build --target scatter_bubble_example && ./build/scatter_bubble_example
```

```cpp
--8<-- "examples/scatter_bubble_example.cc"
```

| Scatter | Bubble |
|---|---|
| ![Scatter chart](img/gallery/scatter_chart.png) | ![Bubble chart](img/gallery/bubble_chart.png) |

## podofo_report_example

Requires PoDoFo — see [Getting started](getting-started.md#cmake-podofo-pdf). Produces a multi-page PDF report with embedded charts (`chart_report.pdf`) and a vector-pie PDF (`pie_vector.pdf`); both are PDF output, so there's no PNG preview here.

```bash
cmake -B build-podofo \
  -DINSIGHTDOCK_WITH_PODOFO=ON \
  -DINSIGHTDOCK_PODOFO_DIR=/path/to/podofo
cmake --build build-podofo --target podofo_report_example
export LD_LIBRARY_PATH="$(pwd)/build-podofo/podofo/target:${LD_LIBRARY_PATH}"
./build-podofo/podofo_report_example
```

```cpp
--8<-- "examples/podofo_report_example.cc"
```
