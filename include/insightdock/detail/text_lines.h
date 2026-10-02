// SPDX-License-Identifier: MIT
// Copyright (c) InsightDock contributors
#pragma once

#include "insightdock/draw_target.h"
#include "insightdock/font.h"

#include <algorithm>
#include <string>

namespace insightdock::charts::detail {

/** @brief Number of '\n'-separated lines in @p text (at least 1). */
inline std::size_t line_count(std::string const &text) {
  return static_cast<std::size_t>(std::count(text.begin(), text.end(), '\n')) + 1;
}

/**
 * @brief Draws possibly multi-line text, aligning each line independently.
 *
 * (x, y) anchors the whole block with @p baseline: Top/Alphabetic anchor the first line, Bottom
 * the last line, Middle the block's centre. A single line is forwarded unchanged.
 */
inline void draw_text_lines(DrawTarget &target, Font const &font, float x, float y,
                            std::string const &text, Color const &color, TextAlign align,
                            TextBaseline baseline) {
  if (text.find('\n') == std::string::npos) {
    target.draw_text(font, x, y, text, color, align, baseline);
    return;
  }
  float lh = font.line_height();
  float n = static_cast<float>(line_count(text));
  float top = y;
  if (baseline == TextBaseline::Bottom)
    top = y - (n - 1.0f) * lh;
  else if (baseline == TextBaseline::Middle)
    top = y - (n - 1.0f) * lh * 0.5f;
  std::size_t start = 0;
  for (float i = 0.0f;; i += 1.0f) {
    std::size_t nl = text.find('\n', start);
    std::string line = text.substr(start, nl == std::string::npos ? nl : nl - start);
    target.draw_text(font, x, top + i * lh, line, color, align, baseline);
    if (nl == std::string::npos)
      break;
    start = nl + 1;
  }
}

} // namespace insightdock::charts::detail
