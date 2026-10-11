// SPDX-License-Identifier: MIT
// Copyright (c) InsightDock contributors

/**
 * @file pie_chart.h
 * @brief 2D pie chart with labels, legend, and Tableau/Chart.js palettes.
 */

#ifndef INSIGHTDOCK_CHARTS_PIE_CHART_H
#define INSIGHTDOCK_CHARTS_PIE_CHART_H

#include "insightdock/chart.h"
#include "insightdock/chart_style.h"
#include "insightdock/color.h"
#include "insightdock/export_macros.h"
#include "insightdock/palette.h"
#include <string>
#include <vector>

namespace insightdock::charts {

/** @brief One slice of a @ref PieChart. */
struct PieSlice {
  std::string label;  ///< Slice label (legend and on-chart text).
  double value = 0.0; ///< Nonnegative value; negative/NaN/infinity inputs are stored as zero.
  Color color;        ///< Fill color (set automatically when using add_slice without color).
};

/** @brief Legend placement for pie charts. */
enum class PieLegendPosition {
  Bottom, ///< Legend below the pie.
  Right   ///< Legend to the right of the pie.
};

/**
 * @brief Pie-specific layout and styling.
 *
 * Defaults match report-style pies: semi-transparent fills, white slice strokes,
 * external labels with percentages.
 */
struct PieOptions {
  double center_x = 0.0;    ///< Pie center X (both coordinates 0 = auto-center at render time).
  double center_y = 0.0;    ///< Pie center Y (both coordinates 0 = auto-center at render time).
  double radius = 0.0;      ///< Pie radius (0 = auto-fit with room for title, legend and callouts).
  double gap_degrees = 0.0; ///< Angular gap between slices in degrees.
  bool skip_zero = true;    ///< Omit slices with value ≤ 0.
  /**
   * Doughnut cutout, as a fraction of @ref radius, in [0, 1). 0 (default) draws a
   * plain pie; > 0 punches a hole in the center down to @ref Chart::options()'s
   * background color. Only correct against a solid background (no gradient/image
   * backdrop is supported by this library).
   */
  double inner_radius_ratio = 0.0;

  PaletteKind palette = PaletteKind::Tableau10; ///< Color cycle for auto-colored slices.
  /**
   * Draw measured external callouts, separated vertically outside the pie. Overfull columns
   * keep larger slices first (input-order ties); blocks that cannot fit are omitted in full.
   * Slice geometry and legend entries remain independent of callout visibility.
   */
  bool show_labels = true;
  bool show_percentage = true; ///< Include percentage in labels.
  bool show_legend = true;     ///< Draw a measured, wrapped legend with +N more overflow.
  PieLegendPosition legend_position = PieLegendPosition::Bottom;

  float fill_opacity = 0.6f;           ///< Multiplier for slice color alpha (clamped to 0–1).
  Color stroke_color = Color::white(); ///< Outline between slices.
  float stroke_width = 2.0f;           ///< Slice outline width in pixels.
  Color label_color = Color::rgb(34, 34, 34); ///< Label text color.
  float label_font_size = 11.0f;              ///< Label font size in pixels.

  /** In-slice value/percentage label, distinct from the external leader-line label above. */
  DataLabelOptions data_labels;
};

/**
 * @brief Pie chart rendered to an @ref ImageBuffer.
 *
 * @example
 * @code
 * PieChart pie;
 * pie.add_slice("Revenue", 100);
 * pie.add_slice("Costs", 60);
 * ChartExporter::save(pie, "pie.png");
 * @endcode
 *
 * For vector PDF output, use @ref ChartExporter with @ref PdfBackend::Vector or
 * @ref insightdock::charts::podofo::draw_pie.
 */
class INSIGHTDOCK_API PieChart : public Chart {
public:
  VectorCapability vector_capability() const noexcept override {
    return VectorCapability::Native;
  }
  std::unique_ptr<Chart> clone() const override {
    return std::make_unique<PieChart>(*this);
  }

  PieChart();

  /**
   * @brief Add a slice with an explicit color.
   * @param label Slice name.
   * @param value Numeric value.
   * @param color Fill color.
   */
  void add_slice(std::string label, double value, Color color);

  /**
   * @brief Add a slice; color is chosen from @ref PieOptions::palette.
   * @param label Slice name.
   * @param value Numeric value.
   */
  void add_slice(std::string label, double value);

  /** @brief Replace all pie-specific options. */
  void set_pie_options(PieOptions opts);
  /** @brief Current pie options. */
  PieOptions const &pie_options() const {
    return pie_opts_;
  }

  /** @brief Remove all slices. */
  void clear_slices();
  /** @brief Number of slices. */
  std::size_t slice_count() const {
    return slices_.size();
  }
  /** @brief Read-only access to slice data (e.g. for PDF vector export). */
  std::vector<PieSlice> const &slices() const {
    return slices_;
  }

  /** @brief Render the pie chart to RGBA pixels. */
  ImageBuffer render() const override;
  /** @brief Draw the same pie geometry, labels and styles into a raster or vector target. */
  bool draw(DrawTarget &target) const override;

private:
  std::vector<PieSlice> slices_;
  PieOptions pie_opts_;
};

} // namespace insightdock::charts

#endif // INSIGHTDOCK_CHARTS_PIE_CHART_H
