// SPDX-License-Identifier: MIT
// Copyright (c) InsightDock contributors

#include "insightdock/cartesian_chart.h"
#include "insightdock/detail/finite_range.h"
#include "insightdock/detail/tick_format.h"
#include "insightdock/palette.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <ctime>
#include <sstream>
#include <string>

namespace insightdock::charts {

namespace {

// Compute the effective y-axis tick hint from min/max_y_labels.
// Returns the number to pass to nice_ticks / linear_nice.
static int y_tick_hint(CartesianOptions const &opts) {
  int lo = opts.min_y_labels > 0 ? opts.min_y_labels : 0;
  int hi = opts.max_y_labels > 0 ? opts.max_y_labels : 0;
  if (lo > 0 && hi > 0 && lo > hi)
    std::swap(lo, hi); // normalize inverted constraint
  if (hi > 0)
    return hi;
  if (lo > 0)
    return lo;
  return 6; // historical default
}

} // namespace

void CartesianChart::set_categories(std::vector<std::string> categories) {
  categories_ = std::move(categories);
}

void CartesianChart::add_series(std::string label, std::vector<double> values) {
  DataSeries s;
  s.label = std::move(label);
  s.values = std::move(values);
  s.color = palette_color(series_.size());
  s.auto_color = true;
  series_.push_back(std::move(s));
}

void CartesianChart::add_series(std::string label, std::vector<double> values, Color color) {
  DataSeries s;
  s.label = std::move(label);
  s.values = std::move(values);
  s.color = color;
  series_.push_back(std::move(s));
}

void CartesianChart::clear_series() {
  series_.clear();
}

double CartesianChart::compute_y_min() const {
  detail::FiniteRange r;
  for (auto const &s : series_) {
    for (double v : s.values)
      r.add(v);
  }
  if (!r.any)
    return 0.0;
  // A log domain can't include or approach zero (log(0) is undefined), so
  // begin_at_zero is meaningless there regardless of the caller's setting —
  // forcing it anyway would stretch the domain across hundreds of decades.
  if (cartesian_.begin_at_zero && cartesian_.y_scale_type != ScaleType::Log)
    return std::min(0.0, r.lo);
  return r.lo;
}

double CartesianChart::compute_y_max() const {
  detail::FiniteRange r;
  for (auto const &s : series_) {
    for (double v : s.values)
      r.add(v);
  }
  return r.max_from_zero();
}

Rect CartesianChart::compute_plot_rect(ImageBuffer const &buf, Font const &font) const {
  float w = static_cast<float>(buf.width());
  float h = static_cast<float>(buf.height());
  Margins m = options_.margins;

  if (!options_.title.empty()) {
    m.top = std::max(m.top, font.line_height() + 16.0f);
  }
  if (options_.legend.enabled && options_.legend.position == LegendOptions::Position::Bottom) {
    float lh = font.line_height();
    float legend_row = lh + 10.0f;
    float x_title_h = cartesian_.x_axis_label.empty() ? 0.0f : lh + 8.0f;
    // x-label gap + x-label row + gap + legend row + optional x-axis title
    m.bottom = std::max(m.bottom, 8.0f + lh + 8.0f + legend_row + x_title_h);
  }
  if (options_.legend.enabled && options_.legend.position == LegendOptions::Position::Right) {
    m.right = std::max(m.right, 100.0f);
  }
  if (has_secondary_y_axis()) {
    float rh = cartesian_.y2_axis_label.empty() ? 0.0f : font.line_height() + 8.0f;
    m.right = std::max(m.right, 56.0f + rh);
  }
  if (is_horizontal()) {
    // Category labels move from the bottom to the left in horizontal mode; widen the
    // left margin to fit the longest one instead of the (typically shorter) numeric
    // value-tick labels the default margin was sized for.
    float max_cat_w = 0.0f;
    for (auto const &cat : categories_)
      max_cat_w = std::max(max_cat_w, font.measure_width(cat));
    if (max_cat_w > 0.0f)
      m.left = std::max(m.left, max_cat_w + 16.0f);
  }

  return Rect(m.left, m.top, w - m.left - m.right, h - m.top - m.bottom);
}

void CartesianChart::draw_background(ImageBuffer &buf) const {
  buf.clear(options_.background);
}

void CartesianChart::draw_title(ImageBuffer &buf, Font &font) const {
  if (options_.title.empty())
    return;
  font.set_size(options_.font_size + 4.0f);
  float tw = font.measure_width(options_.title);
  float x = (static_cast<float>(buf.width()) - tw) * 0.5f;
  font.draw_text(buf, x, 12.0f, options_.title, options_.title_color, TextAlign::Left,
                 TextBaseline::Top);
  font.set_size(options_.font_size);
}

void CartesianChart::draw_axes_and_grid(PlotContext &ctx) const {
  auto &buf = ctx.buffer;
  Rect const &plot = ctx.plot;
  Font font;
  font.set_size(ctx.opts.font_size);

  // Value-axis ticks — respect min_y_labels / max_y_labels
  int y_lo = ctx.cart_opts.min_y_labels;
  int y_hi = ctx.cart_opts.max_y_labels;
  if (y_lo > 0 && y_hi > 0 && y_lo > y_hi)
    std::swap(y_lo, y_hi);
  int yhint = y_hi > 0 ? y_hi : (y_lo > 0 ? y_lo : 6);
  auto y_ticks = ctx.y_scale.nice_ticks(yhint);
  // If min_y_labels is uncapped and nice-rounding produced fewer ticks than
  // requested, retry with a finer step (doubled hint) for a best-effort result.
  if (y_lo > 0 && y_hi == 0 && static_cast<int>(y_ticks.size()) < y_lo) {
    auto more = ctx.y_scale.nice_ticks(yhint * 2);
    if (more.size() > y_ticks.size())
      y_ticks = std::move(more);
  }
  if (ctx.horizontal) {
    // Horizontal bar mode: the value axis runs along the bottom instead of the left —
    // vertical gridlines, labels centered under each tick (same style as a DateTime
    // x-axis tick label).
    for (double v : y_ticks) {
      float px = ctx.y_scale.to_pixel(v);
      if (px < plot.left() || px > plot.right())
        continue;
      if (ctx.opts.show_grid)
        buf.draw_line(px, plot.top(), px, plot.bottom(), 1.0f, ctx.opts.grid_color);
      std::string label = detail::format_tick_value(v, ctx.cart_opts.y_tick_format,
                                                    ctx.cart_opts.custom_y_formatter);
      font.draw_text(buf, px, plot.bottom() + 8.0f, label, ctx.opts.text_color, TextAlign::Center,
                     TextBaseline::Top);
    }
  } else {
    for (double v : y_ticks) {
      float py = ctx.y_scale.to_pixel(v);
      if (py < plot.top() || py > plot.bottom())
        continue;

      if (ctx.opts.show_grid) {
        buf.draw_line(plot.left(), py, plot.right(), py, 1.0f, ctx.opts.grid_color);
      }

      std::string label = detail::format_tick_value(v, ctx.cart_opts.y_tick_format,
                                                    ctx.cart_opts.custom_y_formatter);
      font.draw_text(buf, plot.left() - 8.0f, py, label, ctx.opts.text_color, TextAlign::Right,
                     TextBaseline::Middle);
    }
  }

  // Secondary (right) y-axis ticks, if enabled.
  if (ctx.has_y2) {
    auto y2_ticks = ctx.y_scale2.nice_ticks(6);
    for (double v : y2_ticks) {
      float py = ctx.y_scale2.to_pixel(v);
      if (py < plot.top() || py > plot.bottom())
        continue;

      std::string label = detail::format_tick_value(v, ctx.cart_opts.y2_tick_format,
                                                    ctx.cart_opts.custom_y2_formatter);
      font.draw_text(buf, plot.right() + 8.0f, py, label, ctx.opts.text_color, TextAlign::Left,
                     TextBaseline::Middle);
    }
  }

  if (ctx.horizontal) {
    // Horizontal bar mode: category labels move to the left (in place of the numeric
    // value-tick labels the vertical layout would draw there), one per even vertical
    // slot, top-to-bottom in category order. No collision-avoidance skip-step: with
    // categories running down the plot's height rather than across its width, the
    // usual dense-x-axis crowding this chart type is meant to solve doesn't apply.
    std::size_t n_cats = ctx.categories.empty() ? 1 : ctx.categories.size();
    float slot = plot.h / static_cast<float>(n_cats);
    for (std::size_t i = 0; i < ctx.categories.size(); ++i) {
      float py = plot.top() + slot * (static_cast<float>(i) + 0.5f);
      font.draw_text(buf, plot.left() - 8.0f, py, ctx.categories[i], ctx.opts.text_color,
                     TextAlign::Right, TextBaseline::Middle);
    }
  } else if (ctx.continuous_x) {
    // Continuous (DateTime) x-axis: nice ticks from the numeric domain, formatted the same
    // way as y-axis DateTime ticks, positioned by real value via x_scale rather than slot.
    int xhint = ctx.cart_opts.max_x_labels > 0 ? ctx.cart_opts.max_x_labels : 8;
    for (double v : ctx.x_scale.nice_ticks(xhint)) {
      float cx = ctx.x_scale.to_pixel(v);
      if (cx < plot.left() || cx > plot.right())
        continue;
      if (ctx.opts.show_grid)
        buf.draw_line(cx, plot.top(), cx, plot.bottom(), 1.0f, ctx.opts.grid_color);
      std::string label = detail::format_tick_value(v, TickFormat::DateTime, nullptr);
      font.draw_text(buf, cx, plot.bottom() + 8.0f, label, ctx.opts.text_color, TextAlign::Center,
                     TextBaseline::Top);
    }
  } else {
    // X-axis category labels with collision avoidance
    std::size_t n_cats = ctx.categories.empty() ? 1 : ctx.categories.size();
    float slot = plot.w / static_cast<float>(n_cats);

    // Find the smallest step such that adjacent drawn labels do not overlap.
    int step = 1;
    if (!ctx.categories.empty()) {
      float max_label_w = 0.0f;
      for (auto const &cat : ctx.categories)
        max_label_w = std::max(max_label_w, font.measure_width(cat));
      // Each drawn label needs slot*step pixels; require a 4px gap.
      while (static_cast<float>(step) * slot < max_label_w + 4.0f &&
             step < static_cast<int>(ctx.categories.size()))
        ++step;
    }

    // Apply max_x_labels ceiling on top of the collision-avoidance step.
    if (ctx.cart_opts.max_x_labels > 0 &&
        static_cast<int>(ctx.categories.size()) > ctx.cart_opts.max_x_labels) {
      int step_from_limit = static_cast<int>(
          std::ceil(static_cast<double>(ctx.categories.size()) / ctx.cart_opts.max_x_labels));
      step = std::max(step, step_from_limit);
    }

    for (std::size_t i = 0; i < ctx.categories.size(); i += static_cast<std::size_t>(step)) {
      float cx = plot.left() + slot * (static_cast<float>(i) + 0.5f);
      font.draw_text(buf, cx, plot.bottom() + 8.0f, ctx.categories[i], ctx.opts.text_color,
                     TextAlign::Center, TextBaseline::Top);
    }
  }

  // Axis lines
  Color axis = ctx.opts.axis_color;
  buf.draw_line(plot.left(), plot.bottom(), plot.right(), plot.bottom(), 1.5f, axis);
  buf.draw_line(plot.left(), plot.top(), plot.left(), plot.bottom(), 1.5f, axis);
  if (ctx.has_y2)
    buf.draw_line(plot.right(), plot.top(), plot.right(), plot.bottom(), 1.5f, axis);

  // Axis titles
  if (!ctx.cart_opts.y_axis_label.empty()) {
    float lh = font.line_height();
    font.draw_text_rotated_ccw(buf, lh * 0.5f + 4.0f, plot.center_y(), ctx.cart_opts.y_axis_label,
                               ctx.opts.text_color);
  }
  if (ctx.has_y2 && !ctx.cart_opts.y2_axis_label.empty()) {
    float lh = font.line_height();
    font.draw_text_rotated_cw(buf, static_cast<float>(buf.width()) - lh * 0.5f - 4.0f,
                              plot.center_y(), ctx.cart_opts.y2_axis_label, ctx.opts.text_color);
  }
  if (!ctx.cart_opts.x_axis_label.empty()) {
    font.draw_text(buf, plot.center_x(), static_cast<float>(buf.height()) - 8.0f,
                   ctx.cart_opts.x_axis_label, ctx.opts.text_color, TextAlign::Center,
                   TextBaseline::Bottom);
  }
}

void CartesianChart::draw_legend(ImageBuffer &buf, Font &font, Rect plot) const {
  if (!options_.legend.enabled || series_.empty())
    return;

  font.set_size(options_.font_size);
  float box = 10.0f;
  float gap = 16.0f;
  float item_w = 0.0f;
  for (auto const &s : series_) {
    item_w = std::max(item_w, box + 6.0f + font.measure_width(s.label));
  }
  float total_w =
      item_w * static_cast<float>(series_.size()) + gap * static_cast<float>(series_.size() - 1);
  float x = (static_cast<float>(buf.width()) - total_w) * 0.5f;
  // Position legend below x-axis category labels: gap + label row + gap
  float y = plot.bottom() + 8.0f + font.line_height() + 8.0f;

  if (options_.legend.position == LegendOptions::Position::Right) {
    x = plot.right() + 12.0f;
    y = plot.top();
    for (std::size_t i = 0; i < series_.size(); ++i) {
      auto const &s = series_[i];
      buf.fill_rect(x, y, box, box, s.color);
      font.draw_text(buf, x + box + 6.0f, y + box * 0.5f, s.label, options_.text_color,
                     TextAlign::Left, TextBaseline::Middle);
      y += box + 8.0f;
    }
    return;
  }

  for (auto const &s : series_) {
    buf.fill_rect(x, y, box, box, s.color);
    font.draw_text(buf, x + box + 6.0f, y + box * 0.5f, s.label, options_.text_color,
                   TextAlign::Left, TextBaseline::Middle);
    x += item_w + gap;
  }
}

ImageBuffer CartesianChart::render() const {
  ImageBuffer buf(options_.width, options_.height);
  draw_background(buf);

  Font font;
  font.set_size(options_.font_size);
  draw_title(buf, font);

  Rect plot = compute_plot_rect(buf, font);
  if (plot.w <= 0.0f || plot.h <= 0.0f)
    return buf;

  double ymin = compute_y_min();
  double ymax = compute_y_max();

  bool horizontal = is_horizontal();
  int yticks = y_tick_hint(cartesian_);
  Scale y_scale;
  if (horizontal) {
    // Horizontal bar mode: y_scale still represents the "value" axis, but its pixel
    // range now spans the plot's horizontal extent (domain min at the left edge,
    // domain max at the right — same swapped-argument convention as the left-to-right
    // x_scale below).
    y_scale = cartesian_.y_scale_type == ScaleType::Log
                  ? Scale::log_nice(ymin, ymax, plot.right(), plot.left(), yticks)
                  : Scale::linear_nice(ymin, ymax, plot.right(), plot.left(), yticks);
  } else {
    // pixel_min = plot.top()   (small y, physically at top)   → where data_max maps
    // pixel_max = plot.bottom() (large y, physically at bottom) → where data_min maps
    y_scale = cartesian_.y_scale_type == ScaleType::Log
                  ? Scale::log_nice(ymin, ymax, plot.top(), plot.bottom(), yticks)
                  : Scale::linear_nice(ymin, ymax, plot.top(), plot.bottom(), yticks);
  }

  bool continuous_x = supports_continuous_x_axis() &&
                      cartesian_.x_scale_type == ScaleType::DateTime && !x_values_.empty();

  Scale x_scale;
  if (continuous_x) {
    double xmin = x_values_.front();
    double xmax = x_values_.back();
    if (xmax <= xmin)
      xmax = xmin + 1.0;
    // Same pixel_min/pixel_max swap as the category x_scale below, for the same reason:
    // domain min must land at the left edge, domain max at the right edge.
    x_scale = Scale(xmin, xmax, plot.right(), plot.left());
    x_scale.set_type(ScaleType::DateTime);
  } else {
    std::size_t n = std::max<std::size_t>(categories_.size(), 1);
    // Scale::to_pixel() maps its domain min to pixel_max and domain max to pixel_min
    // (the convention that makes higher y-values render higher on screen). For a
    // left-to-right x-axis that inverts the mapping, so pixel_min/pixel_max are passed
    // swapped here — the same fix ScatterChart's x_scale already applies.
    x_scale = Scale(0.0, static_cast<double>(n), plot.right(), plot.left());
  }

  bool has_y2 = has_secondary_y_axis();
  Scale y_scale2;
  if (has_y2)
    y_scale2 = Scale::linear_nice(compute_y2_min(), compute_y2_max(), plot.top(), plot.bottom(), 6);

  PlotContext ctx{buf,        plot, x_scale,  y_scale,    y_scale2,    has_y2,  continuous_x,
                  horizontal, font, options_, cartesian_, categories_, series_, x_values_};
  draw_axes_and_grid(ctx);
  draw_series(ctx);
  draw_overlay(ctx);
  draw_legend(buf, font, plot);

  return buf;
}

} // namespace insightdock::charts
