// SPDX-License-Identifier: MIT
// Copyright (c) InsightDock contributors

/**
 * @file histogram.h
 * @brief Binning helper and a ready-made histogram built on BarChart.
 */

#ifndef INSIGHTDOCK_CHARTS_HISTOGRAM_H
#define INSIGHTDOCK_CHARTS_HISTOGRAM_H

#include "insightdock/bar_chart.h"
#include "insightdock/export_macros.h"

#include <cstddef>
#include <string>
#include <vector>

namespace insightdock::charts {

/** @brief How @ref compute_histogram bins its input. */
struct HistogramOptions {
  /** Number of equal-width bins; 0 picks one with Sturges' rule (1 + log2 n), capped at 100. */
  std::size_t bins = 0;
  /** Lower bound of the first bin; used only when `min_value < max_value`, else the data minimum. */
  double min_value = 0.0;
  /** Upper bound of the last bin (see @p min_value). Values outside the range are ignored. */
  double max_value = 0.0;
};

/** @brief Result of @ref compute_histogram: `edges.size() == counts.size() + 1`. */
struct Histogram {
  std::vector<double> edges;       ///< Bin boundaries, ascending.
  std::vector<std::size_t> counts; ///< Number of samples per bin; the last bin includes its upper edge.
};

/**
 * @brief Bins @p values into equal-width bins. NaN/Inf are skipped.
 * @return An empty histogram when there is no finite data.
 */
INSIGHTDOCK_API Histogram compute_histogram(std::vector<double> const &values,
                                            HistogramOptions const &options = {});

/**
 * @brief Builds a bar chart of a histogram: one touching bar per bin, labelled "lo-hi".
 *
 * The result is an ordinary @ref BarChart, so titles, colors, data labels and export work as
 * usual. Pass `series_name` to rename the single "Count" series (shown in the legend).
 */
INSIGHTDOCK_API BarChart make_histogram_chart(std::vector<double> const &values,
                                              HistogramOptions const &options = {},
                                              std::string series_name = "Count");

} // namespace insightdock::charts

#endif // INSIGHTDOCK_CHARTS_HISTOGRAM_H
