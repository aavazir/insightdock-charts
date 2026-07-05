// SPDX-License-Identifier: MIT
// Copyright (c) InsightDock contributors

#ifndef INSIGHTDOCK_CHARTS_DETAIL_TICK_FORMAT_H
#define INSIGHTDOCK_CHARTS_DETAIL_TICK_FORMAT_H

#include "insightdock/cartesian_chart.h"

#include <functional>
#include <string>

namespace insightdock::charts::detail {

/**
 * @brief Formats a numeric value per TickFormat, honoring a custom formatter override.
 *
 * Shared by CartesianChart's y-axis tick labels and by data-label rendering across
 * Bar/Line/Area/Combo chart types so a label matches its axis's formatting.
 */
std::string format_tick_value(double v, TickFormat format,
                              std::function<std::string(double)> const &custom_formatter);

/**
 * @brief Formats a data-label value.
 * @param decimals Fixed decimal places, or -1 for auto (integer if whole, else 1 decimal).
 *
 * Delegates to format_tick_value() for TickFormat semantics when @p decimals is auto;
 * PieChart (which has no TickFormat concept of its own) can still call this with
 * TickFormat::Raw or TickFormat::Percent.
 */
std::string format_label_value(double v, TickFormat format, int decimals);

} // namespace insightdock::charts::detail

#endif // INSIGHTDOCK_CHARTS_DETAIL_TICK_FORMAT_H
