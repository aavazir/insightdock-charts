// SPDX-License-Identifier: MIT
// Copyright (c) InsightDock contributors

#ifndef INSIGHTDOCK_CHARTS_DETAIL_BAR_SHAPE_H
#define INSIGHTDOCK_CHARTS_DETAIL_BAR_SHAPE_H

#include "insightdock/color.h"
#include "insightdock/export_macros.h"
#include "insightdock/image_buffer.h"

namespace insightdock::charts::detail {

/**
 * @brief Fills one bar rectangle, optionally rounded and/or bordered.
 *
 * Shared by BarChart and ComboChart so both apply identical corner-radius/border
 * handling. A rounded bar with a border draws a plain rectangular outline (accepted
 * v1 limitation — the outline slightly overlaps the rounded corners).
 */
INSIGHTDOCK_API void fill_bar(ImageBuffer &buffer, float x, float y, float w, float h,
                              Color const &fill_color, float corner_radius,
                              Color const &border_color, float border_width);

} // namespace insightdock::charts::detail

#endif // INSIGHTDOCK_CHARTS_DETAIL_BAR_SHAPE_H
