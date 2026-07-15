# InsightDock Charts for C++

> An MIT-licensed C++ charting engine for developers who need high-quality charts in reports and applications, with vector PDF output and an extensible architecture from day one.

Production-oriented C++17 charting for PDF reports, PNG/JPEG export, and embedded devices. Designed for easy extension (new chart types plug into shared bases) and OpenWrt cross-compilation (gcc 10+, zero link-time dependencies in the core).

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

See [Examples](examples.md) for the full set of sample programs, or jump straight to the [API Reference](api/index.html).

## Features

- **Chart types** — pie, vertical bar (grouped / stacked), multi-series line (optional area fill), area (stacked), combo (bar + line), scatter and bubble
- **Easy export** — one API, `ChartExporter::save(chart, "output.png")`, for PNG, JPEG, SVG, and PDF
- **Output** — RGBA `ImageBuffer`, PNG/JPEG, SVG (raster embed everywhere; native vector paths for pie), PDF raster embed, native vector PDF pies
- **Styling** — Chart.js / AmCharts 12-color palette, Tableau-10 palette (pie), axes, grid, legend, title
- **Text** — embedded ASCII-subset TTF (~20 KB); optional runtime TTF load for full Unicode
- **Core deps** — C++17 stdlib only (stb headers compiled into the library)
- **Optional PDF** — PoDoFo adapter via `insightdock/pdf/podofo.h`

## Where to go next

- [Getting started](getting-started.md) — build with Make or CMake, with or without PDF support
- [Concepts](concepts.md) — class hierarchy, render pipeline, and how to extend or add a chart type
- [Guides](guides/usage.md) — error handling, theming, tick formatting, PDF reports, troubleshooting
- [Chart types](chart-types/pie.md) — every option for pie, bar, line, area, combo, and scatter/bubble
- [Examples](examples.md) — every sample program, source and rendered output side by side
- [API Reference](api/index.html) — generated from the headers by [MrDocs](https://mrdocs.com)
- [Comparison](comparison.md) — how this stacks up against commercial and open-source alternatives
- [Changelog](changelog.md) — release history
