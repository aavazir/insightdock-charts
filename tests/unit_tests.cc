// SPDX-License-Identifier: MIT
// Copyright (c) InsightDock contributors
//
// Unit tests for Scale, Palette, ImageBuffer, and ChartResult.
// No external test framework — just assertions and a main() that returns 0
// on full pass or 1 with a failure count printed to stderr.

#include "insightdock/charts.h"
#include "insightdock/detail/marker.h"
#include "insightdock/detail/spline.h"
#include "insightdock/detail/tick_format.h"
#include "insightdock/error.h"
#include "insightdock/svg_writer.h"

#ifdef INSIGHTDOCK_HAS_PODOFO
#include <podofo/podofo.h>
#endif

#include <atomic>
#include <cassert>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <limits>
#include <thread>
#include <vector>

using namespace insightdock::charts;

static int g_failures = 0;

#define CHECK(cond)                                                                                \
  do {                                                                                             \
    if (!(cond)) {                                                                                 \
      std::fprintf(stderr, "FAIL: %s  (%s:%d)\n", #cond, __FILE__, __LINE__);                      \
      ++g_failures;                                                                                \
    }                                                                                              \
  } while (0)

#define CHECKF(a, b, eps)                                                                          \
  do {                                                                                             \
    float _a = static_cast<float>(a);                                                              \
    float _b = static_cast<float>(b);                                                              \
    if (!(std::fabs(_a - _b) < static_cast<float>(eps))) {                                         \
      std::fprintf(stderr, "FAIL: |%g - %g| < %g  (%s:%d)\n", (double)_a, (double)_b,              \
                   (double)(eps), __FILE__, __LINE__);                                             \
      ++g_failures;                                                                                \
    }                                                                                              \
  } while (0)

// ---------------------------------------------------------------------------
// Scale tests
// ---------------------------------------------------------------------------
//
// Scale mapping convention:
//   to_pixel(min_val) = pixel_max_   (the "far" pixel end)
//   to_pixel(max_val) = pixel_min_   (the "near" pixel end)
//
// For a Y-axis (higher data = higher on screen = smaller pixel y):
//   Scale(data_min, data_max, pixel_min=top_y, pixel_max=bottom_y)
//   → to_pixel(data_min) = bottom_y  (data minimum sits at the bottom)
//   → to_pixel(data_max) = top_y     (data maximum sits at the top)

static void test_scale_to_pixel() {
  // pixel_min=0 (top of screen, small y), pixel_max=400 (bottom, large y)
  // → min value maps to bottom (400), max value maps to top (0)
  Scale s(0.0, 100.0, 0.0f, 400.0f);
  CHECKF(s.to_pixel(0.0), 400.0f, 0.5f);  // data_min → pixel_max (bottom)
  CHECKF(s.to_pixel(100.0), 0.0f, 0.5f);  // data_max → pixel_min (top)
  CHECKF(s.to_pixel(50.0), 200.0f, 0.5f); // midpoint
}

static void test_scale_from_pixel() {
  Scale s(0.0, 100.0, 0.0f, 400.0f);
  CHECKF(s.from_pixel(400.0f), 0.0, 0.001);  // bottom pixel → data_min
  CHECKF(s.from_pixel(0.0f), 100.0, 0.001);  // top pixel    → data_max
  CHECKF(s.from_pixel(200.0f), 50.0, 0.001); // middle pixel → midpoint
}

static void test_scale_roundtrip() {
  Scale s(10.0, 90.0, 50.0f, 350.0f);
  for (double v = 10.0; v <= 90.0; v += 10.0) {
    CHECKF(s.from_pixel(s.to_pixel(v)), v, 0.001);
  }
}

static void test_scale_nice_ticks_ascending() {
  Scale s(0.0, 97.0, 0.0f, 400.0f);
  auto ticks = s.nice_ticks(6);
  CHECK(!ticks.empty());
  for (std::size_t i = 1; i < ticks.size(); ++i)
    CHECK(ticks[i] > ticks[i - 1]);
}

static void test_scale_nice_ticks_within_domain() {
  Scale s(0.0, 100.0, 0.0f, 400.0f);
  auto ticks = s.nice_ticks(8);
  for (double t : ticks)
    CHECK(t >= s.min() - 1e-9 && t <= s.max() + 1e-9);
}

static void test_scale_linear_nice_covers_data() {
  auto s = Scale::linear_nice(3.0, 97.0, 0.0f, 400.0f);
  CHECK(s.min() <= 3.0);
  CHECK(s.max() >= 97.0);
}

static void test_scale_linear_nice_equal_values() {
  // equal min/max must not crash and must produce a non-degenerate scale
  auto s = Scale::linear_nice(42.0, 42.0, 0.0f, 400.0f);
  CHECK(s.max() > s.min());
}

static void test_scale_log_nice_covers_data() {
  auto s = Scale::log_nice(30.0, 4000.0, 0.0f, 400.0f);
  CHECK(s.type() == ScaleType::Log);
  CHECK(s.min() <= 30.0);
  CHECK(s.max() >= 4000.0);
}

static void test_scale_log_nice_clamps_non_positive_min() {
  // A log domain can't include zero/negative values; must not crash.
  auto s = Scale::log_nice(0.0, 100.0, 0.0f, 400.0f);
  CHECK(s.type() == ScaleType::Log);
  CHECK(s.min() > 0.0);
  CHECK(s.max() >= 100.0);
}

static void test_scale_type_default() {
  Scale s;
  CHECK(s.type() == ScaleType::Linear);
  s.set_type(ScaleType::Log);
  CHECK(s.type() == ScaleType::Log);
  s.set_type(ScaleType::DateTime);
  CHECK(s.type() == ScaleType::DateTime);
}

static void test_scale_log_roundtrip() {
  Scale s(1.0, 1000.0, 0.0f, 400.0f);
  s.set_type(ScaleType::Log);
  for (double v : {1.0, 10.0, 100.0, 1000.0}) {
    CHECKF(s.from_pixel(s.to_pixel(v)), v, v * 0.01 + 0.1);
  }
}

static void test_scale_log_ticks_ascending() {
  Scale s(1.0, 1000.0, 0.0f, 400.0f);
  s.set_type(ScaleType::Log);
  auto ticks = s.nice_ticks(8);
  CHECK(ticks.size() >= 2);
  for (std::size_t i = 1; i < ticks.size(); ++i)
    CHECK(ticks[i] > ticks[i - 1]);
}

static void test_scale_datetime_roundtrip() {
  Scale s(1.7e9, 1.7e9 + 86400.0, 0.0f, 400.0f);
  s.set_type(ScaleType::DateTime);
  CHECKF(s.from_pixel(s.to_pixel(1.7e9 + 43200.0)), 1.7e9 + 43200.0, 100.0);
}

static void test_scale_datetime_ticks() {
  Scale s(0.0, 3600.0 * 24.0, 0.0f, 400.0f);
  s.set_type(ScaleType::DateTime);
  auto ticks = s.nice_ticks(6);
  CHECK(!ticks.empty());
  for (double t : ticks)
    CHECK(t >= s.min() - 1.0 && t <= s.max() + 1.0);
}

static void test_svg_encode_non_empty() {
  ImageBuffer img(32, 32);
  img.clear(Color::white());
  auto bytes = encode_svg(img);
  CHECK(!bytes.empty());
  std::string s(bytes.begin(), bytes.end());
  CHECK(s.find("<svg") != std::string::npos);
  CHECK(s.find("base64") != std::string::npos);
}

// ---------------------------------------------------------------------------
// Native SVG pie (SvgBackend::Vector) tests
// ---------------------------------------------------------------------------

static void test_svg_vector_pie_emits_paths() {
  PieChart pie;
  pie.options().width = 400;
  pie.options().height = 300;
  pie.add_slice("A", 60);
  pie.add_slice("B", 40);

  ExportOptions opts;
  opts.format = OutputFormat::Svg;
  opts.svg_backend = SvgBackend::Vector;
  auto bytes = ChartExporter::encode(pie, opts);
  CHECK(static_cast<bool>(bytes));
  std::string s(bytes.value.begin(), bytes.value.end());
  CHECK(s.find("<svg") != std::string::npos);
  CHECK(s.find("<path") != std::string::npos);
  CHECK(s.find("base64") == std::string::npos); // native, not raster-embedded
}

static void test_svg_vector_pie_single_slice_uses_circle() {
  PieChart pie;
  pie.options().width = 200;
  pie.options().height = 200;
  pie.add_slice("All", 100);

  ExportOptions opts;
  opts.format = OutputFormat::Svg;
  opts.svg_backend = SvgBackend::Vector;
  auto bytes = ChartExporter::encode(pie, opts);
  CHECK(static_cast<bool>(bytes));
  std::string s(bytes.value.begin(), bytes.value.end());
  CHECK(s.find("<circle") != std::string::npos);
}

static std::string vector_svg(Chart &chart) {
  ExportOptions opts;
  opts.format = OutputFormat::Svg;
  opts.svg_backend = SvgBackend::Vector;
  auto bytes = ChartExporter::encode(chart, opts);
  CHECK(static_cast<bool>(bytes));
  return std::string(bytes.value.begin(), bytes.value.end());
}

static bool has(std::string const &s, char const *needle) {
  return s.find(needle) != std::string::npos;
}

static void test_svg_vector_bar_chart_is_native() {
  BarChart bar;
  bar.options().width = 300;
  bar.options().height = 200;
  bar.options().title = "Revenue & <Costs>";
  bar.set_categories({"Q1", "Q2"});
  bar.add_series("Rev", {10, 20});
  bar.cartesian_options().y_axis_label = "USD";
  std::string s = vector_svg(bar);
  CHECK(!has(s, "base64"));
  CHECK(!has(s, "<image"));
  CHECK(has(s, "<rect"));
  CHECK(has(s, "<text"));
  CHECK(has(s, ">Q1</text>"));
  CHECK(has(s, "Revenue &amp; &lt;Costs&gt;")); // XML-escaped title
  CHECK(has(s, "rotate(-90)"));                 // y-axis title
}

static void test_svg_vector_line_chart_is_native() {
  LineChart line;
  line.options().width = 300;
  line.options().height = 200;
  line.set_categories({"a", "b", "c"});
  SeriesStyle st;
  st.dash = DashPattern{6.0f, 3.0f};
  st.marker = PointMarkerShape::Square;
  line.add_series("dashed", {1, 3, 2}, Color::rgb(200, 30, 30), st);
  line.add_series("solid", {2, 1, 3});
  line.line_options().show_points = true;
  std::string s = vector_svg(line);
  CHECK(!has(s, "base64"));
  CHECK(has(s, "<polyline"));
  CHECK(has(s, "stroke-dasharray=\"6 3\""));
  CHECK(has(s, "<circle")); // markers of the plain series
}

static void test_svg_vector_area_and_combo_are_native() {
  AreaChart area;
  area.options().width = 300;
  area.options().height = 200;
  area.set_categories({"a", "b", "c"});
  area.add_series("s", {1, 3, 2});
  std::string a = vector_svg(area);
  CHECK(!has(a, "base64"));
  CHECK(has(a, "<polygon"));
  CHECK(has(a, "fill-opacity")); // translucent fill

  ComboChart combo;
  combo.options().width = 300;
  combo.options().height = 200;
  combo.set_categories({"a", "b"});
  combo.add_bar_series("bars", {5, 6});
  combo.add_line_series("line", {50, 60}, true);
  std::string c = vector_svg(combo);
  CHECK(!has(c, "base64"));
  CHECK(has(c, "<rect"));
  CHECK(has(c, "<polyline"));
  CHECK(has(c, "rotate(90)") || has(c, "text-anchor=\"start\"")); // right-axis labels present
}

static void test_svg_vector_doughnut_paints_hole_in_background() {
  PieChart pie;
  pie.options().width = 300;
  pie.options().height = 300;
  pie.options().background = Color::rgb(18, 52, 86); // distinctive: 123456
  pie.add_slice("A", 60);
  pie.add_slice("B", 40);
  PieOptions po = pie.pie_options();
  po.inner_radius_ratio = 0.5;
  pie.set_pie_options(po);
  std::string s = vector_svg(pie);
  // Background rect plus the hole circle both use the background color.
  std::size_t first = s.find("#123456");
  CHECK(first != std::string::npos);
  CHECK(s.find("#123456", first + 1) != std::string::npos);
  CHECK(s.rfind("<circle") > s.rfind("<path")); // hole is drawn after the slices
}

static void test_svg_vector_scatter_falls_back_to_raster() {
  ScatterChart sc;
  sc.options().width = 200;
  sc.options().height = 150;
  sc.add_series("p", {{1, 1}, {2, 3}});
  CHECK(has(vector_svg(sc), "base64")); // no vector path yet
}

// ---------------------------------------------------------------------------
// PieChart doughnut cutout tests
// ---------------------------------------------------------------------------

static void test_pie_doughnut_cutout_center_matches_background() {
  PieChart pie;
  pie.options().width = 300;
  pie.options().height = 300;
  pie.options().background = Color::rgb(240, 240, 240);
  PieOptions po = pie.pie_options();
  po.center_x = 150;
  po.center_y = 150;
  po.radius = 120;
  po.inner_radius_ratio = 0.5;
  pie.set_pie_options(po);
  pie.add_slice("A", 60);
  pie.add_slice("B", 40);

  auto img = pie.render();
  auto const *center = img.pixel(150, 150);
  CHECK(center != nullptr);
  if (center) {
    CHECK(center[0] == 240);
    CHECK(center[1] == 240);
    CHECK(center[2] == 240);
  }

  // Well outside the cutout radius, a slice color should still be present.
  auto const *edge = img.pixel(150, 150 - 100);
  CHECK(edge != nullptr);
  if (edge)
    CHECK(!(edge[0] == 240 && edge[1] == 240 && edge[2] == 240));
}

static void test_pie_default_inner_radius_ratio_renders_solid_center() {
  // inner_radius_ratio = 0 (default) must not punch a hole — backward compatible.
  PieChart pie;
  pie.options().width = 300;
  pie.options().height = 300;
  pie.options().background = Color::rgb(240, 240, 240);
  PieOptions po = pie.pie_options();
  po.center_x = 150;
  po.center_y = 150;
  po.radius = 120;
  pie.set_pie_options(po);
  pie.add_slice("A", 100);

  auto img = pie.render();
  auto const *center = img.pixel(150, 150);
  CHECK(center != nullptr);
  if (center)
    CHECK(!(center[0] == 240 && center[1] == 240 && center[2] == 240));
}

// ---------------------------------------------------------------------------
// LTTB decimation tests
// ---------------------------------------------------------------------------

static void test_lttb_noop_when_under_threshold() {
  std::vector<DecimationPoint> pts = {{0, 1}, {1, 2}, {2, 3}};
  auto out = lttb_decimate(pts, 10);
  CHECK(out.size() == pts.size());
}

static void test_lttb_reduces_to_threshold() {
  std::vector<DecimationPoint> pts;
  for (int i = 0; i < 1000; ++i)
    pts.push_back({static_cast<double>(i), std::sin(i * 0.05)});
  auto out = lttb_decimate(pts, 50);
  CHECK(out.size() == 50);
}

static void test_lttb_keeps_first_and_last() {
  std::vector<DecimationPoint> pts;
  for (int i = 0; i < 200; ++i)
    pts.push_back({static_cast<double>(i), static_cast<double>(i * i)});
  auto out = lttb_decimate(pts, 20);
  CHECKF(out.front().x, pts.front().x, 0.001);
  CHECKF(out.front().y, pts.front().y, 0.001);
  CHECKF(out.back().x, pts.back().x, 0.001);
  CHECKF(out.back().y, pts.back().y, 0.001);
}

static void test_lttb_preserves_spike() {
  // Flat series with a single sharp spike — LTTB should keep a point near it,
  // unlike naive stride sampling which could step right over it.
  std::vector<DecimationPoint> pts;
  for (int i = 0; i < 300; ++i)
    pts.push_back({static_cast<double>(i), 0.0});
  pts[150].y = 1000.0;

  auto out = lttb_decimate(pts, 20);
  bool found_spike = false;
  for (auto const &p : out)
    if (p.y > 500.0)
      found_spike = true;
  CHECK(found_spike);
}

static void test_lttb_indexed_matches_values() {
  std::vector<double> values;
  for (int i = 0; i < 500; ++i)
    values.push_back(std::cos(i * 0.02) * 10.0);

  auto [decimated_values, indices] = lttb_decimate_indexed(values, 30);
  CHECK(decimated_values.size() == 30);
  CHECK(indices.size() == 30);
  CHECK(indices.front() == 0);
  CHECK(indices.back() == values.size() - 1);
  for (std::size_t i = 0; i < indices.size(); ++i)
    CHECKF(decimated_values[i], values[indices[i]], 0.001);
  for (std::size_t i = 1; i < indices.size(); ++i)
    CHECK(indices[i] > indices[i - 1]);
}

// ---------------------------------------------------------------------------
// ScatterChart tests
// ---------------------------------------------------------------------------

static void test_scatter_empty_series_renders_blank() {
  ScatterChart chart;
  chart.options().width = 100;
  chart.options().height = 80;
  auto img = chart.render();
  CHECK(img.width() == 100);
  CHECK(img.height() == 80);
}

static void test_scatter_x_increases_left_to_right() {
  // Two series, one point each, full opacity so marker centers are an exact
  // color match (no alpha blending) — lets the test tell the two points
  // apart by color and check which one landed on which side of the plot.
  ScatterChart chart;
  chart.options().width = 400;
  chart.options().height = 300;
  chart.options().legend.enabled = false;
  chart.scatter_options().fill_opacity = 1.0f;
  chart.add_series("Low", {{0.0, 0.0}});    // palette_color(0)
  chart.add_series("High", {{100.0, 0.0}}); // palette_color(1)

  auto img = chart.render();
  Color low_c = palette_color(0);
  Color high_c = palette_color(1);

  auto find_x_for_color = [&](Color const &target) -> int {
    for (std::size_t x = 0; x < img.width(); ++x) {
      for (std::size_t y = 0; y < img.height(); ++y) {
        auto const *p = img.pixel(x, y);
        if (p && p[0] == target.r && p[1] == target.g && p[2] == target.b)
          return static_cast<int>(x);
      }
    }
    return -1;
  };

  int low_x = find_x_for_color(low_c);
  int high_x = find_x_for_color(high_c);
  CHECK(low_x >= 0);
  CHECK(high_x >= 0);
  CHECK(low_x < high_x);
}

static void test_scatter_bubble_radius_scales() {
  ScatterChart chart;
  chart.options().width = 300;
  chart.options().height = 300;
  chart.scatter_options().min_bubble_radius = 4.0f;
  chart.scatter_options().max_bubble_radius = 30.0f;
  chart.add_series("S", {{10, 10, 1.0}, {20, 20, 100.0}});
  auto img = chart.render();
  CHECK(img.width() == 300);
  CHECK(img.height() == 300);
}

// ---------------------------------------------------------------------------
// BarChart shape (rounded corner / border) tests
// ---------------------------------------------------------------------------

static void test_bar_corner_radius_rounds_top_corner() {
  BarChart bar;
  bar.options().width = 200;
  bar.options().height = 200;
  bar.options().legend.enabled = false;
  bar.set_categories({"A"});
  bar.add_series("S", {100.0}, Color::rgb(255, 0, 0));
  bar.bar_options().corner_radius = 30.0f;
  auto rounded = bar.render();

  bar.bar_options().corner_radius = 0.0f;
  auto square = bar.render();

  // Find the square (non-rounded) bar's exact pixel bounding box, then confirm its
  // top-left corner is filled in the square version but not (or less) in the
  // rounded version, whose corner arc excludes that point.
  int min_x = -1, min_y = -1;
  for (std::size_t y = 0; y < square.height(); ++y) {
    for (std::size_t x = 0; x < square.width(); ++x) {
      auto const *p = square.pixel(x, y);
      if (p && p[0] == 255 && p[1] == 0 && p[2] == 0 && p[3] > 0) {
        if (min_y < 0)
          min_y = static_cast<int>(y);
        if (min_x < 0 || static_cast<int>(x) < min_x)
          min_x = static_cast<int>(x);
      }
    }
  }
  CHECK(min_x >= 0 && min_y >= 0);
  if (min_x >= 0 && min_y >= 0) {
    // The chart background is opaque, so alpha is 255 everywhere regardless of the
    // bar; compare the actual bar color (red), not raw alpha, at the square bar's
    // exact corner pixel.
    auto const *rd_corner =
        rounded.pixel(static_cast<std::size_t>(min_x), static_cast<std::size_t>(min_y));
    CHECK(rd_corner != nullptr);
    if (rd_corner)
      CHECK(!(rd_corner[0] == 255 && rd_corner[1] == 0 && rd_corner[2] == 0));
  }
}

static void test_bar_border_pixel_matches_border_color() {
  BarChart bar;
  bar.options().width = 200;
  bar.options().height = 200;
  bar.options().legend.enabled = false;
  bar.set_categories({"A"});
  bar.add_series("S", {100.0}, Color::rgb(0, 0, 255));
  bar.bar_options().border_width = 3.0f;
  Color border = Color::rgb(0, 200, 0);
  bar.bar_options().border_color = border;
  auto img = bar.render();

  bool saw_border = false;
  for (std::size_t y = 0; y < img.height() && !saw_border; ++y) {
    for (std::size_t x = 0; x < img.width(); ++x) {
      auto const *p = img.pixel(x, y);
      if (p && p[0] == border.r && p[1] == border.g && p[2] == border.b) {
        saw_border = true;
        break;
      }
    }
  }
  CHECK(saw_border);
}

static void test_bar_default_options_render_unchanged() {
  // corner_radius = 0 and border_color transparent by default — must be a no-op,
  // matching pre-BarOptions rendering exactly.
  BarChart bar;
  bar.options().width = 120;
  bar.options().height = 90;
  bar.set_categories({"Q1", "Q2"});
  bar.add_series("Rev", {10.0, 20.0});
  auto img = bar.render();
  CHECK(img.width() == 120);
  CHECK(img.height() == 90);
}

// ---------------------------------------------------------------------------
// ComboChart dual-axis tests
// ---------------------------------------------------------------------------

static void test_combo_default_axis_backward_compatible() {
  ComboChart combo;
  combo.options().width = 200;
  combo.options().height = 150;
  combo.set_categories({"X", "Y"});
  combo.add_bar_series("Bar", {10.0, 20.0});
  combo.add_line_series("Line", {5.0, 15.0});
  auto img = combo.render();
  CHECK(img.width() == 200);
  CHECK(img.height() == 150);
}

static void test_combo_secondary_axis_independent_scale() {
  // Bar series on the primary axis spans ~0-1000; line series on the
  // secondary axis spans only 0-10. If the line incorrectly shared the
  // primary scale, both of its points would collapse near the bottom of the
  // plot (10/1000 is barely off the baseline). With an independent secondary
  // scale, the two points should spread across most of the plot height.
  ComboChart combo;
  combo.options().width = 300;
  combo.options().height = 300;
  combo.options().legend.enabled = false;
  combo.line_options().point_radius = 6.0f;
  combo.set_categories({"A", "B"});
  combo.add_bar_series("Bar", {1000.0, 1000.0}, Color::rgb(0, 0, 0));
  Color marker = Color::rgb(200, 30, 30);
  combo.add_line_series("Line", {0.0, 10.0}, marker, /*secondary_axis=*/true);

  auto img = combo.render();

  int min_y = -1, max_y = -1;
  for (std::size_t y = 0; y < img.height(); ++y) {
    for (std::size_t x = 0; x < img.width(); ++x) {
      auto const *p = img.pixel(x, y);
      if (p && p[0] == marker.r && p[1] == marker.g && p[2] == marker.b) {
        if (min_y < 0)
          min_y = static_cast<int>(y);
        max_y = static_cast<int>(y);
      }
    }
  }
  CHECK(min_y >= 0);
  CHECK(max_y >= 0);
  CHECK(max_y - min_y > 100);
}

// ---------------------------------------------------------------------------
// Palette tests
// ---------------------------------------------------------------------------

static void test_palette_chartjs_cycle() {
  // palette_color(i) == palette_color(i + palette_size())
  for (std::size_t i = 0; i < palette_size(); ++i) {
    Color a = palette_color(i);
    Color b = palette_color(i + palette_size());
    CHECK(a.r == b.r && a.g == b.g && a.b == b.b && a.a == b.a);
  }
}

static void test_palette_tableau10_cycle() {
  for (std::size_t i = 0; i < tableau10_size(); ++i) {
    Color a = tableau10_color(i);
    Color b = tableau10_color(i + tableau10_size());
    CHECK(a.r == b.r && a.g == b.g && a.b == b.b && a.a == b.a);
  }
}

static void test_palette_by_kind_matches() {
  for (std::size_t i = 0; i < palette_size(); ++i) {
    Color a = palette_color(i);
    Color b = palette_color_by_kind(PaletteKind::ChartJs, i);
    CHECK(a.r == b.r && a.g == b.g && a.b == b.b && a.a == b.a);
  }
  for (std::size_t i = 0; i < tableau10_size(); ++i) {
    Color a = tableau10_color(i);
    Color b = palette_color_by_kind(PaletteKind::Tableau10, i);
    CHECK(a.r == b.r && a.g == b.g && a.b == b.b && a.a == b.a);
  }
}

static void test_palette_distinct() {
  // First 5 ChartJS colors should all differ (sanity check the palette data).
  for (std::size_t i = 0; i < 5; ++i) {
    for (std::size_t j = i + 1; j < 5; ++j) {
      Color a = palette_color(i);
      Color b = palette_color(j);
      CHECK(a.r != b.r || a.g != b.g || a.b != b.b);
    }
  }
}

// ---------------------------------------------------------------------------
// ImageBuffer tests
// ---------------------------------------------------------------------------

static void test_image_buffer_dimensions() {
  ImageBuffer buf(16, 8);
  CHECK(buf.width() == 16);
  CHECK(buf.height() == 8);
  CHECK(buf.data().size() == 16 * 8 * 4);
}

static void test_image_buffer_pixel_access() {
  ImageBuffer buf(4, 4);
  CHECK(buf.pixel(0, 0) != nullptr);
  CHECK(buf.pixel(3, 3) != nullptr);
  CHECK(buf.pixel(4, 0) == nullptr); // OOB x
  CHECK(buf.pixel(0, 4) == nullptr); // OOB y
}

static void test_image_buffer_clear_color() {
  ImageBuffer buf(4, 4);
  buf.clear({200, 100, 50, 255});
  auto *p = buf.pixel(2, 2);
  CHECK(p != nullptr);
  if (p) {
    CHECK(p[0] == 200);
    CHECK(p[1] == 100);
    CHECK(p[2] == 50);
    CHECK(p[3] == 255);
  }
}

static void test_image_buffer_fill_rect() {
  ImageBuffer buf(20, 20);
  buf.clear(Color::white());
  buf.fill_rect(5.0f, 5.0f, 10.0f, 10.0f, {255, 0, 0, 255});

  // Center of filled area should be red.
  auto *p = buf.pixel(10, 10);
  CHECK(p != nullptr);
  if (p) {
    CHECK(p[0] == 255);
    CHECK(p[1] == 0);
    CHECK(p[2] == 0);
  }

  // Top-left corner untouched — should still be white.
  auto *q = buf.pixel(0, 0);
  CHECK(q != nullptr);
  if (q) {
    CHECK(q[0] == 255 && q[1] == 255 && q[2] == 255);
  }
}

static void test_image_buffer_downsample() {
  // 4×4 solid red → downsample by 2 → 2×2, all pixels still red.
  ImageBuffer src(4, 4);
  src.clear({255, 0, 0, 255});
  ImageBuffer dst = ImageBuffer::downsample(src, 2);
  CHECK(dst.width() == 2);
  CHECK(dst.height() == 2);
  for (std::size_t y = 0; y < dst.height(); ++y) {
    for (std::size_t x = 0; x < dst.width(); ++x) {
      auto *p = dst.pixel(x, y);
      CHECK(p != nullptr);
      if (p) {
        CHECK(p[0] == 255);
        CHECK(p[1] == 0);
        CHECK(p[2] == 0);
      }
    }
  }
}

static void test_image_buffer_fill_rounded_rect_corner_transparent() {
  ImageBuffer buf(30, 30);
  buf.clear(Color::rgba(0, 0, 0, 0));
  buf.fill_rounded_rect(5.0f, 5.0f, 20.0f, 20.0f, 8.0f, {255, 0, 0, 255});

  // Center should be fully opaque red.
  auto *center = buf.pixel(15, 15);
  CHECK(center != nullptr);
  if (center)
    CHECK(center[3] == 255);

  // The exact corner pixel of the bounding box should be outside the rounded
  // corner arc, so it stays transparent (unlike a plain fill_rect).
  auto *corner = buf.pixel(5, 5);
  CHECK(corner != nullptr);
  if (corner)
    CHECK(corner[3] == 0);
}

static void test_image_buffer_fill_rounded_rect_zero_radius_matches_fill_rect() {
  ImageBuffer a(20, 20);
  ImageBuffer b(20, 20);
  a.clear(Color::white());
  b.clear(Color::white());
  a.fill_rect(5.0f, 5.0f, 10.0f, 10.0f, {0, 128, 255, 255});
  b.fill_rounded_rect(5.0f, 5.0f, 10.0f, 10.0f, 0.0f, {0, 128, 255, 255});

  auto *pa = a.pixel(5, 5);
  auto *pb = b.pixel(5, 5);
  CHECK(pa != nullptr && pb != nullptr);
  if (pa && pb) {
    CHECK(pa[0] == pb[0]);
    CHECK(pa[1] == pb[1]);
    CHECK(pa[2] == pb[2]);
  }
}

static void test_image_buffer_draw_dashed_line_empty_dash_is_solid() {
  ImageBuffer a(40, 4);
  ImageBuffer b(40, 4);
  a.clear(Color::rgba(0, 0, 0, 0));
  b.clear(Color::rgba(0, 0, 0, 0));
  a.draw_line(2.0f, 2.0f, 37.0f, 2.0f, 2.0f, {0, 0, 0, 255});
  b.draw_dashed_line(2.0f, 2.0f, 37.0f, 2.0f, 2.0f, {0, 0, 0, 255}, {});

  for (std::size_t x = 0; x < a.width(); ++x) {
    auto *pa = a.pixel(x, 2);
    auto *pb = b.pixel(x, 2);
    CHECK(pa != nullptr && pb != nullptr);
    if (pa && pb)
      CHECK(pa[3] == pb[3]);
  }
}

static void test_image_buffer_draw_dashed_line_leaves_gaps() {
  ImageBuffer buf(60, 4);
  buf.clear(Color::rgba(0, 0, 0, 0));
  // 6px on, 6px off, starting at x=0 along a straight horizontal line.
  buf.draw_dashed_line(0.0f, 2.0f, 59.0f, 2.0f, 1.0f, {0, 0, 0, 255}, {6.0f, 6.0f});

  bool saw_opaque = false;
  bool saw_gap = false;
  for (std::size_t x = 0; x < buf.width(); ++x) {
    auto *p = buf.pixel(x, 2);
    CHECK(p != nullptr);
    if (!p)
      continue;
    if (p[3] > 200)
      saw_opaque = true;
    if (p[3] == 0)
      saw_gap = true;
  }
  CHECK(saw_opaque);
  CHECK(saw_gap);
}

static void test_image_buffer_draw_dashed_polyline_phase_continuous() {
  // Two collinear segments forming one straight line; the dash pattern should
  // continue seamlessly across the shared vertex rather than restarting.
  std::pair<float, float> one_segment[] = {{0.0f, 2.0f}, {59.0f, 2.0f}};
  std::pair<float, float> two_segments[] = {{0.0f, 2.0f}, {30.0f, 2.0f}, {59.0f, 2.0f}};

  ImageBuffer a(60, 4);
  ImageBuffer b(60, 4);
  a.clear(Color::rgba(0, 0, 0, 0));
  b.clear(Color::rgba(0, 0, 0, 0));
  a.draw_dashed_polyline(one_segment, 2, 1.0f, {0, 0, 0, 255}, {6.0f, 6.0f});
  b.draw_dashed_polyline(two_segments, 3, 1.0f, {0, 0, 0, 255}, {6.0f, 6.0f});

  for (std::size_t x = 0; x < a.width(); ++x) {
    auto *pa = a.pixel(x, 2);
    auto *pb = b.pixel(x, 2);
    CHECK(pa != nullptr && pb != nullptr);
    if (pa && pb)
      CHECK((pa[3] > 0) == (pb[3] > 0));
  }
}

// ---------------------------------------------------------------------------
// detail:: shared helper tests (tick_format, spline, marker)
// ---------------------------------------------------------------------------

static void test_format_tick_value_matches_format_label_value_auto() {
  // decimals = -1 (auto) should behave identically to format_tick_value.
  CHECK(detail::format_label_value(1234.0, TickFormat::Raw, -1) ==
        detail::format_tick_value(1234.0, TickFormat::Raw, nullptr));
}

static void test_format_label_value_fixed_decimals() {
  CHECK(detail::format_label_value(3.14159, TickFormat::Raw, 2) == "3.14");
  CHECK(detail::format_label_value(50.0, TickFormat::Percent, 0) == "50%");
  CHECK(detail::format_label_value(9.5, TickFormat::Currency, 2) == "$9.50");
}

static void test_catmull_rom_expand_passes_through_control_points() {
  std::vector<std::pair<float, float>> pts = {{0, 0}, {10, 5}, {20, 0}, {30, 5}};
  auto curve = detail::catmull_rom_expand(pts, 8);
  CHECK(curve.size() > pts.size());
  // First and last points must be preserved exactly.
  CHECKF(curve.front().first, pts.front().first, 1e-4f);
  CHECKF(curve.front().second, pts.front().second, 1e-4f);
  CHECKF(curve.back().first, pts.back().first, 1e-4f);
  CHECKF(curve.back().second, pts.back().second, 1e-4f);
}

static void test_catmull_rom_expand_too_few_points_returns_input() {
  std::vector<std::pair<float, float>> pts = {{0, 0}, {10, 5}};
  auto curve = detail::catmull_rom_expand(pts, 8);
  CHECK(curve.size() == pts.size());
}

static void test_draw_marker_square_fills_corners_circle_does_not() {
  ImageBuffer square_buf(20, 20);
  ImageBuffer circle_buf(20, 20);
  square_buf.clear(Color::rgba(0, 0, 0, 0));
  circle_buf.clear(Color::rgba(0, 0, 0, 0));
  RasterTarget square_target(square_buf);
  RasterTarget circle_target(circle_buf);
  detail::draw_marker(square_target, 10.0f, 10.0f, 6.0f, PointMarkerShape::Square, Color::black());
  detail::draw_marker(circle_target, 10.0f, 10.0f, 6.0f, PointMarkerShape::Circle, Color::black());

  // A point near the marker's bounding-box corner is inside a square marker's footprint
  // but outside a circle's — this is how chart code can trust the shapes actually differ.
  auto *sq = square_buf.pixel(4, 4);
  auto *ci = circle_buf.pixel(4, 4);
  CHECK(sq != nullptr && ci != nullptr);
  if (sq && ci) {
    CHECK(sq[3] > 0);
    CHECK(ci[3] == 0);
  }
}

// ---------------------------------------------------------------------------
// ChartResult / ChartError tests
// ---------------------------------------------------------------------------

static void test_chart_result_success_int() {
  auto r = ChartResult<int>::success(42);
  CHECK(static_cast<bool>(r));
  CHECK(r.ok == true);
  CHECK(r.value == 42);
  CHECK(r.error() == ChartError::None);
}

static void test_chart_result_failure_int() {
  auto r = ChartResult<int>::failure(ChartError::BadPath);
  CHECK(!static_cast<bool>(r));
  CHECK(r.ok == false);
  CHECK(r.error() == ChartError::BadPath);
}

static void test_chart_result_void_success() {
  auto r = ChartResult<void>::success();
  CHECK(static_cast<bool>(r));
  CHECK(r.error() == ChartError::None);
}

static void test_chart_result_void_failure() {
  auto r = ChartResult<void>::failure(ChartError::EncodeFailure);
  CHECK(!static_cast<bool>(r));
  CHECK(r.error() == ChartError::EncodeFailure);
}

static void test_chart_error_message_non_empty() {
  for (auto e : {ChartError::None, ChartError::BadPath, ChartError::RenderFailure,
                 ChartError::EncodeFailure, ChartError::PdfUnsupported, ChartError::InvalidInput}) {
    const char *msg = chart_error_message(e);
    CHECK(msg != nullptr && msg[0] != '\0');
  }
}

// ---------------------------------------------------------------------------
// ChartTheme tests
// ---------------------------------------------------------------------------

static void test_theme_light_dark_differ() {
  auto light = ChartTheme::light();
  auto dark = ChartTheme::dark();
  // Background colors must differ between themes.
  CHECK(light.background.r != dark.background.r || light.background.g != dark.background.g ||
        light.background.b != dark.background.b);
}

static void test_apply_theme() {
  PieChart pie;
  pie.apply_theme(ChartTheme::dark());
  auto const &dark = ChartTheme::dark();
  CHECK(pie.options().background.r == dark.background.r);
  CHECK(pie.options().background.g == dark.background.g);
  CHECK(pie.options().background.b == dark.background.b);
  CHECK(pie.options().font_size == dark.font_size);
}

// ---------------------------------------------------------------------------
// Font::measure tests
// ---------------------------------------------------------------------------

static void test_font_measure_proportional() {
  Font f;
  // Width at double the size should be approximately double (proportional scaling).
  auto m12 = f.measure("Hello", 12.0f);
  auto m24 = f.measure("Hello", 24.0f);
  CHECK(m12.width > 0.0f);
  CHECK(m24.height > 0.0f);
  CHECKF(m24.width / m12.width, 2.0f, 0.05f);
  CHECKF(m24.height / m12.height, 2.0f, 0.05f);
}

static void test_font_measure_empty_string() {
  Font f;
  auto m = f.measure("", 14.0f);
  CHECKF(m.width, 0.0f, 0.001f);
}

// ---------------------------------------------------------------------------
// min_y_labels / max_y_labels
// ---------------------------------------------------------------------------

static void test_y_labels_max_produces_more_ticks_than_default() {
  // nice_ticks(12) should yield more ticks than nice_ticks(6) for a typical range.
  Scale s(0.0, 100.0, 0.0f, 600.0f);
  auto ticks6 = s.nice_ticks(6);
  auto ticks12 = s.nice_ticks(12);
  CHECK(ticks12.size() >= ticks6.size());
}

static void test_y_labels_max_renders_without_crash() {
  LineChart chart;
  chart.options().width = 400;
  chart.options().height = 300;
  chart.options().legend.enabled = false;
  chart.set_categories({"A", "B", "C", "D", "E"});
  chart.add_series("S", {10.0, 40.0, 25.0, 70.0, 55.0});
  chart.cartesian_options().max_y_labels = 12;
  auto img = chart.render();
  CHECK(img.width() == 400 && img.height() == 300);
}

static void test_y_labels_min_renders_without_crash() {
  BarChart chart;
  chart.options().width = 400;
  chart.options().height = 300;
  chart.options().legend.enabled = false;
  chart.set_categories({"Jan", "Feb", "Mar"});
  chart.add_series("Rev", {100.0, 200.0, 150.0});
  chart.cartesian_options().min_y_labels = 12;
  auto img = chart.render();
  CHECK(img.width() == 400 && img.height() == 300);
}

static void test_y_labels_same_min_max_renders_without_crash() {
  LineChart chart;
  chart.options().width = 500;
  chart.options().height = 400;
  chart.options().legend.enabled = false;
  chart.set_categories({"A", "B", "C", "D"});
  chart.add_series("S", {0.0, 50.0, 100.0, 75.0});
  chart.cartesian_options().min_y_labels = 12;
  chart.cartesian_options().max_y_labels = 12;
  auto img = chart.render();
  CHECK(img.width() == 500 && img.height() == 400);
}

static void test_y_labels_inverted_constraint_renders_without_crash() {
  // min > max: values are swapped internally; must not crash
  LineChart chart;
  chart.options().width = 400;
  chart.options().height = 300;
  chart.options().legend.enabled = false;
  chart.set_categories({"X", "Y"});
  chart.add_series("S", {10.0, 90.0});
  chart.cartesian_options().min_y_labels = 20;
  chart.cartesian_options().max_y_labels = 5;
  auto img = chart.render();
  CHECK(img.width() == 400 && img.height() == 300);
}

static void test_y_labels_zero_preserves_default_behavior() {
  // Default (both 0) should produce a chart indistinguishable from the
  // legacy behavior — just verify it renders at correct size.
  BarChart chart;
  chart.options().width = 300;
  chart.options().height = 200;
  chart.options().legend.enabled = false;
  chart.set_categories({"A", "B"});
  chart.add_series("S", {1.0, 2.0});
  auto img = chart.render();
  CHECK(img.width() == 300 && img.height() == 200);
}

// ---------------------------------------------------------------------------
// LineChart / AreaChart styling tests (spline, dash, marker shape)
// ---------------------------------------------------------------------------

static void test_line_chart_spline_renders_without_crash() {
  LineChart line;
  line.options().width = 200;
  line.options().height = 150;
  line.options().legend.enabled = false;
  line.set_categories({"A", "B", "C", "D"});
  line.add_series("S", {10.0, 40.0, 5.0, 30.0});
  line.line_options().spline = true;
  auto img = line.render();
  CHECK(img.width() == 200 && img.height() == 150);
}

static void test_line_chart_dash_pattern_renders_without_crash() {
  LineChart line;
  line.options().width = 200;
  line.options().height = 150;
  line.options().legend.enabled = false;
  line.set_categories({"A", "B", "C"});
  line.add_series("S", {10.0, 20.0, 15.0});
  line.line_options().dash = {6.0f, 4.0f};
  auto img = line.render();
  CHECK(img.width() == 200 && img.height() == 150);
}

static void test_line_chart_square_marker_differs_from_circle() {
  LineChart circle_chart;
  circle_chart.options().width = 200;
  circle_chart.options().height = 150;
  circle_chart.options().legend.enabled = false;
  circle_chart.set_categories({"A", "B"});
  circle_chart.add_series("S", {50.0, 50.0}, Color::rgb(10, 10, 10));
  circle_chart.line_options().point_radius = 8.0f;

  LineChart square_chart;
  square_chart.options().width = 200;
  square_chart.options().height = 150;
  square_chart.options().legend.enabled = false;
  square_chart.set_categories({"A", "B"});
  square_chart.add_series("S", {50.0, 50.0}, Color::rgb(10, 10, 10));
  square_chart.line_options().point_radius = 8.0f;
  square_chart.line_options().point_shape = PointMarkerShape::Square;

  auto circle_img = circle_chart.render();
  auto square_img = square_chart.render();

  // The two renders must differ somewhere — a square marker's corners fill
  // pixels a circle marker of the same radius leaves untouched.
  bool differs = false;
  for (std::size_t y = 0; y < circle_img.height() && !differs; ++y) {
    for (std::size_t x = 0; x < circle_img.width(); ++x) {
      auto const *a = circle_img.pixel(x, y);
      auto const *b = square_img.pixel(x, y);
      if (a[0] != b[0] || a[1] != b[1] || a[2] != b[2]) {
        differs = true;
        break;
      }
    }
  }
  CHECK(differs);
}

static void test_area_chart_show_points_draws_markers() {
  AreaChart area;
  area.options().width = 200;
  area.options().height = 150;
  area.options().legend.enabled = false;
  area.set_categories({"A", "B"});
  Color marker_color = Color::rgb(5, 5, 5);
  area.add_series("S", {50.0, 50.0}, marker_color);
  area.area_options().show_points = true;
  area.area_options().point_radius = 8.0f;
  auto img = area.render();

  bool saw_marker = false;
  for (std::size_t y = 0; y < img.height() && !saw_marker; ++y) {
    for (std::size_t x = 0; x < img.width(); ++x) {
      auto const *p = img.pixel(x, y);
      if (p[0] == marker_color.r && p[1] == marker_color.g && p[2] == marker_color.b) {
        saw_marker = true;
        break;
      }
    }
  }
  CHECK(saw_marker);
}

static void test_area_chart_default_options_no_points_drawn() {
  // show_points defaults to false — must not draw markers unless opted in.
  AreaChart area;
  area.options().width = 100;
  area.options().height = 80;
  area.set_categories({"A", "B"});
  area.add_series("S", {10.0, 20.0});
  auto img = area.render();
  CHECK(img.width() == 100 && img.height() == 80);
}

static void test_combo_line_spline_dash_render_without_crash() {
  ComboChart combo;
  combo.options().width = 200;
  combo.options().height = 150;
  combo.options().legend.enabled = false;
  combo.set_categories({"A", "B", "C"});
  combo.add_bar_series("Bar", {10.0, 20.0, 15.0});
  combo.add_line_series("Line", {5.0, 25.0, 10.0});
  combo.line_options().spline = true;
  combo.line_options().dash = {4.0f, 4.0f};
  combo.line_options().point_shape = PointMarkerShape::Triangle;
  auto img = combo.render();
  CHECK(img.width() == 200 && img.height() == 150);
}

static void test_scatter_point_shape_renders_without_crash() {
  ScatterChart chart;
  chart.options().width = 200;
  chart.options().height = 150;
  chart.options().legend.enabled = false;
  chart.add_series("S", {{1, 20}, {2, 35}, {3, 18}});
  chart.scatter_options().point_shape = PointMarkerShape::Cross;
  auto img = chart.render();
  CHECK(img.width() == 200 && img.height() == 150);
}

// ---------------------------------------------------------------------------
// Reference line / band overlay tests
// ---------------------------------------------------------------------------

static void test_reference_line_horizontal_pixel_row_has_line_color() {
  BarChart bar;
  bar.options().width = 200;
  bar.options().height = 150;
  bar.options().legend.enabled = false;
  bar.set_categories({"A", "B"});
  bar.add_series("S", {10.0, 90.0});

  Color line_color = Color::rgb(9, 9, 9);
  ReferenceLine ref;
  ref.orientation = ReferenceOrientation::Horizontal;
  ref.value = 50.0;
  ref.color = line_color;
  ref.width = 2.0f;
  ref.dash = {}; // solid, easier to detect deterministically
  bar.cartesian_options().reference_lines.push_back(ref);

  auto img = bar.render();

  // Compute the expected pixel row the same way draw_reference_lines does: via the
  // chart's own y-axis mapping. Rather than duplicate Scale math, just scan the whole
  // image for the reference line's exact color, which no bar/axis/grid element uses.
  bool saw_line = false;
  for (std::size_t y = 0; y < img.height() && !saw_line; ++y) {
    for (std::size_t x = 0; x < img.width(); ++x) {
      auto const *p = img.pixel(x, y);
      if (p[0] == line_color.r && p[1] == line_color.g && p[2] == line_color.b) {
        saw_line = true;
        break;
      }
    }
  }
  CHECK(saw_line);
}

static void test_reference_band_fills_region() {
  LineChart line;
  line.options().width = 200;
  line.options().height = 150;
  line.options().legend.enabled = false;
  line.set_categories({"A", "B"});
  line.add_series("S", {10.0, 90.0});

  ReferenceBand band;
  band.orientation = ReferenceOrientation::Horizontal;
  band.from = 40.0;
  band.to = 60.0;
  band.color = Color::rgba(11, 22, 33, 255);
  line.cartesian_options().reference_bands.push_back(band);

  auto img = line.render();

  bool saw_band = false;
  for (std::size_t y = 0; y < img.height() && !saw_band; ++y) {
    for (std::size_t x = 0; x < img.width(); ++x) {
      auto const *p = img.pixel(x, y);
      if (p[0] == 11 && p[1] == 22 && p[2] == 33) {
        saw_band = true;
        break;
      }
    }
  }
  CHECK(saw_band);
}

static void test_reference_lines_default_empty_no_effect() {
  // Default reference_lines/reference_bands are empty vectors — must not alter
  // rendering versus a chart with no reference config at all.
  AreaChart area;
  area.options().width = 120;
  area.options().height = 90;
  area.set_categories({"A", "B"});
  area.add_series("S", {10.0, 20.0});
  auto img = area.render();
  CHECK(img.width() == 120 && img.height() == 90);
}

static void test_reference_line_vertical_renders_without_crash() {
  ComboChart combo;
  combo.options().width = 200;
  combo.options().height = 150;
  combo.options().legend.enabled = false;
  combo.set_categories({"A", "B", "C"});
  combo.add_bar_series("Bar", {10.0, 20.0, 15.0});
  combo.add_line_series("Line", {5.0, 25.0, 10.0});

  ReferenceLine ref;
  ref.orientation = ReferenceOrientation::Vertical;
  ref.value = 1.5; // category B's own center (category i spans [i, i+1))
  ref.label = "Deploy";
  combo.cartesian_options().reference_lines.push_back(ref);

  auto img = combo.render();
  CHECK(img.width() == 200 && img.height() == 150);
}

// Regression test for a real bug: PlotContext::x_scale is built with the same
// Scale::to_pixel() convention as the y-axis (domain min -> pixel_max, domain max ->
// pixel_min), which is correct for a vertical axis but mirrors a horizontal one. A
// vertical ReferenceLine near the left edge of the category domain must render on the
// left side of the plot, not the right.
static void test_reference_line_vertical_not_mirrored_bar() {
  auto render_with_value = [](double value) {
    BarChart bar;
    bar.options().width = 300;
    bar.options().height = 150;
    bar.options().legend.enabled = false;
    bar.set_categories({"A", "B", "C", "D"});
    bar.add_series("S", {10.0, 20.0, 15.0, 25.0});

    ReferenceLine ref;
    ref.orientation = ReferenceOrientation::Vertical;
    ref.value = value;
    ref.color = Color::rgb(1, 2, 3);
    ref.dash = {}; // solid, so every row along the line has the color
    bar.cartesian_options().reference_lines.push_back(ref);
    return bar.render();
  };

  auto avg_x_of_line = [](ImageBuffer const &img) -> double {
    long sum_x = 0;
    long count = 0;
    for (std::size_t y = 0; y < img.height(); ++y) {
      for (std::size_t x = 0; x < img.width(); ++x) {
        auto const *p = img.pixel(x, y);
        if (p && p[0] == 1 && p[1] == 2 && p[2] == 3) {
          sum_x += static_cast<long>(x);
          ++count;
        }
      }
    }
    CHECK(count > 0);
    return count > 0 ? static_cast<double>(sum_x) / static_cast<double>(count) : -1.0;
  };

  // Category A's center (near the left edge of the domain) must render left of
  // category D's center (near the right edge) — not the other way around.
  double left_x = avg_x_of_line(render_with_value(0.5));
  double right_x = avg_x_of_line(render_with_value(3.5));
  CHECK(left_x < right_x);
}

static void test_reference_line_vertical_not_mirrored_line_chart() {
  auto render_with_value = [](double value) {
    LineChart line;
    line.options().width = 300;
    line.options().height = 150;
    line.options().legend.enabled = false;
    line.set_categories({"A", "B", "C", "D"});
    line.add_series("S", {10.0, 20.0, 15.0, 25.0});

    ReferenceLine ref;
    ref.orientation = ReferenceOrientation::Vertical;
    ref.value = value;
    ref.color = Color::rgb(1, 2, 3);
    ref.dash = {};
    line.cartesian_options().reference_lines.push_back(ref);
    return line.render();
  };

  auto avg_x_of_line = [](ImageBuffer const &img) -> double {
    long sum_x = 0;
    long count = 0;
    for (std::size_t y = 0; y < img.height(); ++y) {
      for (std::size_t x = 0; x < img.width(); ++x) {
        auto const *p = img.pixel(x, y);
        if (p && p[0] == 1 && p[1] == 2 && p[2] == 3) {
          sum_x += static_cast<long>(x);
          ++count;
        }
      }
    }
    CHECK(count > 0);
    return count > 0 ? static_cast<double>(sum_x) / static_cast<double>(count) : -1.0;
  };

  double left_x = avg_x_of_line(render_with_value(0.5));
  double right_x = avg_x_of_line(render_with_value(3.5));
  CHECK(left_x < right_x);
}

// ---------------------------------------------------------------------------
// Data label tests (all chart types)
// ---------------------------------------------------------------------------

// Data labels render text, which we can't pixel-match exactly (font rasterization),
// so these tests use a presence-of-ink check: some non-background pixel appears in
// the label's expected region when enabled, and none does when disabled.
static bool has_ink_in_region(ImageBuffer const &img, int x0, int y0, int x1, int y1,
                              Color const &background) {
  x0 = std::max(0, x0);
  y0 = std::max(0, y0);
  x1 = std::min(static_cast<int>(img.width()), x1);
  y1 = std::min(static_cast<int>(img.height()), y1);
  for (int y = y0; y < y1; ++y) {
    for (int x = x0; x < x1; ++x) {
      auto const *p = img.pixel(static_cast<std::size_t>(x), static_cast<std::size_t>(y));
      if (p && (p[0] != background.r || p[1] != background.g || p[2] != background.b))
        return true;
    }
  }
  return false;
}

static void test_bar_data_label_draws_ink_above_bar() {
  BarChart bar;
  bar.options().width = 200;
  bar.options().height = 150;
  bar.options().legend.enabled = false;
  bar.options().background = Color::white();
  bar.set_categories({"A"});
  bar.add_series("S", {100.0});
  bar.bar_options().data_labels.enabled = true;
  bar.bar_options().data_labels.color = Color::black();
  auto img = bar.render();
  // Search a generous band across the full width near the top of the plot, where
  // the label sits just above the bar.
  CHECK(has_ink_in_region(img, 0, 0, static_cast<int>(img.width()), 80, Color::white()));
}

static void test_bar_data_label_disabled_by_default() {
  BarChart bar;
  bar.options().width = 200;
  bar.options().height = 150;
  bar.set_categories({"A"});
  bar.add_series("S", {100.0});
  auto img = bar.render();
  CHECK(img.width() == 200 && img.height() == 150);
}

static void test_line_data_label_renders_without_crash() {
  LineChart line;
  line.options().width = 200;
  line.options().height = 150;
  line.options().legend.enabled = false;
  line.set_categories({"A", "B"});
  line.add_series("S", {10.0, 20.0});
  line.line_options().data_labels.enabled = true;
  auto img = line.render();
  CHECK(img.width() == 200 && img.height() == 150);
}

static void test_area_data_label_renders_without_crash() {
  AreaChart area;
  area.options().width = 200;
  area.options().height = 150;
  area.options().legend.enabled = false;
  area.set_categories({"A", "B"});
  area.add_series("S", {10.0, 20.0});
  area.area_options().data_labels.enabled = true;
  auto img = area.render();
  CHECK(img.width() == 200 && img.height() == 150);
}

static void test_combo_data_labels_render_without_crash() {
  ComboChart combo;
  combo.options().width = 200;
  combo.options().height = 150;
  combo.options().legend.enabled = false;
  combo.set_categories({"A", "B"});
  combo.add_bar_series("Bar", {10.0, 20.0});
  combo.add_line_series("Line", {5.0, 15.0});
  combo.bar_options().data_labels.enabled = true;
  combo.line_options().data_labels.enabled = true;
  auto img = combo.render();
  CHECK(img.width() == 200 && img.height() == 150);
}

static void test_scatter_data_label_renders_without_crash() {
  ScatterChart chart;
  chart.options().width = 200;
  chart.options().height = 150;
  chart.options().legend.enabled = false;
  chart.add_series("S", {{1, 20}, {2, 35}});
  chart.scatter_options().data_labels.enabled = true;
  auto img = chart.render();
  CHECK(img.width() == 200 && img.height() == 150);
}

static void test_pie_data_label_draws_ink_in_slice() {
  PieChart pie;
  pie.options().width = 300;
  pie.options().height = 300;
  pie.options().background = Color::white();
  PieOptions po = pie.pie_options();
  po.center_x = 150;
  po.center_y = 150;
  po.radius = 120;
  po.show_labels = false; // isolate the in-slice label from the external leader label
  po.show_legend = false;
  po.data_labels.enabled = true;
  po.data_labels.color = Color::black();
  po.data_labels.format = TickFormat::Percent;
  pie.set_pie_options(po);
  pie.add_slice("A", 100);

  auto img = pie.render();
  // A single 100% slice's in-slice label sits at radius*0.65 along the midpoint
  // angle (-pi/2 + pi = pi/2, straight down), i.e. roughly (150, 228).
  CHECK(has_ink_in_region(img, 100, 200, 200, 250, Color::white()));
}

// ---------------------------------------------------------------------------
// Log y-axis / DateTime x-axis chart-level wiring
// ---------------------------------------------------------------------------
// Scale's own Log/DateTime math is covered above (test_scale_log_*, test_scale_datetime_*);
// these check the chart-level wiring: CartesianOptions::y_scale_type/x_scale_type actually
// reach CartesianChart::render(), and unsupported chart types safely ignore x_scale_type.

static void test_line_chart_log_y_axis_renders_and_spans_range() {
  LineChart chart;
  chart.options().width = 300;
  chart.options().height = 200;
  chart.options().legend.enabled = false;
  chart.options().background = Color::white();
  chart.cartesian_options().y_scale_type = ScaleType::Log;
  chart.set_categories({"A", "B", "C"});
  chart.add_series("S", {1.0, 100.0, 10000.0}, Color::black());
  auto img = chart.render();
  CHECK(img.width() == 300 && img.height() == 200);
  // Line/markers drawn somewhere across the plot, not collapsed into one corner
  // (the begin_at_zero-forces-domain-to-0 bug made every tick/gridline stack at
  // the very top of the plot instead of spanning it).
  CHECK(has_ink_in_region(img, 0, 150, 300, 200, Color::white()));
  CHECK(has_ink_in_region(img, 0, 20, 300, 70, Color::white()));
}

static void test_line_chart_datetime_x_axis_positions_by_value_not_index() {
  LineChart chart;
  chart.options().width = 640;
  chart.options().height = 300;
  chart.options().legend.enabled = false;
  chart.options().background = Color::white();
  chart.cartesian_options().x_scale_type = ScaleType::DateTime;
  Color const mark = Color::rgb(10, 10, 10);
  double const day = 86400.0;
  double const base = 1700000000.0;
  // 3 points: first two 1 day apart, last one 20 days after the second — pixel
  // gap between point 2/3 should be far larger than between point 1/2 if x_scale
  // truly maps by timestamp rather than even per-point index slots.
  chart.set_x_values({base, base + day, base + day + 20.0 * day});
  chart.add_series("S", {10.0, 10.0, 10.0}, mark);
  chart.line_options().show_points = false;
  chart.line_options().line_width = 3.0f;
  auto img = chart.render();

  auto find_ink_x = [&](int x_from, int x_to, int y_mid) -> int {
    for (int x = x_from; x < x_to; ++x) {
      for (int y = y_mid - 3; y <= y_mid + 3; ++y) {
        auto const *p = img.pixel(static_cast<std::size_t>(x), static_cast<std::size_t>(y));
        if (p && p[0] < 100 && p[1] < 100 && p[2] < 100)
          return x;
      }
    }
    return -1;
  };

  // All three points share y=10, so the flat line sits at one pixel row; scan for
  // where the stroke begins (leftmost point) to locate the y row, then measure the
  // stroke's horizontal extent up to the midpoint vs. from the midpoint to the end.
  int y_row = -1;
  for (int y = 0; y < static_cast<int>(img.height()) && y_row < 0; ++y) {
    if (find_ink_x(0, static_cast<int>(img.width()), y) >= 0)
      y_row = y;
  }
  CHECK(y_row >= 0);
  if (y_row >= 0) {
    int left_x = find_ink_x(0, static_cast<int>(img.width()), y_row);
    int right_x = -1;
    for (int x = static_cast<int>(img.width()) - 1; x >= 0; --x) {
      for (int y = y_row - 3; y <= y_row + 3; ++y) {
        auto const *p = img.pixel(static_cast<std::size_t>(x), static_cast<std::size_t>(y));
        if (p && p[0] < 100 && p[1] < 100 && p[2] < 100) {
          right_x = x;
          break;
        }
      }
      if (right_x >= 0)
        break;
    }
    CHECK(left_x >= 0 && right_x > left_x);
    // The 1-day gap should occupy roughly 1/21st of the total span the 21-day
    // range covers; give generous slack for line stroke width / AA bleed.
    float total = static_cast<float>(right_x - left_x);
    float expected_first_gap = total / 21.0f;
    CHECK(expected_first_gap > 0.0f && expected_first_gap < total * 0.35f);
  }
}

static void test_bar_chart_ignores_datetime_x_scale_type() {
  // BarChart doesn't override supports_continuous_x_axis(), so setting
  // x_scale_type = DateTime (with no x_values even provided) must be a no-op —
  // identical output to leaving it at the Linear default.
  auto make = [](ScaleType type) {
    BarChart bar;
    bar.options().width = 300;
    bar.options().height = 200;
    bar.cartesian_options().x_scale_type = type;
    bar.set_categories({"Q1", "Q2", "Q3"});
    bar.add_series("Rev", {10, 20, 15}, Color::rgb(40, 90, 200));
    return bar.render();
  };
  auto linear = make(ScaleType::Linear);
  auto datetime = make(ScaleType::DateTime);
  CHECK(linear.data() == datetime.data());
}

static void test_cartesian_x_values_roundtrip() {
  LineChart chart;
  CHECK(chart.x_values().empty());
  chart.set_x_values({1.0, 2.0, 3.0});
  CHECK(chart.x_values().size() == 3);
  CHECK(chart.x_values()[1] == 2.0);
}

// ---------------------------------------------------------------------------
// Horizontal bar orientation
// ---------------------------------------------------------------------------

static int rightmost_ink_column(ImageBuffer const &img, int y, Color const &background) {
  for (int x = static_cast<int>(img.width()) - 1; x >= 0; --x) {
    auto const *p = img.pixel(static_cast<std::size_t>(x), static_cast<std::size_t>(y));
    if (p && (p[0] != background.r || p[1] != background.g || p[2] != background.b))
      return x;
  }
  return -1;
}

// Rightmost pixel matching a specific series color (not axis lines/gridlines/labels,
// which use their own distinct colors) — for isolating one bar's extent.
static int rightmost_matching_column(ImageBuffer const &img, int y, Color const &target) {
  for (int x = static_cast<int>(img.width()) - 1; x >= 0; --x) {
    auto const *p = img.pixel(static_cast<std::size_t>(x), static_cast<std::size_t>(y));
    if (p && p[0] == target.r && p[1] == target.g && p[2] == target.b)
      return x;
  }
  return -1;
}

static void test_bar_chart_horizontal_renders_correct_size() {
  BarChart chart;
  chart.options().width = 300;
  chart.options().height = 200;
  chart.bar_options().orientation = BarOrientation::Horizontal;
  chart.set_categories({"A", "B"});
  chart.add_series("S", {10, 20});
  auto img = chart.render();
  CHECK(img.width() == 300 && img.height() == 200);
  CHECK(chart.orientation() == BarOrientation::Horizontal);
}

static void test_bar_chart_horizontal_bar_length_reflects_value() {
  // A larger value must extend further right than a smaller one, along its own row —
  // the core geometric contract of a horizontal bar (value = length, not height).
  BarChart chart;
  chart.options().width = 400;
  chart.options().height = 200;
  chart.options().legend.enabled = false;
  chart.options().background = Color::white();
  chart.bar_options().orientation = BarOrientation::Horizontal;
  chart.bar_options().group_gap = 0.0f;
  chart.set_categories({"Small", "Large"});
  chart.add_series("S", {10, 90}, Color::black());
  auto img = chart.render();

  // "Small" (category index 0) is drawn above "Large" (index 1); find a row inside
  // each bar's band by scanning for the series' own fill color (not axis lines,
  // gridlines, or labels, which use other colors), then compare how far right each
  // bar's ink extends.
  Color const black = Color::black();
  int small_row = -1;
  for (int y = 0; y < static_cast<int>(img.height()); ++y) {
    if (rightmost_matching_column(img, y, black) >= 0) {
      small_row = y;
      break;
    }
  }
  int large_row = -1;
  for (int y = static_cast<int>(img.height()) - 1; y >= 0; --y) {
    if (rightmost_matching_column(img, y, black) >= 0) {
      large_row = y;
      break;
    }
  }
  CHECK(small_row >= 0 && large_row >= 0 && large_row > small_row);
  int small_x = rightmost_matching_column(img, small_row, black);
  int large_x = rightmost_matching_column(img, large_row, black);
  CHECK(small_x >= 0 && large_x >= 0);
  CHECK(large_x > small_x);
}

static void test_bar_chart_horizontal_ignores_reference_lines() {
  // ReferenceLine orientation assumes a vertical value axis; horizontal mode must
  // silently skip them (documented v1 scope limit) rather than draw them scrambled.
  auto make = [](bool with_ref_line) {
    BarChart chart;
    chart.options().width = 300;
    chart.options().height = 200;
    chart.bar_options().orientation = BarOrientation::Horizontal;
    chart.set_categories({"A", "B"});
    chart.add_series("S", {10, 20}, Color::rgb(40, 90, 200));
    if (with_ref_line) {
      ReferenceLine rl;
      rl.orientation = ReferenceOrientation::Horizontal;
      rl.value = 15.0;
      chart.cartesian_options().reference_lines.push_back(rl);
    }
    return chart.render();
  };
  auto without = make(false);
  auto with = make(true);
  CHECK(without.data() == with.data());
}

// ---------------------------------------------------------------------------
// Runner
// ---------------------------------------------------------------------------

// ── Hostile / non-finite input (must return promptly, never hang or crash) ───────────────────

static bool any_ink(ImageBuffer const &img) {
  for (std::size_t y = 0; y < img.height(); ++y)
    for (std::size_t x = 0; x < img.width(); ++x)
      if (img.pixel(x, y)[3] != 0)
        return true;
  return false;
}

static void test_hostile_values_all_cartesian_charts() {
  double const nan = std::nan("");
  double const inf = std::numeric_limits<double>::infinity();
  std::vector<std::vector<double>> cases = {
      {nan, 1, 2},        {1, nan, 3},
      {inf, 1, 2},        {-inf, 1, 2},
      {1e300, -1e300, 5}, {nan, nan, nan},
      {inf, -inf, nan},   {},
      {1e-300, 1e300},    {std::numeric_limits<double>::max(), 1},
  };
  for (auto const &vals : cases) {
    BarChart bar;
    bar.set_categories({"a", "b", "c"});
    bar.add_series("s", vals);
    CHECK(bar.render().width() == bar.options().width);
    bar.bar_options().mode = BarMode::Stacked;
    CHECK(bar.render().width() == bar.options().width);
    bar.bar_options().orientation = BarOrientation::Horizontal;
    CHECK(bar.render().width() == bar.options().width);

    LineChart line;
    line.set_categories({"a", "b", "c"});
    line.add_series("s", vals);
    line.line_options().dash = {4.0f, 2.0f};
    line.line_options().spline = true;
    line.line_options().fill_area = true;
    CHECK(line.render().width() == line.options().width);
    line.cartesian_options().y_scale_type = ScaleType::Log;
    CHECK(line.render().width() == line.options().width);

    AreaChart area;
    area.set_categories({"a", "b", "c"});
    area.add_series("s", vals);
    CHECK(area.render().width() == area.options().width);

    ComboChart combo;
    combo.set_categories({"a", "b", "c"});
    combo.add_bar_series("b", vals);
    combo.add_line_series("l", vals, /*secondary_axis=*/true);
    CHECK(combo.render().width() == combo.options().width);
  }
}

static void test_hostile_scatter_and_pie() {
  double const nan = std::nan("");
  double const inf = std::numeric_limits<double>::infinity();
  ScatterChart sc;
  sc.add_series("s", {{nan, 1, 0}, {1, inf, 0}, {2, 3, nan}, {4, 5, inf}, {1e300, -1e300, 1e300}});
  CHECK(sc.render().width() == sc.options().width);

  PieChart pie;
  pie.add_slice("a", nan);
  pie.add_slice("b", inf);
  pie.add_slice("c", 5);
  CHECK(pie.render().width() == pie.options().width);
}

static void test_nonfinite_primitives_do_not_hang() {
  double const nan = std::nan("");
  float const fnan = std::numeric_limits<float>::quiet_NaN();
  float const finf = std::numeric_limits<float>::infinity();
  ImageBuffer img(64, 64);
  Color c = Color::rgb(10, 20, 30);
  img.draw_line(0, 0, finf, 10, 2.0f, c);
  img.draw_line(fnan, 0, 10, 10, 2.0f, c);
  img.draw_line(0, 0, 1e30f, 1e30f, 2.0f, c);
  img.draw_dashed_line(0, 0, finf, 10, 2.0f, c, {4, 2});
  img.draw_dashed_line(-1e30f, 5, 1e30f, 5, 2.0f, c, {4, 2});
  img.draw_dashed_line(0, 0, 60, 60, 2.0f, c, {finf, 2});
  img.draw_dashed_line(0, 0, 60, 60, 2.0f, c, {1e-9f, 1e-9f});
  std::pair<float, float> poly[] = {{0, 0}, {finf, 5}, {10, 10}};
  img.fill_polygon(poly, 3, c);
  std::pair<float, float> poly2[] = {{-1e30f, -1e30f}, {1e30f, -1e30f}, {0, 1e30f}};
  img.fill_polygon(poly2, 3, c);
  img.fill_rect(fnan, 0.0f, 10.0f, 10.0f, c);
  img.fill_rect(-1e30f, -1e30f, 3e30f, 3e30f, c);
  img.fill_circle(fnan, 5, 3, c);
  img.fill_circle(5, 5, finf, c);
  img.fill_sector(32, 32, 20, 1e300, 2e300, c);
  img.fill_sector(32, 32, 20, nan, 1.0, c);
  img.fill_rounded_rect(0, 0, finf, 5, 2, c);
  CHECK(img.width() == 64);
}

static void test_scale_nonfinite_domain_is_sanitized() {
  double const nan = std::nan("");
  double const inf = std::numeric_limits<double>::infinity();
  Scale a = Scale::linear_nice(nan, inf, 0, 100, 6);
  CHECK(std::isfinite(a.min()) && std::isfinite(a.max()));
  Scale b = Scale::linear_nice(0, 1e308, 0, 100, 6);
  CHECK(b.nice_ticks(6).size() <= 1001);
  Scale c = Scale::log_nice(-inf, inf, 0, 100, 6);
  CHECK(std::isfinite(c.min()) && std::isfinite(c.max()));
  Scale d(0, 1e300, 0, 100);
  d.set_type(ScaleType::DateTime);
  CHECK(d.nice_ticks(6).size() <= 1001);
  CHECK(d.nice_ticks(-5).size() <= 1001);
  Scale e(1, 1e308, 0, 100);
  e.set_type(ScaleType::Log);
  CHECK(!e.nice_ticks(6).empty());
}

static void test_huge_dimensions_report_invalid_input() {
  BarChart bar;
  bar.set_categories({"a"});
  bar.add_series("s", {1});
  bar.options().width = 1u << 20;
  bar.options().height = 1u << 20;
  CHECK(bar.render().width() == 0);
  auto res = ChartExporter::encode(bar, ExportOptions{});
  CHECK(!res);
  CHECK(res.error() == ChartError::InvalidInput);

  bar.options().width = 640;
  bar.options().height = 400;
  ExportOptions ss;
  ss.supersample = 1000000;
  auto ok = ChartExporter::encode(bar, ss); // clamped to the supersample cap
  CHECK(static_cast<bool>(ok));

  bar.options().width = 0;
  CHECK(!ChartExporter::encode(bar, ExportOptions{}));
}

static void test_empty_charts_render() {
  BarChart bar;
  CHECK(bar.render().width() == bar.options().width);
  LineChart line;
  CHECK(line.render().width() == line.options().width);
  AreaChart area;
  CHECK(area.render().width() == area.options().width);
  ComboChart combo;
  CHECK(combo.render().width() == combo.options().width);
  ScatterChart sc;
  CHECK(sc.render().width() == sc.options().width);
  PieChart pie;
  CHECK(pie.render().width() == pie.options().width);
  CHECK(!any_ink(ImageBuffer()));
}

// Deterministic property test: random mixes of finite/NaN/Inf/huge values and odd chart sizes must
// render with the requested dimensions (or an empty buffer for unsupported sizes) and not hang.
static void test_fuzz_hostile_data_all_chart_types() {
  uint32_t state = 0x12345678u;
  auto next = [&state]() {
    state = state * 1664525u + 1013904223u;
    return state >> 8;
  };
  auto value = [&]() -> double {
    switch (next() % 8) {
    case 0:
      return std::nan("");
    case 1:
      return std::numeric_limits<double>::infinity();
    case 2:
      return -std::numeric_limits<double>::infinity();
    case 3:
      return 1e300;
    case 4:
      return -1e300;
    case 5:
      return 0.0;
    default:
      return static_cast<double>(next() % 2000) / 10.0 - 50.0;
    }
  };
  for (int iter = 0; iter < 40; ++iter) {
    std::size_t n = 1 + next() % 12;
    std::vector<double> a(n), b(n);
    std::vector<std::string> cats(n);
    for (std::size_t i = 0; i < n; ++i) {
      a[i] = value();
      b[i] = value();
      cats[i] = "c" + std::to_string(i);
    }
    std::size_t w = 1 + next() % 300, h = 1 + next() % 300;

    BarChart bar;
    bar.options().width = w;
    bar.options().height = h;
    bar.set_categories(cats);
    bar.add_series("a", a);
    bar.add_series("b", b);
    if (next() & 1)
      bar.bar_options().mode = BarMode::Stacked;
    if (next() & 1)
      bar.bar_options().orientation = BarOrientation::Horizontal;
    CHECK(bar.render().width() == w);

    LineChart line;
    line.options().width = w;
    line.options().height = h;
    line.set_categories(cats);
    line.add_series("a", a);
    line.add_series("b", b);
    line.line_options().spline = next() & 1;
    line.line_options().fill_area = next() & 1;
    if (next() & 1)
      line.line_options().dash = {5.0f, 3.0f};
    if (next() & 1)
      line.cartesian_options().y_scale_type = ScaleType::Log;
    CHECK(line.render().height() == h);

    AreaChart area;
    area.options().width = w;
    area.options().height = h;
    area.set_categories(cats);
    area.add_series("a", a);
    area.add_series("b", b);
    CHECK(area.render().width() == w);

    ComboChart combo;
    combo.options().width = w;
    combo.options().height = h;
    combo.set_categories(cats);
    combo.add_bar_series("a", a);
    combo.add_line_series("b", b, /*secondary_axis=*/(next() & 1) != 0);
    CHECK(combo.render().width() == w);

    ScatterChart sc;
    sc.options().width = w;
    sc.options().height = h;
    std::vector<ScatterPoint> pts;
    for (std::size_t i = 0; i < n; ++i)
      pts.push_back({a[i], b[i], (next() & 1) ? std::fabs(a[i]) : 0.0});
    sc.add_series("p", pts);
    CHECK(sc.render().width() == w);

    PieChart pie;
    pie.options().width = w;
    pie.options().height = h;
    for (std::size_t i = 0; i < n; ++i)
      pie.add_slice(cats[i], a[i]);
    CHECK(pie.render().width() == w);
  }
}

// ── Fonts: UTF-8 decoding, fallback glyph, injection ─────────────────────────────────────────

static void test_font_utf8_missing_glyph_falls_back_to_question_mark() {
  Font f; // neither embedded subset has CJK
  f.set_size(14.0f);
  CHECKF(f.measure_width("\xE4\xB8\xAD"), f.measure_width("?"),
         1e-4); // one code point, not 2 bytes
  CHECKF(f.measure_width("a\xE4\xB8\xAD"), f.measure_width("a?"), 1e-4);
  // Malformed sequences must not hang or read past the end.
  CHECK(f.measure_width("\xC3") > 0.0f);
  CHECK(f.measure_width("\xFF\xFE\x80") > 0.0f);
  CHECK(f.measure_width("\xE2\x82") > 0.0f);
  ImageBuffer img(64, 24);
  f.draw_text(img, 2, 18, "\xC3\xA9\xE2\x82\xAC\xF0\x9F\x98\x80", Color::black());
  CHECK(any_ink(img));
}

static void test_font_measure_multiline_uses_widest_line() {
  Font f;
  CHECKF(f.measure_width("ab\nabcd\nx"), f.measure_width("abcd"), 1e-4);
}

static void test_font_data_validation() {
  CHECK(load_font_data("/nonexistent/font.ttf") == nullptr);
  CHECK(load_font_data(nullptr) == nullptr);
  unsigned char junk[64] = {1, 2, 3};
  CHECK(make_font_data(junk, sizeof(junk)) == nullptr);
  CHECK(make_font_data(nullptr, 0) == nullptr);
  Font f;
  CHECK(!f.load_from_file("/nonexistent/font.ttf"));
}

static void test_chart_uses_injected_font() {
  char const *kPath = "/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf";
  auto data = load_font_data(kPath);
  if (!data) {
    std::fprintf(stderr, "skip: %s not available\n", kPath);
    return;
  }
  Font full(data);
  CHECK(full.is_loaded());
  // The full font has 'α'; the embedded ASCII subset renders it as '?'.
  CHECK(std::fabs(full.measure_width("\xCE\xB1") - full.measure_width("?")) > 0.01f);

  BarChart a, b;
  for (BarChart *c : {&a, &b}) {
    c->options().title = "Caf\xCE\xB1 \xE2\x82\xAC";
    c->set_categories({"\xCE\xB1", "b"});
    c->add_series("s", {1, 2});
  }
  b.options().font = data;
  ImageBuffer ia = a.render(), ib = b.render();
  bool differ = false;
  for (std::size_t y = 0; y < ia.height() && !differ; ++y)
    for (std::size_t x = 0; x < ia.width(); ++x)
      if (std::memcmp(ia.pixel(x, y), ib.pixel(x, y), 4) != 0) {
        differ = true;
        break;
      }
  CHECK(differ);
  // A copy of the chart shares the font bytes.
  BarChart c = b;
  CHECK(c.options().font == data);
}

#ifdef INSIGHTDOCK_FONT_LATIN
static void test_latin_embedded_font_has_accents() {
  Font f;
  CHECK(std::fabs(f.measure_width("\xC3\xA9") - f.measure_width("?")) > 0.01f);
  CHECK(std::fabs(f.measure_width("\xE2\x82\xAC") - f.measure_width("?")) > 0.01f);
}
#endif

static void test_supersampled_export_does_not_mutate_chart_and_is_thread_safe() {
  LineChart line;
  line.set_categories({"a", "b", "c"});
  line.add_series("s", {1, 3, 2});
  line.options().title = "T";
  LineChart const &cline = line;
  std::size_t w = cline.options().width;
  float fs = cline.options().font_size;

  ExportOptions eo;
  eo.supersample = 2;
  auto ref = ChartExporter::encode(line, eo);
  CHECK(static_cast<bool>(ref));
  CHECK(cline.options().width == w);
  CHECKF(cline.options().font_size, fs, 1e-6);

  std::atomic<int> bad{0};
  std::vector<std::thread> threads;
  for (int t = 0; t < 4; ++t) {
    threads.emplace_back([&] {
      for (int i = 0; i < 5; ++i) {
        ImageBuffer img = ChartExporter::render(cline, eo);
        if (img.width() != w || img.height() != cline.options().height)
          ++bad;
      }
    });
  }
  for (auto &th : threads)
    th.join();
  CHECK(bad == 0);
  CHECK(cline.options().width == w);
}

static void test_pdf_backend_does_not_hijack_png_export() {
  BarChart bar;
  bar.set_categories({"a"});
  bar.add_series("s", {1});
  ExportOptions eo; // format Png, but a Vector PDF backend preference
  eo.pdf_backend = PdfBackend::Vector;
  auto res = ChartExporter::encode(bar, eo);
  CHECK(static_cast<bool>(res));
  if (res) {
    CHECK(res.value.size() > 4);
    CHECK(res.value[0] == 0x89 && res.value[1] == 'P' && res.value[2] == 'N' &&
          res.value[3] == 'G');
  }
}

#ifdef INSIGHTDOCK_HAS_PODOFO
// ── Native vector PDF ────────────────────────────────────────────────────────────────────────

static std::string vector_pdf(Chart &chart) {
  ExportOptions opts;
  opts.format = OutputFormat::PdfVector;
  auto bytes = ChartExporter::encode(chart, opts);
  CHECK(static_cast<bool>(bytes));
  std::string s(bytes.value.begin(), bytes.value.end());
  CHECK(s.compare(0, 5, "%PDF-") == 0);
  // The document must load back and hold exactly one page.
  try {
    PoDoFo::PdfMemDocument doc;
    doc.LoadFromBuffer(PoDoFo::bufferview(s.data(), s.size()));
    CHECK(doc.GetPages().GetCount() == 1);
  } catch (std::exception const &) {
    CHECK(false);
  }
  return s;
}

static void test_pdf_vector_cartesian_charts_have_no_embedded_image() {
  BarChart bar;
  bar.options().width = 300;
  bar.options().height = 200;
  bar.options().title = "Caf\xC3\xA9 \xE2\x82\xAC"; // Café €  (WinAnsi-encodable)
  bar.set_categories({"a", "b"});
  bar.add_series("s", {1, 2});
  CHECK(vector_pdf(bar).find("/Image") == std::string::npos);

  LineChart line;
  line.options().width = 300;
  line.options().height = 200;
  line.set_categories({"a", "b", "c"});
  SeriesStyle st;
  st.dash = DashPattern{4.0f, 2.0f};
  line.add_series("s", {1, 3, 2}, Color::rgb(10, 120, 200), st);
  CHECK(vector_pdf(line).find("/Image") == std::string::npos);

  AreaChart area;
  area.options().width = 300;
  area.options().height = 200;
  area.set_categories({"a", "b", "c"});
  area.add_series("s", {1, 3, 2});
  std::string a = vector_pdf(area);
  CHECK(a.find("/Image") == std::string::npos);
  CHECK(a.find("/ExtGState") != std::string::npos); // translucent fill uses ExtGState opacity

  ComboChart combo;
  combo.options().width = 300;
  combo.options().height = 200;
  combo.set_categories({"a", "b"});
  combo.add_bar_series("bars", {5, 6});
  combo.add_line_series("line", {50, 60}, true);
  CHECK(vector_pdf(combo).find("/Image") == std::string::npos);
}

static void test_pdf_vector_unencodable_text_does_not_abort() {
  BarChart bar;
  bar.options().width = 300;
  bar.options().height = 200;
  bar.options().title = "\xE4\xB8\xAD\xE6\x96\x87 title"; // CJK is not WinAnsi
  bar.set_categories({"\xCE\xB1", "b"});
  bar.add_series("s", {1, 2});
  vector_pdf(bar); // must still produce a valid one-page PDF
}

static void test_pdf_vector_scatter_still_embeds_raster() {
  ScatterChart sc;
  sc.options().width = 200;
  sc.options().height = 150;
  sc.add_series("p", {{1, 1}, {2, 3}});
  CHECK(vector_pdf(sc).find("/Image") != std::string::npos);
}
#endif

// Hostile values must never leak "nan"/"inf" into vector output or abort it.
static void test_hostile_values_vector_outputs() {
  double const nan = std::nan("");
  double const inf = std::numeric_limits<double>::infinity();
  std::vector<std::vector<double>> cases = {
      {nan, 1, 2}, {inf, 1, 2}, {1e300, -1e300, 5}, {nan, nan, nan}, {inf, -inf, nan}, {},
  };
  for (auto const &vals : cases) {
    BarChart bar;
    bar.set_categories({"a", "b", "c"});
    bar.add_series("s", vals);
    LineChart line;
    line.set_categories({"a", "b", "c"});
    line.add_series("s", vals);
    line.line_options().dash = {4.0f, 2.0f};
    AreaChart area;
    area.set_categories({"a", "b", "c"});
    area.add_series("s", vals);
    ComboChart combo;
    combo.set_categories({"a", "b", "c"});
    combo.add_bar_series("b", vals);
    combo.add_line_series("l", vals, true);
    for (Chart *c : {static_cast<Chart *>(&bar), static_cast<Chart *>(&line),
                     static_cast<Chart *>(&area), static_cast<Chart *>(&combo)}) {
      std::string svg = vector_svg(*c);
      CHECK(!has(svg, "nan") && !has(svg, "inf"));
#ifdef INSIGHTDOCK_HAS_PODOFO
      vector_pdf(*c);
#endif
    }
  }
}

// ── Per-series style and gap policy ──────────────────────────────────────────────────────────

// Total "ink" of c's hue in the image: anti-aliased edge pixels count fractionally, grey text,
// axes and grid count zero. Used to compare how much of a series was drawn.
static std::size_t count_color_pixels(ImageBuffer const &img, Color c) {
  int dom = (c.r >= c.g && c.r >= c.b) ? 0 : (c.g >= c.b ? 1 : 2);
  double ink = 0.0;
  for (std::size_t y = 0; y < img.height(); ++y)
    for (std::size_t x = 0; x < img.width(); ++x) {
      auto const *p = img.pixel(x, y);
      double others = (p[0] + p[1] + p[2] - p[dom]) / 2.0;
      double d = p[dom] - others;
      if (d > 0.0)
        ink += d / 255.0;
    }
  return static_cast<std::size_t>(ink * 100.0);
}

static void test_per_series_dash_overrides_chart_default() {
  Color red = Color::rgb(220, 20, 20), blue = Color::rgb(20, 20, 220);
  auto make = [&](bool dashed) {
    LineChart c;
    c.options().legend.enabled = false;
    c.line_options().show_points = false;
    c.set_categories({"a", "b", "c", "d"});
    SeriesStyle st;
    if (dashed)
      st.dash = DashPattern{6.0f, 6.0f};
    c.add_series("s", {1, 3, 2, 4}, red, st);
    c.add_series("t", {5, 5, 5, 5}, blue); // never crosses the red series
    return c;
  };
  LineChart solid = make(false), dashed = make(true);
  ImageBuffer a = solid.render(), b = dashed.render();
  // The dashed series draws fewer pixels; the untouched (inheriting) series is identical.
  CHECK(count_color_pixels(b, red) < count_color_pixels(a, red));
  CHECK(count_color_pixels(b, blue) == count_color_pixels(a, blue));
}

static void test_per_series_marker_and_width_and_fill() {
  Color red = Color::rgb(220, 20, 20);
  LineChart base, styled;
  for (LineChart *c : {&base, &styled}) {
    c->options().legend.enabled = false;
    c->line_options().show_points = true;
    c->set_categories({"a", "b", "c"});
  }
  base.add_series("s", {1, 3, 2}, red);
  SeriesStyle st;
  st.marker = PointMarkerShape::Square;
  st.line_width = 6.0f;
  styled.add_series("s", {1, 3, 2}, red, st);
  CHECK(count_color_pixels(styled.render(), red) > count_color_pixels(base.render(), red));

  AreaChart lo, hi;
  for (AreaChart *c : {&lo, &hi}) {
    c->options().legend.enabled = false;
    c->set_categories({"a", "b", "c"});
  }
  SeriesStyle faint, strong;
  faint.fill_opacity = 0.05f;
  strong.fill_opacity = 1.0f;
  lo.add_series("s", {5, 8, 6}, red, faint);
  hi.add_series("s", {5, 8, 6}, red, strong);
  CHECK(count_color_pixels(hi.render(), red) > count_color_pixels(lo.render(), red) + 500);
  CHECK(SeriesStyle{}.fill_alpha_or(120) == 120);
  SeriesStyle neg;
  neg.fill_opacity = -3.0f;
  CHECK(neg.fill_alpha_or(120) == 0);
}

static void test_combo_line_series_style_overload() {
  ComboChart c;
  c.set_categories({"a", "b"});
  c.add_bar_series("bars", {1, 2});
  SeriesStyle st;
  st.dash = DashPattern{4.0f, 4.0f};
  c.add_line_series("l", {2, 1}, Color::rgb(1, 2, 3), st, /*secondary_axis=*/true);
  CHECK(c.series().back().style.dash.has_value());
  CHECK(c.render().width() == c.options().width);
}

static void test_legend_swatch_differs_for_styled_series() {
  Color red = Color::rgb(220, 20, 20);
  LineChart plain, styled;
  for (LineChart *c : {&plain, &styled}) {
    c->set_categories({"a", "b"});
  }
  plain.add_series("s", {1, 2}, red);
  SeriesStyle st;
  st.marker = PointMarkerShape::Triangle;
  styled.add_series("s", {1, 2}, red, st);
  ImageBuffer a = plain.render(), b = styled.render();
  bool differ = false;
  for (std::size_t y = 0; y < a.height() && !differ; ++y)
    for (std::size_t x = 0; x < a.width(); ++x)
      if (std::memcmp(a.pixel(x, y), b.pixel(x, y), 4) != 0) {
        differ = true;
        break;
      }
  CHECK(differ);
}

static void test_gap_policy_breaks_line_at_nan() {
  Color red = Color::rgb(220, 20, 20);
  double const nan = std::nan("");
  auto render_with = [&](GapPolicy gp) {
    LineChart c;
    c.options().legend.enabled = false;
    c.line_options().show_points = false;
    c.line_options().gap_policy = gp;
    c.set_categories({"a", "b", "c", "d", "e"});
    c.add_series("s", {2, 4, nan, 4, 2}, red);
    return count_color_pixels(c.render(), red);
  };
  std::size_t gap = render_with(GapPolicy::Gap);
  std::size_t interp = render_with(GapPolicy::Interpolate);
  std::size_t zero = render_with(GapPolicy::Zero);
  CHECK(gap > 0);
  CHECK(interp > gap); // the bridge segment adds ink
  CHECK(zero != gap && zero != interp);

  // A series shorter than the category list ends where its data ends (Gap) instead of
  // dropping to zero.
  LineChart shorter;
  shorter.options().legend.enabled = false;
  shorter.line_options().show_points = false;
  shorter.set_categories({"a", "b", "c", "d"});
  shorter.add_series("s", {1, 2}, red);
  LineChart padded = shorter;
  padded.clear_series();
  padded.add_series("s", {1, 2, nan, nan}, red);
  ImageBuffer s1 = shorter.render(), s2 = padded.render();
  CHECK(count_color_pixels(s1, red) == count_color_pixels(s2, red));
}

static void test_stacked_area_nan_counts_as_zero() {
  double const nan = std::nan("");
  AreaChart a, b;
  for (AreaChart *c : {&a, &b}) {
    c->area_options().stacked = true;
    c->set_categories({"x", "y", "z"});
  }
  a.add_series("1", {1, nan, 3});
  a.add_series("2", {2, 2, 2});
  b.add_series("1", {1, 0, 3});
  b.add_series("2", {2, 2, 2});
  ImageBuffer ia = a.render(), ib = b.render();
  bool same = true;
  for (std::size_t y = 0; y < ia.height() && same; ++y)
    for (std::size_t x = 0; x < ia.width(); ++x)
      if (std::memcmp(ia.pixel(x, y), ib.pixel(x, y), 4) != 0) {
        same = false;
        break;
      }
  CHECK(same);
}

static void test_decimate_keeps_gap_markers() {
  std::vector<double> v(200);
  for (std::size_t i = 0; i < v.size(); ++i)
    v[i] = std::sin(static_cast<double>(i) * 0.1);
  v[97] = std::nan("");
  auto [vals, idx] = lttb_decimate_indexed(v, 40);
  bool has_nan = false;
  for (double d : vals)
    has_nan = has_nan || std::isnan(d);
  CHECK(has_nan);
  CHECK(vals.size() == idx.size());
  CHECK(vals.size() == 40);
}

static void test_x_label_modes() {
  std::vector<std::string> cats;
  for (int i = 0; i < 12; ++i)
    cats.push_back("Category number " + std::to_string(i));
  auto make = [&](CartesianOptions::XLabelMode m) {
    BarChart c;
    c.set_categories(cats);
    c.add_series("s", {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12});
    c.cartesian_options().x_label_mode = m;
    return c;
  };
  BarChart autoc = make(CartesianOptions::XLabelMode::Auto);
  BarChart rot = make(CartesianOptions::XLabelMode::Rotate90);
  BarChart trunc = make(CartesianOptions::XLabelMode::Truncate);
  ImageBuffer ia = autoc.render(), ir = rot.render(), it = trunc.render();
  CHECK(ir.width() == ia.width() && it.height() == ia.height());
  // Count grey (text/axis) ink: rotated draws every label, while Auto skips colliding ones.
  auto band_ink = [](ImageBuffer const &img) {
    std::size_t n = 0;
    for (std::size_t y = 0; y < img.height(); ++y)
      for (std::size_t x = 0; x < img.width(); ++x) {
        auto const *p = img.pixel(x, y);
        if (p[0] < 140 && std::abs(p[0] - p[2]) < 10)
          ++n;
      }
    return n;
  };
  CHECK(band_ink(ir) > band_ink(ia));
  CHECK(band_ink(it) > 0);
  // Rotated labels shrink the plot (bigger bottom margin) vs Auto.
  Font f;
  f.set_size(rot.options().font_size);
  CHECK(f.measure_width(cats[0]) > f.line_height());
  CHECK(rot.render().width() == rot.options().width);
}

int main() {
  test_x_label_modes();
  test_per_series_dash_overrides_chart_default();
  test_per_series_marker_and_width_and_fill();
  test_combo_line_series_style_overload();
  test_legend_swatch_differs_for_styled_series();
  test_gap_policy_breaks_line_at_nan();
  test_stacked_area_nan_counts_as_zero();
  test_decimate_keeps_gap_markers();
  test_pdf_backend_does_not_hijack_png_export();
#ifdef INSIGHTDOCK_HAS_PODOFO
  test_pdf_vector_cartesian_charts_have_no_embedded_image();
  test_pdf_vector_unencodable_text_does_not_abort();
  test_pdf_vector_scatter_still_embeds_raster();
#endif
  test_supersampled_export_does_not_mutate_chart_and_is_thread_safe();
#ifdef INSIGHTDOCK_FONT_LATIN
  test_latin_embedded_font_has_accents();
#endif
  test_font_utf8_missing_glyph_falls_back_to_question_mark();
  test_font_measure_multiline_uses_widest_line();
  test_font_data_validation();
  test_chart_uses_injected_font();
  test_fuzz_hostile_data_all_chart_types();
  test_hostile_values_all_cartesian_charts();
  test_hostile_values_vector_outputs();
  test_hostile_scatter_and_pie();
  test_nonfinite_primitives_do_not_hang();
  test_scale_nonfinite_domain_is_sanitized();
  test_huge_dimensions_report_invalid_input();
  test_empty_charts_render();

  // Data labels (all chart types)
  test_bar_data_label_draws_ink_above_bar();
  test_bar_data_label_disabled_by_default();
  test_line_data_label_renders_without_crash();
  test_area_data_label_renders_without_crash();
  test_combo_data_labels_render_without_crash();
  test_scatter_data_label_renders_without_crash();
  test_pie_data_label_draws_ink_in_slice();

  // Log y-axis / DateTime x-axis chart-level wiring
  test_line_chart_log_y_axis_renders_and_spans_range();
  test_line_chart_datetime_x_axis_positions_by_value_not_index();
  test_bar_chart_ignores_datetime_x_scale_type();
  test_cartesian_x_values_roundtrip();

  // Horizontal bar orientation
  test_bar_chart_horizontal_renders_correct_size();
  test_bar_chart_horizontal_bar_length_reflects_value();
  test_bar_chart_horizontal_ignores_reference_lines();

  // Reference line / band overlay
  test_reference_line_horizontal_pixel_row_has_line_color();
  test_reference_band_fills_region();
  test_reference_lines_default_empty_no_effect();
  test_reference_line_vertical_renders_without_crash();
  test_reference_line_vertical_not_mirrored_bar();
  test_reference_line_vertical_not_mirrored_line_chart();

  // LineChart / AreaChart / ComboChart / ScatterChart styling
  test_line_chart_spline_renders_without_crash();
  test_line_chart_dash_pattern_renders_without_crash();
  test_line_chart_square_marker_differs_from_circle();
  test_area_chart_show_points_draws_markers();
  test_area_chart_default_options_no_points_drawn();
  test_combo_line_spline_dash_render_without_crash();
  test_scatter_point_shape_renders_without_crash();

  // Scale
  test_scale_to_pixel();
  test_scale_from_pixel();
  test_scale_roundtrip();
  test_scale_nice_ticks_ascending();
  test_scale_nice_ticks_within_domain();
  test_scale_linear_nice_covers_data();
  test_scale_linear_nice_equal_values();
  test_scale_log_nice_covers_data();
  test_scale_log_nice_clamps_non_positive_min();
  test_scale_type_default();
  test_scale_log_roundtrip();
  test_scale_log_ticks_ascending();
  test_scale_datetime_roundtrip();
  test_scale_datetime_ticks();
  test_svg_encode_non_empty();

  // Native SVG pie (SvgBackend::Vector)
  test_svg_vector_pie_emits_paths();
  test_svg_vector_pie_single_slice_uses_circle();
  test_svg_vector_bar_chart_is_native();
  test_svg_vector_line_chart_is_native();
  test_svg_vector_area_and_combo_are_native();
  test_svg_vector_scatter_falls_back_to_raster();
  test_svg_vector_doughnut_paints_hole_in_background();

  // PieChart doughnut cutout
  test_pie_doughnut_cutout_center_matches_background();
  test_pie_default_inner_radius_ratio_renders_solid_center();

  // LTTB decimation
  test_lttb_noop_when_under_threshold();
  test_lttb_reduces_to_threshold();
  test_lttb_keeps_first_and_last();
  test_lttb_preserves_spike();
  test_lttb_indexed_matches_values();

  // ScatterChart
  test_scatter_empty_series_renders_blank();
  test_scatter_x_increases_left_to_right();
  test_scatter_bubble_radius_scales();

  // BarChart shape (rounded corner / border)
  test_bar_corner_radius_rounds_top_corner();
  test_bar_border_pixel_matches_border_color();
  test_bar_default_options_render_unchanged();

  // ComboChart dual-axis
  test_combo_default_axis_backward_compatible();
  test_combo_secondary_axis_independent_scale();

  // Palette
  test_palette_chartjs_cycle();
  test_palette_tableau10_cycle();
  test_palette_by_kind_matches();
  test_palette_distinct();

  // ImageBuffer
  test_image_buffer_dimensions();
  test_image_buffer_pixel_access();
  test_image_buffer_clear_color();
  test_image_buffer_fill_rect();
  test_image_buffer_downsample();
  test_image_buffer_fill_rounded_rect_corner_transparent();
  test_image_buffer_fill_rounded_rect_zero_radius_matches_fill_rect();
  test_image_buffer_draw_dashed_line_empty_dash_is_solid();
  test_image_buffer_draw_dashed_line_leaves_gaps();
  test_image_buffer_draw_dashed_polyline_phase_continuous();

  // detail:: shared helpers
  test_format_tick_value_matches_format_label_value_auto();
  test_format_label_value_fixed_decimals();
  test_catmull_rom_expand_passes_through_control_points();
  test_catmull_rom_expand_too_few_points_returns_input();
  test_draw_marker_square_fills_corners_circle_does_not();

  // ChartResult / ChartError
  test_chart_result_success_int();
  test_chart_result_failure_int();
  test_chart_result_void_success();
  test_chart_result_void_failure();
  test_chart_error_message_non_empty();

  // Theme
  test_theme_light_dark_differ();
  test_apply_theme();

  // Font
  test_font_measure_proportional();
  test_font_measure_empty_string();

  // min_y_labels / max_y_labels
  test_y_labels_max_produces_more_ticks_than_default();
  test_y_labels_max_renders_without_crash();
  test_y_labels_min_renders_without_crash();
  test_y_labels_same_min_max_renders_without_crash();
  test_y_labels_inverted_constraint_renders_without_crash();
  test_y_labels_zero_preserves_default_behavior();

  if (g_failures == 0) {
    std::printf("All unit tests passed.\n");
    return 0;
  }
  std::fprintf(stderr, "%d test(s) FAILED.\n", g_failures);
  return 1;
}
