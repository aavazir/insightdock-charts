// SPDX-License-Identifier: MIT
// Copyright (c) InsightDock contributors

#include "insightdock/font.h"

#ifndef INSIGHTDOCK_NO_EMBEDDED_FONT
#ifdef INSIGHTDOCK_FONT_LATIN
#include "insightdock/embedded_font_latin.h"
#else
#include "insightdock/embedded_font.h"
#endif
#endif

#define STB_TRUETYPE_IMPLEMENTATION
#define STBTT_STATIC
#include "../third_party/stb_truetype.h"

#include <algorithm>
#include <cmath>
#include <cstring>
#include <fstream>

namespace insightdock::charts {

struct Font::FontState {
  stbtt_fontinfo info{};
  float scale = 0.0f;
  int ascent = 0;
  int descent = 0;
  int line_gap = 0;
};

namespace {

constexpr std::size_t kMaxFontBytes = std::size_t{64} << 20;

bool parseable(unsigned char const *data, std::size_t size) {
  if (!data || size < 12 || size > kMaxFontBytes)
    return false;
  int off = stbtt_GetFontOffsetForIndex(data, 0);
  if (off < 0)
    return false;
  stbtt_fontinfo info{};
  return stbtt_InitFont(&info, data, off) != 0;
}

// Decodes one UTF-8 code point starting at text[i] and advances i. Malformed sequences yield
// U+FFFD (consuming one byte) so the caller never loops or reads past the end.
int next_codepoint(std::string const &text, std::size_t &i) {
  unsigned char c = static_cast<unsigned char>(text[i++]);
  if (c < 0x80)
    return c;
  int extra;
  int cp;
  if ((c & 0xE0) == 0xC0) {
    extra = 1;
    cp = c & 0x1F;
  } else if ((c & 0xF0) == 0xE0) {
    extra = 2;
    cp = c & 0x0F;
  } else if ((c & 0xF8) == 0xF0) {
    extra = 3;
    cp = c & 0x07;
  } else {
    return 0xFFFD;
  }
  if (i + static_cast<std::size_t>(extra) > text.size())
    return 0xFFFD;
  for (int k = 0; k < extra; ++k) {
    unsigned char cc = static_cast<unsigned char>(text[i + static_cast<std::size_t>(k)]);
    if ((cc & 0xC0) != 0x80)
      return 0xFFFD;
    cp = (cp << 6) | (cc & 0x3F);
  }
  i += static_cast<std::size_t>(extra);
  static constexpr int kMin[] = {0, 0x80, 0x800, 0x10000};
  if (cp < kMin[extra] || cp > 0x10FFFF || (cp >= 0xD800 && cp <= 0xDFFF))
    return 0xFFFD;
  return cp;
}

} // namespace

FontDataPtr load_font_data(char const *path) {
  if (!path)
    return nullptr;
  std::ifstream ifs(path, std::ios::binary);
  if (!ifs)
    return nullptr;
  auto data = std::make_shared<FontData>(std::istreambuf_iterator<char>(ifs),
                                         std::istreambuf_iterator<char>());
  if (!parseable(data->data(), data->size()))
    return nullptr;
  return data;
}

FontDataPtr make_font_data(unsigned char const *data, std::size_t size) {
  if (!parseable(data, size))
    return nullptr;
  return std::make_shared<FontData>(data, data + size);
}

Font::Font() : Font(FontDataPtr{}) {}

Font::Font(FontDataPtr data) : ttf_(std::move(data)) {
  if (!ttf_) {
#ifndef INSIGHTDOCK_NO_EMBEDDED_FONT
    load_from_memory(embedded_font::data, embedded_font::size);
#endif
    // Otherwise no embedded font: call load_from_file() before drawing text.
    return;
  }
  init_font();
}

Font::Font(std::vector<unsigned char> ttf_data)
    : ttf_(std::make_shared<FontData>(std::move(ttf_data))) {
  init_font();
}

bool Font::load_from_file(char const *path) {
  auto data = load_font_data(path);
  if (!data)
    return false;
  ttf_ = std::move(data);
  return init_font();
}

bool Font::load_from_memory(unsigned char const *data, std::size_t size) {
  if (!data || size > kMaxFontBytes)
    return false;
  ttf_ = std::make_shared<FontData>(data, data + size);
  return init_font();
}

void Font::set_size(float pixel_size) {
  pixel_size_ = pixel_size;
  if (loaded_)
    init_font();
}

bool Font::init_font() {
  delete state_;
  state_ = nullptr;
  loaded_ = false;
  if (!ttf_ || ttf_->empty())
    return false;

  int off = stbtt_GetFontOffsetForIndex(ttf_->data(), 0);
  state_ = new FontState();
  if (off < 0 || !stbtt_InitFont(&state_->info, ttf_->data(), off)) {
    delete state_;
    state_ = nullptr;
    return false;
  }

  state_->scale = stbtt_ScaleForPixelHeight(&state_->info, pixel_size_);
  stbtt_GetFontVMetrics(&state_->info, &state_->ascent, &state_->descent, &state_->line_gap);
  loaded_ = true;
  return true;
}

// Maps a code point the font can't draw to '?' so missing glyphs are visible but harmless.
int Font::resolve_glyph(int cp) const {
  if (stbtt_FindGlyphIndex(&state_->info, cp) != 0)
    return cp;
  return '?';
}

float Font::line_height() const {
  if (!loaded_ || !state_)
    return pixel_size_;
  return (state_->ascent - state_->descent + state_->line_gap) * state_->scale;
}

float Font::baseline_offset(TextBaseline baseline) const {
  if (!loaded_ || !state_)
    return 0.0f;
  float asc = state_->ascent * state_->scale;
  float desc = state_->descent * state_->scale;
  switch (baseline) {
  case TextBaseline::Top:
    return asc;
  case TextBaseline::Middle:
    return asc * 0.5f;
  case TextBaseline::Bottom:
    return -desc;
  case TextBaseline::Alphabetic:
  default:
    return 0.0f;
  }
}

float Font::measure_width(std::string const &text) const {
  if (!loaded_ || !state_)
    return 0.0f;
  float best = 0.0f;
  float width = 0.0f;
  int prev = 0;
  std::size_t i = 0;
  while (i < text.size()) {
    int cp = next_codepoint(text, i);
    if (cp == '\n') {
      best = std::max(best, width);
      width = 0.0f;
      prev = 0;
      continue;
    }
    if (cp < 32)
      continue;
    cp = resolve_glyph(cp);
    int advance, lsb;
    stbtt_GetCodepointHMetrics(&state_->info, cp, &advance, &lsb);
    width += advance * state_->scale;
    if (prev != 0) {
      width += stbtt_GetCodepointKernAdvance(&state_->info, prev, cp) * state_->scale;
    }
    prev = cp;
  }
  return std::max(best, width);
}

void Font::draw_text(ImageBuffer &buffer, float x, float y, std::string const &text,
                     Color const &color, TextAlign align, TextBaseline baseline) const {
  if (!loaded_ || !state_ || text.empty())
    return;

  float width = measure_width(text);
  float bx = x;
  if (align == TextAlign::Center)
    bx = x - width * 0.5f;
  else if (align == TextAlign::Right)
    bx = x - width;

  float by = y + baseline_offset(baseline);
  // Non-finite or absurdly distant anchors can't land on the buffer; bail before any float->int
  // cast.
  constexpr float kFar = 1.0e7f;
  if (!std::isfinite(bx) || !std::isfinite(by) || std::fabs(bx) > kFar || std::fabs(by) > kFar)
    return;

  int prev = 0;
  float cursor_x = bx;

  std::size_t ti = 0;
  while (ti < text.size()) {
    int cp = next_codepoint(text, ti);
    if (cp == '\n') {
      cursor_x = bx;
      by += line_height();
      prev = 0;
      continue;
    }
    if (cp < 32)
      continue;
    cp = resolve_glyph(cp);

    int advance, lsb;
    stbtt_GetCodepointHMetrics(&state_->info, cp, &advance, &lsb);
    if (prev != 0) {
      cursor_x += stbtt_GetCodepointKernAdvance(&state_->info, prev, cp) * state_->scale;
    }

    int x0, y0, x1, y1;
    stbtt_GetCodepointBitmapBox(&state_->info, cp, state_->scale, state_->scale, &x0, &y0, &x1,
                                &y1);
    int bw = x1 - x0;
    int bh = y1 - y0;
    if (bw > 0 && bh > 0) {
      unsigned char *bitmap =
          stbtt_GetCodepointBitmap(&state_->info, 0, state_->scale, cp, &bw, &bh, nullptr, nullptr);
      if (bitmap) {
        float base_x = cursor_x + lsb * state_->scale;
        float base_y = by + y0;
        for (int py = 0; py < bh; ++py) {
          for (int px = 0; px < bw; ++px) {
            unsigned char cov = bitmap[py * bw + px];
            if (cov == 0)
              continue;
            float alpha = (color.a / 255.0f) * (cov / 255.0f);
            int ix = static_cast<int>(std::floor(base_x + px));
            int iy = static_cast<int>(std::floor(base_y + py));
            if (ix >= 0 && iy >= 0 && static_cast<std::size_t>(ix) < buffer.width() &&
                static_cast<std::size_t>(iy) < buffer.height()) {
              uint8_t sa = static_cast<uint8_t>(alpha * 255.0f + 0.5f);
              auto *p = buffer.pixel(static_cast<std::size_t>(ix), static_cast<std::size_t>(iy));
              // simple over blend
              float fa = sa / 255.0f;
              float da = p[3] / 255.0f;
              float out_a = fa + da * (1.0f - fa);
              if (out_a > 1e-6f) {
                p[0] =
                    static_cast<uint8_t>((color.r * fa + p[0] * da * (1.0f - fa)) / out_a + 0.5f);
                p[1] =
                    static_cast<uint8_t>((color.g * fa + p[1] * da * (1.0f - fa)) / out_a + 0.5f);
                p[2] =
                    static_cast<uint8_t>((color.b * fa + p[2] * da * (1.0f - fa)) / out_a + 0.5f);
                p[3] = static_cast<uint8_t>(out_a * 255.0f + 0.5f);
              }
            }
          }
        }
        stbtt_FreeBitmap(bitmap, nullptr);
      }
    }
    cursor_x += advance * state_->scale;
    prev = cp;
  }
}

void Font::blit_rotated(ImageBuffer &buffer, float cx, float cy, std::string const &text,
                        Color const &color, bool clockwise) const {
  if (!loaded_ || !state_ || text.empty())
    return;
  if (!std::isfinite(cx) || !std::isfinite(cy) || std::fabs(cx) > 1.0e7f || std::fabs(cy) > 1.0e7f)
    return;

  int iw = static_cast<int>(std::ceil(measure_width(text))) + 2;
  int ih = static_cast<int>(std::ceil(line_height())) + 2;

  ImageBuffer tmp(static_cast<std::size_t>(iw), static_cast<std::size_t>(ih));
  draw_text(tmp, 0.0f, 0.0f, text, color, TextAlign::Left, TextBaseline::Top);

  // Rotated dims: width=ih, height=iw (both directions swap dimensions).
  // CCW: original (px, py) → rotated (py, iw-1-px).
  // CW:  original (px, py) → rotated (ih-1-py, px).
  int dest_x0 = static_cast<int>(std::round(cx - ih * 0.5f));
  int dest_y0 = static_cast<int>(std::round(cy - iw * 0.5f));

  for (int py = 0; py < ih; ++py) {
    for (int px = 0; px < iw; ++px) {
      auto const *src = tmp.pixel(static_cast<std::size_t>(px), static_cast<std::size_t>(py));
      if (!src || src[3] == 0)
        continue;
      int dx = clockwise ? dest_x0 + (ih - 1 - py) : dest_x0 + py;
      int dy = clockwise ? dest_y0 + px : dest_y0 + (iw - 1 - px);
      if (dx < 0 || dy < 0 || static_cast<std::size_t>(dx) >= buffer.width() ||
          static_cast<std::size_t>(dy) >= buffer.height())
        continue;
      auto *dst = buffer.pixel(static_cast<std::size_t>(dx), static_cast<std::size_t>(dy));
      float fa = src[3] / 255.0f;
      float da = dst[3] / 255.0f;
      float out_a = fa + da * (1.0f - fa);
      if (out_a > 1e-6f) {
        dst[0] = static_cast<uint8_t>((src[0] * fa + dst[0] * da * (1.0f - fa)) / out_a + 0.5f);
        dst[1] = static_cast<uint8_t>((src[1] * fa + dst[1] * da * (1.0f - fa)) / out_a + 0.5f);
        dst[2] = static_cast<uint8_t>((src[2] * fa + dst[2] * da * (1.0f - fa)) / out_a + 0.5f);
        dst[3] = static_cast<uint8_t>(out_a * 255.0f + 0.5f);
      }
    }
  }
}

void Font::draw_text_rotated_ccw(ImageBuffer &buffer, float cx, float cy, std::string const &text,
                                 Color const &color) const {
  blit_rotated(buffer, cx, cy, text, color, /*clockwise=*/false);
}

void Font::draw_text_rotated_cw(ImageBuffer &buffer, float cx, float cy, std::string const &text,
                                Color const &color) const {
  blit_rotated(buffer, cx, cy, text, color, /*clockwise=*/true);
}

Font::TextMetrics Font::measure(std::string const &text, float size) const {
  if (!loaded_ || pixel_size_ <= 0.0f || size <= 0.0f)
    return {};
  // stb_truetype metrics scale proportionally with pixel_size, so we can
  // compute at the current size and scale to the requested size.
  float ratio = size / pixel_size_;
  return {measure_width(text) * ratio, line_height() * ratio};
}

Font::~Font() {
  delete state_;
  state_ = nullptr;
}

} // namespace insightdock::charts
