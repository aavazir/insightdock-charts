# Fonts and text

All text — titles, axis labels, legends, data labels — is rasterized with `stb_truetype` and
decoded as **UTF-8**. Code points the active font has no glyph for are drawn as `?`.

## Choosing a font

| Source | How | Coverage |
|---|---|---|
| Embedded (default) | nothing to do | printable ASCII, ~20 KB in the binary |
| Embedded Latin | build with `-DINSIGHTDOCK_FONT_LATIN=ON` (CMake) or `FONT_MODE=embedded-latin` (Make) | ASCII + Latin-1 + Latin Extended-A, common punctuation, `€`, `™`, `−` (~22 KB) |
| Your own TTF | `ChartOptions::font` | whatever the file covers |

```cpp
#include <insightdock/charts.h>
using namespace insightdock::charts;

auto font = load_font_data("/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf");
if (!font) { /* unreadable, > 64 MiB, or not a TrueType file */ }

BarChart chart;
chart.options().font = font;          // shared_ptr: reuse one font across many charts
chart.options().title = "Café — 25 °C";
```

`load_font_data()` validates the file once; `ChartOptions::font` then holds shared, immutable bytes
so copying a chart or building hundreds of charts does not duplicate the font.

### `FONT_MODE=system` (Make)

`FONT_MODE=system` omits the embedded font entirely, which saves ~20 KB on OpenWrt-class targets.
Charts then render **no text** unless you set `ChartOptions::font` on every chart:

```cpp
static auto font = load_font_data("/www/assets/fonts/OpenSans-Regular.ttf");
chart.options().font = font;
```

## Wide x-axis labels

`CartesianOptions::x_label_mode` chooses how category labels cope with narrow slots:

| Mode | Behaviour |
|---|---|
| `Auto` (default) | Horizontal; labels are skipped so neighbours never overlap. |
| `Rotate90` | Every label is drawn rotated 90° (reads bottom-to-top); the bottom margin grows to fit the longest label. |
| `Truncate` | Every label is drawn horizontally, shortened with `...` to fit its slot. |

`max_x_labels` still caps the number of labels in every mode.

## Multi-line text

Chart titles, category labels and the x-axis title may contain `\n`:

```cpp
chart.options().title = "Revenue by quarter\n(thousands of USD)";
chart.set_categories({"Q1\n2024", "Q2\n2024"});
```

Each line is centred on its own width, and the title / label margins grow to fit. Y-axis
titles are single-line. Legend entries support explicit newlines and automatic wrapping.

## Colorblind-safe palette

`chart.options().palette = PaletteKind::ColorblindSafe;` selects the 8-colour Okabe–Ito palette
for series added afterwards without an explicit color (set it before `add_series`).

## Legend layout and overflow

Pie, Cartesian and scatter/bubble legends measure labels with the chart font. Top and bottom
legends pack entries into centered rows with 10 pixels of canvas padding on each side. Right
legends reserve their measured width, up to one third of the canvas, and wrap longer labels.
Each legend uses at most one third of the canvas height. Explicit newlines preserve empty lines;
long tokens split between UTF-8 code points.

Entries appear in insertion order. When the complete legend exceeds that height, whole trailing
entries are replaced by a `+N more` count. The chart still renders their data. Increase the canvas
size, shorten labels or disable the legend to show more data without a summary. If even the count
cannot fit (for example, on a very narrow canvas with a large font), the overflow indicator is omitted.
An individual code point wider than the available label width also triggers overflow.

Automatic pie centers and radii reserve room for the title and legend at render time, including
when dimensions change after `set_pie_options()`. Explicit coordinates and radii remain caller
controlled. Pie callout collisions and arbitrary axis/label collisions still need manual tuning.
The same measured legend layout drives PNG, SVG and PDF chart exports; scatter/bubble currently
uses raster fallback for vector exports, and the legacy PDF pie report helpers use their own layout.
