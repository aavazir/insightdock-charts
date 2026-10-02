# Chart library comparison

InsightDock is positioned against **both commercial and open-source** charting options. This document is the source for the future public website comparison page (GitHub Pages). Claims are kept factual; performance numbers belong in [BENCHMARKS.md](BENCHMARKS.md) once measured.

## Summary

| Category | Examples | InsightDock fit |
|----------|----------|-----------------|
| **Commercial / proprietary** | ChartDirector, StockChartX | Report-grade output, no license keys, MIT source |
| **Open-source (C++)** | ImPlot, PLplot, Qt Charts, matplotlib-cpp | Headless reports, embedded/OpenWrt, simple export API |
| **Open-source (non-C++ / bindings)** | Chart.js, Matplotlib, Plotly | Different stack; InsightDock for native C++ server/embedded |

InsightDock does **not** claim to beat every library on every axis. It targets developers who need **native C++**, **headless report output** (PNG/JPEG/PDF), and an **extensible MIT-licensed core** without GUI toolkits or license keys.

---

## Commercial (proprietary)

### ChartDirector

| | ChartDirector | InsightDock (v0.7) |
|---|---------------|---------------------|
| License | Commercial | MIT |
| Chart breadth | Very broad (3D, heatmap, gantt, …) | Pie, bar, line, area, combo, scatter/bubble |
| Report / PDF | Strong multi-page reports | PNG/JPEG; native vector PDF (pie, bar, line, area, combo) + raster embed; multi-page report builder with chart embedding (PoDoFo) |
| Interactivity | Zoom, tooltips, hit-testing | Static output (roadmap: hooks) |
| Integration | Many language bindings | Native C++17 |
| Embedded / OpenWrt | Possible | Core designed for it (Make, no GUI deps) |
| Source | Closed | Open, extensible |

**InsightDock roadmap:** Phase 1–4 targets ChartDirector breadth (see [ROADMAP.md](../ROADMAP.md)).

### StockChartX

| | StockChartX | InsightDock (v0.7) |
|---|-------------|---------------------|
| License | Commercial | MIT |
| Focus | Finance (OHLC, TA, realtime) | General report charts |
| Price styles | Many (candlestick, Renko, …) | Not yet |
| Technical indicators | 50+ | Not yet |
| Realtime / tick updates | Core strength | Roadmap |
| Report output | Strong | PNG/JPEG/PDF |

**InsightDock roadmap:** Phase 2–3 targets finance depth.

---

## Open-source (C++ and native)

### ImPlot

| | ImPlot | InsightDock (v0.7) |
|---|--------|---------------------|
| License | MIT | MIT |
| Model | Immediate-mode, Dear ImGui | Retained raster/vector export |
| GUI required | Yes (ImGui) | No — headless/server |
| Best for | Live desktop tools, debugging UIs | PDF reports, batch PNG, embedded |
| PDF / print reports | Via screenshot/export, not first-class | First-class (`ChartExporter`, native vector PDF) |
| OpenWrt / firmware | ImGui + backend overhead | STL-only core, Makefile |

### PLplot

| | PLplot | InsightDock (v0.7) |
|---|--------|---------------------|
| License | LGPL (library) | MIT |
| Focus | Scientific plotting | Application/report charts |
| Output | Many devices (SVG, PDF, PNG, …) | PNG, JPEG, SVG (raster + native vector), PDF (PoDoFo) |
| API style | C/Fortran heritage, device drivers | Modern C++17, minimal surface |
| Embedded | Heavier dependency story | Lightweight core |

### Qt Charts

| | Qt Charts | InsightDock (v0.7) |
|---|-----------|---------------------|
| License | GPL / commercial Qt | MIT |
| Model | QWidget / QML, interactive | Static export |
| GUI required | Yes | No |
| PDF reports | Via Qt print/PDF | PoDoFo embed + native vector charts |
| Footprint | Full Qt stack | Core: STL + stb only |

### matplotlib-cpp

| | matplotlib-cpp | InsightDock (v0.7) |
|---|----------------|---------------------|
| License | MIT wrapper; Matplotlib license | MIT |
| Dependencies | Python + Matplotlib runtime | C++17 STL (core) |
| Deployment | Needs Python on server/device | Single binary / static lib |
| Embedded / OpenWrt | Impractical | Supported path (Make) |
| API | Matplotlib idioms in C++ | Purpose-built chart types + `ChartExporter` |

### sciplot / other header-only plotters

| | Typical header-only plotters | InsightDock (v0.7) |
|---|------------------------------|---------------------|
| Maturity | Varies; often academic/scientific | Production report generation (PDF/PNG output) |
| PDF vector in reports | Rare | Native vector charts (pie, bar, line, area, combo) + raster embed |
| Chart types for dashboards | Line/scatter focus | Pie, bar, line, scatter/bubble + report styling |

---

## Open-source (JavaScript / other stacks)

These are common in web dashboards but solve a different problem than a **native C++ report engine**:

| Library | Typical use | vs InsightDock |
|---------|-------------|----------------|
| **Chart.js** | Browser dashboards | InsightDock: server-side PDF/PNG, no browser |
| **Apache ECharts** | Rich web charts | Breadth on web; InsightDock: C++ embed/report |
| **Plotly** | Interactive analytics | InsightDock: deterministic batch reports |
| **Matplotlib (Python)** | Scripts, notebooks | InsightDock: no Python runtime in firmware/server C++ |

Future website may include a **“Choose your stack”** section: web dashboard → Chart.js/ECharts; native C++ report → InsightDock; finance terminal → StockChartX-class (roadmap).

---

## Where InsightDock wins today

- **MIT license** with no keys (vs commercial)
- **No GUI toolkit** required (vs ImPlot, Qt Charts)
- **No Python/runtime** dependency (vs matplotlib-cpp)
- **Simple export API** — `ChartExporter::save(chart, "out.png")` in a few lines
- **Vector PDF** for print-quality reports (PoDoFo) — pie, bar, line, area, combo
- **SVG export** — raster embed for every chart type; native vector shapes and `<text>` for pie, bar, line, area, combo
- **OpenWrt / cross-compile** friendly core (Make + minimal deps)
- **Extensible C++** architecture open to contributions

## Where InsightDock is behind today

- Chart type count (vs ChartDirector, ECharts)
- Finance / TA depth (vs StockChartX)
- Native vector output for scatter (raster embed today)
- Interactivity (vs ImPlot, Qt, web libs)
- Published benchmarks — internal baseline recorded in [BENCHMARKS.md](BENCHMARKS.md); not yet vetted for public performance claims

---

## Future website (GitHub Pages)

Planned comparison UX:

1. **Landing:** “Compare InsightDock” with tabs: **Commercial** | **Open source (C++)** | **Web / other**
2. **Feature matrix:** sortable table (license, headless, PDF vector, embedded, chart count, finance)
3. **Use-case picker:** “I need …” → recommended stack (honest; may point to another OSS lib when better fit)
4. **Gallery:** side-by-side sample outputs where licensing allows
5. **Migration notes:** from ChartDirector-style APIs or from matplotlib-cpp batch scripts

See [FUTURE.md](FUTURE.md) for site structure and code generator plans.

## Maintenance

- Update this file when adding chart types or export formats
- Do not add performance claims until [BENCHMARKS.md](BENCHMARKS.md) has numbers
- Re-check competitor licenses and feature lists before v1.0.0 public launch
