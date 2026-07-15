// SPDX-License-Identifier: MIT
// Copyright (c) InsightDock contributors
//
// Demonstrates a dense time-series LineChart with a real continuous DateTime x-axis:
//   - 100 daily data points (as Unix timestamps) decimated to 25 via LTTB
//   - CartesianOptions::x_scale_type = ScaleType::DateTime positions each point by its
//     actual timestamp (via set_x_values) rather than an evenly-spaced category slot —
//     the axis ticks/labels are generated and formatted automatically from the domain,
//     not hand-formatted date strings
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

  // 100 daily timestamps (Unix seconds, starting 2026-03-22 UTC) and corresponding
  // data-consumed values in GB.
  double const day = 86400.0;
  double const start_ts = 1774137600.0; // 2026-03-22 00:00:00 UTC
  std::vector<double> timestamps;
  timestamps.reserve(100);
  for (int i = 0; i < 100; ++i)
    timestamps.push_back(start_ts + static_cast<double>(i) * day);

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
  // index of each kept point, letting us slice the timestamp list to match.
  auto [dec_values, dec_indices] = lttb_decimate_indexed(data_gb, 25);

  std::vector<double> dec_timestamps;
  dec_timestamps.reserve(dec_indices.size());
  for (std::size_t i : dec_indices)
    dec_timestamps.push_back(timestamps[i]);

  LineChart chart;
  chart.options().width = 1200;
  chart.options().height = 500;
  chart.options().title = "Data Consumption Trend (100 Days)";
  chart.options().legend.position = LegendOptions::Position::Bottom;

  chart.set_x_values(dec_timestamps);
  chart.add_series("Data Consumed", dec_values);

  chart.cartesian_options().x_axis_label = "Date";
  chart.cartesian_options().y_axis_label = "Data Consumed (GB)";
  chart.cartesian_options().x_scale_type = ScaleType::DateTime;
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
