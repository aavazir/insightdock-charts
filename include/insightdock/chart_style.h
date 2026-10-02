// SPDX-License-Identifier: MIT
// Copyright (c) InsightDock contributors

/**
 * @file chart_style.h
 * @brief Shared style types used by more than one chart type's Options struct:
 * point marker shapes, dash patterns, and data-label configuration.
 */

#ifndef INSIGHTDOCK_CHARTS_CHART_STYLE_H
#define INSIGHTDOCK_CHARTS_CHART_STYLE_H

#include "insightdock/cartesian_chart.h"
#include "insightdock/color.h"
#include "insightdock/series_style.h" // PointMarkerShape, DashPattern, GapPolicy

#include <vector>

namespace insightdock::charts {

/** @brief Where a data label is anchored relative to its mark. */
enum class DataLabelPosition { Auto, Above, Below, Center, Inside, Outside };

/** @brief Per-chart configuration for drawing the value of each mark directly on the chart. */
struct DataLabelOptions {
  bool enabled = false;
  Color color = Color::gray(40);
  float font_size = 10.0f;
  DataLabelPosition position = DataLabelPosition::Auto;
  /** Fixed decimal places, or -1 for auto (integer if whole, else 1 decimal place). */
  int decimals = -1;
  /** Reuses the same TickFormat semantics as CartesianChart's y-axis; ignored by PieChart. */
  TickFormat format = TickFormat::Raw;
};

} // namespace insightdock::charts

#endif // INSIGHTDOCK_CHARTS_CHART_STYLE_H
