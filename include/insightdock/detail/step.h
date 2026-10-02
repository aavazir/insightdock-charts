// SPDX-License-Identifier: MIT
// Copyright (c) InsightDock contributors
#pragma once

#include "insightdock/series_style.h"

#include <utility>
#include <vector>

namespace insightdock::charts::detail {

/** @brief Expands a polyline into a staircase according to @p mode (pixel space). */
inline std::vector<std::pair<float, float>> expand_steps(std::vector<std::pair<float, float>> const &pts,
                                                         StepMode mode) {
  if (mode == StepMode::None || pts.size() < 2)
    return pts;
  std::vector<std::pair<float, float>> out;
  out.reserve(pts.size() * 3);
  out.push_back(pts.front());
  for (std::size_t i = 1; i < pts.size(); ++i) {
    auto const &a = pts[i - 1];
    auto const &b = pts[i];
    switch (mode) {
    case StepMode::After:
      out.push_back({b.first, a.second});
      break;
    case StepMode::Before:
      out.push_back({a.first, b.second});
      break;
    case StepMode::Middle: {
      float mx = (a.first + b.first) * 0.5f;
      out.push_back({mx, a.second});
      out.push_back({mx, b.second});
      break;
    }
    case StepMode::None:
      break;
    }
    out.push_back(b);
  }
  return out;
}

} // namespace insightdock::charts::detail
