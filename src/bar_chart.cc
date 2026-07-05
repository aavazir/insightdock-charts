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
  if (bar_opts_.mode != BarMode::Stacked)
    return CartesianChart::compute_y_max();

  std::size_t ncat = categories_.size();
  if (ncat == 0)
    ncat = 1;
  double ymax = 0.0;
  for (std::size_t ci = 0; ci < ncat; ++ci) {
    double sum = 0.0;
    for (auto const &s : series_) {
      if (ci < s.values.size())
        sum += s.values[ci];
    }
    if (sum > ymax)
      ymax = sum;
  }
  return ymax;
}

void BarChart::draw_series(PlotContext &ctx) const {
  if (bar_opts_.mode == BarMode::Stacked)
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

  Font label_font;
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
      detail::fill_bar(ctx.buffer, bx, by_top, bar_w, bh, ser.color, bar_opts_.corner_radius,
                       bar_opts_.border_color, bar_opts_.border_width);

      if (bar_opts_.data_labels.enabled) {
        auto const &dl = bar_opts_.data_labels;
        std::string text = detail::format_label_value(val, dl.format, dl.decimals);
        label_font.draw_text(ctx.buffer, bx + bar_w * 0.5f, by_top - 4.0f, text, dl.color,
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

  Font label_font;
  if (bar_opts_.data_labels.enabled)
    label_font.set_size(bar_opts_.data_labels.font_size);

  for (std::size_t ci = 0; ci < ncat; ++ci) {
    float bx = ctx.plot.left() + slot * static_cast<float>(ci) + slot * bar_opts_.group_gap * 0.5f;
    double stack = 0.0;
    for (auto const &ser : ctx.series) {
      double val = (ci < ser.values.size()) ? ser.values[ci] : 0.0;
      double y1 = stack;
      double y2 = stack + val;
      float py_top = ctx.y_scale.to_pixel(y2);
      float py_bot = ctx.y_scale.to_pixel(y1);
      float top = std::min(py_top, py_bot);
      float height = std::fabs(py_top - py_bot);
      detail::fill_bar(ctx.buffer, bx, top, bar_w, height, ser.color, bar_opts_.corner_radius,
                       bar_opts_.border_color, bar_opts_.border_width);

      if (bar_opts_.data_labels.enabled) {
        auto const &dl = bar_opts_.data_labels;
        std::string text = detail::format_label_value(val, dl.format, dl.decimals);
        label_font.draw_text(ctx.buffer, bx + bar_w * 0.5f, top + height * 0.5f, text, dl.color,
                             TextAlign::Center, TextBaseline::Middle);
      }
      stack = y2;
    }
    (void)y_base;
  }
}

} // namespace insightdock::charts
