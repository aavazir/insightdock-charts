// SPDX-License-Identifier: MIT
// Copyright (c) InsightDock contributors

/**
 * @file line_chart.h
 * @brief Multi-series line chart with optional area fill and point markers.
 */

#ifndef INSIGHTDOCK_CHARTS_LINE_CHART_H
#define INSIGHTDOCK_CHARTS_LINE_CHART_H

#include "insightdock/cartesian_chart.h"
#include "insightdock/chart_style.h"
#include "insightdock/export_macros.h"

namespace insightdock::charts {

/** @brief Line-specific drawing options. */
struct LineChartOptions {
  bool show_points = true;   ///< Draw circular markers at each data point.
  bool fill_area = false;    ///< Fill area under the line (first series only).
  float line_width = 2.0f;   ///< Stroke width in pixels.
  float point_radius = 4.0f; ///< Point marker radius in pixels.
  /** Cubic (Catmull-Rom) interpolation between points instead of straight segments. */
  bool spline = false;
  /** Staircase instead of straight segments (takes precedence over @p spline). */
  StepMode step = StepMode::None;
  /** Alternating on/off stroke lengths in pixels; empty (default) = solid. */
  DashPattern dash;
  /** Marker shape drawn at each point when show_points is true. */
  PointMarkerShape point_shape = PointMarkerShape::Circle;
  /** How NaN/Inf/missing values are drawn (default: break the line). */
  GapPolicy gap_policy = GapPolicy::Gap;
  DataLabelOptions data_labels;
};

/**
 * @brief Multi-series line chart.
 *
 * @example
 * @code
 * LineChart line;
 * line.set_categories({"Jan", "Feb", "Mar"});
 * line.add_series("Sales", {10, 20, 15});
 * line.line_options().fill_area = true;
 * ChartExporter::save(line, "line.png");
 * @endcode
 */
class INSIGHTDOCK_API LineChart : public CartesianChart {
public:
  std::unique_ptr<Chart> clone() const override {
    return std::make_unique<LineChart>(*this);
  }

  /** @brief Mutable line drawing options. */
  LineChartOptions &line_options() {
    return line_opts_;
  }
  LineChartOptions const &line_options() const {
    return line_opts_;
  }

protected:
  void draw_series(PlotContext &ctx) const override;
  void draw_overlay(PlotContext &ctx) const override;
  bool supports_continuous_x_axis() const override {
    return true;
  }

private:
  LineChartOptions line_opts_;
};

} // namespace insightdock::charts

#endif // INSIGHTDOCK_CHARTS_LINE_CHART_H
