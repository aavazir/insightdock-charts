// SPDX-License-Identifier: MIT
// Copyright (c) InsightDock contributors

#include "insightdock/charts.h"

#include <iostream>

int main() {
  using namespace insightdock::charts;

  ComboChart combo;
  combo.options().width = 640;
  combo.options().height = 400;
  combo.options().title = "API Latency vs. SLA Target";
  combo.cartesian_options().y_axis_label = "p95 latency (ms)";
  combo.set_categories({"Mon", "Tue", "Wed", "Thu", "Fri"});
  combo.add_bar_series("p95 latency", {180, 210, 245, 190, 230}, Color::rgb(66, 133, 244));

  // A shaded "healthy" target band plus a hard SLA threshold line — both drawn via
  // the shared reference-line/band overlay that any Bar/Line/Area/Combo chart can use.
  ReferenceBand target;
  target.orientation = ReferenceOrientation::Horizontal;
  target.from = 0;
  target.to = 200;
  target.color = Color::rgba(76, 175, 80, 50);
  target.label = "Target";
  combo.cartesian_options().reference_bands.push_back(target);

  ReferenceLine sla;
  sla.orientation = ReferenceOrientation::Horizontal;
  sla.value = 250;
  sla.color = Color::rgb(200, 40, 40);
  sla.label = "SLA (250ms)";
  combo.cartesian_options().reference_lines.push_back(sla);

  // A vertical line marking a deploy on Wednesday — category i spans [i, i+1), so
  // 2.5 lands on Wed's own center (0-indexed: Mon=0, Tue=1, Wed=2).
  ReferenceLine deploy;
  deploy.orientation = ReferenceOrientation::Vertical;
  deploy.value = 2.5;
  deploy.color = Color::rgb(30, 140, 30);
  deploy.label = "Deploy";
  combo.cartesian_options().reference_lines.push_back(deploy);

  ImageBuffer img = combo.render();
  if (write_png("combo_reference_lines.png", img))
    std::cout << "Wrote combo_reference_lines.png\n";

  return 0;
}
