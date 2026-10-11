// SPDX-License-Identifier: MIT
// Copyright (c) InsightDock contributors

#include "insightdock/detail/svg_pie.h"
#include "insightdock/detail/svg_target.h"

namespace insightdock::charts::detail {

std::string build_pie_svg(PieChart const &chart) {
  return build_chart_svg(chart);
}

} // namespace insightdock::charts::detail
