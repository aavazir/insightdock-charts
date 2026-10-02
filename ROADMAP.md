# InsightDock ROADMAP

Living document for InsightDock Charts for C++.

**Priority order:** fix architectural foundations → reach minimum viable public library → OSS launch on GitHub → chart breadth → finance depth → interactivity and ecosystem.

New chart types and export formats deliberately wait until the core architecture is solid. Adding area or scatter charts on top of a broken error-reporting model or a missing theme system means fixing those same issues across every chart type later.

Contributions for new chart types are welcome once Phase 0.5 foundations are in place — see [CONTRIBUTING.md](CONTRIBUTING.md).

---

## Known issues (bugs, not roadmap gaps)

Found during work on other features; not yet fixed. Pick these up opportunistically or dedicate
a pass to them — listed here so they don't get lost between sessions.

- **Chart.js/amCharts visual-polish features (2026-07-02) don't reach vector PDF/SVG output** —
  rounded bars, dashed lines, spline curves, marker shapes, data labels, reference lines, and the
  pie doughnut cutout all render correctly in PNG/JPEG/raster-embedded PDF/SVG, but the *native
  vector* paths (`src/podofo_vector.cc`'s `draw_pie_impl`, `src/svg_pie.cc`) are a separate
  rendering pipeline that doesn't know about any of this — including the doughnut cutout, which is
  pure geometry and would be cheap to add. Only `PieChart` has vector export at all today — closing
  this for bar/line is now a Phase 0.7 launch-gate item (see below), promoted from Phase 1 on
  2026-07-15: the README's own tagline promises "vector PDF output... from day one," which wasn't
  true for anything but pie.

- **Dash pattern / point marker shape are chart-wide, not per-series** — a deliberate v1 scope call
  (see `CHANGELOG.md` [Unreleased]), not a bug, but the most likely next ask: `LineChartOptions`/
  `AreaChartOptions`/`ComboLineOptions::dash`/`point_shape` apply to every series in the chart
  identically. Supporting e.g. a dashed "forecast" series next to a solid "actual" series in the
  same chart needs new fields on the shared `DataSeries` struct (`chart.h`), which `BarChart` would
  then also carry unused.

---

## Phase 0 — Initial foundations (v0.6.0) — done

- [x] Pie, bar, line charts
- [x] PNG/JPEG export via `ChartExporter`
- [x] PoDoFo raster PDF embed
- [x] Native vector PDF pies (`insightdock::charts::podofo`)
- [x] Rebrand to InsightDock (`insightdock::charts`, `insightdock/charts.h`)
- [x] OpenWrt-friendly Makefile core build
- [x] CI pipeline (core build + CTest)
- [x] Smoke tests (PNG/JPEG magic-byte checks)

---

## Phase 0.5 — Core foundations (v0.7.0) — done

Everything here is an architectural prerequisite. Skipping it means every future chart type or consumer inherits the same missing pieces and they become harder to fix as the surface grows.

### Error reporting ✓
- [x] `ChartError` enum + `ChartResult<T>` in `include/insightdock/error.h`
- [x] `ChartExporter::save` returns `ChartResult<void>`; `encode` returns `ChartResult<std::vector<uint8_t>>`
- [x] Distinguishes `BadPath`, `RenderFailure`, `EncodeFailure`, `PdfUnsupported`, `InvalidInput`
- [x] `explicit operator bool()` keeps `if (!save(...))` pattern working

### Theme / style object ✓
- [x] `ChartTheme` in `include/insightdock/theme.h`; `Chart::apply_theme(theme)` in `chart.h`
- [x] `ChartTheme::light()` and `ChartTheme::dark()` built-in presets
- [x] Per-chart `ChartOptions` fields still take precedence after `apply_theme`

### Scale architecture upgrade ✓
- [x] `ScaleType` enum (`Linear`, `Log`, `DateTime`) added to `scale.h`
- [x] `Scale::set_type()` / `Scale::type()` accessor
- [x] Log scale implementation, including `Scale::log_nice()` nice-domain factory
- [x] DateTime scale implementation
- [x] Wired into charts (not just the `Scale` primitive) — `CartesianOptions::y_scale_type`
  (Log, on Bar/Line/Area/Combo) and `x_scale_type` + `set_x_values()` (DateTime, continuous
  positioning on Line/Area). Previously checked off here based on the primitive alone while
  no chart type actually exposed either mode — see `CHANGELOG.md` [Unreleased].

### Axis tick formatting ✓
- [x] `TickFormat` enum (`Raw`, `SI`, `Currency`, `Percent`) in `cartesian_chart.h`
- [x] `CartesianOptions::y_tick_format` and `CartesianOptions::custom_y_formatter` callback
- [x] `format_y_tick()` helper in `cartesian_chart.cc` replaces hard-coded `%.0f`

### Overlay / plugin hook in CartesianChart ✓
- [x] `virtual void draw_overlay(PlotContext&) const {}` in `CartesianChart`
- [x] Called after `draw_series()` in `CartesianChart::render()`
- [x] Default no-op; override to add reference lines, annotations, combo geometry

### Public text measurement API ✓
- [x] `Font::TextMetrics` struct (`width`, `height`) and `Font::measure(text, size)` in `font.h`
- [x] Implemented in `font.cc` via proportional scaling (no re-initialisation)

### Bug fix: inverted Y-axis ✓ (bonus — found during unit testing)
- [x] `cartesian_chart.cc` `y_scale` construction had `pixel_min`/`pixel_max` swapped
- [x] Bar and line charts now render with higher values at the top (standard orientation)
- [x] `ImageBuffer::pixel()` now returns `nullptr` on OOB as the header docs promised

### PDF table helper ✓
- [x] `include/insightdock/pdf/report.h` and `src/pdf_report.cc` provide `PdfReportBuilder` (compiled only when `INSIGHTDOCK_WITH_PODOFO=ON`)
- [x] Public API: column definitions, wrapping, alignment, section headings, page footers
- [x] Required before multi-page PDF report builder (Phase 1) is possible

### Label collision avoidance for Cartesian charts ✓
- [x] `draw_axes_and_grid()` now computes a skip `step` so adjacent labels do not overlap
- [x] Measures widest label, doubles step until `step × slot_width ≥ label_width + 4px`

### Configurable Y-axis tick density ✓
- [x] `CartesianOptions::min_y_labels` (default `0`) — minimum requested y-axis ticks; retries with a doubled hint if nice-rounding falls short
- [x] `CartesianOptions::max_y_labels` (default `0` = ~6 historical default) — target y-axis tick count passed to `Scale::linear_nice` and `nice_ticks`
- [x] Setting both to the same value targets an exact count; inverted constraints (`min > max`) are swapped internally
- [x] `y_tick_hint()` helper in `cartesian_chart.cc` encapsulates the normalization + fallback to `6`
- [x] Six unit tests in `unit_tests.cc` — tick density increases with higher hint, render smoke tests for max/min/equal/inverted/default

### Unit tests for core components ✓
- [x] `tests/unit_tests.cc` — Scale, Palette, ImageBuffer, ChartResult, ChartTheme, Font::measure
- [x] Registered as `unit_tests` CTest target alongside existing `smoke_render_png`
- [x] All 27 tests pass

### Sanitizer and static analysis CI ✓
- [x] `sanitizers` job in CI (ASan + UBSan via clang)
- [x] clang-tidy job — `lint-tidy` in CI (`continue-on-error: true` until proven stable)

### PoDoFo CI job made automatic ✓
- [x] PoDoFo build runs automatically in CI (no longer manual-only)
- [x] Runs `podofo_report_example` and checks PDF is non-empty

---

## Phase 0.6 — Minimum viable public library

The goal of this phase is to reach the point where the library is genuinely useful to a stranger on GitHub: enough chart types, a clean install path, and outputs they can see before reading any docs.

### SVG export ✓
- [x] Add `OutputFormat::Svg` and implement `ChartExporter::save` / `encode` for SVG
- [x] Core SVG writer as a new `src/svg_writer.cc` with no external dependency (write XML directly, same as stb for PNG)
- [x] Start with SVG raster embed (chart rendered to PNG, base64-inlined in SVG) as a first cut; follow with native SVG paths for pie in a second pass
- [x] Native SVG paths for pie (`ExportOptions::svg_backend = SvgBackend::Vector`, `src/svg_pie.cc`) — sectors as `<path>` arcs (`<circle>` for the single-100%-slice edge case), leader-line labels, legend. Bar/line still raster-embed in `Vector` mode — tracked as a Phase 0.7 launch-gate item below ("Native SVG paths for bar and line")

### Area and stacked-area chart ✓
- [x] `AreaChart` subclasses `CartesianChart`, implements `draw_series()` with polygon fill from the series line down to the x-axis baseline
- [x] Stacked-area variant (series polygons stack, not overlap)
- [ ] Critical for network traffic and time-series dashboards — the primary OpenWrt use case

### Scatter and bubble chart ✓
- [x] `ScatterChart` (`include/insightdock/scatter_chart.h`, `src/scatter_chart.cc`) derives directly from `Chart` — not `CartesianChart` — since both axes are continuous numeric `Scale`s (Phase 0.5 upgrade), not a category list
- [x] Bubble variant: `ScatterPoint::radius` drives circle radius, scaled between `ScatterOptions::min_bubble_radius`/`max_bubble_radius`
- [ ] Straightforward to implement; rounds out the chart types needed for analytics dashboards

### Combo chart ✓
- [x] `ComboChart` overlays bar and line series on the same Cartesian canvas (`src/combo_chart.cc`, `include/insightdock/combo_chart.h`)
- [x] Uses the Phase 0.5 `draw_overlay()` hook
- [x] Optional second Y axis on the right — `add_bar_series()`/`add_line_series()` take a `secondary_axis` flag; `CartesianChart` gained generic `has_secondary_y_axis()`/`compute_y2_min()`/`compute_y2_max()` hooks plus `CartesianOptions::y2_axis_label`/`y2_tick_format`/`custom_y2_formatter`, so other `CartesianChart` subclasses can opt in the same way later

### Visual regression baseline ✓
- [x] Generate reference PNGs for all existing chart types (pie, bar grouped, bar stacked, line, area, combo, scatter, bubble) and commit them to `docs/gallery/`
- [x] Add a CI job that re-renders and compares against the reference with a pixel-diff threshold (`gallery_regression` CTest)
- [ ] Without this, any rendering change is invisible in CI; this must exist before the chart count grows further

### Gallery populated ✓
- [x] `docs/gallery/` populated with one PNG per chart type and variant, generated by CI
- [x] Referenced from README — pie/bar/line/area samples embedded inline, not just linked

### MrDocs API reference in CI ✓
- [x] Add a CI job (`docs` workflow) that runs `mrdocs docs/mrdocs.yml` (mirrors the `lint-tidy` validation gate)
- [x] Job fails when MrDocs exits non-zero (catches undocumented public symbols)
- [x] mrdocs ships x86_64-only Linux binaries; runs natively on GitHub's `ubuntu-latest` runners, no container/emulation needed (`scripts/ci_docs_mrdocs.sh`)
- [x] Theme the HTML output — stock mrdocs 0.8.0 ships with zero CSS (plain browser-default page). Added `docs/mrdocs-theme/wrapper.html.hbs`, copied over the installed template before each run (see CHANGELOG). Verified visually via headless Chrome screenshots locally.
- [x] Publish output to GitHub Pages via `.github/workflows/docs.yml` (`actions/deploy-pages`)

### One-stop doc site (MkDocs Material + embedded API reference) ✓
- [x] Pulled forward from `FUTURE.md`'s post-v1.0.0 plan — `mkdocs.yml` + `docs-site/` (Home, Getting started, Examples, API Reference, Comparison), built via `scripts/build_docs_site.sh`
- [x] Examples page shows each example's full source (via `pymdownx.snippets`, transcluded straight from `examples/*.cc` — can't drift out of sync) alongside its rendered output from `docs/gallery/`
- [x] Comparison page transcludes `docs/COMPARISON.md` directly (single source of truth, not a copy)
- [x] API Reference nav entry points at `docs/reference/` (copied into `docs-site/api/` by the build script) — MkDocs passes non-markdown subdirectories through to the build verbatim, confirmed empirically
- [x] Verified visually with headless Chrome screenshots (home, examples with images, API reference, comparison) — not just "it built without errors"
- [ ] Known seam: `api/` pages don't carry the Material nav chrome (different look/feel when you click through) — acceptable for now, see `DOCUMENTATION.md`
- [ ] `comparison.md`'s transcluded content has a few dangling relative links (`BENCHMARKS.md`, `../ROADMAP.md`, `FUTURE.md` — not part of the site's nav); non-fatal (mkdocs warns, doesn't fail the build) but worth deciding whether to add those as site pages too or strip the links for the site context
- [x] Nav expanded from 5 flat entries (Home, Getting started, Examples, API Reference, Comparison) to a categorized structure closing the "usage guide" gap: `concepts.md` (architecture/extension points), `guides/` (`usage.md` transcludes `docs/API.md`; new `pdf-reports.md`, `troubleshooting.md`), `chart-types/` (one page per chart type — pie/bar/line/area/combo/scatter — with full options tables), and `changelog.md` (transcludes `CHANGELOG.md`). Previously the richest usage content (`docs/API.md`: error handling, theming, tick formatting, extending) existed only in the repo, never reachable from the published site.
- [ ] Same known seam as above applies to the new transcluded pages (`guides/usage.md` ← `docs/API.md`, `changelog.md` ← `CHANGELOG.md`): a few relative links in the source files resolve correctly on GitHub but not when transcluded into the nested site path — accepted for now, consistent with the existing `comparison.md` precedent

### Windows CI feasibility verified
- [ ] Add a GitHub Actions job using a Windows runner or MSVC cross-compile image
- [ ] Goal: confirm the core library and examples build cleanly; not a full test run yet

### Benchmark baseline documented ✓
- [x] Add `benchmarks/bench_render.cc` with a simple timing harness (no external dependency — `std::chrono`; stdout table rather than CSV, sufficient for the manual one-time recording workflow below). `INSIGHTDOCK_BUILD_BENCHMARKS` CMake option, default `OFF`.
- [x] Measure: pie/bar/line render (1k/10k/100k points for line; 50×10 for bar; 20 slices for pie), PNG encode at 640×400, PoDoFo PDF raster embed
- [x] Record one set of numbers on a fixed machine and commit to `docs/BENCHMARKS.md`
- [ ] Required before any performance claim can be made publicly

### Decimation helpers ✓
- [x] `lttb_decimate()` / `lttb_decimate_indexed()` (`include/insightdock/decimate.h`, `src/decimate.cc`) — largest-triangle-three-buckets (LTTB) algorithm for reducing large datasets to a target point count without losing visual shape
- [x] Used as a pre-processing step before passing data to `LineChart` or `AreaChart` — `lttb_decimate_indexed()` returns the original indices alongside decimated values so the category label list can be sliced to match
- [ ] Essential for the "100k point" benchmark scenario and OpenWrt memory constraints

### Chart.js/amCharts-style visual polish ✓
Requested to close the gap between InsightDock's plain default styling and what Chart.js/amCharts users expect out of the box — scoped to the subset that makes sense for a static raster/PDF/SVG-raster renderer (no tooltips/zoom/animation; those stay in Phase 4).
- [x] Data labels on marks — `DataLabelOptions` (new `include/insightdock/chart_style.h`) on all 6 chart types' Options structs, default off
- [x] Pie doughnut cutout — `PieOptions::inner_radius_ratio` (raster only; native vector PDF/SVG pie paths don't draw it yet)
- [x] Bar rounded corners + border — `BarOptions` (promoted from scattered private members), shared with `ComboChart`'s bars via `detail::fill_bar()`
- [x] Point marker shapes — `PointMarkerShape` (Circle/Square/Triangle/Cross) on Line/Area/Combo/Scatter, chart-wide not per-series
- [x] Dash patterns — `DashPattern` on Line/Area/Combo lines and on `ReferenceLine`, chart-wide not per-series
- [x] New `ImageBuffer` primitives backing the above: `fill_rounded_rect()`, `draw_dashed_line()`/`draw_dashed_polyline()`
- [ ] Per-series (rather than chart-wide) dash pattern / marker shape — would need new fields on the shared `DataSeries` struct; deferred, see `CHANGELOG.md`
- [ ] Native vector PDF/SVG support for the new bar/line polish (rounding, dash, spline, markers, labels) — only pie has vector export today; tracked as a Phase 0.7 launch-gate item below ("Native vector PDF for bar and line charts" / "Native SVG paths for bar and line")

### Axis and orientation completeness ✓
Identified during a v1-scope audit of the existing chart types against commonly-expected charting-library features (2026-07-15) — see `CHANGELOG.md` [Unreleased] for full detail.
- [x] Log y-axis wired into Bar/Line/Area/Combo (`CartesianOptions::y_scale_type`)
- [x] Continuous DateTime x-axis wired into Line/Area (`CartesianOptions::x_scale_type` + `set_x_values()`)
- [x] Horizontal bar orientation (`BarOptions::orientation`) — rankings/leaderboards, long category names
- [ ] Horizontal orientation on ComboChart — not attempted; line-series geometry has no horizontal equivalent
- [ ] `ReferenceLine`/`ReferenceBand` support in horizontal bar mode — orientation semantics assume a vertical value axis; explicitly out of scope for v1, silently skipped rather than rendered wrong

---

## Phase 0.7 — OSS launch gate (GitHub)

Do not publish to GitHub until every item here is checked. This is the public first-impression checklist.

- [x] Company-internal code and CI (`fuse/`, GitLab CI) removed entirely from the repo
- [ ] GitHub repo created — `avazir/insightdock` (personal account, not a separate org)
- [x] GitHub Actions CI (build, test, sanitizers, clang-tidy, MrDocs) — `.github/workflows/ci.yml`, `.github/workflows/docs.yml`
- [ ] vcpkg port submitted (`insightdock-charts`) — primary Windows and CMake install path
- [x] `pkg-config` `.pc` file installed by `cmake --install` — `cmake/insightdock.pc.in`, installed to `lib/pkgconfig/insightdock.pc`
- [x] `CHANGELOG.md` placeholder URLs replaced with real GitHub URLs
- [ ] Public MkDocs Material site deployed to GitHub Pages (quick start, gallery, comparison) — workflow in place (`.github/workflows/docs.yml`), unverified until the repo exists on GitHub
- [x] `find_package(InsightDock)` tested end-to-end from a fresh install — manual verification (clean `cmake --install` to a temp prefix + a throwaway consumer `CMakeLists.txt` doing `find_package(InsightDock CONFIG REQUIRED)`); caught and fixed a real bug where the exported target was `InsightDock::InsightDockCharts` instead of `InsightDock::Charts` (missing `EXPORT_NAME` property). Not yet automated in CI.
- [x] OpenWrt packaging sample — `packaging/openwrt/Makefile`, a standard `package.mk`-based feed package wired to the upstream Makefile's `CHART_TYPES`/`EXPORT_FORMATS`/`FONT_MODE` knobs
- [ ] **Native SVG paths for bar and line** (follow-on to Phase 0.6 SVG raster embed) — promoted from Phase 1 on 2026-07-15. Only `PieChart` has native vector SVG today (`src/svg_pie.cc`); bar/line raster-embed even in `SvgBackend::Vector` mode. Two reasons to close this before launch rather than after: (1) it's blocking the launch tagline's "vector PDF output... from day one" claim from being false for every chart type but pie; (2) it's a real competitive differentiator, not just a consistency fix — `docs/COMPARISON.md` already stakes out "PDF vector in reports: Rare" against sciplot-style/header-only plotters and ImPlot/ECharts/Qt Charts, and that claim is much stronger landing with bar/line covered on day one instead of "pie only, more later."
- [ ] **Native vector PDF for bar and line charts** (currently only pie gets vector PDF, via `src/podofo_vector.cc`'s `draw_pie_impl`) — promoted from Phase 1 alongside the SVG item above, same underlying gap and same two reasons. `AreaChart`/`ComboChart` are not in scope for this pass — they still raster-embed in vector mode after this lands, same as today; revisit separately if the bar/line work generalizes cheaply.

---

## Phase 1 — Chart breadth (post-OSS, ChartDirector parity goal)

- [ ] Error bars (x and y; works with bar, scatter, line)
- [x] Spline / cubic-interpolated lines (`LineChartOptions`/`AreaChartOptions`/`ComboLineOptions::spline`, Catmull-Rom via `detail::catmull_rom_expand()`) — pulled forward into the Chart.js/amCharts visual-polish pass; step lines remain open
- [ ] Step lines (option on `LineChartOptions`)
- [ ] Box-whisker chart
- [ ] Heatmap (2D grid, color-mapped; requires `ImageBuffer` color-scale helper)
- [ ] Polar / radar chart
- [ ] Gantt chart (horizontal bars against a time axis; uses Phase 0.5 DateTime scale)
- [x] Multi-page PDF report builder (`insightdock/pdf/report.h` from Phase 0.5 + page layout engine) — `PdfReportBuilder::draw_chart()` / `draw_chart_flow()` embed any `Chart` (bar/line/pie) at mm coordinates with automatic pagination via `ensure_space()`; demonstrated in `examples/podofo_report_example.cc`
- [ ] Native vector PDF/SVG for `AreaChart`/`ComboChart` — follow-on to the Phase 0.7 bar/line vector work if it generalizes cheaply; not required for launch (only bar/line block the launch tagline's vector-PDF claim, since area/combo aren't mentioned there)
- [ ] PostScript / EPS export (`OutputFormat::Eps`) — dependency-free; write PostScript directly from `ImageBuffer` pixels; useful for LaTeX `\includegraphics` workflows and print pipelines
- [ ] C++ source array export (`OutputFormat::CppArray`) — emit a `.h` / `.cc` file containing the rendered RGBA or PNG bytes as a `constexpr uint8_t[]`; useful for embedded firmware and LVGL display buffers where the chart is baked at build time

---

## Phase 2 — Finance foundation (StockChartX parity goal)

- [ ] OHLC / candlestick chart (uses DateTime scale from Phase 0.5)
- [ ] Volume panel (bar chart beneath price panel with shared time axis)
- [ ] Multi-panel finance layout (composable vertical panels with linked time axis)
- [ ] First 10–20 technical indicators: SMA, EMA, RSI, MACD, Bollinger Bands, ATR, stochastic
- [x] Basic annotations and trend lines — `CartesianOptions::reference_lines`/`reference_bands` (horizontal + vertical; diagonal not yet supported), drawn via the Phase 0.5 `draw_overlay()` hook as originally designed, scoped to Bar/Line/Area/Combo (`CartesianChart` subclasses only — Pie/Scatter have no overlay hook). Pulled forward into the Chart.js/amCharts visual-polish pass; diagonal trend lines remain open.

---

## Phase 3 — Finance depth

- [ ] Additional price styles: Renko, Kagi, Point & Figure, Heikin Ashi, EquiVolume, Darvas
- [ ] 50+ validated technical indicators
- [ ] Fibonacci retracements, Gann fans, channel drawing tools
- [ ] Multi-panel layout with independent Y scales per panel

---

## Phase 4 — Ecosystem and interactivity

- [ ] Hit-testing API: `Chart::hit_test(x, y)` returns the data point or slice at pixel coordinates — prerequisite for any interactive embedding
- [ ] Zoom and pan hooks (callback-based; consumer drives the interaction loop)
- [ ] Real-time / incremental update: `Chart::update_series(index, new_values)` without full re-render where possible
- [ ] C binding (`insightdock_c.h`) for FFI from Python, Rust, Go
- [ ] WebAssembly (WASM) build path via Emscripten — core is STL-only, no GUI deps; feasible
- [ ] HTML Canvas/SVG wrap (`OutputFormat::Html`) — emit a self-contained `.html` file with the chart rendered into an `<svg>` or `<canvas>` element; hook into native browser tooltips and zoom via a small embedded JS runtime; builds on the Phase 0.6 SVG foundation
- [ ] Contour plot
- [ ] Published performance benchmarks (follow-on to Phase 0.6 baseline)

---

## Measurement parameters

| Area | Metric |
|------|--------|
| Chart coverage | Supported chart families and variants |
| Export coverage | PNG, JPEG, PDF raster, PDF vector, SVG (native + raster embed), EPS/PostScript, C++ array, HTML wrap |
| Render quality | Label overlap, legend layout, visual regression pass rate |
| Performance | Render/export time at 1k, 10k, 100k, 1M points |
| API simplicity | LOC for common tasks; examples per feature |
| Portability | OpenWrt, Linux distros, Windows, WASM |
| Report readiness | PDF embed, predictable dimensions, headless, table helper |
| Finance readiness | Price styles, indicator count, multi-panel |
| Error handling | No silent failures; all public methods report errors |

---

## Known gaps vs ChartDirector

Tracking what still remains after the phases above:

- 3D charts — not planned (out of scope for headless reports)
- Full interactivity (zoom, tooltips, hit-testing) — Phase 4
- Large-data benchmarks — Phase 0.6 baseline, Phase 4 full story
- SVG — Phase 0.6
- Contour — Phase 4

## Known gaps vs StockChartX

- Finance price styles — Phase 2
- Technical indicators and drawing tools — Phase 2–3
- Multi-panel finance layout — Phase 2
- Tick/realtime performance — Phase 4

## Known gaps vs open-source alternatives

- Chart count vs ImPlot/Qt/ECharts breadth — Phase 1 closes most of this
- SVG export vs PLplot — Phase 0.6 (top priority)
- PostScript/EPS vs PLplot — Phase 1
- HTML interactive wrap vs Chart.js / ECharts — Phase 4
- C++ source array export — Phase 1
- Live interactivity vs ImPlot, Qt Charts, Chart.js — Phase 4
- Python/notebook ergonomics vs matplotlib — C binding in Phase 4 opens a ctypes path

See [docs/COMPARISON.md](docs/COMPARISON.md) for the full matrix.
