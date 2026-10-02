// SPDX-License-Identifier: MIT
// Copyright (c) InsightDock contributors

#ifndef INSIGHTDOCK_CHARTS_DETAIL_SPLINE_H
#define INSIGHTDOCK_CHARTS_DETAIL_SPLINE_H

#include "insightdock/export_macros.h"
#include <utility>
#include <vector>

namespace insightdock::charts::detail {

/**
 * @brief Expands a polyline's control points into a smooth Catmull-Rom curve.
 *
 * Used by LineChart/AreaChart/ComboChart when `spline` is enabled: the returned
 * denser point list is passed to ImageBuffer::draw_polyline()/fill_polygon() unchanged,
 * while the original @p points remain the marker/data-label positions.
 *
 * @param points Control points (category-ordered).
 * @param segments_per_span Points generated between each pair of control points.
 * @return Expanded point list; a straight copy of @p points if fewer than 3 points are given.
 */
INSIGHTDOCK_API std::vector<std::pair<float, float>>
catmull_rom_expand(std::vector<std::pair<float, float>> const &points, int segments_per_span = 8);

} // namespace insightdock::charts::detail

#endif // INSIGHTDOCK_CHARTS_DETAIL_SPLINE_H
