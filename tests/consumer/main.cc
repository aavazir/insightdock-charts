// SPDX-License-Identifier: MIT
// Copyright (c) InsightDock contributors

#include <insightdock/charts.h>

#include <cstdio>

int main() {
  using namespace insightdock::charts;
  BarChart chart;
  chart.set_categories({"A", "B"});
  chart.add_series("S", {1.0, 2.0});
  auto png = encode_png(chart.render());
  if (png.size() < 8 || png[1] != 'P' || png[2] != 'N' || png[3] != 'G') {
    std::fprintf(stderr, "consumer: bad PNG output\n");
    return 1;
  }
  return 0;
}
