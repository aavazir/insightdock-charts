// SPDX-License-Identifier: MIT
// Copyright (c) InsightDock contributors

#ifndef INSIGHTDOCK_CHARTS_DRAW_TARGET_H
#define INSIGHTDOCK_CHARTS_DRAW_TARGET_H

#include "insightdock/color.h"
#include "insightdock/export_macros.h"
#include "insightdock/font.h"
#include "insightdock/image_buffer.h"

#include <cstddef>
#include <string>
#include <utility>
#include <vector>

namespace insightdock::charts {

/**
 * @brief Pixel-space drawing surface shared by all built-in charts.
 *
 * Coordinates are in chart pixels with the origin at the top-left and y growing downwards.
 * The same chart code drives every output: @ref RasterTarget paints an @ref ImageBuffer,
 * while the SVG and PDF targets record vector primitives. Text is always *measured* with the
 * embedded @ref Font so layout is identical across targets; vector targets emit real text
 * objects instead of glyph bitmaps.
 */
class INSIGHTDOCK_API DrawTarget {
public:
  using Point = std::pair<float, float>;

  virtual ~DrawTarget() = default;

  /** @brief Surface width in chart pixels. */
  virtual float width() const = 0;
  /** @brief Surface height in chart pixels. */
  virtual float height() const = 0;

  /** @brief Fill the whole surface with @p color. */
  virtual void clear(Color const &color) = 0;

  virtual void fill_rect(float x, float y, float w, float h, Color const &color) = 0;
  virtual void fill_rounded_rect(float x, float y, float w, float h, float radius,
                                 Color const &color) = 0;
  /** @brief Fill a polygon (area charts rely on concave polygons being filled correctly). */
  virtual void fill_polygon(Point const *points, std::size_t count, Color const &color) = 0;
  virtual void fill_circle(float cx, float cy, float radius, Color const &color) = 0;
  /**
   * @brief Fill a clockwise circular sector; angles are radians from the positive x-axis.
   *
   * The default implementation tessellates into a polygon, so existing custom targets need
   * no new override. Raster and SVG targets use their native sector primitives.
   */
  virtual void fill_sector(float cx, float cy, float radius, double start, double end,
                           Color const &color);

  virtual void draw_line(float x0, float y0, float x1, float y1, float width,
                         Color const &color) = 0;
  virtual void draw_polyline(Point const *points, std::size_t count, float width,
                             Color const &color) = 0;
  virtual void draw_polygon_outline(Point const *points, std::size_t count, float width,
                                    Color const &color) = 0;
  /** @brief Dashed segment; an empty @p dash draws a solid line. */
  virtual void draw_dashed_line(float x0, float y0, float x1, float y1, float width,
                                Color const &color, std::vector<float> const &dash) = 0;
  /** @brief Dashed polyline with the dash phase carried across vertices. */
  virtual void draw_dashed_polyline(Point const *points, std::size_t count, float width,
                                    Color const &color, std::vector<float> const &dash) = 0;

  /** @brief Draw UTF-8 text using @p font's size, anchored per @p align / @p baseline. */
  virtual void draw_text(Font const &font, float x, float y, std::string const &text,
                         Color const &color, TextAlign align = TextAlign::Left,
                         TextBaseline baseline = TextBaseline::Alphabetic) = 0;
  /** @brief Text rotated 90° counter-clockwise (reads bottom-to-top), centered on (cx, cy). */
  virtual void draw_text_rotated_ccw(Font const &font, float cx, float cy, std::string const &text,
                                     Color const &color) = 0;
  /** @brief Text rotated 90° clockwise (reads top-to-bottom), centered on (cx, cy). */
  virtual void draw_text_rotated_cw(Font const &font, float cx, float cy, std::string const &text,
                                    Color const &color) = 0;
};

/** @brief @ref DrawTarget that paints into an @ref ImageBuffer (the original raster path). */
class INSIGHTDOCK_API RasterTarget final : public DrawTarget {
public:
  explicit RasterTarget(ImageBuffer &buffer) : buf_(buffer) {}

  float width() const override {
    return static_cast<float>(buf_.width());
  }
  float height() const override {
    return static_cast<float>(buf_.height());
  }
  void clear(Color const &color) override;
  void fill_rect(float x, float y, float w, float h, Color const &color) override;
  void fill_rounded_rect(float x, float y, float w, float h, float radius,
                         Color const &color) override;
  void fill_polygon(Point const *points, std::size_t count, Color const &color) override;
  void fill_circle(float cx, float cy, float radius, Color const &color) override;
  void fill_sector(float cx, float cy, float radius, double start, double end,
                   Color const &color) override;
  void draw_line(float x0, float y0, float x1, float y1, float width, Color const &color) override;
  void draw_polyline(Point const *points, std::size_t count, float width,
                     Color const &color) override;
  void draw_polygon_outline(Point const *points, std::size_t count, float width,
                            Color const &color) override;
  void draw_dashed_line(float x0, float y0, float x1, float y1, float width, Color const &color,
                        std::vector<float> const &dash) override;
  void draw_dashed_polyline(Point const *points, std::size_t count, float width, Color const &color,
                            std::vector<float> const &dash) override;
  void draw_text(Font const &font, float x, float y, std::string const &text, Color const &color,
                 TextAlign align, TextBaseline baseline) override;
  void draw_text_rotated_ccw(Font const &font, float cx, float cy, std::string const &text,
                             Color const &color) override;
  void draw_text_rotated_cw(Font const &font, float cx, float cy, std::string const &text,
                            Color const &color) override;

private:
  ImageBuffer &buf_;
};

} // namespace insightdock::charts

#endif // INSIGHTDOCK_CHARTS_DRAW_TARGET_H
