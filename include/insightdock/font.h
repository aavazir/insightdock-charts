// SPDX-License-Identifier: MIT
// Copyright (c) InsightDock contributors

/**
 * @file font.h
 * @brief TrueType text rasterization for chart labels and titles.
 *
 * By default uses a small embedded ASCII font. Load a full TTF via
 * @ref Font::load_from_file for Unicode labels.
 */

#ifndef INSIGHTDOCK_CHARTS_FONT_H
#define INSIGHTDOCK_CHARTS_FONT_H

#include "insightdock/color.h"
#include "insightdock/image_buffer.h"
#include <cstddef>
#include <string>
#include <vector>

namespace insightdock::charts {

/** @brief Horizontal text alignment relative to the anchor point. */
enum class TextAlign {
  Left,   ///< Anchor at left edge of text.
  Center, ///< Anchor at horizontal center.
  Right   ///< Anchor at right edge.
};

/** @brief Vertical text alignment relative to the anchor point. */
enum class TextBaseline {
  Top,       ///< Anchor at top of em box.
  Middle,    ///< Anchor at vertical center.
  Bottom,    ///< Anchor at bottom.
  Alphabetic ///< Anchor at alphabetic baseline (default).
};

/**
 * @brief TrueType font rasterizer (stb_truetype backend).
 *
 * Non-copyable. Construct with default embedded font or load custom TTF data.
 */
class Font {
public:
  /** @brief Construct with embedded ASCII-subset font (~20 KB). */
  Font();
  /** @brief Construct from in-memory TTF bytes. */
  explicit Font(std::vector<unsigned char> ttf_data);
  ~Font();
  Font(Font const &) = delete;
  Font &operator=(Font const &) = delete;

  /**
   * @brief Load a TrueType font from disk.
   * @param path Path to .ttf file.
   * @return `true` if loaded successfully.
   */
  bool load_from_file(char const *path);

  /**
   * @brief Load a TrueType font from memory.
   * @param data TTF file bytes.
   * @param size Length of @p data.
   * @return `true` if parsed successfully.
   */
  bool load_from_memory(unsigned char const *data, std::size_t size);

  /** @brief Whether a font is loaded and ready to draw. */
  bool is_loaded() const {
    return loaded_;
  }

  /** @brief Set pixel size for rendering (default 14). */
  void set_size(float pixel_size);
  /** @brief Current pixel size. */
  float size() const {
    return pixel_size_;
  }

  /**
   * @brief Draw text into an image buffer.
   * @param buffer   Target RGBA buffer.
   * @param x        Anchor X in pixels.
   * @param y        Anchor Y in pixels.
   * @param text     UTF-8 string (ASCII subset unless full TTF loaded).
   * @param color    Text color (alpha blended).
   * @param align    Horizontal alignment.
   * @param baseline Vertical alignment.
   */
  void draw_text(ImageBuffer &buffer, float x, float y, std::string const &text, Color const &color,
                 TextAlign align = TextAlign::Left,
                 TextBaseline baseline = TextBaseline::Alphabetic) const;

  /** @brief Measure text width in pixels at current font size. */
  float measure_width(std::string const &text) const;

  /**
   * @brief Draw text rotated 90° counter-clockwise (reads bottom-to-top).
   *
   * Used for y-axis labels. The text is centered on (@p cx, @p cy).
   */
  void draw_text_rotated_ccw(ImageBuffer &buffer, float cx, float cy, std::string const &text,
                             Color const &color) const;

  /**
   * @brief Draw text rotated 90° clockwise (reads top-to-bottom).
   *
   * Used for secondary (right-side) y-axis labels. The text is centered on
   * (@p cx, @p cy).
   */
  void draw_text_rotated_cw(ImageBuffer &buffer, float cx, float cy, std::string const &text,
                            Color const &color) const;

  /** @brief Line height in pixels at current font size. */
  float line_height() const;

  /** @brief Width and height of a text string at an explicit pixel size. */
  struct TextMetrics {
    float width = 0.0f;  ///< Pixel width of the string.
    float height = 0.0f; ///< Line height at @p size.
  };

  /**
   * @brief Measure text at an explicit pixel size without changing the font state.
   * @param text String to measure.
   * @param size Pixel size to use for the measurement.
   * @return Width and height at @p size.
   */
  TextMetrics measure(std::string const &text, float size) const;

private:
  bool init_font();
  float baseline_offset(TextBaseline baseline) const;
  void blit_rotated(ImageBuffer &buffer, float cx, float cy, std::string const &text,
                    Color const &color, bool clockwise) const;

  std::vector<unsigned char> ttf_data_;
  float pixel_size_ = 14.0f;
  bool loaded_ = false;

  struct FontState;
  FontState *state_ = nullptr;
};

} // namespace insightdock::charts

#endif // INSIGHTDOCK_CHARTS_FONT_H
