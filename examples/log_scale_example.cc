// SPDX-License-Identifier: MIT
// Copyright (c) InsightDock contributors

#include "insightdock/charts.h"

#include <iostream>

int main() {
  using namespace insightdock::charts;

  // Network traffic / latency data routinely spans several orders of magnitude in a
  // single series; a linear y-axis would flatten the low end into an unreadable line
  // hugging the bottom. CartesianOptions::y_scale_type = ScaleType::Log fixes that.
  LineChart chart;
  chart.options().width = 640;
  chart.options().height = 400;
  chart.options().title = "Requests per Minute (log scale)";
  chart.cartesian_options().y_scale_type = ScaleType::Log;
  chart.cartesian_options().y_axis_label = "Requests/min";
  chart.set_categories({"00:00", "04:00", "08:00", "12:00", "16:00", "20:00"});
  chart.add_series("API Traffic", {8, 60, 4200, 91000, 15500, 340}, Color::rgb(66, 133, 244));
  chart.line_options().show_points = true;

  ImageBuffer img = chart.render();
  if (write_png("log_scale.png", img))
    std::cout << "Wrote log_scale.png\n";

  return 0;
}
