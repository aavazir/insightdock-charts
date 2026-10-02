// SPDX-License-Identifier: MIT
// Copyright (c) InsightDock contributors

#include "insightdock/detail/svg_pie.h"
#include "insightdock/detail/math_constants.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <sstream>

namespace insightdock::charts::detail {

namespace {

std::string xml_escape(std::string const &s) {
  std::string out;
  out.reserve(s.size());
  for (char c : s) {
    switch (c) {
    case '&':
      out += "&amp;";
      break;
    case '<':
      out += "&lt;";
      break;
    case '>':
      out += "&gt;";
      break;
    case '"':
      out += "&quot;";
      break;
    default:
      out += c;
    }
  }
  return out;
}

std::string fmt(double v) {
  char buf[64];
  std::snprintf(buf, sizeof(buf), "%.2f", v);
  return buf;
}

std::string rgb(Color const &c) {
  char buf[16];
  std::snprintf(buf, sizeof(buf), "#%02x%02x%02x", c.r, c.g, c.b);
  return buf;
}

constexpr double deg_to_rad(double deg) {
  return deg * M_PI / 180.0;
}

struct SliceDraw {
  PieSlice const *slice;
  double start = 0.0;
  double end = 0.0;
  double mid = 0.0;
  double pct = 0.0;
};

void append_sector_path(std::ostringstream &svg, double cx, double cy, double radius, double start,
                        double end, double gap_rad, Color fill, float fill_opacity, Color stroke,
                        float stroke_width, bool full_circle) {
  if (full_circle) {
    svg << "  <circle cx=\"" << fmt(cx) << "\" cy=\"" << fmt(cy) << "\" r=\"" << fmt(radius)
        << "\" fill=\"" << rgb(fill) << "\" fill-opacity=\"" << fmt(fill_opacity) << "\"";
    if (stroke_width > 0.0f)
      svg << " stroke=\"" << rgb(stroke) << "\" stroke-width=\"" << fmt(stroke_width) << "\"";
    svg << "/>\n";
    return;
  }

  double s = std::max(start, start + gap_rad);
  double e = std::min(end, end - gap_rad);
  double sx = cx + radius * std::cos(s);
  double sy = cy + radius * std::sin(s);
  double ex = cx + radius * std::cos(e);
  double ey = cy + radius * std::sin(e);
  int large_arc = (e - s) > M_PI ? 1 : 0;

  svg << "  <path d=\"M " << fmt(cx) << "," << fmt(cy) << " L " << fmt(sx) << "," << fmt(sy)
      << " A " << fmt(radius) << "," << fmt(radius) << " 0 " << large_arc << " 1 " << fmt(ex) << ","
      << fmt(ey) << " Z\" fill=\"" << rgb(fill) << "\" fill-opacity=\"" << fmt(fill_opacity)
      << "\"";
  if (stroke_width > 0.0f)
    svg << " stroke=\"" << rgb(stroke) << "\" stroke-width=\"" << fmt(stroke_width) << "\"";
  svg << "/>\n";
}

void append_text(std::ostringstream &svg, double x, double y, std::string const &text,
                 Color const &color, char const *anchor, float font_size,
                 char const *baseline = "middle") {
  if (text.empty())
    return;
  svg << "  <text x=\"" << fmt(x) << "\" y=\"" << fmt(y) << "\" text-anchor=\"" << anchor
      << "\" dominant-baseline=\"" << baseline << "\" font-family=\"Helvetica, Arial, sans-serif\""
      << " font-size=\"" << fmt(font_size) << "\" fill=\"" << rgb(color) << "\">"
      << xml_escape(text) << "</text>\n";
}

} // namespace

std::string build_pie_svg(PieChart const &chart) {
  ChartOptions const &opts = chart.options();
  PieOptions const &pie_opts = chart.pie_options();
  auto const &slices = chart.slices();

  if (opts.width == 0 || opts.height == 0)
    return {};

  std::ostringstream svg;
  svg << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n";
  svg << "<svg xmlns=\"http://www.w3.org/2000/svg\" width=\"" << opts.width << "\" height=\""
      << opts.height << "\" viewBox=\"0 0 " << opts.width << " " << opts.height << "\">\n";
  svg << "  <rect x=\"0\" y=\"0\" width=\"" << opts.width << "\" height=\"" << opts.height
      << "\" fill=\"" << rgb(opts.background) << "\"/>\n";

  double total = 0.0;
  for (auto const &s : slices) {
    if (pie_opts.skip_zero && s.value <= 0.0)
      continue;
    total += s.value;
  }

  if (total <= 0.0) {
    svg << "</svg>\n";
    return svg.str();
  }

  double cx = pie_opts.center_x;
  double cy = pie_opts.center_y;
  if (cx == 0.0 && cy == 0.0) {
    cx = static_cast<double>(opts.width) / 2.0;
    cy = static_cast<double>(opts.height) / 2.0;
  }
  double radius = pie_opts.radius;
  double max_fit = std::min({cx, static_cast<double>(opts.width) - cx, cy,
                             static_cast<double>(opts.height) - cy}) *
                   0.65;
  if (max_fit < 20.0)
    max_fit = 20.0;
  if (radius <= 0.0 || cx - radius < 0 || cx + radius > static_cast<double>(opts.width) ||
      cy - radius < 0 || cy + radius > static_cast<double>(opts.height)) {
    radius = max_fit;
  }

  double gap = deg_to_rad(pie_opts.gap_degrees);
  std::vector<SliceDraw> drawn;
  std::size_t n_visible = 0;
  for (auto const &s : slices) {
    if (pie_opts.skip_zero && s.value <= 0.0)
      continue;
    ++n_visible;
  }

  double angle = -M_PI / 2.0;
  for (auto const &slice : slices) {
    if (pie_opts.skip_zero && slice.value <= 0.0)
      continue;

    double slice_angle = (slice.value / total) * 2.0 * M_PI;
    double end_angle = angle + slice_angle;
    double mid = angle + slice_angle * 0.5;

    append_sector_path(svg, cx, cy, radius, angle, end_angle, gap, slice.color,
                       pie_opts.fill_opacity, pie_opts.stroke_color, pie_opts.stroke_width,
                       n_visible == 1);

    drawn.push_back({&slice, angle, end_angle, mid, (slice.value / total) * 100.0});
    angle = end_angle;
  }

  // Doughnut cutout: paint the hole in the background color, as the raster path does.
  if (pie_opts.inner_radius_ratio > 0.0) {
    svg << "  <circle cx=\"" << fmt(cx) << "\" cy=\"" << fmt(cy) << "\" r=\""
        << fmt(radius * std::min(pie_opts.inner_radius_ratio, 0.999)) << "\" fill=\""
        << rgb(opts.background) << "\"/>\n";
  }

  if (pie_opts.show_labels) {
    float fs = pie_opts.label_font_size;
    float knee_len = fs * 2.0f;
    float h_len = fs * 2.5f;
    float text_gap = fs * 0.4f;
    float line_gap = fs * 0.62f;
    Color leader = Color::gray(150);

    for (auto const &d : drawn) {
      double cosA = std::cos(d.mid);
      double sinA = std::sin(d.mid);
      bool right = cosA >= 0.0;

      double sx = cx + radius * cosA;
      double sy = cy + radius * sinA;
      double kx = cx + (radius + knee_len) * cosA;
      double ky = cy + (radius + knee_len) * sinA;
      double ex = kx + (right ? h_len : -h_len);
      double ey = ky;

      svg << "  <line x1=\"" << fmt(sx) << "\" y1=\"" << fmt(sy) << "\" x2=\"" << fmt(kx)
          << "\" y2=\"" << fmt(ky) << "\" stroke=\"" << rgb(leader) << "\" stroke-width=\""
          << fmt(std::max(1.0f, fs * 0.09f)) << "\"/>\n";
      svg << "  <line x1=\"" << fmt(kx) << "\" y1=\"" << fmt(ky) << "\" x2=\"" << fmt(ex)
          << "\" y2=\"" << fmt(ey) << "\" stroke=\"" << rgb(leader) << "\" stroke-width=\""
          << fmt(std::max(1.0f, fs * 0.09f)) << "\"/>\n";

      char buf[32];
      std::snprintf(buf, sizeof(buf), "%.1f%%", d.pct);
      std::string pct_text = pie_opts.show_percentage ? buf : std::string();

      char const *anchor = right ? "start" : "end";
      double tx = ex + (right ? text_gap : -text_gap);

      if (pct_text.empty()) {
        append_text(svg, tx, ey, d.slice->label, pie_opts.label_color, anchor, fs);
      } else {
        double line_gap_d = static_cast<double>(line_gap);
        append_text(svg, tx, ey - line_gap_d, d.slice->label, pie_opts.label_color, anchor, fs);
        append_text(svg, tx, ey + line_gap_d, pct_text, Color::gray(120), anchor, fs * 0.9f);
      }
    }
  }

  if (pie_opts.show_legend && !drawn.empty()) {
    float box = 10.0f;
    float gap_px = 8.0f;
    float row_h = box + gap_px;

    if (pie_opts.legend_position == PieLegendPosition::Right) {
      float x = static_cast<float>(opts.width) - 120.0f;
      float y = static_cast<float>(cy) - static_cast<float>(drawn.size()) * row_h * 0.5f;
      for (auto const &d : drawn) {
        svg << "  <rect x=\"" << fmt(x) << "\" y=\"" << fmt(y) << "\" width=\"" << fmt(box)
            << "\" height=\"" << fmt(box) << "\" fill=\"" << rgb(d.slice->color) << "\"/>\n";
        std::ostringstream label;
        label << d.slice->label;
        if (pie_opts.show_percentage)
          label << " (" << static_cast<int>(d.pct) << "%)";
        append_text(svg, x + box + 6.0f, y + box * 0.5f, label.str(), pie_opts.label_color, "start",
                    opts.font_size);
        y += row_h;
      }
    } else {
      float legend_h = static_cast<float>(drawn.size()) * row_h;
      float y = static_cast<float>(opts.height) - legend_h - 10.0f;
      float x = 16.0f;
      for (auto const &d : drawn) {
        svg << "  <rect x=\"" << fmt(x) << "\" y=\"" << fmt(y) << "\" width=\"" << fmt(box)
            << "\" height=\"" << fmt(box) << "\" fill=\"" << rgb(d.slice->color) << "\"/>\n";
        std::ostringstream label;
        label << d.slice->label;
        if (pie_opts.show_percentage)
          label << " (" << static_cast<int>(d.pct) << "%)";
        append_text(svg, x + box + 6.0f, y + box * 0.5f, label.str(), pie_opts.label_color, "start",
                    opts.font_size);
        y += row_h;
      }
    }
  }

  if (!opts.title.empty()) {
    append_text(svg, static_cast<double>(opts.width) / 2.0, 12.0 + opts.font_size + 4.0, opts.title,
                opts.title_color, "middle", opts.font_size + 4.0f, "hanging");
  }

  svg << "</svg>\n";
  return svg.str();
}

} // namespace insightdock::charts::detail
