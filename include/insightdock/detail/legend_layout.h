// SPDX-License-Identifier: MIT
// Copyright (c) InsightDock contributors
#pragma once

#include "insightdock/detail/text_wrap.h"
#include "insightdock/font.h"

#include <algorithm>
#include <string>
#include <vector>

namespace insightdock::charts::detail {

struct LegendItem {
  std::size_t index;
  float x = 0, y = 0, width = 0, height = 0;
  std::vector<std::string> lines;
};

struct LegendLayout {
  std::vector<LegendItem> items;
  float width = 0, height = 0;
};

// Positions are relative to the legend origin. index == labels.size() denotes the overflow
// indicator. Keep complete entries in input order, reserving one line for the hidden count.
inline LegendLayout layout_legend(std::vector<std::string> const &labels, Font const &font,
                                  float max_width, float max_height, bool vertical) {
  LegendLayout result;
  float line = std::max(10.0f, font.line_height());
  constexpr float swatch = 16.0f, gap = 16.0f, row_gap = 8.0f;
  if (labels.empty() || max_width <= swatch || max_height < line)
    return result;
  auto measure = [&](std::string const &s) { return font.measure_width(s); };
  float x = 0, y = 0, row_height = 0;
  bool overflow = false;
  for (std::size_t i = 0; i < labels.size(); ++i) {
    auto lines = wrap_text(labels[i], max_width - swatch, measure);
    if (lines.empty())
      lines.emplace_back();
    float width = swatch;
    for (auto const &s : lines)
      width = std::max(width, swatch + measure(s));
    float height = line * static_cast<float>(lines.size());
    if (width > max_width) {
      overflow = true;
      break;
    }
    if (x > 0 && (vertical || x + width > max_width)) {
      y += row_height + row_gap;
      x = 0;
      row_height = 0;
    }
    result.items.push_back({i, x, y, width, height, std::move(lines)});
    row_height = std::max(row_height, height);
    x += width + gap;
  }
  if (overflow || (!result.items.empty() && y + row_height > max_height)) {
    // Put the count on its own final row; account for tall entries in every retained row
    // and for wrapping of the count itself on narrow canvases.
    for (;;) {
      float bottom = 0;
      for (auto const &item : result.items)
        bottom = std::max(bottom, item.y + item.height);
      auto hidden = labels.size() - result.items.size();
      auto lines = wrap_text("+" + std::to_string(hidden) + " more", max_width - swatch, measure);
      float height = line * static_cast<float>(lines.size());
      float count_y = bottom > 0 ? bottom + row_gap : 0;
      if (hidden > 0 && count_y + height <= max_height) {
        float width = swatch;
        for (auto const &text : lines)
          width = std::max(width, swatch + measure(text));
        if (width <= max_width)
          result.items.push_back({labels.size(), 0, count_y, width, height, std::move(lines)});
        break;
      }
      if (result.items.empty())
        break;
      result.items.pop_back();
    }
  }
  for (auto const &item : result.items) {
    result.width = std::max(result.width, item.x + item.width);
    result.height = std::max(result.height, item.y + item.height);
  }
  // Center each horizontal row using its measured width.
  if (!vertical) {
    for (std::size_t first = 0; first < result.items.size();) {
      auto end = first + 1;
      while (end < result.items.size() && result.items[end].y == result.items[first].y)
        ++end;
      float row_width = result.items[end - 1].x + result.items[end - 1].width;
      float offset = (max_width - row_width) * 0.5f;
      for (auto i = first; i < end; ++i)
        result.items[i].x += offset;
      first = end;
    }
    result.width = max_width;
  }
  return result;
}

} // namespace insightdock::charts::detail
