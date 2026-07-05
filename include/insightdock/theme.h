// SPDX-License-Identifier: MIT
// Copyright (c) InsightDock contributors

/**
 * @file theme.h
 * @brief @ref ChartTheme — apply a consistent visual style across multiple charts.
 *
 * A theme collects the color and typography fields from @ref ChartOptions into a
 * single reusable object.  Call @ref Chart::apply_theme on each chart before
 * rendering to produce consistent output across a multi-chart report.
 *
 * @code
 * auto theme = ChartTheme::dark();
 * pie.apply_theme(theme);
 * bar.apply_theme(theme);
 * line.apply_theme(theme);
 * @endcode
 *
 * Per-chart options set *after* `apply_theme` take precedence; the theme is a
 * convenience layer that batch-sets defaults, not a lock.
 */

#ifndef INSIGHTDOCK_CHARTS_THEME_H
#define INSIGHTDOCK_CHARTS_THEME_H

#include "insightdock/color.h"

namespace insightdock::charts {

/**
 * @brief Visual style applied to any @ref Chart via @ref Chart::apply_theme.
 *
 * Fields mirror the color/typography subset of @ref ChartOptions.
 * Two built-in presets are provided: @ref light and @ref dark.
 */
struct ChartTheme {
  Color background = Color::white();                  ///< Canvas background.
  Color title_color = Color::black();                 ///< Title text color.
  Color axis_color = Color::gray(80);                 ///< Axis line color.
  Color grid_color = Color::rgba(200, 200, 200, 180); ///< Grid line color.
  Color text_color = Color::gray(60);                 ///< Axis label / legend color.
  float font_size = 12.0f;                            ///< Base font size in pixels.
  bool show_grid = true;                              ///< Whether to draw grid lines.

  /**
   * @brief Default light theme — white background, dark text.
   *
   * Matches the factory defaults of @ref ChartOptions; useful as an explicit
   * "reset to light" call.
   */
  static ChartTheme light() {
    return {}; // all fields are already light defaults
  }

  /**
   * @brief Dark theme — near-black background, light text and grid lines.
   *
   * Suitable for dashboards shown on dark-mode UIs or dark PDF backgrounds.
   */
  static ChartTheme dark() {
    ChartTheme t;
    t.background = Color::rgba(30, 30, 36, 255);
    t.title_color = Color::rgba(230, 230, 230, 255);
    t.axis_color = Color::rgba(120, 120, 130, 255);
    t.grid_color = Color::rgba(60, 60, 70, 200);
    t.text_color = Color::rgba(180, 180, 190, 255);
    t.font_size = 12.0f;
    t.show_grid = true;
    return t;
  }
};

} // namespace insightdock::charts

#endif // INSIGHTDOCK_CHARTS_THEME_H
