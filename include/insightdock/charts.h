// SPDX-License-Identifier: MIT
// Copyright (c) InsightDock contributors

/**
 * @file charts.h
 * @brief Umbrella header for InsightDock Charts for C++.
 *
 * Include this single header for pie, bar, and line charts plus PNG/JPEG export.
 * For PDF/PoDoFo integration, also include @ref insightdock/pdf/podofo.h when built
 * with `INSIGHTDOCK_WITH_PODOFO`.
 *
 * @namespace insightdock::charts
 * Primary namespace for chart types, rendering, and export.
 *
 * @example
 * @code
 * #include "insightdock/charts.h"
 * using namespace insightdock::charts;
 *
 * PieChart pie;
 * pie.add_slice("Online", 72);
 * pie.add_slice("Offline", 28);
 * ChartExporter::save(pie, "status.png");
 * @endcode
 */

#ifndef INSIGHTDOCK_CHARTS_H
#define INSIGHTDOCK_CHARTS_H

#include "insightdock/area_chart.h"
#include "insightdock/bar_chart.h"
#include "insightdock/chart.h"
#include "insightdock/chart_style.h"
#include "insightdock/color.h"
#include "insightdock/combo_chart.h"
#include "insightdock/decimate.h"
#include "insightdock/error.h"
#include "insightdock/export.h"
#include "insightdock/font.h"
#include "insightdock/geometry.h"
#include "insightdock/image_buffer.h"
#include "insightdock/line_chart.h"
#include "insightdock/palette.h"
#include "insightdock/pie_chart.h"
#include "insightdock/png_writer.h"
#include "insightdock/scatter_chart.h"
#include "insightdock/svg_writer.h"
#include "insightdock/theme.h"

#endif // INSIGHTDOCK_CHARTS_H
