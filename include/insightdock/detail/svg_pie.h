// SPDX-License-Identifier: MIT
// Copyright (c) InsightDock contributors

#ifndef INSIGHTDOCK_CHARTS_DETAIL_SVG_PIE_H
#define INSIGHTDOCK_CHARTS_DETAIL_SVG_PIE_H

#include "insightdock/export_macros.h"
#include "insightdock/pie_chart.h"
#include <string>

namespace insightdock::charts::detail {

// Native SVG <path>/<text> rendering for PieChart — sectors as circular arcs,
// external labels with leader lines, and a legend. Used by ChartExporter when
// ExportOptions::svg_backend == SvgBackend::Vector. Other chart types fall
// back to the raster-embed SVG writer (src/svg_writer.cc).
INSIGHTDOCK_API std::string build_pie_svg(PieChart const &chart);

} // namespace insightdock::charts::detail

#endif // INSIGHTDOCK_CHARTS_DETAIL_SVG_PIE_H
