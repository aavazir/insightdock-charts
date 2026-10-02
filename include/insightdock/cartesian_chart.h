// SPDX-License-Identifier: MIT
// Copyright (c) InsightDock contributors

/**
 * @file cartesian_chart.h
 * @brief Base class for axis-based charts (bar, line) with categories and series.
 */

#ifndef INSIGHTDOCK_CHARTS_CARTESIAN_CHART_H
#define INSIGHTDOCK_CHARTS_CARTESIAN_CHART_H

#include "insightdock/chart.h"
#include "insightdock/export_macros.h"
#include "insightdock/geometry.h"
#include "insightdock/scale.h"
#include <functional>
#include <string>
#include <vector>

namespace insightdock::charts {

/**
 * @brief How y-axis tick values are formatted as label strings.
 *
 * Set @ref CartesianOptions::y_tick_format to one of these values, or supply
 * a @ref CartesianOptions::custom_y_formatter callback for full control.
 */
enum class TickFormat {
  Raw,      ///< `%.0f` for integers, `%.2f` for fractions (default).
  SI,       ///< SI prefixes: 1 500 000 → "1.5M", 2 300 → "2.3K".
  Currency, ///< Two decimal places with a leading `$`: "1234.56" → "$1234.56".
  Percent,  ///< One decimal place with a trailing `%`: "42.5" → "42.5%".
  DateTime, ///< Unix timestamp → `YYYY-MM-DD HH:MM` (for @ref ScaleType::DateTime axes).
};

/** @brief Which axis a @ref ReferenceLine or @ref ReferenceBand is measured against. */
enum class ReferenceOrientation {
  Horizontal, ///< A constant y-value line/band (e.g. an SLA threshold).
  Vertical    ///< A constant x-position line/band, in fractional category-index units.
};

/**
 * @brief A threshold/annotation line drawn over Bar/Line/Area/Combo charts.
 *
 * Not available on PieChart or ScatterChart, which have no @ref CartesianChart::draw_overlay
 * hook to attach to.
 */
struct ReferenceLine {
  ReferenceOrientation orientation = ReferenceOrientation::Horizontal;
  /**
   * For @ref ReferenceOrientation::Horizontal, a y-axis data value.
   * For @ref ReferenceOrientation::Vertical, a fractional category index (the x_scale
   * domain is `[0, categories.size()]`) — not a category label lookup.
   */
  double value = 0.0;
  /** Horizontal only: plot against the secondary (right) y-axis instead of the primary. */
  bool use_secondary_axis = false;
  Color color = Color::rgb(200, 60, 60);
  float width = 1.5f;
  /** Alternating on/off stroke lengths in pixels; defaults to dashed to stand out from data. */
  std::vector<float> dash = {6.0f, 4.0f};
  /** Optional text drawn at the line's plot-edge end. */
  std::string label;
};

/** @brief A shaded threshold band (e.g. a target range) drawn over Bar/Line/Area/Combo charts. */
struct ReferenceBand {
  ReferenceOrientation orientation = ReferenceOrientation::Horizontal;
  /** Same value semantics as @ref ReferenceLine::value; primary axis only in v1. */
  double from = 0.0;
  double to = 0.0;
  Color color = Color::rgba(255, 200, 0, 60);
  std::string label;
};

/** @brief Axis labels and scale behavior for Cartesian charts. */
struct CartesianOptions {
  std::string x_axis_label;  ///< Label below the x-axis (optional).
  std::string y_axis_label;  ///< Label beside the y-axis (optional).
  bool begin_at_zero = true; ///< Force y-axis minimum to 0 when data are non-negative.

  /**
   * @brief Primary (left) y-axis interpolation mode: @ref ScaleType::Linear (default) or
   * @ref ScaleType::Log. @ref ScaleType::DateTime is not meaningful for a value axis.
   * Log scale clamps non-positive data to a small positive floor (see @ref Scale::log_nice) —
   * suited to network traffic, latency, or financial data spanning multiple orders of magnitude.
   * @ref CartesianOptions::begin_at_zero is ignored when this is Log (a log domain can't
   * include zero). The secondary (right) y-axis is Linear-only in v1.
   */
  ScaleType y_scale_type = ScaleType::Linear;

  /**
   * @brief X-axis interpolation mode: @ref ScaleType::Linear (default, evenly-spaced
   * categories) or @ref ScaleType::DateTime (continuous domain positioned by real value,
   * via @ref CartesianChart::set_x_values). Only honored by chart types that override
   * @ref CartesianChart::supports_continuous_x_axis (LineChart, AreaChart) — ignored
   * (falls back to category mode) on BarChart/ComboChart, whose fixed-width bar-per-category
   * layout doesn't have a well-defined meaning for irregularly-spaced continuous positions.
   */
  ScaleType x_scale_type = ScaleType::Linear;

  /** @brief Built-in tick label format (used when @p custom_y_formatter is null). */
  TickFormat y_tick_format = TickFormat::Raw;

  /**
   * @brief Custom y-axis tick formatter.
   *
   * When non-null, overrides @p y_tick_format.  The callback receives the tick
   * value and returns the label string.
   *
   * @code
   * opts.custom_y_formatter = [](double v) {
   *   char buf[32];
   *   std::snprintf(buf, sizeof(buf), "%.1f kbps", v / 1000.0);
   *   return std::string(buf);
   * };
   * @endcode
   */
  std::function<std::string(double)> custom_y_formatter;

  /**
   * @brief Maximum number of x-axis category labels to render.
   *
   * When 0 (default), labels are shown as densely as collision avoidance
   * allows.  Set to e.g. 10 to show at most 10 evenly-spaced labels across
   * the axis regardless of chart width — useful for dense time-series data.
   */
  int max_x_labels = 0;

  /**
   * @brief Minimum number of y-axis tick labels to render.
   *
   * When 0 (default), no lower bound is enforced.  If the nice-tick
   * algorithm would produce fewer ticks than this value, a finer step
   * size is attempted.  Has no effect when the data range is too small
   * to accommodate the requested density.
   *
   * If both @p min_y_labels and @p max_y_labels are set and
   * @p min_y_labels > @p max_y_labels the values are swapped internally.
   */
  int min_y_labels = 0;

  /**
   * @brief Maximum number of y-axis tick labels to render.
   *
   * When 0 (default), the renderer uses ~6 ticks (the historical default).
   * Set to e.g. 12 to request a denser y-axis.  Setting both
   * @p min_y_labels and @p max_y_labels to the same value instructs the
   * renderer to attempt exactly that many labels.
   */
  int max_y_labels = 0;

  /** @brief Label beside the secondary (right) y-axis, if enabled (optional). */
  std::string y2_axis_label;
  /** @brief Built-in tick label format for the secondary y-axis. */
  TickFormat y2_tick_format = TickFormat::Raw;
  /** @brief Custom secondary y-axis tick formatter; overrides @p y2_tick_format when non-null. */
  std::function<std::string(double)> custom_y2_formatter;

  /** @brief Threshold/annotation lines drawn behind the chart's own series. */
  std::vector<ReferenceLine> reference_lines;
  /** @brief Shaded threshold bands drawn behind the chart's own series. */
  std::vector<ReferenceBand> reference_bands;
};

/**
 * @brief Abstract base for charts with x/y axes, categories, and multiple series.
 *
 * Handles background, title, axes, grid, legend, and plot layout.
 * Subclasses implement @ref draw_series() for bar or line geometry.
 *
 * @see BarChart, LineChart
 */
class INSIGHTDOCK_API CartesianChart : public Chart {
public:
  /**
   * @brief Context passed to @ref draw_series() and @ref draw_overlay() during rendering.
   *
   * Public (not protected) so shared overlay helpers outside the class hierarchy —
   * e.g. the reference-line/band renderer used by Bar/Line/Area/Combo — can take a
   * `PlotContext const&` parameter.
   */
  struct PlotContext {
    ImageBuffer &buffer; ///< Target pixel buffer.
    Rect plot;           ///< Plot area rectangle (excluding margins).
    Scale x_scale;       ///< Category index, or continuous value, → pixel.
    Scale y_scale;       ///< Data value → pixel (primary, left axis).
    Scale y_scale2;      ///< Data value → pixel (secondary, right axis).
    bool has_y2 = false; ///< Whether @ref y_scale2 is active for this render.
    /**
     * Whether @ref x_scale maps a continuous numeric domain (from @ref x_values) rather
     * than category index — true only when the concrete chart type opted in via
     * @ref CartesianChart::supports_continuous_x_axis and @ref CartesianOptions::x_scale_type
     * is @ref ScaleType::DateTime with non-empty @ref x_values. See @ref set_x_values.
     */
    bool continuous_x = false;
    /**
     * Whether the plot is laid out with a horizontal value axis (bottom) and a vertical
     * category axis (left) instead of the default vertical value axis (left) / horizontal
     * category axis (bottom). Only BarChart opts in via @ref CartesianChart::is_horizontal.
     * When true, @ref y_scale's pixel range spans the plot's horizontal extent (left→right)
     * rather than vertical — it still represents the "value" axis, just rotated.
     */
    bool horizontal = false;
    Font const &font;                  ///< Font for labels.
    ChartOptions const &opts;          ///< Common chart options.
    CartesianOptions const &cart_opts; ///< Axis options.
    std::vector<std::string> const &categories;
    std::vector<DataSeries> const &series;
    std::vector<double> const &x_values; ///< Populated only when @ref continuous_x is true.
  };

  /** @brief Mutable Cartesian options. */
  CartesianOptions &cartesian_options() {
    return cartesian_;
  }
  CartesianOptions const &cartesian_options() const {
    return cartesian_;
  }

  /**
   * @brief Set category labels (x-axis).
   * @param categories One label per category; each series must have the same length.
   */
  void set_categories(std::vector<std::string> categories);
  /** @brief Category labels. */
  std::vector<std::string> const &categories() const {
    return categories_;
  }

  /**
   * @brief Set per-point numeric x-values (e.g. Unix timestamps) for continuous x-axis
   * positioning, in place of evenly-spaced categories.
   *
   * Only takes effect when @ref CartesianOptions::x_scale_type is @ref ScaleType::DateTime
   * *and* the concrete chart type overrides @ref supports_continuous_x_axis to return true
   * (LineChart, AreaChart) — series values are then paired with these by index instead of
   * with @ref categories(). Ignored otherwise (falls back to category-slot positioning).
   * Must be strictly non-decreasing and the same length as each series' values.
   */
  void set_x_values(std::vector<double> values) {
    x_values_ = std::move(values);
  }
  /** @brief Numeric x-values set via @ref set_x_values, if any. */
  std::vector<double> const &x_values() const {
    return x_values_;
  }

  /**
   * @brief Add a data series with automatic palette color.
   * @param label  Series name (legend).
   * @param values One value per category.
   */
  void add_series(std::string label, std::vector<double> values);

  /**
   * @brief Add a data series with explicit color.
   * @param label  Series name.
   * @param values One value per category.
   * @param color  Series color.
   */
  void add_series(std::string label, std::vector<double> values, Color color);

  /** @brief All data series. */
  std::vector<DataSeries> const &series() const {
    return series_;
  }
  /** @brief Remove all series (categories unchanged). */
  void clear_series();

  /** @brief Render axes, grid, legend, and series to RGBA. */
  ImageBuffer render() const override;

protected:
  /** @brief Draw bars, lines, or other series geometry (implemented by subclass). */
  virtual void draw_series(PlotContext &ctx) const = 0;

  /**
   * @brief Extension point for custom overlays drawn *after* @ref draw_series.
   *
   * Override to add reference lines, bands, annotations, watermarks, or
   * combo-chart geometry without replacing the full render pipeline.  The
   * default implementation does nothing.
   *
   * This hook is the architectural prerequisite for combo/dual-axis charts
   * (Phase 0.6) and annotation layers (Phase 2).
   */
  virtual void draw_overlay(PlotContext &ctx) const {
    (void)ctx;
  }

  /** @brief Minimum y value for scale (default: 0 or data min). */
  virtual double compute_y_min() const;
  /** @brief Maximum y value for scale (default: data max with padding). */
  virtual double compute_y_max() const;

  /**
   * @brief Whether to render a secondary (right-side) y-axis. Default: false.
   *
   * Override alongside @ref compute_y2_min / @ref compute_y2_max to enable a
   * dual-axis layout (e.g. @ref ComboChart assigning some series to the
   * right axis). When true, @ref PlotContext::y_scale2 is populated and the
   * right margin is widened to fit its tick labels.
   */
  virtual bool has_secondary_y_axis() const {
    return false;
  }
  /**
   * @brief Whether this chart type can position its x-axis by continuous numeric value
   * (via @ref set_x_values / @ref CartesianOptions::x_scale_type) instead of evenly-spaced
   * categories. Default: false. Overridden by LineChart and AreaChart, whose point-based
   * geometry has a well-defined continuous position; not by BarChart/ComboChart, whose
   * fixed-width bar-per-category layout does not.
   */
  virtual bool supports_continuous_x_axis() const {
    return false;
  }

  /**
   * @brief Whether to lay out the plot with a horizontal value axis (bottom) and a
   * vertical category axis (left), instead of the default vertical value axis (left)
   * / horizontal category axis (bottom). Default: false. Overridden by BarChart based
   * on @ref BarOptions::orientation; not meaningful for Line/Area/Combo's point-based
   * geometry, so they don't override it. @ref ReferenceLine / @ref ReferenceBand overlays
   * are not supported in horizontal mode (v1) since their orientation semantics assume
   * a vertical value axis.
   */
  virtual bool is_horizontal() const {
    return false;
  }

  /** @brief Minimum value for the secondary y-axis domain. */
  virtual double compute_y2_min() const {
    return 0.0;
  }
  /** @brief Maximum value for the secondary y-axis domain. */
  virtual double compute_y2_max() const {
    return 0.0;
  }
  /** @brief Compute plot rectangle inside the image buffer. */
  Rect compute_plot_rect(ImageBuffer const &buf, Font const &font) const;

  void draw_background(ImageBuffer &buf) const;
  void draw_title(ImageBuffer &buf, Font &font) const;
  void draw_axes_and_grid(PlotContext &ctx) const;
  void draw_legend(ImageBuffer &buf, Font &font, Rect plot) const;

  std::vector<std::string> categories_;
  std::vector<DataSeries> series_;
  std::vector<double> x_values_;
  CartesianOptions cartesian_;
};

} // namespace insightdock::charts

#endif // INSIGHTDOCK_CHARTS_CARTESIAN_CHART_H
