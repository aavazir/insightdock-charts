// SPDX-License-Identifier: MIT
// Copyright (c) InsightDock contributors

/**
 * @file chart.h
 * @brief Base chart types, options, and the abstract @ref Chart interface.
 */

#ifndef INSIGHTDOCK_CHARTS_CHART_H
#define INSIGHTDOCK_CHARTS_CHART_H

#include "insightdock/color.h"
#include "insightdock/font.h"
#include "insightdock/image_buffer.h"
#include "insightdock/theme.h"
#include <cstddef>
#include <string>
#include <vector>

namespace insightdock::charts {

/** @brief Padding around the chart plot area, in pixels. */
struct Margins {
  float top = 48.0f;    ///< Top margin (title, legend).
  float right = 24.0f;  ///< Right margin.
  float bottom = 56.0f; ///< Bottom margin (x-axis labels, legend).
  float left = 64.0f;   ///< Left margin (y-axis labels).
};

/** @brief Legend placement and visibility. */
struct LegendOptions {
  bool enabled = true; ///< When false, legend is not drawn.
  /** @brief Where to draw the legend relative to the plot. */
  enum class Position { Top, Bottom, Right } position = Position::Bottom;
};

/**
 * @brief Common styling and layout for all chart types.
 *
 * Sensible defaults produce a usable chart without manual tuning.
 * Override width/height, title, colors, and margins as needed.
 */
struct ChartOptions {
  std::size_t width = 640;                            ///< Output image width in pixels.
  std::size_t height = 400;                           ///< Output image height in pixels.
  std::string title;                                  ///< Chart title (empty = no title).
  Color background = Color::white();                  ///< Canvas background color.
  Margins margins;                                    ///< Plot area insets.
  LegendOptions legend;                               ///< Legend visibility and position.
  Color title_color = Color::black();                 ///< Title text color.
  Color axis_color = Color::gray(80);                 ///< Axis line color.
  Color grid_color = Color::rgba(200, 200, 200, 180); ///< Grid line color.
  Color text_color = Color::gray(60);                 ///< General label color.
  float font_size = 12.0f;                            ///< Base font size in pixels.
  bool show_grid = true; ///< Draw horizontal/vertical grid lines (Cartesian charts).
};

/**
 * @brief Named data series for Cartesian charts (bar, line).
 *
 * When @p auto_color is true, @ref palette_color assigns a color by series index.
 */
struct DataSeries {
  std::string label;          ///< Series name (legend, tooltips).
  std::vector<double> values; ///< One value per category.
  Color color;                ///< Series color (ignored if auto_color).
  bool auto_color = false;    ///< Pick color from default palette.
};

/**
 * @brief Abstract base for all chart types.
 *
 * Subclasses implement @ref render() to produce an @ref ImageBuffer.
 * Use @ref ChartExporter for saving to PNG, JPEG, or PDF.
 *
 * @see PieChart, BarChart, LineChart, CartesianChart
 */
class Chart {
public:
  virtual ~Chart() = default;

  /** @brief Mutable chart options (size, title, colors, margins). */
  ChartOptions &options() {
    return options_;
  }
  /** @brief Read-only chart options. */
  ChartOptions const &options() const {
    return options_;
  }

  /**
   * @brief Apply a @ref ChartTheme to this chart's color and typography options.
   *
   * Overwrites background, title, axis, grid, and text colors plus font_size
   * and show_grid. Call before setting per-chart overrides; they will take
   * precedence over the theme values.
   */
  void apply_theme(ChartTheme const &theme) {
    options_.background = theme.background;
    options_.title_color = theme.title_color;
    options_.axis_color = theme.axis_color;
    options_.grid_color = theme.grid_color;
    options_.text_color = theme.text_color;
    options_.font_size = theme.font_size;
    options_.show_grid = theme.show_grid;
  }

  /**
   * @brief Render the chart to an RGBA image buffer.
   * @return New @ref ImageBuffer at options().width × options().height.
   */
  virtual ImageBuffer render() const = 0;

protected:
  ChartOptions options_;
  mutable Font font_;
};

} // namespace insightdock::charts

#endif // INSIGHTDOCK_CHARTS_CHART_H
