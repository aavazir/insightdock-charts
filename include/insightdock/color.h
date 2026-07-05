// SPDX-License-Identifier: MIT
// Copyright (c) InsightDock contributors

/**
 * @file color.h
 * @brief RGBA color type and common color constants.
 */

#ifndef INSIGHTDOCK_CHARTS_COLOR_H
#define INSIGHTDOCK_CHARTS_COLOR_H

#include <cstdint>

namespace insightdock::charts {

/**
 * @brief RGBA color with 8-bit per channel.
 *
 * Used throughout rendering for fills, strokes, and text.
 */
struct Color {
  uint8_t r = 0;   ///< Red component (0–255).
  uint8_t g = 0;   ///< Green component (0–255).
  uint8_t b = 0;   ///< Blue component (0–255).
  uint8_t a = 255; ///< Alpha (0 = transparent, 255 = opaque).

  Color() = default;
  Color(uint8_t r_, uint8_t g_, uint8_t b_, uint8_t a_ = 255) : r(r_), g(g_), b(b_), a(a_) {}

  /** @brief Opaque RGB color (alpha = 255). */
  static Color rgb(uint8_t r, uint8_t g, uint8_t b) {
    return Color(r, g, b, 255);
  }

  /** @brief RGBA color with explicit alpha. */
  static Color rgba(uint8_t r, uint8_t g, uint8_t b, uint8_t a) {
    return Color(r, g, b, a);
  }

  static Color transparent() {
    return Color(0, 0, 0, 0);
  }
  static Color white() {
    return Color(255, 255, 255, 255);
  }
  static Color black() {
    return Color(0, 0, 0, 255);
  }

  /** @brief Gray shade (r = g = b). */
  static Color gray(uint8_t v) {
    return Color(v, v, v, 255);
  }
};

} // namespace insightdock::charts

#endif // INSIGHTDOCK_CHARTS_COLOR_H
