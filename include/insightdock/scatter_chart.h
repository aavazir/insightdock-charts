// SPDX-License-Identifier: MIT
// Copyright (c) InsightDock contributors

/**
 * @file scatter_chart.h
 * @brief Scatter and bubble chart with continuous numeric x/y axes.
 */

#ifndef INSIGHTDOCK_CHARTS_SCATTER_CHART_H
#define INSIGHTDOCK_CHARTS_SCATTER_CHART_H

#include "insightdock/chart.h"
#include "insightdock/chart_style.h"
#include "insightdock/color.h"
#include <string>
#include <vector>

namespace insightdock::charts {

/**
 * @brief One point in a @ref ScatterChart series.
 *
 * @p radius is optional: leave at 0 for a plain scatter marker of uniform
 * size (@ref ScatterOptions::point_radius), or set a positive value to drive
 * marker size for the bubble variant.
 */
struct ScatterPoint {
  double x = 0.0;
  double y = 0.0;
  double radius = 0.0; ///< > 0 enables bubble sizing for this point.
};

/** @brief One data series in a @ref ScatterChart. */
struct ScatterSeries {
  std::string label; ///< Series name (legend).
  std::vector<ScatterPoint> points;
  Color color; ///< Marker fill color.
};

/** @brief Scatter/bubble layout and styling. */
struct ScatterOptions {
  std::string x_axis_label;     ///< Label below the x-axis (optional).
  std::string y_axis_label;     ///< Label beside the y-axis (optional).
  bool begin_at_zero_x = false; ///< Force x-axis minimum to 0.
  bool begin_at_zero_y = false; ///< Force y-axis minimum to 0.

  float point_radius = 5.0f;       ///< Marker radius (pixels) when a point's radius is 0.
  float min_bubble_radius = 4.0f;  ///< Smallest rendered bubble radius (pixels).
  float max_bubble_radius = 40.0f; ///< Largest rendered bubble radius (pixels).
  float fill_opacity = 0.65f;      ///< Marker fill alpha (0-1).
  /** Marker shape for every point (no per-point override). */
  PointMarkerShape point_shape = PointMarkerShape::Circle;
  /** Labels the formatted Y value near each point; no per-point custom label in v1. */
  DataLabelOptions data_labels;
};

/**
 * @brief Scatter chart with continuous numeric x and y axes.
 *
 * Unlike @ref BarChart / @ref LineChart, the x-axis here is not a category
 * list — both axes use linear @ref Scale mapping over the data domain, so
 * `x` values need not be evenly spaced or sorted. This is why ScatterChart
 * derives directly from @ref Chart rather than @ref CartesianChart.
 *
 * Set a positive @ref ScatterPoint::radius on any point to enable the bubble
 * variant: that point's marker is scaled between
 * @ref ScatterOptions::min_bubble_radius and @ref ScatterOptions::max_bubble_radius
 * based on its radius value relative to other bubble points. Points left at
 * radius 0 always draw at the fixed @ref ScatterOptions::point_radius.
 *
 * @example
 * @code
 * ScatterChart chart;
 * chart.add_series("Devices", {{1, 20}, {2, 35}, {3, 18}});
 * ChartExporter::save(chart, "scatter.png");
 * @endcode
 */
class ScatterChart : public Chart {
public:
  std::unique_ptr<Chart> clone() const override {
    return std::make_unique<ScatterChart>(*this);
  }

  /** @brief Mutable scatter options. */
  ScatterOptions &scatter_options() {
    return opts_;
  }
  ScatterOptions const &scatter_options() const {
    return opts_;
  }

  /**
   * @brief Add a series with automatic palette color.
   * @param label  Series name (legend).
   * @param points Data points (set @ref ScatterPoint::radius for bubbles).
   */
  void add_series(std::string label, std::vector<ScatterPoint> points);

  /**
   * @brief Add a series with an explicit color.
   * @param label  Series name.
   * @param points Data points.
   * @param color  Marker color.
   */
  void add_series(std::string label, std::vector<ScatterPoint> points, Color color);

  /** @brief All data series. */
  std::vector<ScatterSeries> const &series() const {
    return series_;
  }
  /** @brief Remove all series. */
  void clear_series();

  /** @brief Render axes, grid, legend, and markers to RGBA. */
  ImageBuffer render() const override;

private:
  std::vector<ScatterSeries> series_;
  ScatterOptions opts_;
};

} // namespace insightdock::charts

#endif // INSIGHTDOCK_CHARTS_SCATTER_CHART_H
