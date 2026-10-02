# Gallery

Generated sample outputs from InsightDock generic examples. Regenerate after visual changes:

```bash
cmake -B build -DINSIGHTDOCK_BUILD_TESTS=ON
cmake --build build --target insightdock_gallery_regression
UPDATE_GALLERY=1 ./build/tests/insightdock_gallery_regression docs/gallery
```

CI runs `gallery_regression` via CTest and compares against these reference files.

## Expected outputs

| File | Source |
|------|--------|
| `pie_chart.png` | `bar_line_example` / `example_main` |
| `bar_grouped.png` | `bar_line_example` |
| `bar_stacked.png` | `bar_line_example` |
| `line_chart.png` | `bar_line_example` |
| `area_stacked.png` | `gallery_regression` test |
| `combo_chart.png` | `combo_chart_example` / `gallery_regression` test |
| `scatter_chart.png` | `scatter_bubble_example` / `gallery_regression` test |
| `bubble_chart.png` | `scatter_bubble_example` / `gallery_regression` test |

PDF samples (PoDoFo required):

| File | Source |
|------|--------|
| `chart_report.pdf` | `podofo_report_example` |
| `pie_vector.pdf` | `podofo_report_example` |

## Vector output sample

`vector_vs_matplotlib.png` shows the same grouped bar chart as a vector PDF from InsightDock and from
Matplotlib 3.10 (pages rasterised with `pdftoppm` for the preview). `insightdock_bar.svg` is the native
SVG. For the InsightDock PDF, `pdfimages -list` shows no embedded images, so bars and text are real
vector objects. PDFs are git-ignored, so regenerate them with a PoDoFo build. The comparison is of
output style only; it makes no claim about speed or quality.
