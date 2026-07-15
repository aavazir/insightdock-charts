// SPDX-License-Identifier: MIT
// Copyright (c) InsightDock contributors

#include "insightdock/charts.h"

#include <iostream>

int main() {
  using namespace insightdock::charts;

  // Horizontal bars suit rankings/leaderboards and long category names that would
  // collide if crammed under a vertical bar chart's x-axis.
  BarChart bar;
  bar.options().width = 640;
  bar.options().height = 400;
  bar.options().title = "Top Regions by Revenue (millions USD)";
  bar.bar_options().orientation = BarOrientation::Horizontal;
  bar.set_categories({"North America", "Europe", "Asia-Pacific", "Latin America", "Africa"});
  bar.add_series("Revenue", {320, 280, 410, 150, 95}, Color::rgb(66, 133, 244));
  bar.bar_options().data_labels.enabled = true;
  bar.bar_options().data_labels.color = Color::gray(40);

  ImageBuffer img = bar.render();
  if (write_png("bar_horizontal.png", img))
    std::cout << "Wrote bar_horizontal.png\n";

  return 0;
}
