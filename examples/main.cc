// SPDX-License-Identifier: MIT
// Copyright (c) InsightDock contributors

#include "insightdock/charts.h"
#include <iostream>

int main() {
  insightdock::charts::PieChart chart;
  chart.options().width = 600;
  chart.options().height = 600;
  chart.options().background = insightdock::charts::Color::white();
  chart.options().title = "Sample Pie";

  chart.add_slice("Revenue", 100.0);
  chart.add_slice("Expenses", 60.0);
  chart.add_slice("Profit", 40.0);

  insightdock::charts::ImageBuffer img = insightdock::charts::ChartExporter::render(chart);
  std::cout << "Chart: " << img.width() << " x " << img.height()
            << " pixels, " << img.data().size() << " bytes\n";
  std::cout << "Slices: " << chart.slice_count() << '\n';

  if (insightdock::charts::ChartExporter::save(chart, "chart_output.png"))
    std::cout << "Wrote chart_output.png\n";

  insightdock::charts::ExportOptions jpg_opts;
  jpg_opts.format = insightdock::charts::OutputFormat::Jpeg;
  jpg_opts.jpeg_quality = 90;
  if (insightdock::charts::ChartExporter::save(chart, "chart_output.jpg", jpg_opts))
    std::cout << "Wrote chart_output.jpg\n";

  return 0;
}
