# InsightDock Agent Skill (draft)

Use this document when building Cursor skills or agent workflows for chart generation.

## Quick patterns

### Pie chart to PNG

```cpp
#include "insightdock/charts.h"
using namespace insightdock::charts;

PieChart pie;
pie.add_slice("A", 40);
pie.add_slice("B", 60);
ChartExporter::save(pie, "output.png");
```

### Bar chart

```cpp
BarChart bar;
bar.set_categories({"Q1", "Q2", "Q3"});
bar.add_series("Sales", {100, 120, 90});
ChartExporter::save(bar, "sales.png");
```

### Vector PDF pie (PoDoFo)

```cpp
#include "insightdock/charts.h"
#include "insightdock/pdf/podofo.h"

ExportOptions opts;
opts.pdf_backend = PdfBackend::Vector;
ChartExporter::save(pie, "report.pdf", opts);
```

## Agent instructions

1. Prefer `insightdock/charts.h` only unless PDF embedding is required.
2. Use sensible defaults; avoid unnecessary `ChartOptions` unless the user asks.
3. For multi-chart PDF reports, use PoDoFo page layout in application code; use `ChartExporter::draw_pdf` to embed charts.

## JSON to chart

Map JSON fields to slices or series labels/values, then call `ChartExporter::save`. Validate input size and types in application code.

See [docs/FUTURE.md](docs/FUTURE.md) for online code generator plans.
