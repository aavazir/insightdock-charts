// SPDX-License-Identifier: MIT
// Copyright (c) InsightDock contributors

#include "insightdock/area_chart.h"
#include "insightdock/detail/marker.h"
#include "insightdock/detail/reference_lines.h"
#include "insightdock/detail/series_runs.h"
#include "insightdock/detail/spline.h"
#include "insightdock/detail/tick_format.h"
#include "insightdock/font.h"

#include <cmath>
#include <cstdint>
#include <vector>

namespace insightdock::charts {

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

  if (area_opts_.stacked) {
    std::vector<double> cumulative(ncat, 0.0);
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
        if (!std::isfinite(val))
          val = 0.0;
        double base = cumulative[i];
        cumulative[i] += val;
        float px = ctx.continuous_x ? ctx.x_scale.to_pixel(ctx.x_values[i])
                                    : ctx.plot.left() + slot * (static_cast<float>(i) + 0.5f);
        top_pts.push_back({px, ctx.y_scale.to_pixel(cumulative[i])});
        bottom_pts.push_back({px, ctx.y_scale.to_pixel(base)});
      }

      std::vector<std::pair<float, float>> top_curve =
          area_opts_.spline ? detail::catmull_rom_expand(top_pts) : top_pts;
      std::vector<std::pair<float, float>> bottom_curve =
          area_opts_.spline ? detail::catmull_rom_expand(bottom_pts) : bottom_pts;

      std::vector<std::pair<float, float>> poly = top_curve;
      for (auto it = bottom_curve.rbegin(); it != bottom_curve.rend(); ++it)
        poly.push_back(*it);

      if (poly.size() >= 3) {
        Color fill = ser.color;
        fill.a = fill_alpha;
        ctx.buffer.fill_polygon(poly.data(), poly.size(), fill);
      }

      if (area_opts_.show_line && top_curve.size() >= 2) {
        ctx.buffer.draw_dashed_polyline(top_curve.data(), top_curve.size(), line_width, ser.color,
                                        dash);
      }

      if (area_opts_.show_points) {
        for (auto const &p : top_pts) {
          detail::draw_marker(ctx.buffer, p.first, p.second, area_opts_.point_radius, shape,
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
          label_font.draw_text(ctx.buffer, top_pts[i].first, top_pts[i].second - 4.0f, text,
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
          area_opts_.spline ? detail::catmull_rom_expand(pts) : pts;

      std::vector<std::pair<float, float>> poly = curve;
      poly.push_back({curve.back().first, y0});
      poly.push_back({curve.front().first, y0});
      Color fill = ser.color;
      fill.a = fill_alpha;
      ctx.buffer.fill_polygon(poly.data(), poly.size(), fill);

      if (area_opts_.show_line) {
        ctx.buffer.draw_dashed_polyline(curve.data(), curve.size(), line_width, ser.color, dash);
      }

      if (area_opts_.show_points) {
        for (auto const &p : pts) {
          detail::draw_marker(ctx.buffer, p.first, p.second, area_opts_.point_radius, shape,
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
          label_font.draw_text(ctx.buffer, pts[k].first, pts[k].second - 4.0f, text, dl.color,
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
