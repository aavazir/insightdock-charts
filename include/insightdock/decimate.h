// SPDX-License-Identifier: MIT
// Copyright (c) InsightDock contributors

/**
 * @file decimate.h
 * @brief Largest-Triangle-Three-Buckets (LTTB) downsampling for large datasets.
 */

#ifndef INSIGHTDOCK_CHARTS_DECIMATE_H
#define INSIGHTDOCK_CHARTS_DECIMATE_H

#include <cstddef>
#include <utility>
#include <vector>

namespace insightdock::charts {

/** @brief A single (x, y) sample for decimation. */
struct DecimationPoint {
  double x = 0.0;
  double y = 0.0;
};

/**
 * @brief Downsample a series to @p threshold points using LTTB.
 *
 * Picks, per bucket, the point that forms the largest triangle with the
 * previously chosen point and the next bucket's average — this preserves
 * visual shape (spikes, valleys) far better than naive stride sampling.
 * Always keeps the first and last point.
 *
 * @param points    Input series, ordered by x.
 * @param threshold Target point count. No-op (returns @p points unchanged)
 *                  when @p threshold is 0, < 3, or >= `points.size()`.
 * @return Decimated series.
 */
std::vector<DecimationPoint> lttb_decimate(std::vector<DecimationPoint> const &points,
                                           std::size_t threshold);

/**
 * @brief Convenience overload for category-aligned data (x = index).
 *
 * Use before passing large datasets to @ref LineChart or @ref AreaChart, whose
 * series values are one-per-category. Returns the decimated values together
 * with the original indices they came from, so the category label list can be
 * sliced to match.
 *
 * @param values    Input series values (x is the implicit index 0..N-1).
 * @param threshold Target point count; see @ref lttb_decimate for no-op rules.
 * @return Pair of (decimated values, original indices), same length.
 */
std::pair<std::vector<double>, std::vector<std::size_t>>
lttb_decimate_indexed(std::vector<double> const &values, std::size_t threshold);

} // namespace insightdock::charts

#endif // INSIGHTDOCK_CHARTS_DECIMATE_H
