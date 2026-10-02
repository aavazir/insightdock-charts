// SPDX-License-Identifier: MIT
// Copyright (c) InsightDock contributors

#include "insightdock/scale.h"

#include <algorithm>
#include <cmath>

namespace insightdock::charts {

namespace {

// Hard caps keep tick generation bounded for pathological domains (huge ranges, tiny steps).
constexpr int kMaxTicks = 1000;

bool finite_domain(double lo, double hi) {
  return std::isfinite(lo) && std::isfinite(hi);
}

double nice_num(double range, bool round) {
  if (!(range > 0.0) || !std::isfinite(range))
    return 1.0;
  double exp = std::floor(std::log10(range));
  double f = range / std::pow(10.0, exp);
  double nf;
  if (round) {
    if (f < 1.5)
      nf = 1.0;
    else if (f < 3.0)
      nf = 2.0;
    else if (f < 7.0)
      nf = 5.0;
    else
      nf = 10.0;
  } else {
    if (f <= 1.0)
      nf = 1.0;
    else if (f <= 2.0)
      nf = 2.0;
    else if (f <= 5.0)
      nf = 5.0;
    else
      nf = 10.0;
  }
  return nf * std::pow(10.0, exp);
}

double clamp_positive(double v, double floor_val) {
  return std::max(v, floor_val);
}

double domain_t(ScaleType type, double min_val, double max_val, double value) {
  switch (type) {
  case ScaleType::Log: {
    double lo = clamp_positive(min_val, 1e-300);
    double hi = clamp_positive(max_val, lo * 1.001);
    double v = clamp_positive(value, lo);
    double log_lo = std::log10(lo);
    double log_hi = std::log10(hi);
    if (log_hi == log_lo)
      return 0.5;
    return (std::log10(v) - log_lo) / (log_hi - log_lo);
  }
  case ScaleType::DateTime:
  case ScaleType::Linear:
  default:
    if (max_val == min_val)
      return 0.5;
    return (value - min_val) / (max_val - min_val);
  }
}

double value_from_t(ScaleType type, double min_val, double max_val, double t) {
  switch (type) {
  case ScaleType::Log: {
    double lo = clamp_positive(min_val, 1e-300);
    double hi = clamp_positive(max_val, lo * 1.001);
    double log_lo = std::log10(lo);
    double log_hi = std::log10(hi);
    if (log_hi == log_lo)
      return lo;
    return std::pow(10.0, log_lo + t * (log_hi - log_lo));
  }
  case ScaleType::DateTime:
  case ScaleType::Linear:
  default:
    if (max_val == min_val)
      return min_val;
    return min_val + t * (max_val - min_val);
  }
}

} // namespace

Scale::Scale(double min_val, double max_val, float pixel_min, float pixel_max)
    : min_(min_val), max_(max_val), pixel_min_(pixel_min), pixel_max_(pixel_max) {
  if (!finite_domain(min_, max_)) {
    min_ = 0.0;
    max_ = 1.0;
  }
}

void Scale::set_domain(double min_val, double max_val) {
  if (!finite_domain(min_val, max_val)) {
    min_val = 0.0;
    max_val = 1.0;
  }
  min_ = min_val;
  max_ = max_val;
}

void Scale::set_range(float pixel_min, float pixel_max) {
  pixel_min_ = pixel_min;
  pixel_max_ = pixel_max;
}

float Scale::to_pixel(double value) const {
  if (max_ == min_ && type_ != ScaleType::Log)
    return (pixel_min_ + pixel_max_) * 0.5f;
  double t = domain_t(type_, min_, max_, value);
  return pixel_max_ - static_cast<float>(t) * (pixel_max_ - pixel_min_);
}

double Scale::from_pixel(float pixel) const {
  if (pixel_max_ == pixel_min_)
    return min_;
  float t = (pixel_max_ - pixel) / (pixel_max_ - pixel_min_);
  return value_from_t(type_, min_, max_, t);
}

std::vector<double> Scale::nice_ticks(int max_ticks) const {
  std::vector<double> ticks;
  max_ticks = std::max(1, std::min(max_ticks, kMaxTicks));
  if (max_ <= min_ && type_ != ScaleType::Log) {
    ticks.push_back(min_);
    return ticks;
  }

  if (type_ == ScaleType::Log) {
    double lo = clamp_positive(min_, 1e-300);
    lo = std::min(lo, 1e300);
    double hi = std::min(clamp_positive(max_, lo * 10.0), 1e308);
    int exp_lo = static_cast<int>(std::floor(std::log10(lo)));
    int exp_hi = static_cast<int>(std::ceil(std::log10(hi)));
    for (int e = exp_lo; e <= exp_hi; ++e) {
      for (double m : {1.0, 2.0, 5.0}) {
        double v = m * std::pow(10.0, e);
        if (v >= lo * 0.999 && v <= hi * 1.001)
          ticks.push_back(v);
      }
    }
    if (ticks.empty()) {
      ticks.push_back(lo);
      ticks.push_back(hi);
    }
    return ticks;
  }

  if (type_ == ScaleType::DateTime) {
    double range = max_ - min_;
    if (range <= 0) {
      ticks.push_back(min_);
      return ticks;
    }
    static double const steps[] = {1.0,      2.0,      5.0,       10.0,      15.0,      30.0,
                                   60.0,     120.0,    300.0,     600.0,     900.0,     1800.0,
                                   3600.0,   7200.0,   14400.0,   21600.0,   43200.0,   86400.0,
                                   172800.0, 604800.0, 2592000.0, 7776000.0, 15552000.0};
    double step = steps[0];
    for (double s : steps) {
      step = s;
      if (range / s <= static_cast<double>(max_ticks))
        break;
    }
    double start = std::ceil(min_ / step) * step;
    double limit = max_ + step * 0.5;
    for (int i = 0; i < kMaxTicks && start + static_cast<double>(i) * step <= limit; ++i) {
      double v = start + static_cast<double>(i) * step;
      if (v >= min_ - step * 0.5 && v <= limit)
        ticks.push_back(v);
    }
    if (ticks.empty()) {
      ticks.push_back(min_);
      ticks.push_back(max_);
    }
    return ticks;
  }

  double range = nice_num(max_ - min_, false);
  double step = nice_num(range / static_cast<double>(max_ticks), true);
  double start = std::ceil(min_ / step) * step;
  double limit = max_ + step * 0.5;
  for (int i = 0; i < kMaxTicks && start + static_cast<double>(i) * step <= limit; ++i) {
    double v = start + static_cast<double>(i) * step;
    if (v >= min_ - step * 0.5 && v <= limit)
      ticks.push_back(v);
  }
  if (ticks.empty()) {
    ticks.push_back(min_);
    ticks.push_back(max_);
  }
  return ticks;
}

Scale Scale::linear_nice(double data_min, double data_max, float pixel_min, float pixel_max,
                         int max_ticks) {
  if (!finite_domain(data_min, data_max)) {
    data_min = 0.0;
    data_max = 1.0;
  }
  if (data_max <= data_min) {
    if (data_max == data_min) {
      data_min -= 1.0;
      data_max += 1.0;
    } else {
      std::swap(data_min, data_max);
    }
  }
  double range = data_max - data_min;
  double padding = range * 0.05;
  if (padding == 0.0)
    padding = 1.0;
  data_min -= padding;
  data_max += padding;

  Scale s(data_min, data_max, pixel_min, pixel_max);
  auto ticks = s.nice_ticks(max_ticks);
  if (!ticks.empty()) {
    double nice_min = ticks.front();
    double nice_max = ticks.back();
    // If ticks stop short of data_max, extend by one step so the peak data
    // point sits between two labelled gridlines rather than above the last one.
    if (nice_max < data_max && ticks.size() >= 2) {
      double step = ticks.back() - ticks[ticks.size() - 2];
      for (int i = 0; i < kMaxTicks && step > 0.0 && nice_max < data_max; ++i)
        nice_max += step;
    }
    s.set_domain(nice_min, nice_max);
  }
  return s;
}

Scale Scale::log_nice(double data_min, double data_max, float pixel_min, float pixel_max,
                      int max_ticks) {
  if (!finite_domain(data_min, data_max)) {
    data_min = 1.0;
    data_max = 10.0;
  }
  data_min = clamp_positive(data_min, 1e-300);
  data_max = clamp_positive(data_max, data_min * 1.001);
  if (data_max <= data_min)
    data_max = data_min * 10.0;

  Scale s(data_min, data_max, pixel_min, pixel_max);
  s.set_type(ScaleType::Log);
  auto ticks = s.nice_ticks(max_ticks);
  if (!ticks.empty()) {
    // nice_ticks() only returns 1-2-5-mantissa values inside [data_min, data_max]; unlike
    // linear_nice's padded range, that set doesn't necessarily reach the actual endpoints,
    // so extend (never shrink) to guarantee the domain still covers all real data.
    s.set_domain(std::min(data_min, ticks.front()), std::max(data_max, ticks.back()));
  }
  return s;
}

} // namespace insightdock::charts
