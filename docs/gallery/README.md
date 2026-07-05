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
