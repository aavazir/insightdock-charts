// SPDX-License-Identifier: MIT
// Copyright (c) InsightDock contributors

#include "insightdock/pie_chart.h"
#include "insightdock/detail/legend_layout.h"
#include "insightdock/detail/math_constants.h"
#include "insightdock/detail/text_wrap.h"
#include "insightdock/detail/tick_format.h"
#include "insightdock/draw_target.h"

#include <algorithm>
#include <cmath>

namespace insightdock::charts {

namespace {

constexpr double deg_to_rad(double deg) {
  return deg * M_PI / 180.0;
}

std::vector<std::pair<float, float>>
slice_polygon(float cx, float cy, float radius, double start_rad, double end_rad, double gap_rad) {
  double s = std::max(start_rad, start_rad + gap_rad);
  double e = std::min(end_rad, end_rad - gap_rad);

  if (e <= s)
    return {};

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
  // A pie encodes nonnegative proportions; invalid or negative inputs count as zero.
  slice.value = std::isfinite(value) && value > 0.0 ? value : 0.0;
  slice.color = color;
  slices_.push_back(std::move(slice));
}

void PieChart::add_slice(std::string label, double value) {
  add_slice(std::move(label), value, palette_color_by_kind(pie_opts_.palette, slices_.size()));
}

void PieChart::set_pie_options(PieOptions opts) {
  pie_opts_ = opts;
}

void PieChart::clear_slices() {
  slices_.clear();
}

ImageBuffer PieChart::render() const {
  ImageBuffer img(options_.width, options_.height);
  RasterTarget target(img);
  draw(target);
  return img;
}

bool PieChart::draw(DrawTarget &target) const {
  target.clear(options_.background);

  if (slices_.empty())
    return true;

  double largest = 0.0;
  for (auto const &s : slices_)
    largest = std::max(largest, s.value);
  if (largest == 0.0)
    return true;

  // Normalize first so multiple finite values near DBL_MAX cannot overflow the total.
  double total = 0.0;
  for (auto const &s : slices_) {
    if (pie_opts_.skip_zero && s.value <= 0.0)
      continue;
    total += s.value / largest;
  }
  if (total <= 0.0)
    return true;

  Font legend_font(options_.font);
  legend_font.set_size(options_.font_size);
  std::vector<std::string> legend_labels;
  if (pie_opts_.show_legend) {
    for (auto const &slice : slices_) {
      if (pie_opts_.skip_zero && slice.value <= 0.0)
        continue;
      auto label = slice.label;
      if (pie_opts_.show_percentage)
        label += " (" +
                 detail::format_label_value((slice.value / largest) / total * 100.0,
                                            TickFormat::Percent, 0) +
                 ")";
      legend_labels.push_back(std::move(label));
    }
  }
  bool right_legend = pie_opts_.legend_position == PieLegendPosition::Right;
  float width = static_cast<float>(options_.width);
  float height = static_cast<float>(options_.height);
  auto legend =
      detail::layout_legend(legend_labels, legend_font, right_legend ? width / 3.0f : width - 20.0f,
                            height / 3.0f, right_legend);
  double plot_right = width - (right_legend && !legend.items.empty() ? legend.width + 22.0f : 0);
  double plot_bottom =
      height - (!right_legend && !legend.items.empty() ? legend.height + 22.0f : 0);
  double plot_top =
      options_.title.empty()
          ? 0.0
          : 12.0 + legend_font.line_height() * (options_.font_size + 4) / options_.font_size + 8.0;
  double cx = pie_opts_.center_x;
  double cy = pie_opts_.center_y;
  if (cx == 0.0 && cy == 0.0) {
    cx = plot_right / 2.0;
    cy = (plot_top + plot_bottom) / 2.0;
  }
  Font font(options_.font);
  font.set_size(pie_opts_.label_font_size);
  float fs = font.size();
  float text_gap = fs * 0.4f;
  float leader_margin = fs;
  double radius = pie_opts_.radius;
  double max_fit =
      std::max(0.0, std::min({cx, plot_right - cx, cy - plot_top, plot_bottom - cy})) * 0.65;
  if (radius <= 0.0 || cx - radius < 0 || cx + radius > width || cy - radius < 0 ||
      cy + radius > height)
    radius = max_fit;
  if (pie_opts_.radius <= 0.0 && pie_opts_.show_labels) {
    // Reserve measured text columns on both sides before drawing automatic geometry.
    float label_width = 0.0f;
    for (auto const &slice : slices_) {
      if (pie_opts_.skip_zero && slice.value <= 0.0)
        continue;
      auto lines = detail::wrap_text(slice.label, fs * 7.0f, [&](std::string const &text) {
        return font.measure_width(text);
      });
      for (auto const &line : lines)
        label_width = std::max(label_width, font.measure_width(line));
    }
    if (pie_opts_.show_percentage)
      label_width = std::max(label_width, font.measure_width("100.0%"));
    label_width = std::min(label_width, static_cast<float>(plot_right) / 3.0f);
    double horizontal_fit =
        std::min(cx, plot_right - cx) - label_width - leader_margin - text_gap - 8.0;
    radius = std::min(radius, std::max(0.0, horizontal_fit));
  }

  float opacity =
      std::isfinite(pie_opts_.fill_opacity) ? std::clamp(pie_opts_.fill_opacity, 0.0f, 1.0f) : 0.0f;
  auto fill_a = static_cast<uint8_t>(opacity * 255.0f + 0.5f);

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

    double fraction = (slice.value / largest) / total;
    double slice_angle = fraction * 2.0 * M_PI;
    double end_angle = angle + slice_angle;
    double mid = angle + slice_angle * 0.5;

    double s = std::max(angle, angle + gap);
    double e = std::min(end_angle, end_angle - gap);

    drawn.push_back({&slice, angle, end_angle, mid, fraction * 100.0});

    // Gaps that consume the whole wedge produce no stray triangle or reversed arc.
    if (e <= s) {
      angle = end_angle;
      continue;
    }

    Color fill = slice.color;
    fill.a = static_cast<uint8_t>((static_cast<unsigned>(fill.a) * fill_a + 127) / 255);
    target.fill_sector(static_cast<float>(cx), static_cast<float>(cy), static_cast<float>(radius),
                       s, e, fill);

    if (pie_opts_.stroke_width > 0.0f) {
      auto poly = slice_polygon(static_cast<float>(cx), static_cast<float>(cy),
                                static_cast<float>(radius), angle, end_angle, gap);
      target.draw_polygon_outline(poly.data(), poly.size(), pie_opts_.stroke_width,
                                  pie_opts_.stroke_color);
    }

    angle = end_angle;
  }

  if (pie_opts_.inner_radius_ratio > 0.0) {
    double inner_radius = radius * std::min(pie_opts_.inner_radius_ratio, 0.999);
    target.fill_circle(static_cast<float>(cx), static_cast<float>(cy),
                       static_cast<float>(inner_radius), options_.background);
  }

  if (pie_opts_.data_labels.enabled) {
    Font dl_font(options_.font);
    auto const &dl = pie_opts_.data_labels;
    dl_font.set_size(dl.font_size);
    for (auto const &d : drawn) {
      double shown = dl.format == TickFormat::Percent ? d.pct : d.slice->value;
      std::string text = detail::format_label_value(shown, dl.format, dl.decimals);
      float lx = static_cast<float>(cx) +
                 static_cast<float>(radius) * 0.65f * static_cast<float>(std::cos(d.mid));
      float ly = static_cast<float>(cy) +
                 static_cast<float>(radius) * 0.65f * static_cast<float>(std::sin(d.mid));
      target.draw_text(dl_font, lx, ly, text, dl.color, TextAlign::Center, TextBaseline::Middle);
    }
  }

  if (pie_opts_.show_labels) {
    struct Callout {
      SliceDraw const *slice;
      std::vector<std::string> lines;
      std::string percentage;
      float sx, sy, preferred_y, top, height;
    };
    float lh = font.line_height();
    float gap_between = std::max(2.0f, fs * 0.4f);
    float pct_gap = fs * 0.24f;
    float top_bound = static_cast<float>(plot_top) + 8.0f;
    float bottom_bound = static_cast<float>(plot_bottom) - 8.0f;
    Color leader = Color::gray(150);

    for (bool right : {false, true}) {
      float kx = static_cast<float>(cx) +
                 (right ? 1.0f : -1.0f) * (static_cast<float>(radius) + leader_margin);
      float rail_x = static_cast<float>(cx) +
                     (right ? 1.0f : -1.0f) * (static_cast<float>(radius) + leader_margin * 0.35f);
      float tx = kx + (right ? text_gap : -text_gap);
      float available_width = right ? static_cast<float>(plot_right) - tx - 8.0f : tx - 8.0f;
      float max_width = std::min(available_width, fs * 7.0f);
      if (max_width <= 0.0f || bottom_bound <= top_bound)
        continue;
      std::vector<Callout> candidates;
      for (auto const &d : drawn) {
        float cos_a = static_cast<float>(std::cos(d.mid));
        if ((cos_a >= 0.0f) != right)
          continue;
        Callout label{};
        label.slice = &d;
        label.lines = detail::wrap_text(d.slice->label, max_width, [&](std::string const &text) {
          return font.measure_width(text);
        });
        if (pie_opts_.show_percentage)
          label.percentage = detail::format_label_value(d.pct, TickFormat::Percent, 1);
        bool fits = font.measure_width(label.percentage) <= available_width;
        for (auto const &line : label.lines)
          fits = fits && font.measure_width(line) <= available_width;
        label.height = static_cast<float>(label.lines.size()) * lh;
        if (!label.percentage.empty())
          label.height += lh + (label.lines.empty() ? 0.0f : pct_gap);
        if (!fits || label.height == 0.0f)
          continue;
        float sin_a = static_cast<float>(std::sin(d.mid));
        label.sx = static_cast<float>(cx) + static_cast<float>(radius) * cos_a;
        label.sy = static_cast<float>(cy) + static_cast<float>(radius) * sin_a;
        label.preferred_y =
            static_cast<float>(cy) + (static_cast<float>(radius) + leader_margin) * sin_a;
        candidates.push_back(std::move(label));
      }
      // In an overfull column, prioritize larger slices with stable input-order ties.
      std::stable_sort(candidates.begin(), candidates.end(),
                       [](auto const &a, auto const &b) { return a.slice->pct > b.slice->pct; });
      std::vector<Callout> labels;
      float used = 0.0f;
      for (auto &label : candidates) {
        float needed = label.height + (labels.empty() ? 0.0f : gap_between);
        if (used + needed <= bottom_bound - top_bound) {
          used += needed;
          labels.push_back(std::move(label));
        }
      }
      // Preserve vertical slice order so leaders do not cross on either side.
      std::stable_sort(labels.begin(), labels.end(),
                       [](auto const &a, auto const &b) { return a.preferred_y < b.preferred_y; });
      float next_top = top_bound;
      for (auto &label : labels) {
        label.top = std::max(next_top, label.preferred_y - label.height * 0.5f);
        next_top = label.top + label.height + gap_between;
      }
      float next_bottom = bottom_bound;
      for (auto it = labels.rbegin(); it != labels.rend(); ++it) {
        it->top = std::min(it->top, next_bottom - it->height);
        next_bottom = it->top - gap_between;
      }
      for (auto const &label : labels) {
        float elbow_y = label.top + label.height * 0.5f;
        // Horizontal exits stay outside the circle. Two fixed rails preserve endpoint order
        // when vertically displaced blocks would otherwise route diagonals through the pie.
        target.draw_line(label.sx, label.sy, rail_x, label.sy, std::max(1.0f, fs * 0.09f), leader);
        target.draw_line(rail_x, label.sy, kx, elbow_y, std::max(1.0f, fs * 0.09f), leader);
        target.draw_line(kx, elbow_y, tx, elbow_y, std::max(1.0f, fs * 0.09f), leader);
        auto align = right ? TextAlign::Left : TextAlign::Right;
        for (std::size_t i = 0; i < label.lines.size(); ++i)
          target.draw_text(font, tx, label.top + static_cast<float>(i) * lh, label.lines[i],
                           pie_opts_.label_color, align, TextBaseline::Top);
        if (!label.percentage.empty())
          target.draw_text(font, tx, label.top + label.height - lh, label.percentage,
                           Color::gray(120), align, TextBaseline::Top);
      }
    }
  }

  if (!legend.items.empty()) {
    float x = right_legend ? width - legend.width - 10.0f : 10.0f;
    float y = right_legend ? std::max(static_cast<float>(plot_top), (height - legend.height) * 0.5f)
                           : height - legend.height - 10.0f;
    float line = std::max(10.0f, legend_font.line_height());
    for (auto const &item : legend.items) {
      if (item.index < drawn.size()) {
        Color sw = drawn[item.index].slice->color;
        sw.a = 255;
        target.fill_rect(x + item.x, y + item.y + (line - 10.0f) * 0.5f, 10.0f, 10.0f, sw);
      }
      for (std::size_t i = 0; i < item.lines.size(); ++i)
        target.draw_text(legend_font, x + item.x + 16.0f, y + item.y + static_cast<float>(i) * line,
                         item.lines[i], pie_opts_.label_color, TextAlign::Left, TextBaseline::Top);
    }
  }

  if (!options_.title.empty()) {
    font.set_size(options_.font_size + 4.0f);
    float tw = font.measure_width(options_.title);
    float x = (static_cast<float>(options_.width) - tw) * 0.5f;
    target.draw_text(font, x, 12.0f, options_.title, options_.title_color, TextAlign::Left,
                     TextBaseline::Top);
  }

  return true;
}

} // namespace insightdock::charts
