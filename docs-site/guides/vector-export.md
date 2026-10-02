# Vector export (SVG and PDF)

Bar, line, area, combo and pie charts export as **native vector** SVG and PDF: shapes are real
paths and rectangles, labels are real text (selectable and searchable), and the result stays sharp at
any zoom or print size. Scatter and bubble charts still embed a raster image in SVG and PDF.

| Chart | SVG `Vector` | PDF `Vector` |
|---|---|---|
| Pie / doughnut | yes | yes |
| Bar (grouped, stacked, horizontal) | yes | yes |
| Line, Area, Combo | yes | yes |
| Scatter / bubble | raster embed | raster embed |

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

## What carries over

Everything the raster renderer draws is emitted as vector: per-series dash patterns and markers,
translucent fills (PDF uses ExtGState opacity), rounded and bordered bars, data labels, reference
lines and bands, the secondary axis, rotated axis titles, `x_label_mode`, and the legend.

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
