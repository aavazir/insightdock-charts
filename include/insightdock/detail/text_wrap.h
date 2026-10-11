// SPDX-License-Identifier: MIT
// Copyright (c) InsightDock contributors
#pragma once

#include <string>
#include <utility>
#include <vector>

namespace insightdock::charts::detail {

/**
 * @brief Wrap words and long tokens using a caller-provided width measurement.
 *
 * Valid UTF-8 tokens are split only between code points. Explicit newlines preserve paragraph
 * boundaries, including empty lines. A code point wider than the limit occupies its own line.
 * This does not implement grapheme segmentation or complex-script shaping.
 */
template <typename Measure>
std::vector<std::string> wrap_text(std::string const &text, double max_width, Measure measure) {
  if (text.empty())
    return {};
  std::vector<std::string> lines;
  std::size_t paragraph_start = 0;
  while (paragraph_start <= text.size()) {
    auto end = text.find('\n', paragraph_start);
    auto paragraph_end = end == std::string::npos ? text.size() : end;
    std::string current;
    auto before = lines.size();
    std::size_t pos = paragraph_start;
    while (pos < paragraph_end) {
      if (text[pos] == ' ') {
        ++pos;
        continue;
      }
      auto token_end = text.find(' ', pos);
      if (token_end == std::string::npos || token_end > paragraph_end)
        token_end = paragraph_end;
      auto token = text.substr(pos, token_end - pos);
      if (!current.empty()) {
        auto candidate = current + ' ' + token;
        if (max_width <= 0.0 || measure(candidate) <= max_width) {
          current = std::move(candidate);
          pos = token_end;
          continue;
        }
        lines.push_back(std::move(current));
        current.clear();
      }
      for (std::size_t i = 0; i < token.size();) {
        auto next = i + 1;
        while (next < token.size() && (static_cast<unsigned char>(token[next]) & 0xC0) == 0x80)
          ++next;
        auto codepoint = token.substr(i, next - i);
        auto candidate = current + codepoint;
        if (!current.empty() && max_width > 0.0 && measure(candidate) > max_width) {
          lines.push_back(std::move(current));
          current = std::move(codepoint);
        } else {
          current = std::move(candidate);
        }
        i = next;
      }
      pos = token_end;
    }
    if (!current.empty() || lines.size() == before)
      lines.push_back(std::move(current));
    if (end == std::string::npos)
      break;
    paragraph_start = end + 1;
  }
  return lines;
}

} // namespace insightdock::charts::detail
