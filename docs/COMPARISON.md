# Chart library comparison

InsightDock Charts is a **native C++17, headless, report-oriented** charting library. This page
compares it with the alternatives a C++ developer is likely to consider. It is deliberately
honest: for several jobs another library is the better choice, and those are called out.

Versions and licenses were checked against each project's public documentation when this page was
last edited (see [Maintenance](#maintenance)). Performance numbers live only in
[BENCHMARKS.md](https://github.com/aavazir/insightdock-charts/blob/main/docs/BENCHMARKS.md).

## Which library should I use?

| If you need… | Use |
|---|---|
| Interactive plots inside a desktop app, zoom/pan, live data | **ImPlot** (Dear ImGui) or **QCustomPlot** / **Qt Charts** (Qt) |
| PNG / SVG / **vector PDF** from a server, CLI tool or firmware — no GUI, no Python | **InsightDock Charts** |
| Scientific plotting with MATLAB-style API, 3D, many plot kinds | **Matplot++** or **PLplot** |
| Charts in a web page | **Chart.js**, **Apache ECharts**, **Plotly** |
| The widest chart catalogue in C++ and you accept a commercial license | **ChartDirector** |
| Candlesticks, indicators, tick-by-tick finance | A finance-specific library (InsightDock Charts has none of this) |

## At a glance

| | InsightDock Charts | QCustomPlot | Matplot++ | ImPlot | Qt Charts | PLplot | ChartDirector |
|---|---|---|---|---|---|---|---|
| License | MIT | GPL-3 / commercial | MIT | MIT | GPL-3 / commercial (Qt) | LGPL | Commercial |
| Needs a GUI toolkit | No | Qt | No | Dear ImGui | Qt | No | No |
| Needs Gnuplot / Python | No | No | Gnuplot (rendering) | No | No | No | No |
| Headless / server use | Yes | Possible (Qt without display) | Yes, via Gnuplot | Awkward | Possible | Yes | Yes |
| Interactive | No | Yes | Limited | Yes | Yes | Limited | Limited |
| PNG / JPEG | Yes | Yes | Yes | Via screenshot | Via `grab()` | Yes | Yes |
| SVG | Native vector (pie, bar, line, area, combo); raster embed otherwise | Not built in (PDF + raster only) | Yes | No | Via `QSvgGenerator` | Yes | Yes |
| Vector PDF | Yes, with optional PoDoFo (pie, bar, line, area, combo) | Yes (`savePdf`) | Yes | No | Via `QPdfWriter` | Yes | Yes |
| Embedded / OpenWrt build | Yes: Make, STL + stb only | No | No | No | No | Heavy | Not a target |
| Chart breadth | Pie, doughnut, bar, line, area, combo, scatter/bubble, histogram | Many 2D plot kinds | Very broad, incl. 3D | Many 2D plot kinds | Standard business charts | Very broad | Very broad |

Entries marked "Possible" or "Awkward" reflect how the library is normally used, not a hard limit.

---

## Open source, C++

### QCustomPlot

Mature, widely used Qt widget with excellent interactivity and plot variety. Pick it if your
application is already Qt and you want users to explore data. Its GPL-3 license (commercial option
available) matters for closed-source products. InsightDock Charts differs in not needing Qt at all, so it
runs in a daemon, a CLI or a router, and it ships a permissive MIT license. QCustomPlot is the
better fit for anything interactive.

### Matplot++

A C++ take on MATLAB/matplotlib plotting with a very large range of plot types, 3D included. It
renders through Gnuplot, so a Gnuplot installation must be present at runtime. InsightDock Charts has a
much smaller catalogue but renders in-process with no external program, which is what makes the
OpenWrt and container cases work. Pick Matplot++ for exploratory or scientific work.

### ImPlot

Immediate-mode plots for Dear ImGui: fast, live, interactive. It is a debugging and tooling
library, not a report engine; producing a file means capturing the framebuffer. InsightDock Charts is the
opposite trade: static, deterministic output, no window.

### Qt Charts

Standard business charts for QWidget/QML with animations and interaction. Requires the Qt stack
(GPL-3 or commercial). Choose it inside a Qt application; choose InsightDock Charts when there is no Qt.

### PLplot

Long-established scientific plotting with many output devices. LGPL, C-style API with Fortran
heritage, heavier to build and package. InsightDock Charts offers a modern C++17 API and a much smaller
footprint but far fewer plot kinds.

### matplotlib-cpp

A wrapper that embeds a Python interpreter and calls Matplotlib. Output quality is Matplotlib's;
the cost is a Python runtime on every machine that renders. Not practical for firmware.

---

## Commercial

### ChartDirector

The broadest commercial C++ charting catalogue (3D, gauges, Gantt, finance, many language
bindings), with strong PDF output. InsightDock Charts does not try to match that breadth; it covers the
common business charts, is open source, and has no license keys. If you need charts that
InsightDock Charts does not have, ChartDirector is a legitimate choice.

### StockChartX

A finance charting component: candlesticks, technical indicators, real-time updates. InsightDock Charts has
none of these and is not aiming at that niche for 1.0.

---

## Web and other stacks

| Library | Typical use | Relation to InsightDock Charts |
|---|---|---|
| **Chart.js** | Browser dashboards | Different runtime. InsightDock Charts' default palette is Chart.js-style so server-rendered reports match a web dashboard. |
| **Apache ECharts** | Rich interactive web charts | Far broader; use it for the web. |
| **Plotly** | Interactive analytics | Use it for exploration; InsightDock Charts is for deterministic batch output. |
| **Matplotlib (Python)** | Scripts and notebooks | Use it if Python is acceptable on the machine that renders. |

---

## What InsightDock Charts is good at

- **MIT license**, no keys, no copyleft obligations.
- **No GUI toolkit, no Python, no Gnuplot.** The core is the C++ standard library plus vendored stb headers.
- **Native vector SVG and PDF** for all built-in charts: real paths and selectable text. PDF output needs the optional PoDoFo dependency.
- **Simple export API:** `ChartExporter::save(chart, "out.png")`, with the format inferred from the extension.
- **Small, cross-compilable build:** a Make build for OpenWrt with selectable chart types, export formats and font mode. Sizes are in the [README](https://github.com/aavazir/insightdock-charts/blob/main/README.md).
- **Deterministic rendering** with an embedded font, so the same input gives the same chart on every machine.
- **CI on Linux x86-64 and ARM64, macOS and Windows** (see the workflow for the current matrix).

## Where InsightDock Charts is limited today

- **Static output only.** No zoom, pan, tooltips or hit-testing.
- **Chart catalogue.** No heatmap, box plot, radar, gauge, candlestick or 3D yet; see [ROADMAP.md](https://github.com/aavazir/insightdock-charts/blob/main/ROADMAP.md).
- **No error bars** in 1.0.
- **Text shaping.** Glyphs are drawn left to right from the TTF with no complex-script shaping and no right-to-left support. The embedded fonts cover ASCII or Latin-1/Latin-Ext; other scripts need a user-supplied TTF.
- **Source compatibility only.** Binary (ABI) compatibility between releases is not promised; see the [release process](https://github.com/aavazir/insightdock-charts/blob/main/docs/RELEASE.md).
- **Young project.** 1.0 is the first public release; the competitors above have years of field use.

---

## Maintenance

- Update this file when adding chart types or export formats.
- Do not add performance claims until [BENCHMARKS.md](https://github.com/aavazir/insightdock-charts/blob/main/docs/BENCHMARKS.md) has numbers that back them.
- Before each release, re-check each competitor's license and feature list against its own documentation.
