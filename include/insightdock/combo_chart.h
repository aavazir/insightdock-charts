// SPDX-License-Identifier: MIT
// Copyright (c) InsightDock contributors

#ifndef INSIGHTDOCK_CHARTS_COMBO_CHART_H
#define INSIGHTDOCK_CHARTS_COMBO_CHART_H

#include "insightdock/bar_chart.h"
#include "insightdock/cartesian_chart.h"
#include <vector>

namespace insightdock::charts {

/** @brief Geometry kind for each series in a ComboChart. */
enum class ComboSeriesKind {
  Bar,  ///< Rendered as a grouped vertical bar.
  Line, ///< Rendered as a line trace overlaid on the bars.
};

/** @brief Drawing options for line series in a ComboChart. */
struct ComboLineOptions {
  bool show_points = true;   ///< Draw circle markers at each data point.
  float line_width = 2.0f;   ///< Stroke width in pixels.
  float point_radius = 3.0f; ///< Point marker radius in pixels.
  /** Cubic (Catmull-Rom) interpolation between points instead of straight segments. */
  bool spline = false;
  /** Alternating on/off stroke lengths in pixels; empty (default) = solid. */
  DashPattern dash;
  PointMarkerShape point_shape = PointMarkerShape::Circle;
  DataLabelOptions data_labels;
};

/**
 * @brief Mixed bar-and-line chart on a shared axis.
 *
 * Bar series are drawn as grouped vertical bars; line series are overlaid as
 * polylines with optional point markers.  Both share the same x-categories
 * and y-axis, and all series appear in the legend.
 *
 * @example
 * @code
 * ComboChart combo;
 * combo.set_categories({"Mon", "Tue", "Wed", "Thu", "Fri"});
 * combo.add_bar_series("Average", {30, 45, 28, 52, 38},
 *                       Color::rgb(75, 192, 77));
 * combo.add_line_series("Peak", {55, 70, 48, 80, 65},
 *                        Color::rgb(75, 135, 192));
 * ChartExporter::save(combo, "sessions.png");
 * @endcode
 */
class ComboChart : public CartesianChart {
public:
  /** @brief Mutable line-series drawing options. */
  ComboLineOptions &line_options() {
    return line_opts_;
  }
  ComboLineOptions const &line_options() const {
    return line_opts_;
  }

  /** @brief Mutable access to the bar series' layout/shape/label options (mode is unused here). */
  BarOptions &bar_options() {
    return bar_opts_;
  }
  BarOptions const &bar_options() const {
    return bar_opts_;
  }

  /**
   * @brief Gap between category groups as a fraction of the slot width.
   * @param gap Value in [0, 0.5]; default 0.2.
   */
  void set_group_gap(float gap) {
    bar_opts_.group_gap = gap;
  }

  /**
   * @brief Add a series drawn as grouped bars (auto palette color).
   * @param secondary_axis When true, the series is scaled against the
   *        right-side y-axis instead of the left one (see @ref
   *        cartesian_options for axis label/format and
   *        CartesianOptions::y2_axis_label).
   */
  void add_bar_series(std::string label, std::vector<double> values, bool secondary_axis = false);
  /** @brief Add a series drawn as grouped bars with explicit color. */
  void add_bar_series(std::string label, std::vector<double> values, Color color,
                      bool secondary_axis = false);

  /** @brief Add a series drawn as a line overlay (auto palette color). */
  void add_line_series(std::string label, std::vector<double> values, bool secondary_axis = false);
  /** @brief Add a series drawn as a line overlay with explicit color. */
  void add_line_series(std::string label, std::vector<double> values, Color color,
                       bool secondary_axis = false);

  /** @brief Clear all series and kind tags. */
  void clear_series();

protected:
  void draw_series(PlotContext &ctx) const override;
  void draw_overlay(PlotContext &ctx) const override;

  bool has_secondary_y_axis() const override;
  double compute_y_min() const override;
  double compute_y_max() const override;
  double compute_y2_min() const override;
  double compute_y2_max() const override;

private:
  Scale const &axis_scale(PlotContext const &ctx, std::size_t series_index) const;

  std::vector<ComboSeriesKind> kinds_; ///< Parallel to series_; tracks bar vs line.
  std::vector<bool> use_y2_;           ///< Parallel to series_; true = right axis.
  ComboLineOptions line_opts_;
  BarOptions bar_opts_;
};

} // namespace insightdock::charts

#endif // INSIGHTDOCK_CHARTS_COMBO_CHART_H
