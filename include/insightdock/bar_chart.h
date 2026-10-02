// SPDX-License-Identifier: MIT
// Copyright (c) InsightDock contributors

/**
 * @file bar_chart.h
 * @brief Vertical bar chart with grouped or stacked series.
 */

#ifndef INSIGHTDOCK_CHARTS_BAR_CHART_H
#define INSIGHTDOCK_CHARTS_BAR_CHART_H

#include "insightdock/cartesian_chart.h"
#include "insightdock/chart_style.h"
#include "insightdock/color.h"

namespace insightdock::charts {

/** @brief How multiple series are drawn within each category. */
enum class BarMode {
  Grouped, ///< Side-by-side bars per category.
  Stacked  ///< Bars stacked vertically per category.
};

/** @brief Bar direction. */
enum class BarOrientation {
  Vertical,  ///< Bars rise from a horizontal category axis (default).
  Horizontal ///< Bars extend from a vertical category axis on the left — rankings,
             ///< leaderboards, long category names. Not available on ComboChart, whose
             ///< line-series geometry has no horizontal-orientation equivalent; also
             ///< doesn't support @ref ReferenceLine / @ref ReferenceBand overlays in v1
             ///< (their orientation semantics assume a vertical value axis).
};

/** @brief Layout, shape, and label options for BarChart (and reused by ComboChart's bars). */
struct BarOptions {
  BarMode mode = BarMode::Grouped;
  BarOrientation orientation = BarOrientation::Vertical;
  /** Gap between category groups as a fraction of slot width, in [0, 0.5]. */
  float group_gap = 0.2f;
  /** Gap between bars within a group (grouped mode only), in [0, 0.5]. */
  float bar_gap = 0.1f;
  /** Corner radius in pixels; 0 = square corners (default). */
  float corner_radius = 0.0f;
  /** Transparent (default) means no border is drawn regardless of border_width. */
  Color border_color = Color::rgba(0, 0, 0, 0);
  float border_width = 0.0f;
  DataLabelOptions data_labels;
};

/**
 * @brief Vertical bar chart.
 *
 * @example
 * @code
 * BarChart bar;
 * bar.set_categories({"Q1", "Q2", "Q3"});
 * bar.add_series("2024", {100, 120, 90});
 * bar.add_series("2025", {110, 130, 95});
 * ChartExporter::save(bar, "bars.png");
 * @endcode
 */
class BarChart : public CartesianChart {
public:
  std::unique_ptr<Chart> clone() const override {
    return std::make_unique<BarChart>(*this);
  }

  /** @brief Mutable access to all bar layout/shape/label options. */
  BarOptions &bar_options() {
    return bar_opts_;
  }
  BarOptions const &bar_options() const {
    return bar_opts_;
  }

  /** @brief Set grouped (default) or stacked bar layout. */
  void set_mode(BarMode mode) {
    bar_opts_.mode = mode;
  }
  /** @brief Current bar layout mode. */
  BarMode mode() const {
    return bar_opts_.mode;
  }

  /**
   * @brief Gap between category groups as a fraction of slot width.
   * @param gap Value in [0, 0.5]; default 0.2.
   */
  void set_group_gap(float gap) {
    bar_opts_.group_gap = gap;
  }

  /**
   * @brief Gap between bars within a group (grouped mode only).
   * @param gap Value in [0, 0.5]; default 0.1.
   */
  void set_bar_gap(float gap) {
    bar_opts_.bar_gap = gap;
  }

  /** @brief Set vertical (default) or horizontal bar direction. */
  void set_orientation(BarOrientation orientation) {
    bar_opts_.orientation = orientation;
  }
  /** @brief Current bar direction. */
  BarOrientation orientation() const {
    return bar_opts_.orientation;
  }

protected:
  void draw_series(PlotContext &ctx) const override;
  void draw_overlay(PlotContext &ctx) const override;
  double compute_y_max() const override;
  bool is_horizontal() const override {
    return bar_opts_.orientation == BarOrientation::Horizontal;
  }

private:
  void draw_grouped(PlotContext &ctx) const;
  void draw_stacked(PlotContext &ctx) const;
  void draw_grouped_horizontal(PlotContext &ctx) const;
  void draw_stacked_horizontal(PlotContext &ctx) const;

  BarOptions bar_opts_;
};

} // namespace insightdock::charts

#endif // INSIGHTDOCK_CHARTS_BAR_CHART_H
