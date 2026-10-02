// SPDX-License-Identifier: MIT
// Copyright (c) InsightDock contributors

#include "insightdock/line_chart.h"
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

void LineChart::draw_series(PlotContext &ctx) const {
  std::size_t ncat =
      ctx.continuous_x ? ctx.x_values.size() : std::max<std::size_t>(ctx.categories.size(), 1);
  if (ctx.series.empty())
    return;

  float slot = ctx.continuous_x ? 0.0f : ctx.plot.w / static_cast<float>(ncat);

  Font label_font(options_.font);
  if (line_opts_.data_labels.enabled)
    label_font.set_size(line_opts_.data_labels.font_size);

  for (auto const &ser : ctx.series) {
    auto const &dash = ser.style.dash_or(line_opts_.dash);
    float line_width = ser.style.line_width_or(line_opts_.line_width);
    PointMarkerShape shape = ser.style.marker_or(line_opts_.point_shape);
    std::uint8_t fill_alpha = ser.style.fill_alpha_or(60);

    auto runs = detail::make_runs(
        ncat, line_opts_.gap_policy,
        [&](std::size_t i) { return detail::value_or_nan(ser.values, i); },
        [&](std::size_t i) {
          return ctx.continuous_x ? ctx.x_scale.to_pixel(ctx.x_values[i])
                                  : ctx.plot.left() + slot * (static_cast<float>(i) + 0.5f);
        },
        [&](double v) { return ctx.y_scale.to_pixel(v); });

    for (auto const &run : runs) {
      // Points remain at the raw per-category positions for markers/labels; the spline
      // curve (if enabled) is a separate, denser point list used only for the stroke/fill.
      std::vector<std::pair<float, float>> curve_pts =
          line_opts_.spline ? detail::catmull_rom_expand(run.pts) : run.pts;

      if (line_opts_.fill_area && curve_pts.size() >= 2) {
        std::vector<std::pair<float, float>> poly = curve_pts;
        float y0 = ctx.y_scale.to_pixel(ctx.y_scale.min());
        poly.push_back({curve_pts.back().first, y0});
        poly.push_back({curve_pts.front().first, y0});
        Color fill = ser.color;
        fill.a = fill_alpha;
        ctx.buffer.fill_polygon(poly.data(), poly.size(), fill);
      }

      if (curve_pts.size() >= 2) {
        ctx.buffer.draw_dashed_polyline(curve_pts.data(), curve_pts.size(), line_width, ser.color,
                                        dash);
      }
    }

    for (auto const &run : runs) {
      if (line_opts_.show_points) {
        for (auto const &p : run.pts) {
          detail::draw_marker(ctx.buffer, p.first, p.second, line_opts_.point_radius, shape,
                              ser.color);
        }
      }

      if (line_opts_.data_labels.enabled) {
        auto const &dl = line_opts_.data_labels;
        for (std::size_t k = 0; k < run.pts.size(); ++k) {
          double val = detail::value_or_nan(ser.values, run.idx[k]);
          if (!std::isfinite(val))
            val = 0.0;
          std::string text = detail::format_label_value(val, dl.format, dl.decimals);
          label_font.draw_text(ctx.buffer, run.pts[k].first,
                               run.pts[k].second - line_opts_.point_radius - 4.0f, text, dl.color,
                               TextAlign::Center, TextBaseline::Bottom);
        }
      }
    }
  }
}

void LineChart::draw_overlay(PlotContext &ctx) const {
  detail::draw_reference_lines(ctx);
}

} // namespace insightdock::charts
