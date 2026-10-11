# OSS gap review — 2026-10-10

This is the initial inspection snapshot. Follow [the roadmap](../ROADMAP.md) and
[the changelog](../CHANGELOG.md) for fixes completed after the review.

InsightDock Charts has a credible foundation for native C++17 chart generation in servers, reports,
and embedded devices. Its most useful next milestone is reliable coverage of common 2D reporting
and analytics workflows, with consistent output across PNG, SVG, and PDF.

This review examined public headers, rendering and export implementations, packaging, CI,
existing tests, documentation, and the committed time-series gallery image. Competitor features
were checked against official sources linked below. The initial review ran no builds, tests, or
benchmarks; its code findings are based on inspection. Public deployment, published releases,
and registry availability were not confirmed. The review itself added only this document.

## Existing strengths

- Pie/doughnut, grouped/stacked/100%/horizontal bar, line/step/spline, area, combo with a second
  value axis, scatter/bubble, and histogram already cover many reporting tasks.
- The shared `DrawTarget` path gives bar, line, area, and combo native SVG/PDF output.
- Themes, palettes, per-series styles, reference lines/bands, missing-value policies, font
  injection, supersampling, and decimation are already present.
- MIT licensing, vendored core dependencies, Make/CMake builds, installable CMake targets,
  consumer tests, sanitizers, and a platform matrix provide a useful adoption foundation.
- Documentation, gallery regression tests, contribution guidelines, and security policy exist.

## Fix these before adding more chart families

| Priority | Finding and evidence | Recommended change |
|---|---|---|
| High | `src/tick_format.cc:46` formats timestamps with `localtime_r/localtime_s`. Identical timestamps produce different labels in different machine timezones. | Default to UTC; add an explicit timezone policy and customizable x-axis formatting. Narrow the cross-machine determinism claim until this is resolved. |
| High | `src/tick_format.cc:57` rounds every Raw value with magnitude at least 1 to an integer and smaller values to two decimals. Thus 1.2 becomes `1`, and 0.001 becomes `0.00`. | Select precision from tick spacing, preserve meaningful fractions, support scientific notation, and ensure adjacent ticks have distinct labels. |
| High | `src/podofo_target.cc:155` substitutes text through WinAnsi; its default PDF font is Helvetica. `src/svg_target.cc:238` names system fallback fonts rather than embedding `ChartOptions::font`. | Use the same font for measurement and output. Embed/subset fonts in PDF and provide embedded SVG fonts or an explicit glyph-outline mode. Expose missing-glyph diagnostics. |
| High | `src/podofo_vector.cc:533` reduces pie slices to label/value pairs; `slice_fill()` reconstructs colors from palette indices. Explicit slice colors are lost. Pie also uses separate raster/SVG/PDF layout paths. | Preserve each slice's color and styling. Move pie geometry/layout into a shared representation or extend `DrawTarget` with the required path primitives. |
| High | `.github/workflows/ci.yml` never enables `INSIGHTDOCK_WITH_PODOFO`; PDF tests in `tests/unit_tests.cc` are conditional on that feature. | Add a required PDF-enabled job with a pinned supported PoDoFo version, report examples, and an installed consumer. Expand platform coverage when the initial job is reliable. |
| High | `packaging/vcpkg/insightdock-charts/portfile.cmake:6` has `SHA512 0`. This is a template rather than a finished reproducible port. | Pin a real release archive checksum, verify default/PDF/font features, and publish the port. Add documented FetchContent integration; consider Conan next. |
| High | `CMakeLists.txt:78` uses a major-only shared-library version after 1.0 while `docs-site/guides/thread-safety-and-abi.md` explicitly allows ABI breaks in minor releases. | Choose an ABI policy: preserve ABI within a shared-library identity, or change that identity whenever ABI changes. Include an upgrade compatibility check before shipping binaries. |
| Medium | `src/cartesian_chart.cc:411` handles Right legends separately, then places both Top and Bottom below the plot. Horizontal legends are one unwrapped row; right legends reserve a fixed 100 px. | Implement Top placement, measured legend dimensions, row wrapping, and overflow handling. Test long names and many series. |
| Medium | `src/export.cc:95` scales common options and selected pie options for supersampling, but does not scale Cartesian line widths, point radii, dash lengths, or data-label font sizes. | Scale all pixel-space styles through a renderer transform or a shared style-scaling contract, so higher sampling improves quality without shrinking marks. |
| Medium | `src/export.cc:366` reports only an empty path as `BadPath`; other write failures become `EncodeFailure`. PDF operations can throw outside the result contract; vector export branches bypass the raster dimension check. | Define consistent validation, resource limits, I/O errors, and exception behavior for every backend. Add `validate()` with a strict mode and a permissive mode that reports skipped/repaired input. |
| Medium | Installed CMake targets do not advertise `cxx_std_17` through `target_compile_features`; `cmake/insightdock.pc.in` does not express the optional PDF link dependency. | Propagate the language requirement and required compile definitions; support static PDF linking through pkg-config and relocatable prefixes. Make examples/tests opt-in when embedded as a dependency. |

These are inspection findings, not claims that every affected scenario was reproduced. Several
deserve small focused regression cases when they are implemented.

Also review UTF-8 shortening: `truncate_to_width()` in `src/cartesian_chart.cc:37` can retain the
leading byte of a multibyte character after removing its continuation bytes. Pie token wrapping
also walks bytes. Both should shorten or wrap at valid code-point boundaries.

## Additions with the largest practical value

| Order | Addition | Current limitation | Completion criterion |
|---|---|---|---|
| 1 | Reusable axis configuration | No normal public options for explicit bounds, custom ticks, reversal, or continuous numeric x positions on line/area. DateTime is the only mode that consumes their x-values; scatter has its own linear-only options. | Shared `AxisOptions` across Cartesian/scatter: min/max, linear/log/time, explicit ticks, formatter, reversal, and visibility. Demonstrate irregular numeric XY data without pretending it is time. |
| 2 | Complete vector scatter/bubble | Scatter derives directly from `Chart`, implements `render()` only, and falls back to an image for vector exports. It also lacks Cartesian reference overlays and formatters. | Scatter draws through `DrawTarget`, uses shared axes and per-series styles, and produces native SVG/PDF marks and text. Unsupported custom chart backends are detectable through capabilities or a strict vector policy. |
| 3 | Automatic layout and clipping | Title/category sizing, measured legends with bounded overflow and pie callout collision avoidance exist; value ticks and general label collisions still need layout work. `DrawTarget` has no plot clipping or arbitrary-angle text contract. | A measure/layout/draw pass, backend clipping, 45-degree/arbitrary label angles, adaptive time ticks, and collision policies for mark labels. Long labels and dense data fit at small and large export sizes. |
| 4 | Figures and subplots | Charts export individually. PDF placement/pagination is available, but there is no reusable multi-chart figure for PNG/SVG/PDF. | Grid layout, aligned plot areas, shared axes and legends, captions, and one-call export of a 2x2 report figure in all three formats. |
| 5 | Error bars and uncertainty bands | No error bars or general lower/upper confidence ribbons. A fill-to-baseline line option does not represent uncertainty. | Asymmetric x/y error bars on relevant charts and a lower/upper ribbon on line plots, with matching vector output. |
| 6 | Heatmap and color scales | No matrix/grid chart, continuous colormap, or colorbar. | Sequential/diverging maps, missing cells, normalization, colorbar ticks, annotations, and controllable vector versus raster export for large grids. |
| 7 | Box plot and richer histogram | Histograms offer equal-width counts only; no box plot. | Defined quartile/whisker/outlier behavior; histogram custom edges, weights, density/probability, and cumulative output. |
| 8 | Report-oriented chart breadth | No waterfall, floating/range bars, radar/polar, gauge, Gantt, or treemap. | Start with waterfall and floating/range bars, then choose further types from actual adopter requests. Pareto can build on bars plus a cumulative line. |
| 9 | Annotation and series lifecycle APIs | Reference lines/bands and subclass overlays exist; arbitrary public text/arrows and per-series replacement/removal do not. | Text/arrow annotations in data or normalized coordinates; series IDs and replace/remove/update operations that preserve styling and metadata. |
| 10 | Accessible, international output | A colorblind palette exists, but there is no SVG title/description API or data-table export. Complex shaping/RTL is explicitly absent. | Semantic SVG metadata, CSV/table companion output, patterns/markers that distinguish series in grayscale, and optional shaping/fallback fonts for international reports. |

Physical size and output resolution should also be explicit: add DPI/mm/inch options, configurable
PDF paper size/orientation, and documented transparency behavior. Existing PDF export fits charts
on A4, and `PdfPageLayout` hardcodes A4 constants. Report users need Letter, landscape, and
chart-sized PDF pages as well.

## Comparison targets

| Reference | What it establishes for this review |
|---|---|
| [Matplot++ official documentation](https://alandefreitas.github.io/matplotplusplus/) | C++ expectations include error bars, heatmaps, box plots, explicit axis limits, annotations, subplots, tiled layouts, and colorbars. |
| [PLplot official feature summary](https://plplot.sourceforge.net/) | Headless plotting already includes multi-graph pages, vector file formats, language bindings, and complex-script support on suitable devices. Vector output alone is insufficient differentiation. |
| [sciplot official documentation](https://sciplot.github.io/) | Simple C++ XY plotting and figure/canvas composition are useful ergonomic benchmarks. Its Gnuplot runtime dependency creates a clear opportunity for InsightDock Charts' in-process rendering. |
| [ChartDirector official gallery](https://www.advsofteng.com/gallery.html) | A longer-term reporting catalogue includes box-whisker, heatmap, Gantt, polar/radar, treemap, meters, and finance. This is a breadth reference, not a single-release requirement. |
| [Chart.js official axis documentation](https://www.chartjs.org/docs/latest/axes/cartesian/linear.html) | Explicit bounds, reversal, tick control, clipping, and rotation are normal configuration features. |
| [Matplotlib official gallery](https://matplotlib.org/stable/gallery/index.html) | Visual and statistical references include uncertainty bands, weighted/density histograms, heatmaps, shared axes, and constrained layout. |
| [ImPlot official project](https://github.com/epezent/implot) | Interactive desktop exploration is a separate capability requiring interaction and embedding APIs. |

Refresh `docs/COMPARISON.md` with dated primary-source links and feature-level comparisons. Qt's
[current documentation](https://doc.qt.io/qt-6/qtcharts-index.html) says Qt Charts is deprecated
since Qt 6.10 and recommends Qt Graphs for new projects; the current comparison should include
that development.

## Suggested delivery sequence

**First milestone: dependable core and installation.** Resolve the high-priority findings,
legend placement, supersampling semantics, and stale documentation. Publish a working installation
path and enable PDF CI. Completion means a clean consumer can install the selected release and
export charts with consistent data, colors, fonts, labels, and reported failures.

**Second milestone: common 2D parity.** Deliver shared axis options, vector scatter, clipping,
layout improvements, and figures/subplots. Completion means a user can create a polished 2x2
analytics figure with irregular XY data, time data, and a shared legend in PNG/SVG/PDF without
manual pixel positioning.

**Third milestone: analytics breadth.** Add error bars/ribbons, heatmaps/colorbars, box plots,
and richer histograms, followed by waterfall/range bars. Require examples, documentation, missing
data semantics, and matching backend behavior for each feature.

**Fourth milestone: ecosystem driven by adoption.** Add an optional C ABI and Python wrapper,
Conan packaging, and a WASM build when there is demand. Hit testing and zoom/pan belong to an
explicit interactive workstream. Candlesticks and indicators belong to an explicit finance
workstream after shared time axes and subplot composition are established.

The existing roadmap devotes substantial space to finance and interactivity while basic axis,
layout, and figure composition remain incomplete. Move those foundational user capabilities
ahead of finance depth. Treat EPS, source-array output, 3D, and extra finance styles as additions
that need a concrete adopter or workflow.

## Evidence to add as the project matures

- Extend the existing raster gallery regression suite with long labels, many series, small
  canvases, decimal/log axes, UTF-8, negative stacks, and multi-chart figures.
- Render SVG/PDF through independent readers and compare visible results. Checking for paths or
  absence of embedded images cannot detect changed colors, missing text, or clipped legends.
- Exercise optional PDF and Latin font features, installed static/shared consumers, and minimal
  embedded builds in CI. Keep the existing sanitizer/property cases.
- Refresh the existing benchmark baseline: release/compiler/CPU, repeated-run distributions,
  peak memory, output size, render versus encode, raw versus decimated data, and all backends.
  Include 1k/10k/100k/1M workloads where practical. Profile the recorded slow pie case before
  assigning a cause or making a performance claim.
- Add a documented embedded memory budget and optional pixel-aware decimation. Preserve gaps,
  spikes, and original x positions when reducing data.
- Replace stale roadmap statements: vector bar/line/area/combo, per-series style overrides,
  doughnut vector cutouts, and horizontal reference lines already exist. Update the benchmark
  export/platform tables and export header comments as well.

The recommended acceptance target is a consistent, easy-to-install library for common static
2D reporting and analytics, with evidence for its rendering quality and embedded footprint.

Supersampling follow-up (2026-10-10): built-in charts now keep layout in logical pixels and scale
all primitives at the output boundary, preserving styles and layout decisions. Scatter/bubble
shares this path and emits native SVG/PDF. Custom render-only charts retain the documented legacy
scaling path; capability queries and strict vector fallback remain open.

Export-contract follow-up (2026-10-11): capability queries now report advertised native support
and compiled PDF availability without drawing. Strict SVG/PDF export checks actual draw support
and rejects raster fallback with a structured error; existing custom draw overrides still work.
Const save/encode overloads and exception contracts are documented. Allowed custom raster
fallback now follows normal supersampling/clone/pixel checks. Input validation policies,
configurable resource budgets and backend font fidelity remain open.
