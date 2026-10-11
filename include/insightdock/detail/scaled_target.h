// SPDX-License-Identifier: MIT
// Copyright (c) InsightDock contributors

#ifndef INSIGHTDOCK_CHARTS_DETAIL_SCALED_TARGET_H
#define INSIGHTDOCK_CHARTS_DETAIL_SCALED_TARGET_H

#include "insightdock/draw_target.h"
#include <memory>

namespace insightdock::charts::detail {

// Keep layout in logical chart pixels; scale every primitive only at the output boundary.
// The exporter supplies an integer scale in [2, 8] and a correspondingly sized raster surface.
class ScaledTarget final : public DrawTarget {
public:
  ScaledTarget(DrawTarget &target, float scale) : target_(target), scale_(scale) {}
  float width() const override {
    return target_.width() / scale_;
  }
  float height() const override {
    return target_.height() / scale_;
  }
  void clear(Color const &color) override {
    target_.clear(color);
  }
  void fill_rect(float x, float y, float w, float h, Color const &color) override {
    target_.fill_rect(x * scale_, y * scale_, w * scale_, h * scale_, color);
  }
  void fill_rounded_rect(float x, float y, float w, float h, float radius,
                         Color const &color) override {
    target_.fill_rounded_rect(x * scale_, y * scale_, w * scale_, h * scale_, radius * scale_,
                              color);
  }
  void fill_polygon(Point const *points, std::size_t count, Color const &color) override {
    auto p = scaled_points(points, count);
    target_.fill_polygon(p.data(), p.size(), color);
  }
  void fill_circle(float cx, float cy, float radius, Color const &color) override {
    target_.fill_circle(cx * scale_, cy * scale_, radius * scale_, color);
  }
  void fill_sector(float cx, float cy, float radius, double start, double end,
                   Color const &color) override {
    target_.fill_sector(cx * scale_, cy * scale_, radius * scale_, start, end, color);
  }
  void draw_line(float x0, float y0, float x1, float y1, float width, Color const &color) override {
    target_.draw_line(x0 * scale_, y0 * scale_, x1 * scale_, y1 * scale_, width * scale_, color);
  }
  void draw_polyline(Point const *points, std::size_t count, float width,
                     Color const &color) override {
    auto p = scaled_points(points, count);
    target_.draw_polyline(p.data(), p.size(), width * scale_, color);
  }
  void draw_polygon_outline(Point const *points, std::size_t count, float width,
                            Color const &color) override {
    auto p = scaled_points(points, count);
    target_.draw_polygon_outline(p.data(), p.size(), width * scale_, color);
  }
  void draw_dashed_line(float x0, float y0, float x1, float y1, float width, Color const &color,
                        std::vector<float> const &dash) override {
    target_.draw_dashed_line(x0 * scale_, y0 * scale_, x1 * scale_, y1 * scale_, width * scale_,
                             color, scaled_dash(dash));
  }
  void draw_dashed_polyline(Point const *points, std::size_t count, float width, Color const &color,
                            std::vector<float> const &dash) override {
    auto p = scaled_points(points, count);
    target_.draw_dashed_polyline(p.data(), p.size(), width * scale_, color, scaled_dash(dash));
  }
  void draw_text(Font const &font, float x, float y, std::string const &text, Color const &color,
                 TextAlign align, TextBaseline baseline) override {
    target_.draw_text(scaled_font(font), x * scale_, y * scale_, text, color, align, baseline);
  }
  void draw_text_rotated_ccw(Font const &font, float cx, float cy, std::string const &text,
                             Color const &color) override {
    target_.draw_text_rotated_ccw(scaled_font(font), cx * scale_, cy * scale_, text, color);
  }
  void draw_text_rotated_cw(Font const &font, float cx, float cy, std::string const &text,
                            Color const &color) override {
    target_.draw_text_rotated_cw(scaled_font(font), cx * scale_, cy * scale_, text, color);
  }

private:
  std::vector<Point> scaled_points(Point const *points, std::size_t count) const {
    std::vector<Point> result;
    result.reserve(count);
    for (std::size_t i = 0; i < count; ++i)
      result.emplace_back(points[i].first * scale_, points[i].second * scale_);
    return result;
  }
  std::vector<float> scaled_dash(std::vector<float> const &dash) const {
    auto result = dash;
    for (auto &length : result)
      length *= scale_;
    return result;
  }
  Font const &scaled_font(Font const &font) {
    if (!font_ || font_->data() != font.data())
      font_ = std::make_unique<Font>(font.data());
    font_->set_size(font.size() * scale_);
    return *font_;
  }
  DrawTarget &target_;
  float scale_;
  std::unique_ptr<Font> font_;
};

} // namespace insightdock::charts::detail
#endif // INSIGHTDOCK_CHARTS_DETAIL_SCALED_TARGET_H
