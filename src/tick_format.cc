// SPDX-License-Identifier: MIT
// Copyright (c) InsightDock contributors

#include "insightdock/detail/tick_format.h"

#include <cmath>
#include <cstdio>
#include <ctime>

namespace insightdock::charts::detail {

std::string format_tick_value(double v, TickFormat format,
                              std::function<std::string(double)> const &custom_formatter) {
  if (std::fabs(v) < 1e-9)
    v = 0.0; // prevent "-0" from floating-point rounding near zero

  if (custom_formatter)
    return custom_formatter(v);

  char buf[48];
  switch (format) {
  case TickFormat::SI: {
    const char *prefixes[] = {"", "K", "M", "G", "T"};
    double abs_v = std::fabs(v);
    double scaled = v;
    int tier = 0;
    while (abs_v >= 1000.0 && tier < 4) {
      scaled /= 1000.0;
      abs_v /= 1000.0;
      ++tier;
    }
    if (tier == 0)
      std::snprintf(buf, sizeof(buf), "%.0f", scaled);
    else if (abs_v < 10.0)
      std::snprintf(buf, sizeof(buf), "%.1f%s", scaled, prefixes[tier]);
    else
      std::snprintf(buf, sizeof(buf), "%.0f%s", scaled, prefixes[tier]);
    break;
  }
  case TickFormat::Currency:
    std::snprintf(buf, sizeof(buf), "$%.2f", v);
    break;
  case TickFormat::Percent:
    std::snprintf(buf, sizeof(buf), "%.1f%%", v);
    break;
  case TickFormat::DateTime: {
    std::time_t ts = static_cast<std::time_t>(v);
    std::tm tm_buf{};
#if defined(_WIN32)
    localtime_s(&tm_buf, &ts);
#else
    localtime_r(&ts, &tm_buf);
#endif
    std::strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M", &tm_buf);
    break;
  }
  case TickFormat::Raw:
  default:
    if (std::fabs(v) >= 1.0 || v == 0.0)
      std::snprintf(buf, sizeof(buf), "%.0f", v);
    else
      std::snprintf(buf, sizeof(buf), "%.2f", v);
    break;
  }
  return buf;
}

std::string format_label_value(double v, TickFormat format, int decimals) {
  if (decimals < 0)
    return format_tick_value(v, format, nullptr);

  if (std::fabs(v) < 1e-9)
    v = 0.0;
  char buf[48];
  std::snprintf(buf, sizeof(buf), "%.*f", decimals, v);
  std::string s = buf;
  switch (format) {
  case TickFormat::Percent:
    s += "%";
    break;
  case TickFormat::Currency:
    s = "$" + s;
    break;
  default:
    break;
  }
  return s;
}

} // namespace insightdock::charts::detail
