// SPDX-License-Identifier: MIT
// Copyright (c) InsightDock contributors

#include "insightdock/detail/spline.h"

namespace insightdock::charts::detail {

std::vector<std::pair<float, float>>
catmull_rom_expand(std::vector<std::pair<float, float>> const &points, int segments_per_span) {
  std::size_t n = points.size();
  if (n < 3 || segments_per_span < 1)
    return points;

  auto get = [&](long i) -> std::pair<float, float> {
    if (i < 0)
      return points.front();
    if (static_cast<std::size_t>(i) >= n)
      return points.back();
    return points[static_cast<std::size_t>(i)];
  };

  std::vector<std::pair<float, float>> out;
  out.reserve(n * static_cast<std::size_t>(segments_per_span) + 1);
  out.push_back(points.front());

  for (std::size_t i = 0; i + 1 < n; ++i) {
    auto p0 = get(static_cast<long>(i) - 1);
    auto p1 = get(static_cast<long>(i));
    auto p2 = get(static_cast<long>(i) + 1);
    auto p3 = get(static_cast<long>(i) + 2);

    for (int s = 1; s <= segments_per_span; ++s) {
      float t = static_cast<float>(s) / static_cast<float>(segments_per_span);
      float t2 = t * t;
      float t3 = t2 * t;
      float x = 0.5f * ((2.0f * p1.first) + (-p0.first + p2.first) * t +
                        (2.0f * p0.first - 5.0f * p1.first + 4.0f * p2.first - p3.first) * t2 +
                        (-p0.first + 3.0f * p1.first - 3.0f * p2.first + p3.first) * t3);
      float y = 0.5f * ((2.0f * p1.second) + (-p0.second + p2.second) * t +
                        (2.0f * p0.second - 5.0f * p1.second + 4.0f * p2.second - p3.second) * t2 +
                        (-p0.second + 3.0f * p1.second - 3.0f * p2.second + p3.second) * t3);
      out.push_back({x, y});
    }
  }
  return out;
}

} // namespace insightdock::charts::detail
