// SPDX-License-Identifier: MIT
// Copyright (c) InsightDock contributors

#include "insightdock/combo_chart.h"
#include "insightdock/detail/bar_shape.h"
#include "insightdock/detail/finite_range.h"
#include "insightdock/detail/marker.h"
#include "insightdock/detail/reference_lines.h"
#include "insightdock/detail/spline.h"
#include "insightdock/detail/tick_format.h"
#include "insightdock/font.h"

#include <algorithm>
#include <cmath>
#include <vector>

namespace insightdock::charts {

namespace {

// Mirrors CartesianChart::compute_y_min/max, but over a filtered subset of
// series (used to give the primary and secondary axes independent domains).
bool series_uses_secondary(std::vector<bool> const &use_y2, std::size_t i) {
  return i < use_y2.size() && use_y2[i];
}

detail::FiniteRange axis_range(std::vector<DataSeries> const &series,
                               std::vector<bool> const &use_y2, bool secondary) {
  detail::FiniteRange r;
  for (std::size_t i = 0; i < series.size(); ++i) {
    if (series_uses_secondary(use_y2, i) != secondary)
      continue;
    for (double v : series[i].values)
      r.add(v);
  }
  return r;
}

double axis_min(std::vector<DataSeries> const &series, std::vector<bool> const &use_y2,
                bool secondary, bool begin_at_zero) {
  auto r = axis_range(series, use_y2, secondary);
  if (!r.any)
    return 0.0;
  return begin_at_zero ? std::min(0.0, r.lo) : r.lo;
}

double axis_max(std::vector<DataSeries> const &series, std::vector<bool> const &use_y2,
                bool secondary) {
  return axis_range(series, use_y2, secondary).max_from_zero();
}

} // namespace

void ComboChart::add_bar_series(std::string label, std::vector<double> values,
                                bool secondary_axis) {
  add_series(std::move(label), std::move(values));
  kinds_.push_back(ComboSeriesKind::Bar);
  use_y2_.push_back(secondary_axis);
}

void ComboChart::add_bar_series(std::string label, std::vector<double> values, Color color,
                                bool secondary_axis) {
  add_series(std::move(label), std::move(values), color);
  kinds_.push_back(ComboSeriesKind::Bar);
  use_y2_.push_back(secondary_axis);
}

void ComboChart::add_line_series(std::string label, std::vector<double> values,
                                 bool secondary_axis) {
  add_series(std::move(label), std::move(values));
  kinds_.push_back(ComboSeriesKind::Line);
  use_y2_.push_back(secondary_axis);
}

void ComboChart::add_line_series(std::string label, std::vector<double> values, Color color,
                                 bool secondary_axis) {
  add_series(std::move(label), std::move(values), color);
  kinds_.push_back(ComboSeriesKind::Line);
  use_y2_.push_back(secondary_axis);
}

void ComboChart::clear_series() {
  CartesianChart::clear_series();
  kinds_.clear();
  use_y2_.clear();
}

bool ComboChart::has_secondary_y_axis() const {
  for (bool b : use_y2_)
    if (b)
      return true;
  return false;
}

double ComboChart::compute_y_min() const {
  return axis_min(series_, use_y2_, /*secondary=*/false, cartesian_.begin_at_zero);
}

double ComboChart::compute_y_max() const {
  return axis_max(series_, use_y2_, /*secondary=*/false);
}

double ComboChart::compute_y2_min() const {
  return axis_min(series_, use_y2_, /*secondary=*/true, cartesian_.begin_at_zero);
}

double ComboChart::compute_y2_max() const {
  return axis_max(series_, use_y2_, /*secondary=*/true);
}

Scale const &ComboChart::axis_scale(PlotContext const &ctx, std::size_t series_index) const {
  return series_uses_secondary(use_y2_, series_index) ? ctx.y_scale2 : ctx.y_scale;
}

void ComboChart::draw_series(PlotContext &ctx) const {
  std::size_t ncat = std::max<std::size_t>(ctx.categories.size(), 1);

  std::size_t nbar = 0;
  for (auto k : kinds_)
    if (k == ComboSeriesKind::Bar)
      ++nbar;
  if (nbar == 0)
    return;

  float slot = ctx.plot.w / static_cast<float>(ncat);
  float group_w = slot * (1.0f - bar_opts_.group_gap);
  float bar_w = group_w / static_cast<float>(nbar);
  // Both axes share the same pixel range (plot.top()..plot.bottom()), so the
  // zero baseline is plot.bottom() regardless of which axis a series uses.
  float y0 = ctx.plot.bottom();

  Font bar_label_font(options_.font);
  if (bar_opts_.data_labels.enabled)
    bar_label_font.set_size(bar_opts_.data_labels.font_size);

  std::size_t bar_idx = 0;
  for (std::size_t si = 0; si < ctx.series.size(); ++si) {
    if (si >= kinds_.size() || kinds_[si] != ComboSeriesKind::Bar)
      continue;
    auto const &ser = ctx.series[si];
    Scale const &scale = axis_scale(ctx, si);
    for (std::size_t ci = 0; ci < ncat; ++ci) {
      double val = (ci < ser.values.size()) ? ser.values[ci] : 0.0;
      float cx =
          ctx.plot.left() + slot * static_cast<float>(ci) + slot * bar_opts_.group_gap * 0.5f;
      float bx = cx + static_cast<float>(bar_idx) * bar_w;
      float by_top = scale.to_pixel(val);
      float bh = y0 - by_top;
      if (bh < 0.0f) {
        by_top = y0;
        bh = -bh;
      }
      detail::fill_bar(ctx.buffer, bx, by_top, bar_w, bh, ser.color, bar_opts_.corner_radius,
                       bar_opts_.border_color, bar_opts_.border_width);

      if (bar_opts_.data_labels.enabled) {
        auto const &dl = bar_opts_.data_labels;
        std::string text = detail::format_label_value(val, dl.format, dl.decimals);
        bar_label_font.draw_text(ctx.buffer, bx + bar_w * 0.5f, by_top - 4.0f, text, dl.color,
                                 TextAlign::Center, TextBaseline::Bottom);
      }
    }
    ++bar_idx;
  }
}

void ComboChart::draw_overlay(PlotContext &ctx) const {
  detail::draw_reference_lines(ctx);

  std::size_t ncat = std::max<std::size_t>(ctx.categories.size(), 1);
  float slot = ctx.plot.w / static_cast<float>(ncat);

  Font line_label_font(options_.font);
  if (line_opts_.data_labels.enabled)
    line_label_font.set_size(line_opts_.data_labels.font_size);

  for (std::size_t si = 0; si < ctx.series.size(); ++si) {
    if (si >= kinds_.size() || kinds_[si] != ComboSeriesKind::Line)
      continue;
    auto const &ser = ctx.series[si];
    Scale const &scale = axis_scale(ctx, si);

    std::vector<std::pair<float, float>> pts;
    pts.reserve(ncat);
    for (std::size_t i = 0; i < ncat; ++i) {
      double val = (i < ser.values.size()) ? ser.values[i] : 0.0;
      float px = ctx.plot.left() + slot * (static_cast<float>(i) + 0.5f);
      float py = scale.to_pixel(val);
      pts.push_back({px, py});
    }

    std::vector<std::pair<float, float>> curve =
        line_opts_.spline ? detail::catmull_rom_expand(pts) : pts;

    if (curve.size() >= 2) {
      ctx.buffer.draw_dashed_polyline(curve.data(), curve.size(), line_opts_.line_width, ser.color,
                                      line_opts_.dash);
    }

    if (line_opts_.show_points) {
      for (auto const &p : pts)
        detail::draw_marker(ctx.buffer, p.first, p.second, line_opts_.point_radius,
                            line_opts_.point_shape, ser.color);
    }

    if (line_opts_.data_labels.enabled) {
      auto const &dl = line_opts_.data_labels;
      for (std::size_t i = 0; i < pts.size(); ++i) {
        double val = (i < ser.values.size()) ? ser.values[i] : 0.0;
        std::string text = detail::format_label_value(val, dl.format, dl.decimals);
        line_label_font.draw_text(ctx.buffer, pts[i].first,
                                  pts[i].second - line_opts_.point_radius - 4.0f, text, dl.color,
                                  TextAlign::Center, TextBaseline::Bottom);
      }
    }
  }
}

} // namespace insightdock::charts
