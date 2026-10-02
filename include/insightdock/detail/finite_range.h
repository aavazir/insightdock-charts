// SPDX-License-Identifier: MIT
// Copyright (c) InsightDock contributors
#pragma once

#include <algorithm>
#include <cmath>

namespace insightdock::charts::detail {

// Running min/max over the finite values fed to add(). NaN and +/-Inf are ignored so a single
// bad sample can neither poison an axis domain nor stretch it to infinity.
struct FiniteRange {
  double lo = 0.0;
  double hi = 0.0;
  bool any = false;

  void add(double v) {
    if (!std::isfinite(v))
      return;
    if (!any) {
      lo = hi = v;
      any = true;
    } else {
      lo = std::min(lo, v);
      hi = std::max(hi, v);
    }
  }

  // Lowest value, or 0 when no finite sample was seen.
  double min_or_zero() const {
    return any ? lo : 0.0;
  }
  // Highest value clamped up to 0 (bars/areas always include the baseline), 0 when empty.
  double max_from_zero() const {
    return any ? std::max(0.0, hi) : 0.0;
  }
};

} // namespace insightdock::charts::detail
