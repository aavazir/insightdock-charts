// SPDX-License-Identifier: MIT
// Copyright (c) InsightDock contributors

#include "insightdock/draw_target.h"

namespace insightdock::charts {

void RasterTarget::clear(Color const &color) {
  buf_.clear(color);
}
void RasterTarget::fill_rect(float x, float y, float w, float h, Color const &color) {
  buf_.fill_rect(x, y, w, h, color);
}
void RasterTarget::fill_rounded_rect(float x, float y, float w, float h, float radius,
                                     Color const &color) {
  buf_.fill_rounded_rect(x, y, w, h, radius, color);
}
void RasterTarget::fill_polygon(Point const *points, std::size_t count, Color const &color) {
  buf_.fill_polygon(points, count, color);
}
void RasterTarget::fill_circle(float cx, float cy, float radius, Color const &color) {
  buf_.fill_circle(cx, cy, radius, color);
}
void RasterTarget::draw_line(float x0, float y0, float x1, float y1, float width,
                             Color const &color) {
  buf_.draw_line(x0, y0, x1, y1, width, color);
}
void RasterTarget::draw_polyline(Point const *points, std::size_t count, float width,
                                 Color const &color) {
  buf_.draw_polyline(points, count, width, color);
}
void RasterTarget::draw_polygon_outline(Point const *points, std::size_t count, float width,
                                        Color const &color) {
  buf_.draw_polygon_outline(points, count, width, color);
}
void RasterTarget::draw_dashed_line(float x0, float y0, float x1, float y1, float width,
                                    Color const &color, std::vector<float> const &dash) {
  buf_.draw_dashed_line(x0, y0, x1, y1, width, color, dash);
}
void RasterTarget::draw_dashed_polyline(Point const *points, std::size_t count, float width,
                                        Color const &color, std::vector<float> const &dash) {
  buf_.draw_dashed_polyline(points, count, width, color, dash);
}
void RasterTarget::draw_text(Font const &font, float x, float y, std::string const &text,
                             Color const &color, TextAlign align, TextBaseline baseline) {
  font.draw_text(buf_, x, y, text, color, align, baseline);
}
void RasterTarget::draw_text_rotated_ccw(Font const &font, float cx, float cy,
                                         std::string const &text, Color const &color) {
  font.draw_text_rotated_ccw(buf_, cx, cy, text, color);
}
void RasterTarget::draw_text_rotated_cw(Font const &font, float cx, float cy,
                                        std::string const &text, Color const &color) {
  font.draw_text_rotated_cw(buf_, cx, cy, text, color);
}

} // namespace insightdock::charts
