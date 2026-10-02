# Pie chart

`PieChart` (`insightdock/pie_chart.h`) renders labeled slices with a legend, Tableau-10 or
Chart.js palette, and native vector output for both SVG and PDF, not just raster.

![Pie chart](../img/gallery/pie_chart.png)

## Minimal example

```cpp
#include "insightdock/charts.h"

using namespace insightdock::charts;

PieChart pie;
pie.add_slice("Revenue", 100.0);
pie.add_slice("Expenses", 60.0);
pie.add_slice("Profit", 40.0);
ChartExporter::save(pie, "pie.png");
```

Slice values don't need to sum to 100 — percentages are computed from the total automatically.

## Adding slices

```cpp
pie.add_slice("Online", 72);              // auto color from PieOptions::palette
pie.add_slice("Offline", 28, Color::rgb(200, 60, 60)); // explicit color
```

## `PieOptions`

Set via `pie.set_pie_options(opts)` or mutate in place through a copy — there is no mutable
accessor, since options are applied as a batch:

```cpp
PieOptions opts;
opts.radius = 220.0;
opts.legend_position = PieLegendPosition::Right;
pie.set_pie_options(opts);
```

| Option | Default | Purpose |
|---|---|---|
| `center_x` / `center_y` | `0` | Pie center in pixels; `0` auto-centers in the plot area. |
| `radius` | `0.0` | Pie radius in pixels; `0` = auto-fit to the canvas, reserving margin for labels. |
| `gap_degrees` | `0.0` | Angular gap between slices, in degrees. |
| `skip_zero` | `true` | Omit slices with value ≤ 0. |
| `palette` | `PaletteKind::Tableau10` | Color cycle used for slices added without an explicit color. |
| `show_labels` | `true` | Draw slice labels on the chart. |
| `show_percentage` | `true` | Include the percentage in each label. |
| `show_legend` | `true` | Draw a legend. |
| `legend_position` | `PieLegendPosition::Bottom` | `Bottom` or `Right`. |
| `fill_opacity` | `0.6` | Slice fill alpha, `0`–`1`. |
| `stroke_color` | white | Outline color between slices. |
| `stroke_width` | `2.0` | Slice outline width in pixels. |
| `label_color` | `rgb(34,34,34)` | Label text color. |
| `label_font_size` | `11.0` | Label font size in pixels. |
| `inner_radius_ratio` | `0.0` | Doughnut cutout, as a fraction of `radius`, `[0, 1)`. `0` = plain pie. Drawn in raster, SVG and PDF output (the hole is painted in the chart background color). |
| `data_labels` | disabled | `DataLabelOptions` — an in-slice value/percentage label at `radius*0.65` along each slice's midpoint, distinct from the external leader-line label above. |

## Doughnut chart

```cpp
PieOptions opts = pie.pie_options();
opts.inner_radius_ratio = 0.55;
opts.show_labels = false;              // avoid the external label competing with the in-slice one
opts.data_labels.enabled = true;
opts.data_labels.color = Color::white();
opts.data_labels.format = TickFormat::Percent;
pie.set_pie_options(opts);
```

The cutout assumes a solid background color (already the only backdrop this library supports) —
it's drawn by filling a circle in `options().background` over the slice geometry, not a true hole.

## Multi-line labels

Slice labels may contain `\n` for manual line breaks (long MAC addresses, device names, etc.). The
label and percentage lines stack vertically, and the renderer adjusts percentage position when the
label spans multiple lines — this is handled consistently for raster and PoDoFo vector output.

```cpp
pie.add_slice("aa:bb:cc:dd:ee:ff\n(laptop-01)", 40);
```

## Vector output (SVG and PDF)

Pie has its own dedicated vector pipeline (leader-line labels, legend). Bar, line, area and combo
charts also export as native vector — see [Vector export](../guides/vector-export.md).

```cpp
ExportOptions opts;
opts.svg_backend = SvgBackend::Vector;   // real <path>/<circle>/<text> SVG
ChartExporter::save(pie, "pie.svg", opts);

opts.format = OutputFormat::PdfVector;
opts.pdf_backend = PdfBackend::Vector;   // native PDF path commands
ChartExporter::save(pie, "pie.pdf", opts);
```

See [Usage guide → Error handling](../guides/usage.md#error-handling) for checking the
`ChartResult` returned by `save()`, and [PDF reports](../guides/pdf-reports.md) for embedding a
pie chart inside a multi-page report.
