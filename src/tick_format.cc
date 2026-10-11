// SPDX-License-Identifier: MIT
// Copyright (c) InsightDock contributors

#include "insightdock/detail/tick_format.h"

#include <algorithm>
#include <cmath>
#include <ctime>
#include <iomanip>
#include <limits>
#include <locale>
#include <sstream>

namespace insightdock::charts::detail {

namespace {

std::ostringstream numeric_stream() {
  std::ostringstream out;
  out.imbue(std::locale::classic());
  return out;
}

std::string raw_number(double v) {
  auto out = numeric_stream();
  out << std::setprecision(15) << (v == 0.0 ? 0.0 : v);
  return out.str();
}

std::string utc_timestamp(double v) {
  // An exclusive power-of-two bound avoids casting rounded INT64_MAX back to time_t.
  double upper = std::ldexp(1.0, std::numeric_limits<std::time_t>::digits);
  double lower = std::numeric_limits<std::time_t>::is_signed ? -upper : 0.0;
  if (!std::isfinite(v) || v < lower || v >= upper)
    return {};
  std::time_t ts = static_cast<std::time_t>(v);
  std::tm tm_buf{};
#if defined(_WIN32)
  if (gmtime_s(&tm_buf, &ts) != 0)
    return {};
#else
  if (!gmtime_r(&ts, &tm_buf))
    return {};
#endif
  char buf[64];
  if (std::strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M", &tm_buf) == 0)
    return {};
  return buf;
}

} // namespace

std::string format_tick_value(double v, TickFormat format,
                              std::function<std::string(double)> const &custom_formatter) {
  if (custom_formatter)
    return custom_formatter(v);
  if (!std::isfinite(v))
    return {};
  if (v == 0.0)
    v = 0.0; // normalize negative zero without discarding small measurements

  auto out = numeric_stream();
  switch (format) {
  case TickFormat::SI: {
    char const *prefixes[] = {"", "K", "M", "G", "T"};
    double scaled = v;
    int tier = 0;
    while (std::fabs(scaled) >= 1000.0 && tier < 4) {
      scaled /= 1000.0;
      ++tier;
    }
    if (tier == 0)
      return raw_number(v);
    out << std::fixed << std::setprecision(std::fabs(scaled) < 10.0 ? 1 : 0) << scaled
        << prefixes[tier];
    break;
  }
  case TickFormat::Currency:
    out << '$' << std::fixed << std::setprecision(2) << v;
    break;
  case TickFormat::Percent:
    out << std::fixed << std::setprecision(1) << v << '%';
    break;
  case TickFormat::DateTime:
    return utc_timestamp(v);
  case TickFormat::Raw:
  default:
    return raw_number(v);
  }
  return out.str();
}

std::string format_axis_tick(double v, double spacing, TickFormat format,
                             std::function<std::string(double)> const &custom_formatter) {
  if (custom_formatter || format != TickFormat::Raw || !std::isfinite(v) ||
      !std::isfinite(spacing) || !(spacing > 0.0))
    return format_tick_value(v, format, custom_formatter);

  if ((v != 0.0 && (std::fabs(v) >= 1e9 || std::fabs(v) < 1e-6)) || spacing < 1e-6) {
    auto out = numeric_stream();
    // Retain enough significant digits to distinguish ticks on large-offset domains.
    out << std::setprecision(std::numeric_limits<double>::max_digits10) << (v == 0.0 ? 0.0 : v);
    return out.str();
  }
  int decimals =
      std::clamp(static_cast<int>(std::max(0.0, -std::floor(std::log10(spacing)))), 0, 15);
  double scaled = spacing * std::pow(10.0, decimals);
  while (decimals < 15 && std::fabs(scaled - std::round(scaled)) > 1e-8) {
    ++decimals;
    scaled *= 10.0;
  }
  if (std::fabs(v) < spacing * 1e-10)
    v = 0.0; // suppress tick noise relative to this axis, not a fixed unit
  auto out = numeric_stream();
  out << std::fixed << std::setprecision(decimals) << v;
  return out.str();
}

std::string format_label_value(double v, TickFormat format, int decimals) {
  if (decimals < 0)
    return format_tick_value(v, format, nullptr);
  if (!std::isfinite(v))
    return {};
  auto out = numeric_stream();
  if (format == TickFormat::Currency)
    out << '$';
  out << std::fixed << std::setprecision(std::min(decimals, 15)) << (v == 0.0 ? 0.0 : v);
  if (format == TickFormat::Percent)
    out << '%';
  return out.str();
}

} // namespace insightdock::charts::detail
