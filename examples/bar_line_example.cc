#include "insightdock/charts.h"

// SPDX-License-Identifier: MIT
// Copyright (c) InsightDock contributors


#include <iostream>

int main() {
  // Grouped bar chart
  insightdock::charts::BarChart bar;
  bar.options().width = 640;
  bar.options().height = 400;
  bar.options().title = "Quarterly Revenue";
  bar.set_categories({"Q1", "Q2", "Q3", "Q4"});
  bar.add_series("Revenue", {100, 120, 90, 140});
  bar.add_series("Cost", {60, 70, 65, 80});
  bar.set_mode(insightdock::charts::BarMode::Grouped);
  bar.cartesian_options().y_axis_label = "Amount";

  insightdock::charts::ImageBuffer bar_img = bar.render();
  if (insightdock::charts::write_png("bar_grouped.png", bar_img))
    std::cout << "Wrote bar_grouped.png\n";

  // Stacked bar chart
  insightdock::charts::BarChart stacked;
  stacked.options().width = 640;
  stacked.options().height = 400;
  stacked.options().title = "Stacked Quarterly";
  stacked.set_categories({"Q1", "Q2", "Q3", "Q4"});
  stacked.add_series("Product A", {40, 50, 35, 60});
  stacked.add_series("Product B", {30, 25, 40, 35});
  stacked.add_series("Product C", {20, 30, 25, 30});
  stacked.set_mode(insightdock::charts::BarMode::Stacked);

  insightdock::charts::ImageBuffer stacked_img = stacked.render();
  if (insightdock::charts::write_png("bar_stacked.png", stacked_img))
    std::cout << "Wrote bar_stacked.png\n";

  // Line chart
  insightdock::charts::LineChart line;
  line.options().width = 640;
  line.options().height = 400;
  line.options().title = "Trend";
  line.set_categories({"Jan", "Feb", "Mar", "Apr", "May"});
  line.add_series("Series A", {10, 25, 18, 32, 28});
  line.add_series("Series B", {15, 12, 22, 20, 35});
  line.line_options().fill_area = true;

  insightdock::charts::ImageBuffer line_img = line.render();
  if (insightdock::charts::write_png("line_chart.png", line_img))
    std::cout << "Wrote line_chart.png\n";

  // Pie chart
  insightdock::charts::PieChart pie;
  pie.options().width = 600;
  pie.options().height = 600;
  pie.options().background = insightdock::charts::Color::white();
  pie.options().title = "Budget Split";
  insightdock::charts::PieOptions po;
  po.radius = 220.0;
  pie.set_pie_options(po);
  pie.add_slice("Revenue", 100.0);
  pie.add_slice("Expenses", 60.0);
  pie.add_slice("Profit", 40.0);

  insightdock::charts::ImageBuffer pie_img = pie.render();
  if (insightdock::charts::write_png("pie_chart.png", pie_img))
    std::cout << "Wrote pie_chart.png\n";

  return 0;
}
