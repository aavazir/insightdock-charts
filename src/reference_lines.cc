// SPDX-License-Identifier: MIT
// Copyright (c) InsightDock contributors

#include "insightdock/detail/reference_lines.h"

#include <algorithm>
#include <utility>

namespace insightdock::charts::detail {

namespace {

void draw_band(CartesianChart::PlotContext const &ctx, ReferenceBand const &band) {
  Rect const &plot = ctx.plot;
  float p0, p1;
  std::pair<float, float> pts[4];

  if (band.orientation == ReferenceOrientation::Horizontal) {
    p0 = ctx.y_scale.to_pixel(band.from);
    p1 = ctx.y_scale.to_pixel(band.to);
    float top = std::min(p0, p1);
    float bottom = std::max(p0, p1);
    pts[0] = {plot.left(), top};
    pts[1] = {plot.right(), top};
    pts[2] = {plot.right(), bottom};
    pts[3] = {plot.left(), bottom};
  } else {
    p0 = ctx.x_scale.to_pixel(band.from);
    p1 = ctx.x_scale.to_pixel(band.to);
    float left = std::min(p0, p1);
    float right = std::max(p0, p1);
    pts[0] = {left, plot.top()};
    pts[1] = {right, plot.top()};
    pts[2] = {right, plot.bottom()};
    pts[3] = {left, plot.bottom()};
  }
  ctx.target.fill_polygon(pts, 4, band.color);

  if (!band.label.empty()) {
    Color text_color = band.color;
    text_color.a = 255;
    ctx.target.draw_text(ctx.font, pts[0].first + 4.0f, pts[0].second + 4.0f, band.label,
                       text_color, TextAlign::Left, TextBaseline::Top);
  }
}

void draw_line(CartesianChart::PlotContext const &ctx, ReferenceLine const &line) {
  Rect const &plot = ctx.plot;

  if (line.orientation == ReferenceOrientation::Horizontal) {
    Scale const &scale = (line.use_secondary_axis && ctx.has_y2) ? ctx.y_scale2 : ctx.y_scale;
    float py = scale.to_pixel(line.value);
    if (py < plot.top() || py > plot.bottom())
      return;
    ctx.target.draw_dashed_line(plot.left(), py, plot.right(), py, line.width, line.color,
                                line.dash);
    if (!line.label.empty()) {
      ctx.target.draw_text(ctx.font, plot.right() - 4.0f, py - 4.0f, line.label, line.color,
                         TextAlign::Right, TextBaseline::Bottom);
    }
  } else {
    float px = ctx.x_scale.to_pixel(line.value);
    if (px < plot.left() || px > plot.right())
      return;
    ctx.target.draw_dashed_line(px, plot.top(), px, plot.bottom(), line.width, line.color,
                                line.dash);
    if (!line.label.empty()) {
      ctx.target.draw_text(ctx.font, px + 4.0f, plot.top() + 4.0f, line.label, line.color,
                         TextAlign::Left, TextBaseline::Top);
    }
  }
}

} // namespace

void draw_reference_lines(CartesianChart::PlotContext const &ctx) {
  for (auto const &band : ctx.cart_opts.reference_bands)
    draw_band(ctx, band);
  for (auto const &line : ctx.cart_opts.reference_lines)
    draw_line(ctx, line);
}

} // namespace insightdock::charts::detail
