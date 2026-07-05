// SPDX-License-Identifier: MIT
// Copyright (c) InsightDock contributors
//
// Demonstrates a dense time-series LineChart:
//   - 100 daily data points decimated to 25 via LTTB
//   - max_x_labels caps the x-axis to ~12 readable date labels
//   - custom y-axis title (data already pre-scaled to GB by the caller)

#include "insightdock/charts.h"
#include "insightdock/decimate.h"

#include <cstdio>
#include <string>
#include <vector>

int main(int argc, char **argv) {
  using namespace insightdock::charts;

  std::string out = (argc > 1) ? argv[1] : "time_series_line.png";

  // 100 daily dates (DD/MM) and corresponding data-consumed values in GB.
  std::vector<std::string> dates = {
      "22/03", "23/03", "24/03", "25/03", "26/03", "27/03", "28/03", "29/03", "30/03", "31/03",
      "01/04", "02/04", "03/04", "04/04", "05/04", "06/04", "07/04", "08/04", "09/04", "10/04",
      "11/04", "12/04", "13/04", "14/04", "15/04", "16/04", "17/04", "18/04", "19/04", "20/04",
      "21/04", "22/04", "23/04", "24/04", "25/04", "26/04", "27/04", "28/04", "29/04", "30/04",
      "01/05", "02/05", "03/05", "04/05", "05/05", "06/05", "07/05", "08/05", "09/05", "10/05",
      "11/05", "12/05", "13/05", "14/05", "15/05", "16/05", "17/05", "18/05", "19/05", "20/05",
      "21/05", "22/05", "23/05", "24/05", "25/05", "26/05", "27/05", "28/05", "29/05", "30/05",
      "31/05", "01/06", "02/06", "03/06", "04/06", "05/06", "06/06", "07/06", "08/06", "09/06",
      "10/06", "11/06", "12/06", "13/06", "14/06", "15/06", "16/06", "17/06", "18/06", "19/06",
      "20/06", "21/06", "22/06", "23/06", "24/06", "25/06", "26/06", "27/06", "28/06", "29/06",
  };

  std::vector<double> data_gb = {
      15.87, 22.43, 16.27, 44.91, 11.01, 50.26, 59.18, 14.17, 57.10, 13.75, 48.10, 54.36, 39.50,
      13.09, 43.36, 65.06, 53.91, 55.09, 31.87, 56.41, 16.64, 64.69, 14.76, 49.52, 24.28, 21.49,
      47.77, 51.40, 64.41, 35.54, 50.35, 24.28, 55.12, 40.12, 14.03, 62.53, 44.83, 32.64, 51.42,
      16.82, 41.96, 25.19, 64.41, 11.22, 32.64, 55.06, 24.28, 38.47, 59.64, 18.10, 49.46, 42.96,
      27.08, 51.42, 62.49, 14.76, 35.54, 41.96, 58.25, 28.93, 54.23, 13.09, 43.36, 57.12, 59.18,
      31.87, 53.91, 16.27, 22.43, 39.50, 11.01, 48.10, 14.17, 50.26, 13.75, 55.09, 65.06, 16.64,
      44.91, 44.36, 36.01, 19.62, 52.57, 63.28, 21.49, 38.42, 14.03, 64.69, 49.52, 32.64, 44.83,
      16.82, 51.42, 9.19,  64.54, 41.83, 42.41, 44.13, 41.73, 12.67,
  };

  // Decimate 100 → 25 points with LTTB so the chart stays readable.
  // lttb_decimate_indexed returns the downsampled values and the original
  // index of each kept point, letting us slice the date label list to match.
  auto [dec_values, dec_indices] = lttb_decimate_indexed(data_gb, 25);

  std::vector<std::string> dec_dates;
  dec_dates.reserve(dec_indices.size());
  for (std::size_t i : dec_indices)
    dec_dates.push_back(dates[i]);

  LineChart chart;
  chart.options().width = 1200;
  chart.options().height = 500;
  chart.options().title = "Data Consumption Trend (100 Days)";
  chart.options().legend.position = LegendOptions::Position::Bottom;

  chart.set_categories(dec_dates);
  chart.add_series("Data Consumed", dec_values);

  chart.cartesian_options().x_axis_label = "Date (DD/MM)";
  chart.cartesian_options().y_axis_label = "Data Consumed (GB)";
  chart.cartesian_options().max_x_labels = 12;
  chart.cartesian_options().begin_at_zero = true;

  chart.line_options().show_points = true;
  chart.line_options().point_radius = 4.0f;
  chart.line_options().line_width = 2.5f;

  if (!ChartExporter::save(chart, out)) {
    std::fprintf(stderr, "save failed: %s\n", out.c_str());
    return 1;
  }
  std::printf("saved %zu points → %s\n", dec_values.size(), out.c_str());
  return 0;
}
