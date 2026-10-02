// SPDX-License-Identifier: MIT
// Copyright (c) InsightDock contributors

#include "insightdock/area_chart.h"
#include "insightdock/detail/finite_range.h"
#include "insightdock/detail/marker.h"
#include "insightdock/detail/reference_lines.h"
#include "insightdock/detail/series_runs.h"
#include "insightdock/detail/spline.h"
#include "insightdock/detail/step.h"
#include "insightdock/detail/tick_format.h"
#include "insightdock/font.h"

#include <cmath>
#include <cstdint>
#include <vector>

namespace insightdock::charts {

double AreaChart::compute_y_max() const {
  if (area_opts_.stacked_percent)
    return 100.0;
  if (!area_opts_.stacked)
    return CartesianChart::compute_y_max();
  // Stacked: the axis must fit the per-category totals, not the tallest single series.
  std::size_t ncat = std::max<std::size_t>(categories_.size(), 1);
  for (auto const &s : series_)
    ncat = std::max(ncat, s.values.size());
  detail::FiniteRange r;
  for (std::size_t ci = 0; ci < ncat; ++ci) {
    double sum = 0.0;
    for (auto const &s : series_) {
      if (ci < s.values.size() && std::isfinite(s.values[ci]))
        sum += s.values[ci];
    }
    r.add(sum);
  }
  return r.max_from_zero();
}

void AreaChart::draw_series(PlotContext &ctx) const {
  std::size_t ncat =
      ctx.continuous_x ? ctx.x_values.size() : std::max<std::size_t>(ctx.categories.size(), 1);
  if (ctx.series.empty())
    return;

  float slot = ctx.continuous_x ? 0.0f : ctx.plot.w / static_cast<float>(ncat);
  float y0 = ctx.y_scale.to_pixel(ctx.y_scale.min());

  Font label_font(options_.font);
  if (area_opts_.data_labels.enabled)
    label_font.set_size(area_opts_.data_labels.font_size);

  if (area_opts_.stacked || area_opts_.stacked_percent) {
    std::vector<double> cumulative(ncat, 0.0);
    // 100%-stacked: per-category factor mapping raw values onto a 0-100 share.
    std::vector<double> factor(ncat, 1.0);
    if (area_opts_.stacked_percent) {
      for (std::size_t i = 0; i < ncat; ++i) {
        double total = 0.0;
        for (auto const &s : ctx.series) {
          double v = detail::value_or_nan(s.values, i);
          if (std::isfinite(v) && v > 0.0)
            total += v;
        }
        factor[i] = total > 0.0 ? 100.0 / total : 0.0;
      }
    }
    for (auto const &ser : ctx.series) {
      auto const &dash = ser.style.dash_or(area_opts_.dash);
      float line_width = ser.style.line_width_or(area_opts_.line_width);
      PointMarkerShape shape = ser.style.marker_or(area_opts_.point_shape);
      std::uint8_t fill_alpha = ser.style.fill_alpha_or(180);
      std::vector<std::pair<float, float>> top_pts;
      std::vector<std::pair<float, float>> bottom_pts;
      top_pts.reserve(ncat);
      bottom_pts.reserve(ncat);

      for (std::size_t i = 0; i < ncat; ++i) {
        // A missing value contributes nothing to the stack (a stack can't have a hole).
        double val = detail::value_or_nan(ser.values, i);
        if (!std::isfinite(val) || (area_opts_.stacked_percent && val < 0.0))
          val = 0.0;
        double base = cumulative[i];
        cumulative[i] += val * factor[i];
        float px = ctx.continuous_x ? ctx.x_scale.to_pixel(ctx.x_values[i])
                                    : ctx.plot.left() + slot * (static_cast<float>(i) + 0.5f);
        top_pts.push_back({px, ctx.y_scale.to_pixel(cumulative[i])});
        bottom_pts.push_back({px, ctx.y_scale.to_pixel(base)});
      }

      auto shape_curve = [&](std::vector<std::pair<float, float>> const &pts) {
        if (area_opts_.step != StepMode::None)
          return detail::expand_steps(pts, area_opts_.step);
        return area_opts_.spline ? detail::catmull_rom_expand(pts) : pts;
      };
      std::vector<std::pair<float, float>> top_curve = shape_curve(top_pts);
      std::vector<std::pair<float, float>> bottom_curve = shape_curve(bottom_pts);

      std::vector<std::pair<float, float>> poly = top_curve;
      for (auto it = bottom_curve.rbegin(); it != bottom_curve.rend(); ++it)
        poly.push_back(*it);

      if (poly.size() >= 3) {
        Color fill = ser.color;
        fill.a = fill_alpha;
        ctx.target.fill_polygon(poly.data(), poly.size(), fill);
      }

      if (area_opts_.show_line && top_curve.size() >= 2) {
        ctx.target.draw_dashed_polyline(top_curve.data(), top_curve.size(), line_width, ser.color,
                                        dash);
      }

      if (area_opts_.show_points) {
        for (auto const &p : top_pts) {
          detail::draw_marker(ctx.target, p.first, p.second, area_opts_.point_radius, shape,
                              ser.color);
        }
      }

      if (area_opts_.data_labels.enabled) {
        auto const &dl = area_opts_.data_labels;
        for (std::size_t i = 0; i < top_pts.size(); ++i) {
          double val = detail::value_or_nan(ser.values, i);
          if (!std::isfinite(val))
            val = 0.0;
          std::string text = detail::format_label_value(val, dl.format, dl.decimals);
          ctx.target.draw_text(label_font, top_pts[i].first, top_pts[i].second - 4.0f, text,
                               dl.color, TextAlign::Center, TextBaseline::Bottom);
        }
      }
    }
    return;
  }

  for (auto const &ser : ctx.series) {
    auto const &dash = ser.style.dash_or(area_opts_.dash);
    float line_width = ser.style.line_width_or(area_opts_.line_width);
    PointMarkerShape shape = ser.style.marker_or(area_opts_.point_shape);
    std::uint8_t fill_alpha = ser.style.fill_alpha_or(120);

    auto runs = detail::make_runs(
        ncat, area_opts_.gap_policy,
        [&](std::size_t i) { return detail::value_or_nan(ser.values, i); },
        [&](std::size_t i) {
          return ctx.continuous_x ? ctx.x_scale.to_pixel(ctx.x_values[i])
                                  : ctx.plot.left() + slot * (static_cast<float>(i) + 0.5f);
        },
        [&](double v) { return ctx.y_scale.to_pixel(v); });

    for (auto const &run : runs) {
      auto const &pts = run.pts;
      if (pts.size() < 2)
        continue;
      std::vector<std::pair<float, float>> curve =
          area_opts_.step != StepMode::None
              ? detail::expand_steps(pts, area_opts_.step)
              : (area_opts_.spline ? detail::catmull_rom_expand(pts) : pts);

      std::vector<std::pair<float, float>> poly = curve;
      poly.push_back({curve.back().first, y0});
      poly.push_back({curve.front().first, y0});
      Color fill = ser.color;
      fill.a = fill_alpha;
      ctx.target.fill_polygon(poly.data(), poly.size(), fill);

      if (area_opts_.show_line) {
        ctx.target.draw_dashed_polyline(curve.data(), curve.size(), line_width, ser.color, dash);
      }

      if (area_opts_.show_points) {
        for (auto const &p : pts) {
          detail::draw_marker(ctx.target, p.first, p.second, area_opts_.point_radius, shape,
                              ser.color);
        }
      }

      if (area_opts_.data_labels.enabled) {
        auto const &dl = area_opts_.data_labels;
        for (std::size_t k = 0; k < pts.size(); ++k) {
          double val = detail::value_or_nan(ser.values, run.idx[k]);
          if (!std::isfinite(val))
            val = 0.0;
          std::string text = detail::format_label_value(val, dl.format, dl.decimals);
          ctx.target.draw_text(label_font, pts[k].first, pts[k].second - 4.0f, text, dl.color,
                               TextAlign::Center, TextBaseline::Bottom);
        }
      }
    }
  }
}

void AreaChart::draw_overlay(PlotContext &ctx) const {
  detail::draw_reference_lines(ctx);
}

} // namespace insightdock::charts
