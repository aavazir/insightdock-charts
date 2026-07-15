// SPDX-License-Identifier: MIT
// Copyright (c) InsightDock contributors

#include "insightdock/pie_chart.h"
#include "insightdock/detail/math_constants.h"
#include "insightdock/detail/tick_format.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <sstream>

namespace insightdock::charts {

namespace {

void append_wrapped_token(Font const &font, float max_width, std::string const &token,
                          std::string &cur, std::vector<std::string> &out) {
  if (token.empty())
    return;
  if (cur.empty()) {
    for (char c : token) {
      std::string next = cur + c;
      if (!cur.empty() && font.measure_width(next) > max_width) {
        out.push_back(cur);
        cur = std::string(1, c);
      } else {
        cur = next;
      }
    }
    return;
  }
  std::string spaced = cur + ' ' + token;
  if (font.measure_width(spaced) <= max_width) {
    cur = spaced;
    return;
  }
  if (!cur.empty()) {
    out.push_back(cur);
    cur.clear();
  }
  append_wrapped_token(font, max_width, token, cur, out);
}

std::vector<std::string> wrap_label_text(Font const &font, std::string const &text,
                                         float max_width) {
  if (text.empty())
    return {};
  if (max_width <= 1.0f)
    return {text};

  std::vector<std::string> out;
  std::size_t para_start = 0;
  while (para_start <= text.size()) {
    auto para_end = text.find('\n', para_start);
    std::string para = (para_end == std::string::npos)
                           ? text.substr(para_start)
                           : text.substr(para_start, para_end - para_start);

    std::string cur;
    std::size_t i = 0;
    while (i < para.size()) {
      while (i < para.size() && para[i] == ' ')
        ++i;
      if (i >= para.size())
        break;
      auto j = para.find(' ', i);
      append_wrapped_token(font, max_width,
                           j == std::string::npos ? para.substr(i) : para.substr(i, j - i), cur,
                           out);
      i = j == std::string::npos ? para.size() : j + 1;
    }
    if (!cur.empty())
      out.push_back(cur);
    if (para_end == std::string::npos)
      break;
    para_start = para_end + 1;
  }
  if (out.empty())
    return {text};
  return out;
}

constexpr double deg_to_rad(double deg) {
  return deg * M_PI / 180.0;
}

std::vector<std::pair<float, float>>
slice_polygon(float cx, float cy, float radius, double start_rad, double end_rad, double gap_rad) {
  double s = std::max(start_rad, start_rad + gap_rad);
  double e = std::min(end_rad, end_rad - gap_rad);

  if (e - s < 0.001) {
    return {{cx, cy}, {cx + 1.0f, cy}, {cx, cy + 1.0f}};
  }

  std::vector<std::pair<float, float>> poly;
  poly.push_back({cx, cy});
  int steps = 96;
  double step = (e - s) / steps;
  for (int i = 0; i <= steps; ++i) {
    double a = s + i * step;
    poly.push_back({cx + radius * static_cast<float>(std::cos(a)),
                    cy + radius * static_cast<float>(std::sin(a))});
  }
  return poly;
}

} // namespace

PieChart::PieChart() {
  pie_opts_.palette = PaletteKind::Tableau10;
}

void PieChart::add_slice(std::string label, double value, Color color) {
  PieSlice slice;
  slice.label = std::move(label);
  slice.value = value;
  slice.color = color;
  slices_.push_back(std::move(slice));
}

void PieChart::add_slice(std::string label, double value) {
  add_slice(std::move(label), value, palette_color_by_kind(pie_opts_.palette, slices_.size()));
}

void PieChart::set_pie_options(PieOptions opts) {
  if (opts.center_x == 0.0 && opts.center_y == 0.0 && options_.width > 0 && options_.height > 0) {
    opts.center_x = options_.width / 2.0;
    opts.center_y = options_.height / 2.0;
  }
  if (opts.radius == 0.0 && options_.width > 0 && options_.height > 0) {
    opts.radius = std::min(options_.width, options_.height) / 2.0 - 80.0;
  }
  pie_opts_ = opts;
}

void PieChart::clear_slices() {
  slices_.clear();
}

ImageBuffer PieChart::render() const {
  ImageBuffer img(options_.width, options_.height);
  img.clear(options_.background);

  if (slices_.empty())
    return img;

  double total = 0.0;
  for (auto const &s : slices_) {
    if (pie_opts_.skip_zero && s.value <= 0.0)
      continue;
    total += s.value;
  }
  if (total <= 0.0)
    return img;

  double cx = pie_opts_.center_x;
  double cy = pie_opts_.center_y;
  if (cx == 0.0 && cy == 0.0) {
    cx = options_.width / 2.0;
    cy = options_.height / 2.0;
  }
  double radius = pie_opts_.radius;
  // Auto-fit: if radius is unset (0, the sentinel — same convention as center_x/
  // center_y above) or would overflow the image, shrink to fit. Leave ~35% of the
  // shorter half for callout labels. An explicitly-configured non-zero radius that
  // already fits the canvas is trusted as-is, even if a near-horizontal slice's
  // leader line ends up tight on space — that's a caller choice, not a default.
  double max_fit = std::min({cx, static_cast<double>(options_.width) - cx, cy,
                             static_cast<double>(options_.height) - cy}) *
                   0.65;
  if (max_fit < 20.0)
    max_fit = 20.0;
  if (radius <= 0.0 || cx - radius < 0 || cx + radius > static_cast<double>(options_.width) ||
      cy - radius < 0 || cy + radius > static_cast<double>(options_.height)) {
    radius = max_fit;
  }

  float fill_a = static_cast<uint8_t>(pie_opts_.fill_opacity * 255.0f + 0.5f);

  struct SliceDraw {
    PieSlice const *slice;
    double start;
    double end;
    double mid;
    double pct;
  };
  std::vector<SliceDraw> drawn;

  double angle = -M_PI / 2.0;
  double gap = deg_to_rad(pie_opts_.gap_degrees);

  for (auto const &slice : slices_) {
    if (pie_opts_.skip_zero && slice.value <= 0.0)
      continue;

    double slice_angle = (slice.value / total) * 2.0 * M_PI;
    double end_angle = angle + slice_angle;
    double mid = angle + slice_angle * 0.5;

    double s = std::max(angle, angle + gap);
    double e = std::min(end_angle, end_angle - gap);

    Color fill = slice.color;
    fill.a = fill_a;
    img.fill_sector(static_cast<float>(cx), static_cast<float>(cy), static_cast<float>(radius), s,
                    e, fill);

    if (pie_opts_.stroke_width > 0.0f) {
      auto poly = slice_polygon(static_cast<float>(cx), static_cast<float>(cy),
                                static_cast<float>(radius), angle, end_angle, gap);
      img.draw_polygon_outline(poly.data(), poly.size(), pie_opts_.stroke_width,
                               pie_opts_.stroke_color);
    }

    drawn.push_back({&slice, angle, end_angle, mid, (slice.value / total) * 100.0});
    angle = end_angle;
  }

  if (pie_opts_.inner_radius_ratio > 0.0) {
    double inner_radius = radius * std::min(pie_opts_.inner_radius_ratio, 0.999);
    img.fill_circle(static_cast<float>(cx), static_cast<float>(cy),
                    static_cast<float>(inner_radius), options_.background);
  }

  if (pie_opts_.data_labels.enabled) {
    Font dl_font;
    auto const &dl = pie_opts_.data_labels;
    dl_font.set_size(dl.font_size);
    for (auto const &d : drawn) {
      double shown = dl.format == TickFormat::Percent ? d.pct : d.slice->value;
      std::string text = detail::format_label_value(shown, dl.format, dl.decimals);
      float lx = static_cast<float>(cx) +
                 static_cast<float>(radius) * 0.65f * static_cast<float>(std::cos(d.mid));
      float ly = static_cast<float>(cy) +
                 static_cast<float>(radius) * 0.65f * static_cast<float>(std::sin(d.mid));
      dl_font.draw_text(img, lx, ly, text, dl.color, TextAlign::Center, TextBaseline::Middle);
    }
  }

  Font font;
  font.set_size(pie_opts_.label_font_size);

  if (pie_opts_.show_labels) {
    float fs = pie_opts_.label_font_size;
    float knee_len = fs * 2.0f;  // radial leader beyond the slice edge
    float h_len = fs * 2.5f;     // horizontal elbow segment
    float text_gap = fs * 0.4f;  // gap between elbow end and text
    float line_gap = fs * 0.62f; // half vertical spacing between the two lines
    Color leader = Color::gray(150);

    for (auto const &d : drawn) {
      float cosA = static_cast<float>(std::cos(d.mid));
      float sinA = static_cast<float>(std::sin(d.mid));
      bool right = cosA >= 0.0f;

      float sx = static_cast<float>(cx) + static_cast<float>(radius) * cosA;
      float sy = static_cast<float>(cy) + static_cast<float>(radius) * sinA;
      float kx = static_cast<float>(cx) + (static_cast<float>(radius) + knee_len) * cosA;
      float ky = static_cast<float>(cy) + (static_cast<float>(radius) + knee_len) * sinA;
      float ex = kx + (right ? h_len : -h_len);
      float ey = ky;

      img.draw_line(sx, sy, kx, ky, std::max(1.0f, fs * 0.09f), leader);
      img.draw_line(kx, ky, ex, ey, std::max(1.0f, fs * 0.09f), leader);

      char line1[160];
      std::snprintf(line1, sizeof(line1), "%s", d.slice->label.c_str());

      std::string line2;
      if (pie_opts_.show_percentage) {
        char buf[32];
        std::snprintf(buf, sizeof(buf), "%.1f%%", d.pct);
        line2 = buf;
      }

      TextAlign align = right ? TextAlign::Left : TextAlign::Right;
      float tx = ex + (right ? text_gap : -text_gap);

      float max_label_w = right ? static_cast<float>(options_.width) - tx - 8.0f : tx - 8.0f;
      max_label_w = std::max(max_label_w, 40.0f);
      max_label_w = std::min(max_label_w, fs * 7.0f);
      auto label_lines = wrap_label_text(font, line1, max_label_w);
      float lh = font.line_height();
      float label_pct_gap = line_gap * 2.0f;

      if (line2.empty()) {
        if (label_lines.size() <= 1) {
          font.draw_text(img, tx, ey, line1, pie_opts_.label_color, align, TextBaseline::Middle);
        } else {
          float top_y = ey - static_cast<float>(label_lines.size() - 1) * lh * 0.5f;
          for (std::size_t li = 0; li < label_lines.size(); ++li) {
            font.draw_text(img, tx, top_y + static_cast<float>(li) * lh, label_lines[li],
                           pie_opts_.label_color, align, TextBaseline::Top);
          }
        }
      } else {
        Color sub = Color::gray(120);
        // Match vector layout: % fixed below elbow; wrapped lines stack upward.
        float pct_y = ey + line_gap;
        float last_label_top = pct_y - label_pct_gap - lh;
        float first_label_top = last_label_top - static_cast<float>(label_lines.size() - 1) * lh;
        for (std::size_t li = 0; li < label_lines.size(); ++li) {
          font.draw_text(img, tx, first_label_top + static_cast<float>(li) * lh, label_lines[li],
                         pie_opts_.label_color, align, TextBaseline::Top);
        }
        font.draw_text(img, tx, pct_y, line2, sub, align, TextBaseline::Top);
      }
    }
  }

  if (pie_opts_.show_legend && !drawn.empty()) {
    float box = 10.0f;
    float gap = 8.0f;
    float row_h = box + gap;
    float legend_h = static_cast<float>(drawn.size()) * row_h;
    float y = static_cast<float>(options_.height) - legend_h - 10.0f;
    float x = 16.0f;

    if (pie_opts_.legend_position == PieLegendPosition::Right) {
      x = static_cast<float>(options_.width) - 120.0f;
      y = static_cast<float>(cy) - static_cast<float>(drawn.size()) * (box + gap) * 0.5f;
    }

    font.set_size(options_.font_size);
    for (auto const &d : drawn) {
      Color sw = d.slice->color;
      sw.a = 255;
      img.fill_rect(x, y, box, box, sw);
      std::ostringstream oss;
      oss << d.slice->label;
      if (pie_opts_.show_percentage)
        oss << " (" << static_cast<int>(d.pct) << "%)";
      font.draw_text(img, x + box + 6.0f, y + box * 0.5f, oss.str(), pie_opts_.label_color,
                     TextAlign::Left, TextBaseline::Middle);
      y += box + gap;
    }
  }

  if (!options_.title.empty()) {
    font.set_size(options_.font_size + 4.0f);
    float tw = font.measure_width(options_.title);
    float x = (static_cast<float>(img.width()) - tw) * 0.5f;
    font.draw_text(img, x, 12.0f, options_.title, options_.title_color, TextAlign::Left,
                   TextBaseline::Top);
  }

  return img;
}

} // namespace insightdock::charts
