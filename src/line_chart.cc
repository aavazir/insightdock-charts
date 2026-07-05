// SPDX-License-Identifier: MIT
// Copyright (c) InsightDock contributors

#include "insightdock/line_chart.h"
#include "insightdock/detail/marker.h"
#include "insightdock/detail/reference_lines.h"
#include "insightdock/detail/spline.h"
#include "insightdock/detail/tick_format.h"
#include "insightdock/font.h"

#include <vector>

namespace insightdock::charts {

void LineChart::draw_series(PlotContext &ctx) const {
  std::size_t ncat = std::max<std::size_t>(ctx.categories.size(), 1);
  if (ctx.series.empty())
    return;

  float slot = ctx.plot.w / static_cast<float>(ncat);

  Font label_font;
  if (line_opts_.data_labels.enabled)
    label_font.set_size(line_opts_.data_labels.font_size);

  for (auto const &ser : ctx.series) {
    std::vector<std::pair<float, float>> pts;
    pts.reserve(ncat);

    for (std::size_t i = 0; i < ncat; ++i) {
      double val = (i < ser.values.size()) ? ser.values[i] : 0.0;
      float px = ctx.plot.left() + slot * (static_cast<float>(i) + 0.5f);
      float py = ctx.y_scale.to_pixel(val);
      pts.push_back({px, py});
    }

    // Points remain at the raw per-category positions for markers/labels; the spline
    // curve (if enabled) is a separate, denser point list used only for the stroke/fill.
    std::vector<std::pair<float, float>> curve_pts =
        line_opts_.spline ? detail::catmull_rom_expand(pts) : pts;

    if (line_opts_.fill_area && curve_pts.size() >= 2) {
      std::vector<std::pair<float, float>> poly = curve_pts;
      float y0 = ctx.y_scale.to_pixel(ctx.y_scale.min());
      poly.push_back({curve_pts.back().first, y0});
      poly.push_back({curve_pts.front().first, y0});
      Color fill = ser.color;
      fill.a = 60;
      ctx.buffer.fill_polygon(poly.data(), poly.size(), fill);
    }

    if (curve_pts.size() >= 2) {
      ctx.buffer.draw_dashed_polyline(curve_pts.data(), curve_pts.size(), line_opts_.line_width,
                                      ser.color, line_opts_.dash);
    }

    if (line_opts_.show_points) {
      for (auto const &p : pts) {
        detail::draw_marker(ctx.buffer, p.first, p.second, line_opts_.point_radius,
                            line_opts_.point_shape, ser.color);
      }
    }

    if (line_opts_.data_labels.enabled) {
      auto const &dl = line_opts_.data_labels;
      for (std::size_t i = 0; i < pts.size(); ++i) {
        double val = (i < ser.values.size()) ? ser.values[i] : 0.0;
        std::string text = detail::format_label_value(val, dl.format, dl.decimals);
        label_font.draw_text(ctx.buffer, pts[i].first,
                             pts[i].second - line_opts_.point_radius - 4.0f, text, dl.color,
                             TextAlign::Center, TextBaseline::Bottom);
      }
    }
  }
}

void LineChart::draw_overlay(PlotContext &ctx) const {
  detail::draw_reference_lines(ctx);
}

} // namespace insightdock::charts
