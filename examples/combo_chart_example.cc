// SPDX-License-Identifier: MIT
// Copyright (c) InsightDock contributors

#include "insightdock/charts.h"

#include <iostream>

int main() {
  using namespace insightdock::charts;

  // Bars use the left axis (Active Users); the line uses an independent
  // right axis (Avg. Duration) since the two series have different units
  // and magnitude.
  ComboChart combo;
  combo.options().width = 640;
  combo.options().height = 400;
  combo.options().title = "Daily Active Users & Avg. Session Duration";
  combo.cartesian_options().y_axis_label = "Active Users";
  combo.cartesian_options().y2_axis_label = "Avg. Duration (min)";
  combo.set_categories({"Mon", "Tue", "Wed", "Thu", "Fri"});
  combo.add_bar_series("Active Users", {320, 415, 390, 445, 510});
  combo.add_line_series("Avg. Duration (min)", {4.2, 5.1, 3.8, 6.0, 5.5}, Color::rgb(192, 75, 75),
                        /*secondary_axis=*/true);

  ImageBuffer combo_img = combo.render();
  if (write_png("combo_chart.png", combo_img))
    std::cout << "Wrote combo_chart.png\n";

  return 0;
}
