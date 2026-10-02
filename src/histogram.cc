// SPDX-License-Identifier: MIT
// Copyright (c) InsightDock contributors

#include "insightdock/histogram.h"

#include <algorithm>
#include <cmath>
#include <cstdio>

namespace insightdock::charts {

Histogram compute_histogram(std::vector<double> const &values, HistogramOptions const &options) {
  Histogram h;
  std::vector<double> data;
  data.reserve(values.size());
  for (double v : values) {
    if (std::isfinite(v))
      data.push_back(v);
  }
  if (data.empty())
    return h;

  double lo, hi;
  if (options.min_value < options.max_value) {
    lo = options.min_value;
    hi = options.max_value;
  } else {
    auto mm = std::minmax_element(data.begin(), data.end());
    lo = *mm.first;
    hi = *mm.second;
  }
  if (!(hi > lo)) // all samples equal: one unit-wide bin around the value
    hi = lo + 1.0;

  std::size_t bins = options.bins;
  if (bins == 0) {
    bins = 1 + static_cast<std::size_t>(std::ceil(std::log2(static_cast<double>(data.size()))));
    bins = std::min<std::size_t>(std::max<std::size_t>(bins, 1), 100);
  }
  bins = std::min<std::size_t>(bins, 10000);

  double width = (hi - lo) / static_cast<double>(bins);
  h.edges.resize(bins + 1);
  for (std::size_t i = 0; i <= bins; ++i)
    h.edges[i] = lo + width * static_cast<double>(i);
  h.edges[bins] = hi;
  h.counts.assign(bins, 0);

  for (double v : data) {
    if (v < lo || v > hi)
      continue;
    auto idx = static_cast<std::size_t>((v - lo) / width);
    if (idx >= bins) // v == hi lands in the last bin
      idx = bins - 1;
    ++h.counts[idx];
  }
  return h;
}

namespace {

std::string edge_label(double v) {
  char buf[32];
  std::snprintf(buf, sizeof(buf), "%.4g", v);
  return buf;
}

} // namespace

BarChart make_histogram_chart(std::vector<double> const &values, HistogramOptions const &options,
                              std::string series_name) {
  Histogram h = compute_histogram(values, options);
  BarChart chart;
  std::vector<std::string> cats;
  std::vector<double> counts;
  cats.reserve(h.counts.size());
  counts.reserve(h.counts.size());
  for (std::size_t i = 0; i < h.counts.size(); ++i) {
    cats.push_back(edge_label(h.edges[i]) + "-" + edge_label(h.edges[i + 1]));
    counts.push_back(static_cast<double>(h.counts[i]));
  }
  chart.set_categories(std::move(cats));
  chart.add_series(std::move(series_name), std::move(counts));
  chart.bar_options().group_gap = 0.0f;
  chart.bar_options().bar_gap = 0.0f;
  chart.bar_options().border_color = Color::white();
  chart.bar_options().border_width = 1.0f;
  chart.options().legend.enabled = false;
  return chart;
}

} // namespace insightdock::charts
