# InsightDock examples

Generic, synthetic examples for the reusable core library.

## example_main

Minimal pie chart → PNG and JPEG.

```bash
make example_main && ./example_main
# or
cmake --build build --target example_main && ./build/example_main
```

Output: `chart_output.png`, `chart_output.jpg`

## bar_line_example

Grouped bar, stacked bar, line (with area fill), and pie in one run.

```bash
make bar_line_example && ./bar_line_example
```

Outputs: `bar_grouped.png`, `bar_stacked.png`, `line_chart.png`, `pie_chart.png`

## time_series_line_example

Dense time-series `LineChart`: 100 daily data points decimated to 25 via LTTB, with `max_x_labels = 12` to cap the x-axis to ~12 readable date labels.

```bash
cmake --build build --target time_series_line_example && ./build/time_series_line_example
```

Output: `time_series_line.png`

Source: [time_series_line_example.cc](time_series_line_example.cc)

## scatter_bubble_example

Plain scatter (continuous numeric x/y, two series) and a bubble chart (third value drives marker radius).

```bash
cmake --build build --target scatter_bubble_example && ./build/scatter_bubble_example
```

Outputs: `scatter_chart.png`, `bubble_chart.png`

Source: [scatter_bubble_example.cc](scatter_bubble_example.cc)

## bar_polish_example

Rounded bar corners, a border, and data labels on `BarChart`.

```bash
cmake --build build --target bar_polish_example && ./build/bar_polish_example
```

Output: `bar_polish.png`

Source: [bar_polish_example.cc](bar_polish_example.cc)

## pie_doughnut_example

Doughnut cutout (`PieOptions::inner_radius_ratio`) with in-slice percentage data labels.

```bash
cmake --build build --target pie_doughnut_example && ./build/pie_doughnut_example
```

Output: `pie_doughnut.png`

Source: [pie_doughnut_example.cc](pie_doughnut_example.cc)

## line_spline_dash_example

Spline (Catmull-Rom) smoothing, a dash pattern, and triangle point markers on `LineChart`. These three style knobs are chart-wide, not per-series.

```bash
cmake --build build --target line_spline_dash_example && ./build/line_spline_dash_example
```

Output: `line_spline_dash.png`

Source: [line_spline_dash_example.cc](line_spline_dash_example.cc)

## combo_reference_lines_example

`ComboChart` with a shaded target `ReferenceBand` and a dashed SLA `ReferenceLine`, drawn via the shared reference-line/band overlay available on any Bar/Line/Area/Combo chart.

```bash
cmake --build build --target combo_reference_lines_example && ./build/combo_reference_lines_example
```

Output: `combo_reference_lines.png`

Source: [combo_reference_lines_example.cc](combo_reference_lines_example.cc)

## podofo_report_example

Requires PoDoFo. See [../README.md](../README.md#cmake--podofo-pdf).

```bash
cmake -B build-podofo \
  -DINSIGHTDOCK_WITH_PODOFO=ON \
  -DINSIGHTDOCK_PODOFO_DIR=/path/to/podofo
cmake --build build-podofo --target podofo_report_example
export LD_LIBRARY_PATH="$(pwd)/build-podofo/podofo/target:${LD_LIBRARY_PATH}"
./build-podofo/podofo_report_example
```

Output: `chart_report.pdf`, `pie_vector.pdf`

Source: [podofo_report_example.cc](podofo_report_example.cc)
