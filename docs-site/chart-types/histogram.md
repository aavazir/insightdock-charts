# Histogram

A histogram is a `BarChart` with touching bars, one per bin. `make_histogram_chart()` bins raw
samples and returns an ordinary chart, so titles, colors, data labels and every export format
(including native vector SVG/PDF) work as usual.

```cpp
#include "insightdock/charts.h"
using namespace insightdock::charts;

std::vector<double> latencies = /* thousands of samples */;

HistogramOptions opts;
opts.bins = 20;                       // 0 (default) = Sturges' rule: 1 + log2(n), at most 100
BarChart hist = make_histogram_chart(latencies, opts, "Requests");
hist.options().title = "Latency distribution (ms)";
hist.options().legend.enabled = true;  // off by default (single series)
ChartExporter::save(hist, "latency.png");
```

## Binning only

`compute_histogram()` returns the edges and counts without building a chart — useful for feeding
another chart type or your own table:

```cpp
Histogram h = compute_histogram(latencies, opts);
// h.edges.size() == h.counts.size() + 1
```

## `HistogramOptions`

| Field | Default | Purpose |
|---|---|---|
| `bins` | `0` | Number of equal-width bins; `0` chooses automatically. |
| `min_value`, `max_value` | `0`, `0` | Fixed range when `min_value < max_value`; samples outside it are ignored. Otherwise the data's own min/max. |

NaN and Inf samples are skipped. The last bin includes its upper edge. Bars are labelled
`lo-hi`; on a dense chart the usual label skipping keeps the axis readable
(see [x-axis label modes](../guides/fonts-and-text.md#wide-x-axis-labels)).
