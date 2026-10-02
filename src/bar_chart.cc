// SPDX-License-Identifier: MIT
// Copyright (c) InsightDock contributors

#include "insightdock/bar_chart.h"
#include "insightdock/detail/bar_shape.h"
#include "insightdock/detail/reference_lines.h"
#include "insightdock/detail/tick_format.h"
#include "insightdock/font.h"

#include <algorithm>
#include <cmath>

namespace insightdock::charts {

double BarChart::compute_y_max() const {
  if (bar_opts_.mode == BarMode::Stacked100)
    return 100.0;
  if (bar_opts_.mode != BarMode::Stacked)
    return CartesianChart::compute_y_max();

  std::size_t ncat = categories_.size();
  if (ncat == 0)
    ncat = 1;
  double ymax = 0.0;
  for (std::size_t ci = 0; ci < ncat; ++ci) {
    double sum = 0.0;
    for (auto const &s : series_) {
      if (ci < s.values.size() && std::isfinite(s.values[ci]))
        sum += s.values[ci];
    }
    if (sum > ymax)
      ymax = sum;
  }
  return ymax;
}

namespace {

// Factor mapping a category's raw values to its stacked extents: 1 for plain stacking, or
// 100/total for 100%-stacked (0 when the category has nothing positive to show).
double stack_scale(std::vector<DataSeries> const &series, std::size_t ci, bool percent) {
  if (!percent)
    return 1.0;
  double total = 0.0;
  for (auto const &s : series) {
    if (ci < s.values.size() && std::isfinite(s.values[ci]) && s.values[ci] > 0.0)
      total += s.values[ci];
  }
  return total > 0.0 ? 100.0 / total : 0.0;
}

// The value a stacked segment contributes (non-finite is 0; negatives are 0 when normalising).
double stack_value(DataSeries const &ser, std::size_t ci, bool percent) {
  double v = ci < ser.values.size() ? ser.values[ci] : 0.0;
  if (!std::isfinite(v))
    return 0.0;
  return (percent && v < 0.0) ? 0.0 : v;
}

} // namespace

void BarChart::draw_series(PlotContext &ctx) const {
  if (bar_opts_.orientation == BarOrientation::Horizontal) {
    if (bar_opts_.mode != BarMode::Grouped)
      draw_stacked_horizontal(ctx);
    else
      draw_grouped_horizontal(ctx);
    return;
  }
  if (bar_opts_.mode != BarMode::Grouped)
    draw_stacked(ctx);
  else
    draw_grouped(ctx);
}

void BarChart::draw_overlay(PlotContext &ctx) const {
  detail::draw_reference_lines(ctx);
}

void BarChart::draw_grouped(PlotContext &ctx) const {
  std::size_t ncat = std::max<std::size_t>(ctx.categories.size(), 1);
  std::size_t nser = ctx.series.size();
  if (nser == 0)
    return;

  float slot = ctx.plot.w / static_cast<float>(ncat);
  float group_w = slot * (1.0f - bar_opts_.group_gap);
  float bar_w = group_w / static_cast<float>(nser);
  bar_w *= (1.0f - bar_opts_.bar_gap);

  float y0 = ctx.y_scale.to_pixel(ctx.y_scale.min());

  Font label_font(options_.font);
  if (bar_opts_.data_labels.enabled)
    label_font.set_size(bar_opts_.data_labels.font_size);

  for (std::size_t si = 0; si < nser; ++si) {
    auto const &ser = ctx.series[si];
    for (std::size_t ci = 0; ci < ncat; ++ci) {
      double val = (ci < ser.values.size()) ? ser.values[ci] : 0.0;
      float cx =
          ctx.plot.left() + slot * static_cast<float>(ci) + slot * bar_opts_.group_gap * 0.5f;
      float bx = cx + static_cast<float>(si) * (group_w / static_cast<float>(nser));
      float by_top = ctx.y_scale.to_pixel(val);
      float bh = y0 - by_top;
      if (bh < 0.0f) {
        float t = by_top;
        by_top = y0;
        bh = t - y0;
      }
      detail::fill_bar(ctx.target, bx, by_top, bar_w, bh, ser.color, bar_opts_.corner_radius,
                       bar_opts_.border_color, bar_opts_.border_width);

      if (bar_opts_.data_labels.enabled) {
        auto const &dl = bar_opts_.data_labels;
        std::string text = detail::format_label_value(val, dl.format, dl.decimals);
        ctx.target.draw_text(label_font, bx + bar_w * 0.5f, by_top - 4.0f, text, dl.color,
                             TextAlign::Center, TextBaseline::Bottom);
      }
    }
  }
}

void BarChart::draw_stacked(PlotContext &ctx) const {
  std::size_t ncat = std::max<std::size_t>(ctx.categories.size(), 1);
  if (ctx.series.empty())
    return;

  float slot = ctx.plot.w / static_cast<float>(ncat);
  float bar_w = slot * (1.0f - bar_opts_.group_gap);
  float y_base = ctx.y_scale.to_pixel(0.0);

  Font label_font(options_.font);
  if (bar_opts_.data_labels.enabled)
    label_font.set_size(bar_opts_.data_labels.font_size);

  bool percent = bar_opts_.mode == BarMode::Stacked100;
  for (std::size_t ci = 0; ci < ncat; ++ci) {
    float bx = ctx.plot.left() + slot * static_cast<float>(ci) + slot * bar_opts_.group_gap * 0.5f;
    double stack = 0.0;
    double k = stack_scale(ctx.series, ci, percent);
    for (auto const &ser : ctx.series) {
      double val = stack_value(ser, ci, percent);
      double y1 = stack;
      double y2 = stack + val * k;
      float py_top = ctx.y_scale.to_pixel(y2);
      float py_bot = ctx.y_scale.to_pixel(y1);
      float top = std::min(py_top, py_bot);
      float height = std::fabs(py_top - py_bot);
      detail::fill_bar(ctx.target, bx, top, bar_w, height, ser.color, bar_opts_.corner_radius,
                       bar_opts_.border_color, bar_opts_.border_width);

      if (bar_opts_.data_labels.enabled) {
        auto const &dl = bar_opts_.data_labels;
        std::string text = detail::format_label_value(val, dl.format, dl.decimals);
        ctx.target.draw_text(label_font, bx + bar_w * 0.5f, top + height * 0.5f, text, dl.color,
                             TextAlign::Center, TextBaseline::Middle);
      }
      stack = y2;
    }
    (void)y_base;
  }
}

void BarChart::draw_grouped_horizontal(PlotContext &ctx) const {
  std::size_t ncat = std::max<std::size_t>(ctx.categories.size(), 1);
  std::size_t nser = ctx.series.size();
  if (nser == 0)
    return;

  float slot = ctx.plot.h / static_cast<float>(ncat);
  float group_h = slot * (1.0f - bar_opts_.group_gap);
  float bar_h = group_h / static_cast<float>(nser);
  bar_h *= (1.0f - bar_opts_.bar_gap);

  float x0 = ctx.y_scale.to_pixel(ctx.y_scale.min());

  Font label_font(options_.font);
  if (bar_opts_.data_labels.enabled)
    label_font.set_size(bar_opts_.data_labels.font_size);

  for (std::size_t si = 0; si < nser; ++si) {
    auto const &ser = ctx.series[si];
    for (std::size_t ci = 0; ci < ncat; ++ci) {
      double val = (ci < ser.values.size()) ? ser.values[ci] : 0.0;
      float cy = ctx.plot.top() + slot * static_cast<float>(ci) + slot * bar_opts_.group_gap * 0.5f;
      float by = cy + static_cast<float>(si) * (group_h / static_cast<float>(nser));
      float bx_end = ctx.y_scale.to_pixel(val);
      float bx_left = x0;
      float bw = bx_end - x0;
      if (bw < 0.0f) {
        bx_left = bx_end;
        bw = x0 - bx_end;
      }
      detail::fill_bar(ctx.target, bx_left, by, bw, bar_h, ser.color, bar_opts_.corner_radius,
                       bar_opts_.border_color, bar_opts_.border_width);

      if (bar_opts_.data_labels.enabled) {
        auto const &dl = bar_opts_.data_labels;
        std::string text = detail::format_label_value(val, dl.format, dl.decimals);
        ctx.target.draw_text(label_font, bx_end + 4.0f, by + bar_h * 0.5f, text, dl.color,
                             TextAlign::Left, TextBaseline::Middle);
      }
    }
  }
}

void BarChart::draw_stacked_horizontal(PlotContext &ctx) const {
  std::size_t ncat = std::max<std::size_t>(ctx.categories.size(), 1);
  if (ctx.series.empty())
    return;

  float slot = ctx.plot.h / static_cast<float>(ncat);
  float bar_h = slot * (1.0f - bar_opts_.group_gap);

  Font label_font(options_.font);
  if (bar_opts_.data_labels.enabled)
    label_font.set_size(bar_opts_.data_labels.font_size);

  bool percent = bar_opts_.mode == BarMode::Stacked100;
  for (std::size_t ci = 0; ci < ncat; ++ci) {
    float by = ctx.plot.top() + slot * static_cast<float>(ci) + slot * bar_opts_.group_gap * 0.5f;
    double stack = 0.0;
    double k = stack_scale(ctx.series, ci, percent);
    for (auto const &ser : ctx.series) {
      double val = stack_value(ser, ci, percent);
      double x1 = stack;
      double x2 = stack + val * k;
      float px_end = ctx.y_scale.to_pixel(x2);
      float px_start = ctx.y_scale.to_pixel(x1);
      float left = std::min(px_end, px_start);
      float width = std::fabs(px_end - px_start);
      detail::fill_bar(ctx.target, left, by, width, bar_h, ser.color, bar_opts_.corner_radius,
                       bar_opts_.border_color, bar_opts_.border_width);

      if (bar_opts_.data_labels.enabled) {
        auto const &dl = bar_opts_.data_labels;
        std::string text = detail::format_label_value(val, dl.format, dl.decimals);
        ctx.target.draw_text(label_font, left + width * 0.5f, by + bar_h * 0.5f, text, dl.color,
                             TextAlign::Center, TextBaseline::Middle);
      }
      stack = x2;
    }
  }
}

} // namespace insightdock::charts
