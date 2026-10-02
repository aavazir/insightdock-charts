// SPDX-License-Identifier: MIT
// Copyright (c) InsightDock contributors

#include "insightdock/scatter_chart.h"
#include "insightdock/detail/marker.h"
#include "insightdock/detail/tick_format.h"
#include "insightdock/font.h"
#include "insightdock/geometry.h"
#include "insightdock/palette.h"
#include "insightdock/scale.h"

#include <algorithm>
#include <cmath>
#include <cstdio>

namespace insightdock::charts {

namespace {

std::string format_tick(double v) {
  if (std::fabs(v) < 1e-9)
    v = 0.0;
  char buf[48];
  if (std::fabs(v) >= 1.0 || v == 0.0)
    std::snprintf(buf, sizeof(buf), "%.0f", v);
  else
    std::snprintf(buf, sizeof(buf), "%.2f", v);
  return buf;
}

} // namespace

void ScatterChart::add_series(std::string label, std::vector<ScatterPoint> points) {
  ScatterSeries s;
  s.label = std::move(label);
  s.points = std::move(points);
  s.color = palette_color_by_kind(options_.palette, series_.size());
  series_.push_back(std::move(s));
}

void ScatterChart::add_series(std::string label, std::vector<ScatterPoint> points, Color color) {
  ScatterSeries s;
  s.label = std::move(label);
  s.points = std::move(points);
  s.color = color;
  series_.push_back(std::move(s));
}

void ScatterChart::clear_series() {
  series_.clear();
}

ImageBuffer ScatterChart::render() const {
  ImageBuffer buf(options_.width, options_.height);
  RasterTarget raster(buf);
  buf.clear(options_.background);

  if (series_.empty())
    return buf;

  Font font(options_.font);
  font.set_size(options_.font_size);

  if (!options_.title.empty()) {
    font.set_size(options_.font_size + 4.0f);
    float tw = font.measure_width(options_.title);
    float tx = (static_cast<float>(buf.width()) - tw) * 0.5f;
    font.draw_text(buf, tx, 12.0f, options_.title, options_.title_color, TextAlign::Left,
                   TextBaseline::Top);
    font.set_size(options_.font_size);
  }

  double xmin = 0.0, xmax = 0.0, ymin = 0.0, ymax = 0.0;
  double rmin = 0.0, rmax = 0.0;
  bool any = false;
  bool any_radius = false;
  for (auto const &s : series_) {
    for (auto const &p : s.points) {
      if (!std::isfinite(p.x) || !std::isfinite(p.y))
        continue;
      if (!any) {
        xmin = xmax = p.x;
        ymin = ymax = p.y;
        any = true;
      } else {
        xmin = std::min(xmin, p.x);
        xmax = std::max(xmax, p.x);
        ymin = std::min(ymin, p.y);
        ymax = std::max(ymax, p.y);
      }
      if (p.radius > 0.0 && std::isfinite(p.radius)) {
        if (!any_radius) {
          rmin = rmax = p.radius;
          any_radius = true;
        } else {
          rmin = std::min(rmin, p.radius);
          rmax = std::max(rmax, p.radius);
        }
      }
    }
  }
  if (!any)
    return buf;

  if (opts_.begin_at_zero_x)
    xmin = std::min(xmin, 0.0);
  if (opts_.begin_at_zero_y)
    ymin = std::min(ymin, 0.0);

  float lh = font.line_height();
  Margins m = options_.margins;
  if (!options_.title.empty())
    m.top = std::max(m.top, font.line_height() + 16.0f);
  if (options_.legend.enabled && options_.legend.position == LegendOptions::Position::Bottom) {
    float legend_row = lh + 10.0f;
    float x_title_h = opts_.x_axis_label.empty() ? 0.0f : lh + 8.0f;
    m.bottom = std::max(m.bottom, 8.0f + lh + 8.0f + legend_row + x_title_h);
  }
  if (options_.legend.enabled && options_.legend.position == LegendOptions::Position::Right)
    m.right = std::max(m.right, 100.0f);

  Rect plot(m.left, m.top, static_cast<float>(buf.width()) - m.left - m.right,
            static_cast<float>(buf.height()) - m.top - m.bottom);
  if (plot.w <= 0.0f || plot.h <= 0.0f)
    return buf;

  // X increases left→right: to_pixel(min)=pixel_max, to_pixel(max)=pixel_min
  // (see Scale convention), so the pixel range is passed reversed here.
  Scale x_scale = Scale::linear_nice(xmin, xmax, plot.right(), plot.left(), 6);
  Scale y_scale = Scale::linear_nice(ymin, ymax, plot.top(), plot.bottom(), 6);

  auto x_ticks = x_scale.nice_ticks(6);
  auto y_ticks = y_scale.nice_ticks(6);

  if (options_.show_grid) {
    for (double v : y_ticks) {
      float py = y_scale.to_pixel(v);
      if (py < plot.top() || py > plot.bottom())
        continue;
      buf.draw_line(plot.left(), py, plot.right(), py, 1.0f, options_.grid_color);
    }
    for (double v : x_ticks) {
      float px = x_scale.to_pixel(v);
      if (px < plot.left() || px > plot.right())
        continue;
      buf.draw_line(px, plot.top(), px, plot.bottom(), 1.0f, options_.grid_color);
    }
  }

  for (double v : y_ticks) {
    float py = y_scale.to_pixel(v);
    if (py < plot.top() || py > plot.bottom())
      continue;
    font.draw_text(buf, plot.left() - 8.0f, py, format_tick(v), options_.text_color,
                   TextAlign::Right, TextBaseline::Middle);
  }
  for (double v : x_ticks) {
    float px = x_scale.to_pixel(v);
    if (px < plot.left() || px > plot.right())
      continue;
    font.draw_text(buf, px, plot.bottom() + 8.0f, format_tick(v), options_.text_color,
                   TextAlign::Center, TextBaseline::Top);
  }

  buf.draw_line(plot.left(), plot.bottom(), plot.right(), plot.bottom(), 1.5f, options_.axis_color);
  buf.draw_line(plot.left(), plot.top(), plot.left(), plot.bottom(), 1.5f, options_.axis_color);

  if (!opts_.y_axis_label.empty())
    font.draw_text_rotated_ccw(buf, lh * 0.5f + 4.0f, plot.center_y(), opts_.y_axis_label,
                               options_.text_color);
  if (!opts_.x_axis_label.empty())
    font.draw_text(buf, plot.center_x(), static_cast<float>(buf.height()) - 8.0f,
                   opts_.x_axis_label, options_.text_color, TextAlign::Center,
                   TextBaseline::Bottom);

  uint8_t fill_a = static_cast<uint8_t>(opts_.fill_opacity * 255.0f + 0.5f);

  Font label_font(options_.font);
  if (opts_.data_labels.enabled)
    label_font.set_size(opts_.data_labels.font_size);

  for (auto const &s : series_) {
    Color fill = s.color;
    fill.a = fill_a;
    for (auto const &p : s.points) {
      float px = x_scale.to_pixel(p.x);
      float py = y_scale.to_pixel(p.y);
      float radius = opts_.point_radius;
      if (p.radius > 0.0 && any_radius) {
        radius = rmax > rmin ? opts_.min_bubble_radius +
                                   static_cast<float>((p.radius - rmin) / (rmax - rmin)) *
                                       (opts_.max_bubble_radius - opts_.min_bubble_radius)
                             : opts_.max_bubble_radius;
      }
      detail::draw_marker(raster, px, py, radius, opts_.point_shape, fill);

      if (opts_.data_labels.enabled) {
        auto const &dl = opts_.data_labels;
        std::string text = detail::format_label_value(p.y, dl.format, dl.decimals);
        label_font.draw_text(buf, px, py - radius - 4.0f, text, dl.color, TextAlign::Center,
                             TextBaseline::Bottom);
      }
    }
  }

  if (options_.legend.enabled) {
    font.set_size(options_.font_size);
    float box = 10.0f;
    float gap = 16.0f;

    if (options_.legend.position == LegendOptions::Position::Right) {
      float x = plot.right() + 12.0f;
      float y = plot.top();
      for (auto const &s : series_) {
        buf.fill_rect(x, y, box, box, s.color);
        font.draw_text(buf, x + box + 6.0f, y + box * 0.5f, s.label, options_.text_color,
                       TextAlign::Left, TextBaseline::Middle);
        y += box + 8.0f;
      }
    } else {
      float item_w = 0.0f;
      for (auto const &s : series_)
        item_w = std::max(item_w, box + 6.0f + font.measure_width(s.label));
      float total_w = item_w * static_cast<float>(series_.size()) +
                      gap * static_cast<float>(series_.size() - 1);
      float x = (static_cast<float>(buf.width()) - total_w) * 0.5f;
      float y = plot.bottom() + 8.0f + lh + 8.0f;
      for (auto const &s : series_) {
        buf.fill_rect(x, y, box, box, s.color);
        font.draw_text(buf, x + box + 6.0f, y + box * 0.5f, s.label, options_.text_color,
                       TextAlign::Left, TextBaseline::Middle);
        x += item_w + gap;
      }
    }
  }

  return buf;
}

} // namespace insightdock::charts
