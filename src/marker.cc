// SPDX-License-Identifier: MIT
// Copyright (c) InsightDock contributors

#include "insightdock/detail/marker.h"

#include <algorithm>
#include <utility>

namespace insightdock::charts::detail {

void draw_marker(ImageBuffer &buffer, float x, float y, float radius, PointMarkerShape shape,
                 Color const &color) {
  switch (shape) {
  case PointMarkerShape::Circle:
    buffer.fill_circle(x, y, radius, color);
    break;
  case PointMarkerShape::Square: {
    float side = radius * 1.8f; // roughly matches a circle's visual footprint at the same radius
    buffer.fill_rect(x - side * 0.5f, y - side * 0.5f, side, side, color);
    break;
  }
  case PointMarkerShape::Triangle: {
    std::pair<float, float> pts[3] = {
        {x, y - radius},
        {x - radius * 0.866f, y + radius * 0.5f},
        {x + radius * 0.866f, y + radius * 0.5f},
    };
    buffer.fill_polygon(pts, 3, color);
    break;
  }
  case PointMarkerShape::Cross: {
    float stroke = std::max(1.5f, radius * 0.5f);
    buffer.draw_line(x - radius, y, x + radius, y, stroke, color);
    buffer.draw_line(x, y - radius, x, y + radius, stroke, color);
    break;
  }
  }
}

} // namespace insightdock::charts::detail
