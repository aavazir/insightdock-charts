// SPDX-License-Identifier: MIT
// Copyright (c) InsightDock contributors

#ifndef INSIGHTDOCK_CHARTS_DETAIL_MARKER_H
#define INSIGHTDOCK_CHARTS_DETAIL_MARKER_H

#include "insightdock/chart_style.h"
#include "insightdock/color.h"
#include "insightdock/image_buffer.h"

namespace insightdock::charts::detail {

/** @brief Draws a single point marker of the given shape, centered at (x, y). */
void draw_marker(ImageBuffer &buffer, float x, float y, float radius, PointMarkerShape shape,
                 Color const &color);

} // namespace insightdock::charts::detail

#endif // INSIGHTDOCK_CHARTS_DETAIL_MARKER_H
