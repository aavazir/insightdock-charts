// SPDX-License-Identifier: MIT
// Copyright (c) InsightDock contributors

#ifndef INSIGHTDOCK_CHARTS_DETAIL_REFERENCE_LINES_H
#define INSIGHTDOCK_CHARTS_DETAIL_REFERENCE_LINES_H

#include "insightdock/cartesian_chart.h"

namespace insightdock::charts::detail {

/**
 * @brief Draws CartesianOptions::reference_bands then CartesianOptions::reference_lines.
 *
 * Shared by BarChart, LineChart, AreaChart, and ComboChart's draw_overlay() overrides so
 * reference lines/bands render identically across every Cartesian chart type. Intended to
 * be called before a subclass draws its own overlay geometry (e.g. ComboChart's line
 * series), so reference lines sit behind the chart's own data.
 */
void draw_reference_lines(CartesianChart::PlotContext const &ctx);

} // namespace insightdock::charts::detail

#endif // INSIGHTDOCK_CHARTS_DETAIL_REFERENCE_LINES_H
