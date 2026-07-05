// SPDX-License-Identifier: MIT
// Copyright (c) InsightDock contributors

/**
 * @file geometry.h
 * @brief 2D point and rectangle helpers for layout calculations.
 */

#ifndef INSIGHTDOCK_CHARTS_GEOMETRY_H
#define INSIGHTDOCK_CHARTS_GEOMETRY_H

#include <algorithm>
#include <cmath>

namespace insightdock::charts {

/** @brief 2D point in pixel coordinates. */
struct Point {
  float x = 0.0f;
  float y = 0.0f;

  Point() = default;
  Point(float x_, float y_) : x(x_), y(y_) {}
};

/**
 * @brief Axis-aligned rectangle (x,y = top-left, w,h = size).
 */
struct Rect {
  float x = 0.0f;
  float y = 0.0f;
  float w = 0.0f;
  float h = 0.0f;

  Rect() = default;
  Rect(float x_, float y_, float w_, float h_) : x(x_), y(y_), w(w_), h(h_) {}

  float left() const {
    return x;
  }
  float top() const {
    return y;
  }
  float right() const {
    return x + w;
  }
  float bottom() const {
    return y + h;
  }

  float center_x() const {
    return x + w * 0.5f;
  }
  float center_y() const {
    return y + h * 0.5f;
  }

  /** @brief True if (px, py) lies inside the rectangle. */
  bool contains(float px, float py) const {
    return px >= x && px < x + w && py >= y && py < y + h;
  }

  /** @brief Rectangle inset by @p margin on all sides. */
  Rect inset(float margin) const {
    return Rect(x + margin, y + margin, std::max(0.0f, w - 2.0f * margin),
                std::max(0.0f, h - 2.0f * margin));
  }

  /** @brief Smallest rectangle containing this and @p other. */
  Rect union_with(Rect const &other) const {
    if (w <= 0.0f && h <= 0.0f)
      return other;
    if (other.w <= 0.0f && other.h <= 0.0f)
      return *this;
    float l = std::min(left(), other.left());
    float t = std::min(top(), other.top());
    float r = std::max(right(), other.right());
    float b = std::max(bottom(), other.bottom());
    return Rect(l, t, r - l, b - t);
  }
};

} // namespace insightdock::charts

#endif // INSIGHTDOCK_CHARTS_GEOMETRY_H
