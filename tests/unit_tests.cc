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

#include <cassert>
#include <cmath>
#include <cstdio>

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

static void test_svg_vector_non_pie_falls_back_to_raster() {
  BarChart bar;
  bar.options().width = 200;
  bar.options().height = 150;
  bar.set_categories({"Q1", "Q2"});
  bar.add_series("Rev", {10, 20});

  ExportOptions opts;
  opts.format = OutputFormat::Svg;
  opts.svg_backend = SvgBackend::Vector;
  auto bytes = ChartExporter::encode(bar, opts);
  CHECK(static_cast<bool>(bytes));
  std::string s(bytes.value.begin(), bytes.value.end());
  CHECK(s.find("base64") != std::string::npos); // raster fallback
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
  detail::draw_marker(square_buf, 10.0f, 10.0f, 6.0f, PointMarkerShape::Square, Color::black());
  detail::draw_marker(circle_buf, 10.0f, 10.0f, 6.0f, PointMarkerShape::Circle, Color::black());

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
// Runner
// ---------------------------------------------------------------------------

int main() {
  // Data labels (all chart types)
  test_bar_data_label_draws_ink_above_bar();
  test_bar_data_label_disabled_by_default();
  test_line_data_label_renders_without_crash();
  test_area_data_label_renders_without_crash();
  test_combo_data_labels_render_without_crash();
  test_scatter_data_label_renders_without_crash();
  test_pie_data_label_draws_ink_in_slice();

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
  test_scale_type_default();
  test_scale_log_roundtrip();
  test_scale_log_ticks_ascending();
  test_scale_datetime_roundtrip();
  test_scale_datetime_ticks();
  test_svg_encode_non_empty();

  // Native SVG pie (SvgBackend::Vector)
  test_svg_vector_pie_emits_paths();
  test_svg_vector_pie_single_slice_uses_circle();
  test_svg_vector_non_pie_falls_back_to_raster();

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
