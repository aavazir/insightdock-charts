// SPDX-License-Identifier: MIT
// Copyright (c) InsightDock contributors

/**
 * @file palette.h
 * @brief Built-in color palettes for series and pie slices.
 */

#ifndef INSIGHTDOCK_CHARTS_PALETTE_H
#define INSIGHTDOCK_CHARTS_PALETTE_H

#include "insightdock/color.h"
#include <cstddef>

namespace insightdock::charts {

/** @brief Named palette for automatic series/slice coloring. */
enum class PaletteKind {
  ChartJs,  ///< 12-color Chart.js / AmCharts style palette.
  Tableau10 ///< 10-color Tableau palette (common in reports).
};

/**
 * @brief Color from the default 12-color Chart.js palette.
 * @param index Series or slice index (wraps modulo 12).
 */
Color palette_color(std::size_t index);

/**
 * @brief Color from the Tableau 10 palette.
 * @param index Index (wraps modulo 10).
 */
Color tableau10_color(std::size_t index);

/**
 * @brief Color from a specific palette kind.
 * @param kind  Palette to use.
 * @param index Color index (wraps per palette size).
 */
Color palette_color_by_kind(PaletteKind kind, std::size_t index);

/** @brief Number of colors in the Chart.js palette. */
constexpr std::size_t palette_size() {
  return 12;
}
/** @brief Number of colors in the Tableau 10 palette. */
constexpr std::size_t tableau10_size() {
  return 10;
}

} // namespace insightdock::charts

#endif // INSIGHTDOCK_CHARTS_PALETTE_H
