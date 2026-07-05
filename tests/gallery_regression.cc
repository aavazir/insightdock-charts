// SPDX-License-Identifier: MIT
// Copyright (c) InsightDock contributors

#include "insightdock/charts.h"
#include "insightdock/decimate.h"

#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <string>
#include <vector>

namespace {

int g_failures = 0;

void fail(char const *msg) {
  std::fprintf(stderr, "FAIL: %s\n", msg);
  ++g_failures;
}

bool read_file(std::string const &path, std::vector<uint8_t> &out) {
  std::ifstream in(path, std::ios::binary);
  if (!in)
    return false;
  in.seekg(0, std::ios::end);
  auto size = in.tellg();
  in.seekg(0, std::ios::beg);
  out.resize(static_cast<std::size_t>(size));
  in.read(reinterpret_cast<char *>(out.data()), size);
  return static_cast<bool>(in);
}

bool write_file(std::string const &path, std::vector<uint8_t> const &data) {
  std::ofstream out(path, std::ios::binary);
  if (!out)
    return false;
  out.write(reinterpret_cast<char const *>(data.data()), static_cast<std::streamsize>(data.size()));
  return static_cast<bool>(out);
}

bool png_pixel_diff(std::vector<uint8_t> const &a, std::vector<uint8_t> const &b,
                    double max_ratio) {
  if (a == b)
    return true;
  if (a.size() != b.size() || a.size() < 8)
    return false;
  std::size_t diffs = 0;
  for (std::size_t i = 0; i < a.size(); ++i) {
    if (a[i] != b[i])
      ++diffs;
  }
  return static_cast<double>(diffs) / static_cast<double>(a.size()) <= max_ratio;
}

void check_chart(std::string const &name, insightdock::charts::Chart &chart,
                 std::string const &ref_dir, bool update) {
  std::string path = ref_dir + "/" + name;
  auto png = insightdock::charts::encode_png(chart.render());
  if (png.empty()) {
    fail((name + " render failed").c_str());
    return;
  }

  if (update) {
    if (!write_file(path, png)) {
      fail((name + " write failed").c_str());
      return;
    }
    std::fprintf(stderr, "Updated %s\n", path.c_str());
    return;
  }

  std::vector<uint8_t> ref;
  if (!read_file(path, ref)) {
    fail((name + " missing reference — run with UPDATE_GALLERY=1").c_str());
    return;
  }

  if (!png_pixel_diff(png, ref, 0.001)) {
    fail((name + " differs from reference").c_str());
  }
}

} // namespace

int main(int argc, char **argv) {
  std::string ref_dir = "docs/gallery";
  bool update = false;
  if (const char *env = std::getenv("UPDATE_GALLERY")) {
    if (env[0] == '1' || env[0] == 'y' || env[0] == 'Y')
      update = true;
  }
  if (argc > 1)
    ref_dir = argv[1];

  insightdock::charts::PieChart pie;
  pie.options().width = 600;
  pie.options().height = 600;
  pie.options().title = "Budget Split";
  pie.add_slice("Revenue", 100.0);
  pie.add_slice("Expenses", 60.0);
  pie.add_slice("Profit", 40.0);
  check_chart("pie_chart.png", pie, ref_dir, update);

  insightdock::charts::BarChart bar;
  bar.options().width = 640;
  bar.options().height = 400;
  bar.options().title = "Quarterly Revenue";
  bar.set_categories({"Q1", "Q2", "Q3", "Q4"});
  bar.add_series("Revenue", {100, 120, 90, 140});
  bar.add_series("Cost", {60, 70, 65, 80});
  check_chart("bar_grouped.png", bar, ref_dir, update);

  insightdock::charts::BarChart stacked;
  stacked.options().width = 640;
  stacked.options().height = 400;
  stacked.options().title = "Stacked Quarterly";
  stacked.set_categories({"Q1", "Q2", "Q3", "Q4"});
  stacked.add_series("Product A", {40, 50, 35, 60});
  stacked.add_series("Product B", {30, 25, 40, 35});
  stacked.add_series("Product C", {20, 30, 25, 30});
  stacked.set_mode(insightdock::charts::BarMode::Stacked);
  check_chart("bar_stacked.png", stacked, ref_dir, update);

  insightdock::charts::LineChart line;
  line.options().width = 640;
  line.options().height = 400;
  line.options().title = "Trend";
  line.set_categories({"Jan", "Feb", "Mar", "Apr", "May"});
  line.add_series("Series A", {10, 25, 18, 32, 28});
  line.add_series("Series B", {15, 12, 22, 20, 35});
  line.line_options().fill_area = true;
  check_chart("line_chart.png", line, ref_dir, update);

  insightdock::charts::AreaChart area;
  area.options().width = 640;
  area.options().height = 400;
  area.options().title = "Traffic";
  area.set_categories({"Mon", "Tue", "Wed", "Thu", "Fri"});
  area.add_series("Download", {120, 180, 150, 220, 190});
  area.add_series("Upload", {40, 55, 48, 70, 62});
  area.area_options().stacked = true;
  check_chart("area_stacked.png", area, ref_dir, update);

  insightdock::charts::ComboChart combo;
  combo.options().width = 640;
  combo.options().height = 400;
  combo.options().title = "Daily Active Users & Avg. Session Duration";
  combo.cartesian_options().y_axis_label = "Active Users";
  combo.cartesian_options().y2_axis_label = "Avg. Duration (min)";
  combo.set_categories({"Mon", "Tue", "Wed", "Thu", "Fri"});
  combo.add_bar_series("Active Users", {320, 415, 390, 445, 510});
  combo.add_line_series("Avg. Duration (min)", {4.2, 5.1, 3.8, 6.0, 5.5},
                        insightdock::charts::Color::rgb(192, 75, 75), /*secondary_axis=*/true);
  check_chart("combo_chart.png", combo, ref_dir, update);

  insightdock::charts::ScatterChart scatter;
  scatter.options().width = 640;
  scatter.options().height = 400;
  scatter.options().title = "Latency vs Throughput";
  scatter.scatter_options().x_axis_label = "Throughput (Mbps)";
  scatter.scatter_options().y_axis_label = "Latency (ms)";
  scatter.add_series("Device A", {{12, 40}, {25, 35}, {40, 28}, {55, 30}, {70, 18}, {90, 15}});
  scatter.add_series("Device B", {{15, 55}, {30, 48}, {48, 44}, {60, 38}, {78, 33}, {95, 25}});
  check_chart("scatter_chart.png", scatter, ref_dir, update);

  insightdock::charts::ScatterChart bubble;
  bubble.options().width = 640;
  bubble.options().height = 400;
  bubble.options().title = "Site Traffic (bubble size: active users)";
  bubble.scatter_options().x_axis_label = "Avg session (min)";
  bubble.scatter_options().y_axis_label = "Bounce rate (%)";
  bubble.add_series("Sites", {
                                 {3.2, 42, 120},
                                 {5.1, 35, 480},
                                 {2.0, 58, 90},
                                 {7.4, 22, 950},
                                 {4.6, 30, 300},
                                 {1.5, 65, 60},
                             });
  check_chart("bubble_chart.png", bubble, ref_dir, update);

  {
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
    auto [dec_values, dec_indices] = insightdock::charts::lttb_decimate_indexed(data_gb, 25);
    std::vector<std::string> dec_dates;
    for (std::size_t i : dec_indices)
      dec_dates.push_back(dates[i]);

    insightdock::charts::LineChart ts;
    ts.options().width = 1200;
    ts.options().height = 500;
    ts.options().title = "Data Consumption Trend (100 Days)";
    ts.options().legend.position = insightdock::charts::LegendOptions::Position::Bottom;
    ts.set_categories(dec_dates);
    ts.add_series("Data Consumed", dec_values);
    ts.cartesian_options().x_axis_label = "Date (DD/MM)";
    ts.cartesian_options().y_axis_label = "Data Consumed (GB)";
    ts.cartesian_options().max_x_labels = 12;
    ts.cartesian_options().begin_at_zero = true;
    ts.line_options().show_points = true;
    ts.line_options().point_radius = 4.0f;
    ts.line_options().line_width = 2.5f;
    check_chart("time_series_line.png", ts, ref_dir, update);
  }

  if (g_failures > 0) {
    std::fprintf(stderr, "%d gallery regression failure(s)\n", g_failures);
    return 1;
  }
  std::fprintf(stderr, "gallery regression OK\n");
  return 0;
}
