// SPDX-License-Identifier: MIT
// Copyright (c) InsightDock contributors

#ifndef INSIGHTDOCK_CHARTS_DETAIL_SVG_PIE_H
#define INSIGHTDOCK_CHARTS_DETAIL_SVG_PIE_H

#include "insightdock/export_macros.h"
#include "insightdock/pie_chart.h"
#include <string>

namespace insightdock::charts::detail {

// Compatibility wrapper for build_chart_svg; pie geometry and labels come from PieChart::draw.
INSIGHTDOCK_API std::string build_pie_svg(PieChart const &chart);

} // namespace insightdock::charts::detail

#endif // INSIGHTDOCK_CHARTS_DETAIL_SVG_PIE_H
