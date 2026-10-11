# InsightDock Charts roadmap

Last reconciled with the implementation: **2026-10-11**.

The goal is a competitive OSS C++ charting library for reports, analytics, and embedded devices.
Development order: **reliability and installation → common 2D workflows → analytics breadth →
ecosystem → dedicated finance and interaction workstreams**.

**Active target: finish M1.** Current execution is tracked in the
[M1 completion issue](https://github.com/aavazir/insightdock-charts/issues/5), with 14 remaining
work issues and explicit acceptance evidence. See [the M1 delivery plan](docs/planning/M1.md)
for ordering and release gates. M2–M4 and later work remain planned backlog without dates:
[M2](https://github.com/aavazir/insightdock-charts/issues/20),
[M3](https://github.com/aavazir/insightdock-charts/issues/21),
[M4](https://github.com/aavazir/insightdock-charts/issues/22),
[later workstreams](https://github.com/aavazir/insightdock-charts/issues/23).
GitHub issues hold live work status; this document retains scope and local validation history.

A checked item means its implementation and documentation exist in this checkout. It does not
mean a registry submission, deployment, or remote CI run has been confirmed. Release acceptance
requires the checks in [docs/RELEASE.md](docs/RELEASE.md). The detailed rationale and initial
inspection findings are recorded in [docs/GAP_REVIEW.md](docs/GAP_REVIEW.md).

## Completed foundations and chart coverage

### Core and rendering

- [x] C++17 core with vendored stb rendering/encoding; Make and CMake builds
- [x] `Chart`, `CartesianChart`, and backend-independent `DrawTarget` extension points
- [x] Structured export results: `ChartError` and `ChartResult<T>`
- [x] Light/dark themes, Chart.js/Tableau/colorblind-safe palettes
- [x] Per-series dash, marker shape, line width, and fill opacity on line/area/combo
- [x] Missing values: gap, interpolation, or zero policies
- [x] Embedded ASCII font, optional Latin font, UTF-8 decoding, injected TTFs, text measurement
- [x] Clone-based supersampling and concurrent export of built-in const charts
- [x] LTTB decimation, including indexed results and gap preservation

### Chart families and variants

- [x] Pie and doughnut, external labels and legends, optional in-slice data labels
- [x] Grouped, stacked, 100%-stacked, and horizontal bars
- [x] Line charts with area fill, spline and step variants, markers, and data labels
- [x] Area, stacked-area, and 100%-stacked area charts
- [x] Combo bar/line chart with secondary y-axis
- [x] Scatter and bubble charts with continuous linear x/y axes
- [x] Histogram binning and touching-bar chart preset
- [x] Linear and log value axes on Cartesian charts
- [x] Continuous DateTime x positioning on line and area charts
- [x] Category label skipping, truncation and 90-degree rotation; multiline Cartesian titles/labels
- [x] Horizontal/vertical reference lines and bands, including horizontal bar orientation

### Output, reporting, and OSS infrastructure

- [x] PNG, JPEG, raw RGBA, and SVG raster embedding
- [x] Native SVG and PDF for pie/doughnut, bar, line, area, combo, and scatter/bubble
- [x] Optional PoDoFo raster PDF and native vector PDF backend
- [x] Multi-page PDF report builder with tables, chart placement, pagination, and footers
- [x] CMake install/export, `InsightDock::Charts`, and pkg-config metadata
- [x] OpenWrt packaging sample and Make feature selection
- [x] vcpkg overlay template with optional PDF and Latin-font features
- [x] Core CI configuration for Linux x86-64/ARM64, macOS, Windows, static/shared builds
- [x] Unit/property tests, raster gallery regression, installed CMake consumer tests
- [x] ASan/UBSan CI and advisory clang-tidy
- [x] MkDocs site source, MrDocs API generation, examples/gallery and Pages workflow
- [x] MIT license, contribution guide, issue/PR templates, code of conduct, support/security policies
- [x] Benchmark harness and an initial recorded baseline

## Milestone 1 — reliability, installation, and OSS maintenance

Complete this milestone before adding new chart families.

### Formatting and layout correctness

- [x] UTC timestamp labels independent of the process timezone; invalid timestamp range checks
- [x] Continuous x-axis formatter callback for custom date formats and timezone presentation
- [x] Fraction-preserving numeric labels, tick-spacing precision, bounded fixed decimals
- [x] Locale-independent numeric tick/data-label formatting
- [x] Linear nice domains enclose both data extrema, including one-tick hints; preserve zero baselines
- [x] UTF-8 category truncation preserves whole code points
- [x] Top legends render above the plot with reserved space
- [x] UTF-8-safe token wrapping in pie callouts; preserve long labels and explicit empty lines
- [x] Measured legend rows, UTF-8 wrapping, right margin and bounded `+N more` overflow for
      pie, Cartesian and scatter/bubble charts; automatic pie geometry reserves legend space
- [x] Measured pie callout columns with vertical collision avoidance, leaders outside the circle,
      reserved title/legend space and deterministic omission of blocks that cannot fit
- [ ] General axis/data-label collision policies
- [x] Supersampling scales all mark sizes, dashes, widths, and data-label styles consistently
- [ ] Shared font measurement/output: PDF embedding/subsetting and SVG embedding/outline mode
- [x] Preserve explicit pie colors, opacity, stroke and label options in vector PDF chart exports
- [x] Share pie geometry/layout between raster, SVG, and PDF chart exports through `DrawTarget`
- [x] Locale-independent SVG geometry serialization
- [x] Overflow-safe pie proportions for large finite inputs; clamp fill opacity and preserve color alpha
- [ ] Migrate legacy `PieVectorOptions` report layout/overlap policies to shared pie options;
      preserve custom fonts and verify style parity for the direct PDF helpers

### Export contracts and compatibility

- [x] I/O failures report `BadPath`; raster and vector failures return structured results
- [x] PDF-unavailable errors for both raster and vector PDF requests
- [x] Apply dimension limits to native vector export as well as raster export
- [x] Custom charts without `clone()` fail supersampling without mutating const input
- [x] Shared-library identity tracks major.minor to match the source-only compatibility policy
- [ ] Public `validate()` diagnostics and strict/permissive input policies
- [x] Advertised native-vector/build capability queries and strict vector fallback policy
- [ ] Configurable resource budgets for image pixels, point counts, labels, and encoded output
- [x] Const `save()` / `encode()` overloads and explicitly documented exception contracts

### Installation and release readiness

- [x] Public CMake C++17 usage requirement; examples/tests default off when used as a subdirectory
- [x] Relocatable pkg-config metadata with feature definitions and the PDF link dependency
- [x] Ship license/third-party notices in installed packages
- [ ] Complete and verify vcpkg archive checksum against a published immutable release
- [ ] Submit vcpkg port; verify default/PDF/font features and static/shared consumers
- [ ] Document and verify FetchContent integration
- [ ] Confirm public repository, tagged release, release artifacts and deployed documentation
- [ ] Add reproducible release archive/checksum generation and automated release smoke checks
- [ ] Conan recipe and consumer verification

### Quality, organization, and security

- [x] Required PDF-enabled CI job configured with pinned PoDoFo and an installed PDF consumer
- [x] Focused reliability regressions for formatting, layout, export errors, and resource limits
- [x] Correct security scope: chart input and trusted fonts; no nonexistent JSON ingestion API
- [x] Pre-allocation limit for file-based font loading
- [x] Bundled font license text and third-party attribution for both embedded subsets
- [x] Maintainer/contributor responsibilities and contribution acceptance criteria documented
- [ ] Verify remote PDF/platform CI results; add Latin-font CI coverage
- [ ] Add independent SVG/PDF rendering comparisons and extracted-text checks
- [ ] Fuzz numeric input, UTF-8, rendering primitives and exporter boundaries
- [ ] Review/pin all CI actions and tooling; document dependency update policy
- [ ] Dependency review and automated source/static security checks
- [x] Docs build on pull requests in strict mode, with Pages write permissions limited to deploy
- [x] Pin direct documentation tool versions and enable reviewed dependency updates
- [ ] Lock transitive documentation dependencies and verify generated API coverage
- [ ] Refresh benchmarks with repeated runs, peak memory, output sizes and backend comparisons
- [ ] Remove stale claims from API/benchmark/comparison pages; date competitor source checks

Acceptance: a clean consumer installs the selected release, exports equivalent data/styles in
supported backends, receives actionable errors, and can find contribution, security and upgrade
instructions. Platform/feature CI and docs checks pass. Checkboxes above track local completion;
release and external publication gates stay open until verified.

Local validation of the first reliability batch (2026-10-10, Linux x86-64): all seven CTest checks
passed in static, shared, PDF-enabled and ASan/UBSan builds. Relocated CMake/pkg-config consumers,
PDF report examples, default/minimal Make builds, formatting and version checks passed. The
strict MkDocs build passed using the existing generated API reference; fresh API generation and
remote platform CI remain release gates.

Local validation of the pie/export batch (2026-10-10, Linux x86-64): all seven CTest checks
passed in core and PDF-enabled builds; all five unit/gallery/reliability checks passed with
ASan/UBSan and leak detection outside the tracing sandbox. The existing PNG gallery still passes.
Tests cover UTF-8 long-token wrapping, long-label preservation, empty lines, explicit colors/alpha,
PDF text extraction, large-value proportions and C/C++ locale changes (including decimal-comma
`en_DK.UTF-8`). The doughnut example exports PNG/SVG/PDF. Inkscape and Poppler renderings agree
with PNG at five sampled slice-interior/hole points within two channel levels; Poppler text
extraction confirms the title, in-slice percentages and rounded legend percentages. This is a local
spot check, not a complete automated visual comparison. Strict docs and formatting checks pass;
fresh API generation, full backend/font comparisons and remote CI remain open.

Local validation of the legend/layout batch (2026-10-10, Linux x86-64): all seven CTest checks
passed in core and PDF-enabled builds. Exported legend text stays within the tested canvases
without intersecting text rectangles for Top/Bottom/Right layouts, wrapped UTF-8 and explicit
newlines. Dense legends display overflow counts; scatter Top placement has a raster regression.
Linear domain tests now cover both extrema for positive/negative/zero-based ranges and one-tick
hints. Gallery references were visually reviewed and refreshed for the changed layouts and
corrected scatter bounds. Strict MkDocs, formatting and whitespace checks passed. General
axis/data-label collisions, consistent supersampling, font fidelity and remote platform CI remain open.


Local validation of the pie callout batch (2026-10-10, Linux x86-64): all seven CTest checks
passed in core and PDF-enabled builds. Exported text bounds cover dense clusters, wrapped and
explicit multiline labels, percentages on/off, both legend positions, later canvas resizing and
explicit geometry. Overfull columns retain larger slices; oversized blocks are omitted as a whole.
Leader segment checks verify no proper crossings and no paths through the pie interior. Crowded
PNG outputs and the refreshed pie gallery reference were visually reviewed. Strict MkDocs,
formatting and whitespace checks passed. Backend font fidelity and general axis/data-label
collision policies remain open.

Local validation of the supersampling batch (2026-10-10, Linux x86-64): all seven CTest checks
passed in core and PDF-enabled builds. Five unit/gallery/reliability checks passed with GCC
ASan/UBSan (leak detection disabled in the tracing sandbox). Tests compare mark and data-label
ink coverage at 1x/2x/4x across line, bar, area, combo, pie/doughnut, scatter and bubble; stroke
checks cover filled translucent interiors and dash continuity at 1x/2x/4x/8x. Native scatter
SVG/PDF checks reject embedded images. Corrected raster stroke gallery references and 1x/4x
style previews were visually reviewed. Strict MkDocs, formatting and whitespace checks passed.
Custom render-only charts retain the documented legacy scaling path. Font fidelity, strict vector
fallback/capability queries and remote platform CI remain open.

Local validation of the export-contract batch (2026-10-11, Linux x86-64): all eight CTest checks
passed across core, shared and PDF-enabled builds, including installed consumers. The six
unit/gallery/reliability/export-contract checks passed with GCC ASan/UBSan (leak detection
disabled in the tracing sandbox). Tests cover non-rendering capability queries, every built-in
family's strict native SVG/PDF output, existing custom draw overrides, actual draw rejection,
PDF-unavailable precedence, partial standalone output disposal, fallback clone/pixel checks,
const file/byte exports and concurrent const supersampling. PDF checks load the documents and
inspect image objects; embedding tests cover strict exceptions and allowed fallback. Strict
MkDocs (using the existing generated API reference), formatting and whitespace checks passed.
Font fidelity, input validation/resource policies, fresh API generation and remote CI remain open.

## Milestone 2 — common 2D workflow parity

1. [ ] Reusable `AxisOptions`: explicit bounds, reversal, visibility, custom ticks and formatters
2. [ ] Numeric continuous x positions on line/area without labeling the data as timestamps
3. [ ] Shared axis behavior for scatter: log/time options, reference overlays and per-series styles
4. [x] Native vector scatter/bubble through `DrawTarget`
   - [x] Detectable/strict raster fallback for custom charts
5. [ ] Measure/layout/draw stages with clipping across every backend
6. [ ] Arbitrary text angles, including 45-degree category labels
7. [ ] Adaptive/calendar-aware time ticks, timezone policy and readable dense-label behavior
8. [ ] Figure/subplot grid with aligned plot areas, shared axes/legends, captions and export
9. [ ] Public text/arrow annotations in data or normalized coordinates
10. [ ] Series IDs and replace/remove/update APIs preserving style and metadata
11. [ ] DPI and physical size, configurable PDF paper/orientation, transparency contract

Acceptance: a polished 2x2 analytics figure containing irregular numeric XY data, time data,
scatter and bars exports to PNG/SVG/PDF without manual pixel placement or unintended clipping.

## Milestone 3 — statistical and business breadth

In order, with documented data semantics and matching raster/vector output:

1. [ ] Asymmetric x/y error bars for bar, line and scatter
2. [ ] Lower/upper uncertainty ribbons and interval areas
3. [ ] Heatmap: sequential/diverging colormaps, normalization, colorbar and missing cells
4. [ ] Box plot: defined quartiles, whiskers, outliers and styling
5. [ ] Histogram custom edges, weights, density/probability and cumulative variants
6. [ ] Waterfall, floating/range bars, and Pareto preset
7. [ ] Polar/radar, Gantt, gauges and treemap, prioritized from adopter requests
8. [ ] Contour plots after heatmap and continuous-axis foundations

Each addition requires a public API guide, example, representative gallery entry, numerical and
rendering regressions, missing-value behavior, resource bounds and install/build integration.

## Milestone 4 — ecosystem and accessible output

- [ ] SVG title/description metadata and CSV/data-table companion export
- [ ] Patterns/markers and documented grayscale/colorblind reporting styles
- [ ] Optional complex-script shaping, RTL and fallback fonts
- [ ] C ABI and optional Python wrapper with lifecycle/error documentation
- [ ] WASM build and examples
- [ ] Pixel-aware decimation preserving gaps, spikes and original x positions
- [ ] Larger-data and embedded memory/throughput benchmarks
- [ ] Additional package distributions based on adopter demand

## Later workstreams

### Finance

- [ ] OHLC/candlestick with volume panels and shared time axes
- [ ] Validated SMA, EMA, RSI, MACD, Bollinger Bands, ATR and stochastic indicators
- [ ] Independent panel scales, diagonal trends and finance annotations
- [ ] Further price styles/indicators/drawing tools after demonstrated demand

### Interactive embedding

- [ ] Hit testing returning original series/point identities
- [ ] Consumer-driven zoom/pan hooks and incremental updates
- [ ] HTML/browser wrapper after interaction/data identity contracts exist

### Specialized formats and plots

- [ ] EPS/PostScript, C++ source-array output and other exports with a concrete use case
- [ ] Evaluate 3D separately from the 2D reporting scope

## Progress measures

| Area | Evidence |
|---|---|
| Chart coverage | Families, variants and missing-value semantics, not inflated variant counts |
| Export fidelity | Backend feature matrix, independently rendered comparisons and extracted text |
| Layout | Long labels, many series, dense/small canvases and multi-chart figures |
| Reliability | Structured errors, bounded resources, sanitizer/fuzz results |
| Performance | Repeated render/encode times, peak memory, output size, raw/decimated workloads |
| Adoption | Reproducible installs and minimal consumers across supported platforms/features |
| OSS maintenance | Current docs, reviewable changes, release/upgrade policy and dependency updates |

See [docs/COMPARISON.md](docs/COMPARISON.md) for alternatives and
[CONTRIBUTING.md](CONTRIBUTING.md) for how to work on these milestones.
