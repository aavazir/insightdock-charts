// SPDX-License-Identifier: MIT
// One-off visual check for multi-line pie labels + percentage spacing.

#include "insightdock/charts.h"

#include <iostream>

int main() {
  insightdock::charts::PieChart pie;
  pie.options().width = 420;
  pie.options().height = 420;
  pie.options().background = insightdock::charts::Color::white();
  auto po = pie.pie_options();
  po.show_percentage = true;
  po.show_labels = true;
  po.show_legend = false;
  pie.set_pie_options(po);
  pie.add_slice("40-AE-30-69-BD-51111", 1111);
  pie.add_slice("maisam", 1111);

  if (!insightdock::charts::ChartExporter::save(pie, "pie_overlap_test.png")) {
    std::cerr << "Failed to write pie_overlap_test.png\n";
    return 1;
  }
  std::cout << "Wrote pie_overlap_test.png\n";
  return 0;
}
