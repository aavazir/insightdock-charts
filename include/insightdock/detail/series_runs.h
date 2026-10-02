// SPDX-License-Identifier: MIT
// Copyright (c) InsightDock contributors
#pragma once

#include "insightdock/series_style.h"

#include <cmath>
#include <cstddef>
#include <utility>
#include <vector>

namespace insightdock::charts::detail {

/** @brief A maximal sequence of drawable points (pixel space) plus their data indices. */
struct PointRun {
  std::vector<std::pair<float, float>> pts;
  std::vector<std::size_t> idx; ///< Category/data index of each point in @ref pts.
};

/**
 * @brief Splits a series into drawable runs according to a @ref GapPolicy.
 *
 * @param n        Number of positions (categories or x-values).
 * @param value    `double value(std::size_t i)` — the sample, or NaN if the series is too short.
 * @param x_pixel  `float x_pixel(std::size_t i)` — pixel x of position @p i.
 * @param y_pixel  `float y_pixel(double v)` — maps a value to pixel y.
 *
 * - Zero: every position yields a point (non-finite values become 0); one run.
 * - Gap: a non-finite value ends the current run; later points start a new one.
 * - Interpolate: non-finite values are skipped; neighbours stay connected in one run.
 */
template <class ValueFn, class XFn, class YFn>
std::vector<PointRun> make_runs(std::size_t n, GapPolicy policy, ValueFn value, XFn x_pixel,
                                YFn y_pixel) {
  std::vector<PointRun> runs;
  bool open = false;
  for (std::size_t i = 0; i < n; ++i) {
    double v = value(i);
    if (!std::isfinite(v)) {
      if (policy == GapPolicy::Zero) {
        v = 0.0;
      } else {
        if (policy == GapPolicy::Gap)
          open = false;
        continue;
      }
    }
    if (!open) {
      runs.emplace_back();
      open = true;
    }
    runs.back().pts.push_back({x_pixel(i), y_pixel(v)});
    runs.back().idx.push_back(i);
  }
  return runs;
}

/** @brief Series value at @p i, or NaN when the series is shorter than the category list. */
inline double value_or_nan(std::vector<double> const &values, std::size_t i) {
  return i < values.size() ? values[i] : std::nan("");
}

} // namespace insightdock::charts::detail
