# Vector export (SVG and PDF)

Bar, line, area, combo, pie, scatter and bubble charts export as **native vector** SVG and PDF: shapes are real
paths and rectangles, labels are real text (selectable and searchable), and the result stays sharp at
any zoom or print size. Custom charts without `Chart::draw()` embed a raster image.

| Chart | SVG `Vector` | PDF `Vector` |
|---|---|---|
| Pie / doughnut | yes | yes |
| Bar (grouped, stacked, horizontal) | yes | yes |
| Line, Area, Combo | yes | yes |
| Scatter / bubble | yes | yes |

## Saving

```cpp
ExportOptions svg;
svg.svg_backend = SvgBackend::Vector;
ChartExporter::save(chart, "chart.svg", svg);

ExportOptions pdf;
pdf.pdf_backend = PdfBackend::Vector;           // PDF needs INSIGHTDOCK_WITH_PODOFO
ChartExporter::save(chart, "chart.pdf", pdf);
```

`OutputFormat::PdfVector` selects the vector backend directly. Embedding a chart into an existing
PoDoFo page or a `PdfReportBuilder` report works the same way — pass `PdfBackend::Vector` in the
`ExportOptions` (see [PDF reports](pdf-reports.md)).

## Capabilities and strict output

`ChartExporter::capabilities(chart)` reports `vector` (`Native`, `RasterOnly`, or `Unknown`) and
`pdf_available` without drawing, cloning or validating the chart. Built-in charts advertise
`VectorCapability::Native`. Custom charts default to `Unknown`; they can override
`Chart::vector_capability()` to declare support. PDF availability reflects the library build.
These declarations do not guarantee valid input or successful export.

To require native output, set `vector_fallback`:

```cpp
ExportOptions options;
options.format = OutputFormat::Svg;
options.svg_backend = SvgBackend::Vector;
options.vector_fallback = VectorFallback::Reject;
Chart const &view = chart;
auto encoded = ChartExporter::encode(view, options);
if (!encoded && encoded.error() == ChartError::VectorUnsupported) {
  // Choose another format or tell the caller that native drawing is unavailable.
}
```

`save()` and `encode()` check the actual `draw()` return value, so existing custom charts with
`Unknown` capability can still succeed in strict mode. If drawing returns false, strict mode
returns `VectorUnsupported` without rendering an image. A rejected `save()` leaves an existing
file intact. `AllowRaster` remains the default; fallback applies the requested supersampling and
its pixel limits, and requires `clone()` above 1x. Native output ignores supersampling.
`PdfUnsupported` takes precedence over the fallback policy in builds without PDF support.
The policy applies only when a native vector backend is selected.

The void `ChartExporter::draw_pdf()` and `draw_pdf_vector()` embedding APIs throw
`std::runtime_error` on strict rejection and propagate rendering/PoDoFo exceptions. A custom
`draw()` that writes before returning false may leave marks in the supplied PDF page; embedding
does not roll back page contents. Standalone `save()`/`encode()` discard partial vector output.
Direct `podofo::draw_chart_vector()` helpers retain their legacy raster fallback behavior.

## What carries over

Everything the raster renderer draws is emitted as vector: per-series dash patterns and markers,
translucent fills (PDF uses ExtGState opacity), rounded and bordered bars, data labels, reference
lines and bands, the secondary axis, rotated axis titles, `x_label_mode`, and the legend.
Pie chart exports share their geometry and label wrapping with PNG, including explicit slice colors,
color alpha multiplied by `fill_opacity`, gaps, stroke settings, doughnut cutouts and in-slice labels.
SVG geometry uses decimal points regardless of the C or C++ process locale.

## How it differs from PNG

* **Text is drawn with the viewer's font.** Layout (margins, label skipping, legend position) is
  computed with the embedded font, so geometry is identical everywhere, but SVG text uses
  `DejaVu Sans, Verdana, Arial, sans-serif` and PDF text uses Helvetica unless you pass a PoDoFo font.
  Glyph widths can differ by a pixel or two from the PNG.
* **PDF text is WinAnsi.** Latin-1 and `€` are drawn; other characters appear as `?` (the PDF
  encoder can't represent them without embedding a font). SVG text is Unicode.
* **Strokes are not pixel-snapped.** Thin lines look slightly lighter than the anti-aliased raster
  version.
* `supersample` does not apply: vector output has no resolution.

## Writing your own chart type

Cartesian charts draw through `DrawTarget` (`insightdock/draw_target.h`), a small pixel-space interface
implemented by `RasterTarget` (PNG/JPEG), `SvgTarget` and `PdfTarget`. A custom `CartesianChart`
subclass that draws only through `ctx.target` in `draw_series()` and `draw_overlay()` gets vector
output for free. See [Concepts](../concepts.md).

Pie also implements `Chart::draw()`. `DrawTarget::fill_sector()` has a default polygon implementation,
so existing custom drawing targets can render pies without adding an override. Raster and SVG targets
override it with native sector rendering. PDF uses the polygon approximation.

For existing PDF report integrations, `podofo::draw_pie()` / `draw_pie_slices()` and the
`draw_chart_vector()` overload accepting explicit `PieVectorOptions` retain the PDF-specific report
layout. The overload without `PieVectorOptions`, used by `ChartExporter`, preserves the chart layout
across backends. Default PDF pie exports now include the chart title and background, and no longer
switch to the separate report layout.
