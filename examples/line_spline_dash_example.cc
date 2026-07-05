// SPDX-License-Identifier: MIT
// Copyright (c) InsightDock contributors

#include "insightdock/charts.h"

#include <iostream>

int main() {
  using namespace insightdock::charts;

  // Spline smoothing, a dash pattern, and a marker shape are chart-wide settings
  // (not per-series) in this library, so every series in the chart shares them.
  LineChart line;
  line.options().width = 640;
  line.options().height = 400;
  line.options().title = "Weekly Active Users by Region";
  line.set_categories({"Wk1", "Wk2", "Wk3", "Wk4", "Wk5", "Wk6"});

  line.add_series("US", {120, 145, 132, 168, 190, 210}, Color::rgb(66, 133, 244));
  line.add_series("EU", {80, 92, 105, 98, 130, 150}, Color::rgb(200, 120, 20));

  line.line_options().spline = true;
  line.line_options().dash = {8.0f, 4.0f};
  line.line_options().point_shape = PointMarkerShape::Triangle;

  ImageBuffer img = line.render();
  if (write_png("line_spline_dash.png", img))
    std::cout << "Wrote line_spline_dash.png\n";

  return 0;
}
