// SPDX-License-Identifier: MIT
// Copyright (c) InsightDock contributors

#include "insightdock/detail/reference_lines.h"

#include <algorithm>
#include <utility>

namespace insightdock::charts::detail {

namespace {

// Whether the reference geometry runs across the plot's width (a horizontal line/band). In a
// horizontal bar chart the value axis is the horizontal one, so a ReferenceOrientation::Horizontal
// ("constant value") annotation becomes a vertical line and ReferenceOrientation::Vertical (a
// category position) becomes a horizontal one.
bool spans_width(CartesianChart::PlotContext const &ctx, ReferenceOrientation o) {
  return (o == ReferenceOrientation::Horizontal) != ctx.horizontal;
}

// Pixel position of an annotation coordinate along its axis (y for width-spanning, else x).
float coord_pixel(CartesianChart::PlotContext const &ctx, ReferenceOrientation o, double v,
                  bool secondary) {
  if (o == ReferenceOrientation::Horizontal) {
    Scale const &scale = (secondary && ctx.has_y2) ? ctx.y_scale2 : ctx.y_scale;
    return scale.to_pixel(v);
  }
  if (ctx.horizontal) {
    // Category index → vertical slot position (categories run down the plot).
    double n = static_cast<double>(std::max<std::size_t>(ctx.categories.size(), 1));
    return ctx.plot.top() + static_cast<float>(v / n) * ctx.plot.h;
  }
  return ctx.x_scale.to_pixel(v);
}

void draw_band(CartesianChart::PlotContext const &ctx, ReferenceBand const &band) {
  Rect const &plot = ctx.plot;
  std::pair<float, float> pts[4];
  float p0 = coord_pixel(ctx, band.orientation, band.from, false);
  float p1 = coord_pixel(ctx, band.orientation, band.to, false);
  float lo = std::min(p0, p1);
  float hi = std::max(p0, p1);

  if (spans_width(ctx, band.orientation)) {
    pts[0] = {plot.left(), lo};
    pts[1] = {plot.right(), lo};
    pts[2] = {plot.right(), hi};
    pts[3] = {plot.left(), hi};
  } else {
    pts[0] = {lo, plot.top()};
    pts[1] = {hi, plot.top()};
    pts[2] = {hi, plot.bottom()};
    pts[3] = {lo, plot.bottom()};
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
  float p = coord_pixel(ctx, line.orientation, line.value, line.use_secondary_axis);

  if (spans_width(ctx, line.orientation)) {
    if (p < plot.top() || p > plot.bottom())
      return;
    ctx.target.draw_dashed_line(plot.left(), p, plot.right(), p, line.width, line.color,
                                line.dash);
    if (!line.label.empty()) {
      ctx.target.draw_text(ctx.font, plot.right() - 4.0f, p - 4.0f, line.label, line.color,
                           TextAlign::Right, TextBaseline::Bottom);
    }
  } else {
    if (p < plot.left() || p > plot.right())
      return;
    ctx.target.draw_dashed_line(p, plot.top(), p, plot.bottom(), line.width, line.color,
                                line.dash);
    if (!line.label.empty()) {
      ctx.target.draw_text(ctx.font, p + 4.0f, plot.top() + 4.0f, line.label, line.color,
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
