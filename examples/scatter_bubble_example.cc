// SPDX-License-Identifier: MIT
// Copyright (c) InsightDock contributors

#include "insightdock/charts.h"

#include <iostream>

int main() {
  using namespace insightdock::charts;

  // Plain scatter — continuous numeric x/y, no category alignment needed.
  ScatterChart scatter;
  scatter.options().width = 640;
  scatter.options().height = 400;
  scatter.options().title = "Latency vs Throughput";
  scatter.scatter_options().x_axis_label = "Throughput (Mbps)";
  scatter.scatter_options().y_axis_label = "Latency (ms)";
  scatter.add_series("Device A", {{12, 40}, {25, 35}, {40, 28}, {55, 30}, {70, 18}, {90, 15}});
  scatter.add_series("Device B", {{15, 55}, {30, 48}, {48, 44}, {60, 38}, {78, 33}, {95, 25}});

  ImageBuffer scatter_img = scatter.render();
  if (write_png("scatter_chart.png", scatter_img))
    std::cout << "Wrote scatter_chart.png\n";

  // Bubble variant — third value (radius) drives marker size.
  ScatterChart bubble;
  bubble.options().width = 640;
  bubble.options().height = 400;
  bubble.options().title = "Site Traffic (bubble size: active users)";
  bubble.scatter_options().x_axis_label = "Avg session (min)";
  bubble.scatter_options().y_axis_label = "Bounce rate (%)";
  bubble.add_series("Sites", {
                                 {3.2, 42, 120},
                                 {5.1, 35, 480},
                                 {2.0, 58, 90},
                                 {7.4, 22, 950},
                                 {4.6, 30, 300},
                                 {1.5, 65, 60},
                             });

  ImageBuffer bubble_img = bubble.render();
  if (write_png("bubble_chart.png", bubble_img))
    std::cout << "Wrote bubble_chart.png\n";

  return 0;
}
