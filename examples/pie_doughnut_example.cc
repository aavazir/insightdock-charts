// SPDX-License-Identifier: MIT
// Copyright (c) InsightDock contributors

#include "insightdock/charts.h"

#include <iostream>

int main() {
  using namespace insightdock::charts;

  PieChart pie;
  pie.options().width = 500;
  pie.options().height = 500;
  pie.options().title = "Traffic Sources";
  pie.add_slice("Organic", 45);
  pie.add_slice("Paid", 25);
  pie.add_slice("Referral", 18);
  pie.add_slice("Social", 12);

  PieOptions po = pie.pie_options();
  po.inner_radius_ratio = 0.55; // doughnut cutout
  // In-slice percentage labels replace the external leader-line labels here so the
  // two label styles don't compete for the same space.
  po.show_labels = false;
  po.data_labels.enabled = true;
  po.data_labels.color = Color::white();
  po.data_labels.format = TickFormat::Percent;
  pie.set_pie_options(po);

  ImageBuffer img = pie.render();
  if (write_png("pie_doughnut.png", img))
    std::cout << "Wrote pie_doughnut.png\n";

  return 0;
}
