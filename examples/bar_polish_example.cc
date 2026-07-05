// SPDX-License-Identifier: MIT
// Copyright (c) InsightDock contributors

#include "insightdock/charts.h"

#include <iostream>

int main() {
  using namespace insightdock::charts;

  // Rounded corners + a subtle border give bars a Chart.js/amCharts-style look;
  // data labels put the exact value on the chart without a reader needing the axis.
  BarChart bar;
  bar.options().width = 640;
  bar.options().height = 400;
  bar.options().title = "Quarterly Revenue (millions USD)";
  bar.set_categories({"Q1", "Q2", "Q3", "Q4"});
  bar.add_series("2025", {42, 55, 48, 63}, Color::rgb(66, 133, 244));

  bar.bar_options().corner_radius = 8.0f;
  bar.bar_options().border_width = 1.5f;
  bar.bar_options().border_color = Color::rgb(30, 90, 180);
  bar.bar_options().data_labels.enabled = true;
  bar.bar_options().data_labels.color = Color::gray(40);

  ImageBuffer img = bar.render();
  if (write_png("bar_polish.png", img))
    std::cout << "Wrote bar_polish.png\n";

  return 0;
}
