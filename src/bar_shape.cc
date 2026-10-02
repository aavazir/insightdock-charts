// SPDX-License-Identifier: MIT
// Copyright (c) InsightDock contributors

#include "insightdock/detail/bar_shape.h"

#include <utility>

namespace insightdock::charts::detail {

void fill_bar(DrawTarget &buffer, float x, float y, float w, float h, Color const &fill_color,
              float corner_radius, Color const &border_color, float border_width) {
  if (corner_radius > 0.0f)
    buffer.fill_rounded_rect(x, y, w, h, corner_radius, fill_color);
  else
    buffer.fill_rect(x, y, w, h, fill_color);

  if (border_width > 0.0f && border_color.a > 0) {
    std::pair<float, float> pts[4] = {
        {x, y},
        {x + w, y},
        {x + w, y + h},
        {x, y + h},
    };
    buffer.draw_polygon_outline(pts, 4, border_width, border_color);
  }
}

} // namespace insightdock::charts::detail
