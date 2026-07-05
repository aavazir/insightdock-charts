// SPDX-License-Identifier: MIT
// Copyright (c) InsightDock contributors

#include "insightdock/decimate.h"

#include <cmath>

namespace insightdock::charts {

std::vector<DecimationPoint> lttb_decimate(std::vector<DecimationPoint> const &points,
                                           std::size_t threshold) {
  std::size_t n = points.size();
  if (threshold == 0 || threshold < 3 || threshold >= n)
    return points;

  std::vector<DecimationPoint> sampled;
  sampled.reserve(threshold);
  sampled.push_back(points[0]);

  double const every = static_cast<double>(n - 2) / static_cast<double>(threshold - 2);
  std::size_t a = 0;

  for (std::size_t i = 0; i < threshold - 2; ++i) {
    std::size_t avg_start = static_cast<std::size_t>(std::floor((i + 1) * every)) + 1;
    std::size_t avg_end = static_cast<std::size_t>(std::floor((i + 2) * every)) + 1;
    if (avg_end > n)
      avg_end = n;

    double avg_x = 0.0;
    double avg_y = 0.0;
    std::size_t avg_len = avg_end > avg_start ? avg_end - avg_start : 0;
    for (std::size_t j = avg_start; j < avg_end; ++j) {
      avg_x += points[j].x;
      avg_y += points[j].y;
    }
    if (avg_len > 0) {
      avg_x /= static_cast<double>(avg_len);
      avg_y /= static_cast<double>(avg_len);
    } else {
      avg_x = points[a].x;
      avg_y = points[a].y;
    }

    std::size_t bucket_start = static_cast<std::size_t>(std::floor((i + 0) * every)) + 1;
    std::size_t bucket_end = static_cast<std::size_t>(std::floor((i + 1) * every)) + 1;

    double point_a_x = points[a].x;
    double point_a_y = points[a].y;

    double max_area = -1.0;
    std::size_t next_a = bucket_start;
    for (std::size_t j = bucket_start; j < bucket_end; ++j) {
      double area = std::fabs((point_a_x - avg_x) * (points[j].y - point_a_y) -
                              (point_a_x - points[j].x) * (avg_y - point_a_y)) *
                    0.5;
      if (area > max_area) {
        max_area = area;
        next_a = j;
      }
    }

    sampled.push_back(points[next_a]);
    a = next_a;
  }

  sampled.push_back(points[n - 1]);
  return sampled;
}

std::pair<std::vector<double>, std::vector<std::size_t>>
lttb_decimate_indexed(std::vector<double> const &values, std::size_t threshold) {
  std::size_t n = values.size();
  if (threshold == 0 || threshold < 3 || threshold >= n) {
    std::vector<std::size_t> idx(n);
    for (std::size_t i = 0; i < n; ++i)
      idx[i] = i;
    return {values, idx};
  }

  std::vector<DecimationPoint> points;
  points.reserve(n);
  for (std::size_t i = 0; i < n; ++i)
    points.push_back({static_cast<double>(i), values[i]});

  auto decimated = lttb_decimate(points, threshold);

  std::vector<double> out_values;
  std::vector<std::size_t> out_indices;
  out_values.reserve(decimated.size());
  out_indices.reserve(decimated.size());
  for (auto const &p : decimated) {
    out_indices.push_back(static_cast<std::size_t>(p.x + 0.5));
    out_values.push_back(p.y);
  }
  return {std::move(out_values), std::move(out_indices)};
}

} // namespace insightdock::charts
