// SPDX-License-Identifier: MIT
// Copyright (c) InsightDock contributors

/**
 * @file image_buffer.h
 * @brief RGBA raster buffer and low-level drawing primitives.
 *
 * Charts render into @ref ImageBuffer. Use @ref ChartExporter or @ref write_png
 * to encode the buffer to files.
 */

#ifndef INSIGHTDOCK_CHARTS_IMAGE_BUFFER_H
#define INSIGHTDOCK_CHARTS_IMAGE_BUFFER_H

#include "insightdock/color.h"
#include "insightdock/export_macros.h"
#include <cstddef>
#include <cstdint>
#include <utility>
#include <vector>

namespace insightdock::charts {

/**
 * @brief Packed RGBA image (row-major, top-to-bottom).
 *
 * Each pixel is four bytes: R, G, B, A. Origin (0,0) is top-left.
 */
class INSIGHTDOCK_API ImageBuffer {
public:
  /** @brief Empty buffer (0×0). */
  ImageBuffer();
  /**
   * @brief Allocate a cleared transparent buffer.
   * @param width  Pixel width.
   * @param height Pixel height.
   *
   * If `width * height` exceeds @ref kMaxPixels the result is an empty (0×0) buffer rather than
   * a multi-gigabyte allocation; exporters report this as an error.
   */
  ImageBuffer(std::size_t width, std::size_t height);

  /** @brief Largest buffer (in pixels) the library will allocate (~512 MB of RGBA). */
  static constexpr std::size_t kMaxPixels = std::size_t{1} << 27;

  /** @brief True if a `width × height` buffer is within @ref kMaxPixels (overflow-safe). */
  static constexpr bool dimensions_ok(std::size_t width, std::size_t height) {
    return width == 0 || height <= kMaxPixels / width;
  }

  /** @brief Width in pixels. */
  std::size_t width() const {
    return width_;
  }
  /** @brief Height in pixels. */
  std::size_t height() const {
    return height_;
  }

  /**
   * @brief Mutable pointer to pixel at (x, y).
   * @return Pointer to 4 bytes [R,G,B,A], or nullptr if out of bounds.
   */
  uint8_t *pixel(std::size_t x, std::size_t y);
  /** @brief Const pixel access. */
  uint8_t const *pixel(std::size_t x, std::size_t y) const;

  /** @brief Fill entire buffer with a color (alpha blending). */
  void clear(Color const &color);
  /** @brief Fill entire buffer with RGBA components. */
  void clear(uint8_t r, uint8_t g, uint8_t b, uint8_t a = 255);

  /** @brief Raw pixel bytes (size = width × height × 4). */
  std::vector<uint8_t> const &data() const {
    return data_;
  }
  std::vector<uint8_t> &data() {
    return data_;
  }

  /** @brief Fill an axis-aligned rectangle (float coordinates, antialiased edges). */
  void fill_rect(float x, float y, float w, float h, Color const &color);
  /** @brief Fill an integer-coordinate rectangle. */
  void fill_rect(int x, int y, int w, int h, Color const &color);

  /** @brief Fill a convex polygon from vertex list. */
  void fill_polygon(std::pair<float, float> const *points, std::size_t count, Color const &color);
  void fill_polygon(std::pair<float, float> const *points, std::size_t count, uint8_t r, uint8_t g,
                    uint8_t b, uint8_t a = 255);

  /**
   * @brief Filled antialiased line segment with round caps.
   *
   * Pixel centers are at half-integer coordinates; align a horizontal/vertical 1px stroke
   * with these centers for a fully opaque interior.
   * @param width Stroke width in pixels (≥ 1).
   */
  void draw_line(float x0, float y0, float x1, float y1, float width, Color const &color);

  /** @brief Open polyline through @p count points. */
  void draw_polyline(std::pair<float, float> const *points, std::size_t count, float width,
                     Color const &color);

  /** @brief Closed polygon outline. */
  void draw_polygon_outline(std::pair<float, float> const *points, std::size_t count, float width,
                            Color const &color);

  /** @brief Fill an axis-aligned rectangle with rounded corners (antialiased). */
  void fill_rounded_rect(float x, float y, float w, float h, float radius, Color const &color);

  /**
   * @brief Antialiased line segment with a dash pattern.
   * @param dash Alternating on/off lengths in pixels (e.g. {6,4}). Empty means solid,
   *             same as @ref draw_line.
   */
  void draw_dashed_line(float x0, float y0, float x1, float y1, float width, Color const &color,
                        std::vector<float> const &dash);

  /**
   * @brief Open polyline through @p count points with a dash pattern carried continuously
   * across vertices.
   */
  void draw_dashed_polyline(std::pair<float, float> const *points, std::size_t count, float width,
                            Color const &color, std::vector<float> const &dash);

  /** @brief Filled circle with antialiased edge. */
  void fill_circle(float cx, float cy, float radius, Color const &color);

  /**
   * @brief Filled pie slice (sector).
   * @param start_rad Start angle in radians (math convention, y-down adjusted internally).
   * @param end_rad   End angle in radians.
   */
  void fill_sector(float cx, float cy, float radius, double start_rad, double end_rad,
                   Color const &color);

  /**
   * @brief Box-filter downsample for supersampling anti-aliasing.
   * @param src    High-resolution source buffer.
   * @param factor Integer downsample factor (e.g. 2 = half width/height).
   * @return Smaller buffer.
   */
  static ImageBuffer downsample(ImageBuffer const &src, int factor);

private:
  void blend_pixel(std::size_t x, std::size_t y, uint8_t r, uint8_t g, uint8_t b, float alpha);

  std::size_t width_ = 0;
  std::size_t height_ = 0;
  std::vector<uint8_t> data_;
};

} // namespace insightdock::charts

#endif // INSIGHTDOCK_CHARTS_IMAGE_BUFFER_H
