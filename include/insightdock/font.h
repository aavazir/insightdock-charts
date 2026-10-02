// SPDX-License-Identifier: MIT
// Copyright (c) InsightDock contributors

/**
 * @file font.h
 * @brief TrueType text rasterization for chart labels and titles.
 *
 * By default uses a small embedded font. Supply a full TTF via @ref load_font_data and
 * `ChartOptions::font` for wider Unicode coverage; text is decoded as UTF-8.
 */

#ifndef INSIGHTDOCK_CHARTS_FONT_H
#define INSIGHTDOCK_CHARTS_FONT_H

#include "insightdock/color.h"
#include "insightdock/image_buffer.h"
#include <cstddef>
#include <memory>
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

/** @brief Raw TTF file bytes. */
using FontData = std::vector<unsigned char>;
/** @brief Shared, immutable TTF bytes — cheap to copy into many charts and fonts. */
using FontDataPtr = std::shared_ptr<FontData const>;

/**
 * @brief Read and validate a TTF from disk for use as @ref ChartOptions::font.
 * @return Shared font bytes, or `nullptr` if the file is unreadable, too large (> 64 MiB) or
 *         not a parseable TrueType font.
 */
FontDataPtr load_font_data(char const *path);

/** @brief Copy and validate in-memory TTF bytes; `nullptr` if not a parseable TrueType font. */
FontDataPtr make_font_data(unsigned char const *data, std::size_t size);

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
  /**
   * @brief Construct from shared TTF bytes without copying them.
   * @param data Font bytes from @ref load_font_data; `nullptr` selects the embedded font.
   */
  explicit Font(FontDataPtr data);
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
   * @param text     UTF-8 string. Code points the font lacks are drawn as `?`.
   * @param color    Text color (alpha blended).
   * @param align    Horizontal alignment.
   * @param baseline Vertical alignment.
   */
  void draw_text(ImageBuffer &buffer, float x, float y, std::string const &text, Color const &color,
                 TextAlign align = TextAlign::Left,
                 TextBaseline baseline = TextBaseline::Alphabetic) const;

  /** @brief Measure text width in pixels at current font size (widest line if `\n` present). */
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
  int resolve_glyph(int codepoint) const;
  void blit_rotated(ImageBuffer &buffer, float cx, float cy, std::string const &text,
                    Color const &color, bool clockwise) const;

  FontDataPtr ttf_;
  float pixel_size_ = 14.0f;
  bool loaded_ = false;

  struct FontState;
  FontState *state_ = nullptr;
};

} // namespace insightdock::charts

#endif // INSIGHTDOCK_CHARTS_FONT_H
