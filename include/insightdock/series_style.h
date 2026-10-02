// SPDX-License-Identifier: MIT
// Copyright (c) InsightDock contributors

/**
 * @file series_style.h
 * @brief Style types shared by chart options and per-series overrides (dash, marker, gaps).
 *
 * Deliberately depends on nothing else in the library so that @ref DataSeries (chart.h) can hold
 * a @ref SeriesStyle without a circular include.
 */

#ifndef INSIGHTDOCK_CHARTS_SERIES_STYLE_H
#define INSIGHTDOCK_CHARTS_SERIES_STYLE_H

#include <cstdint>
#include <optional>
#include <vector>

namespace insightdock::charts {

/** @brief Shape drawn at each data point marker (line/scatter/combo charts). */
enum class PointMarkerShape { Circle, Square, Triangle, Cross };

/** @brief Alternating on/off stroke lengths in pixels (e.g. {6,4}); empty means solid. */
using DashPattern = std::vector<float>;

/**
 * @brief How a missing value (NaN, Inf, or a series shorter than the category list) is drawn in
 *        line, area and combo-line series.
 */
enum class GapPolicy {
  Zero,        ///< Treat the value as 0 (legacy behaviour).
  Gap,         ///< Break the line/area at the missing value (default).
  Interpolate, ///< Skip the value and connect its neighbours.
};

/**
 * @brief Optional per-series overrides of the chart-wide line/marker options.
 *
 * Every field defaults to "unset", meaning the series inherits the chart-wide value from its
 * options struct (`LineChartOptions`, `AreaChartOptions`, `ComboLineOptions`). Bar series ignore
 * these fields.
 */
struct SeriesStyle {
  std::optional<DashPattern> dash;        ///< Stroke dash pattern; empty vector = solid.
  std::optional<PointMarkerShape> marker; ///< Marker shape at each point.
  std::optional<float> line_width;        ///< Stroke width in pixels.
  std::optional<float> fill_opacity;      ///< Area/line-fill opacity in [0, 1].

  /** @brief True if any field overrides the chart-wide default. */
  bool any() const {
    return dash || marker || line_width || fill_opacity;
  }

  DashPattern const &dash_or(DashPattern const &chart_default) const {
    return dash ? *dash : chart_default;
  }
  PointMarkerShape marker_or(PointMarkerShape chart_default) const {
    return marker ? *marker : chart_default;
  }
  float line_width_or(float chart_default) const {
    return line_width ? *line_width : chart_default;
  }
  /** @brief Fill alpha (0-255): the override converted from [0,1], else @p chart_default_alpha. */
  std::uint8_t fill_alpha_or(std::uint8_t chart_default_alpha) const {
    if (!fill_opacity)
      return chart_default_alpha;
    float o = *fill_opacity;
    if (!(o > 0.0f))
      return 0;
    if (o >= 1.0f)
      return 255;
    return static_cast<std::uint8_t>(o * 255.0f + 0.5f);
  }
};

} // namespace insightdock::charts

#endif // INSIGHTDOCK_CHARTS_SERIES_STYLE_H
