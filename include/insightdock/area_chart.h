// SPDX-License-Identifier: MIT
// Copyright (c) InsightDock contributors

/**
 * @file area_chart.h
 * @brief Area and stacked-area charts built on @ref CartesianChart.
 */

#ifndef INSIGHTDOCK_CHARTS_AREA_CHART_H
#define INSIGHTDOCK_CHARTS_AREA_CHART_H

#include "insightdock/cartesian_chart.h"
#include "insightdock/chart_style.h"

namespace insightdock::charts {

/** @brief Area-specific drawing options. */
struct AreaChartOptions {
  bool stacked = false;    ///< Stack series polygons instead of overlapping.
  bool show_line = true;   ///< Draw stroke on top of each area.
  float line_width = 2.0f; ///< Stroke width when @p show_line is true.
  /** Cubic (Catmull-Rom) interpolation between points instead of straight segments. */
  bool spline = false;
  /** Alternating on/off stroke lengths in pixels for the top edge; empty = solid. */
  DashPattern dash;
  /** Point markers at each data point along the top edge; off by default (Chart.js-style). */
  bool show_points = false;
  float point_radius = 4.0f;
  PointMarkerShape point_shape = PointMarkerShape::Circle;
  DataLabelOptions data_labels;
};

/**
 * @brief Area chart — filled polygons from series values down to the baseline.
 *
 * When @ref AreaChartOptions::stacked is true, each series stacks on the previous.
 */
class AreaChart : public CartesianChart {
public:
  std::unique_ptr<Chart> clone() const override {
    return std::make_unique<AreaChart>(*this);
  }

  AreaChartOptions &area_options() {
    return area_opts_;
  }
  AreaChartOptions const &area_options() const {
    return area_opts_;
  }

protected:
  void draw_series(PlotContext &ctx) const override;
  void draw_overlay(PlotContext &ctx) const override;
  bool supports_continuous_x_axis() const override {
    return true;
  }

private:
  AreaChartOptions area_opts_;
};

} // namespace insightdock::charts

#endif // INSIGHTDOCK_CHARTS_AREA_CHART_H
