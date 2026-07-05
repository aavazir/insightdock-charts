// SPDX-License-Identifier: MIT
// Copyright (c) InsightDock contributors

#include "insightdock/image_buffer.h"
#include "insightdock/detail/math_constants.h"

#include <algorithm>
#include <cmath>
#include <cstring>

namespace insightdock::charts {

namespace {

void blend_pixel_raw(uint8_t *dst, uint8_t sr, uint8_t sg, uint8_t sb, uint8_t sa) {
  float fa = sa / 255.0f;
  float sr2 = sr * fa;
  float sg2 = sg * fa;
  float sb2 = sb * fa;

  float da = dst[3] / 255.0f;
  float dr = dst[0];
  float dg = dst[1];
  float db = dst[2];

  float out_a = fa + da * (1.0f - fa);
  if (out_a < 1e-6f) {
    dst[0] = 0;
    dst[1] = 0;
    dst[2] = 0;
    dst[3] = 0;
    return;
  }

  dst[0] = static_cast<uint8_t>((sr2 + dr * da * (1.0f - fa)) / out_a + 0.5f);
  dst[1] = static_cast<uint8_t>((sg2 + dg * da * (1.0f - fa)) / out_a + 0.5f);
  dst[2] = static_cast<uint8_t>((sb2 + db * da * (1.0f - fa)) / out_a + 0.5f);
  dst[3] = static_cast<uint8_t>(out_a * 255.0f + 0.5f);
}

void plot_aa(ImageBuffer &img, float x, float y, Color const &color, float coverage) {
  if (coverage <= 0.0f)
    return;
  int ix = static_cast<int>(std::floor(x));
  int iy = static_cast<int>(std::floor(y));
  if (ix < 0 || iy < 0 || static_cast<std::size_t>(ix) >= img.width() ||
      static_cast<std::size_t>(iy) >= img.height()) {
    return;
  }
  float alpha = (color.a / 255.0f) * std::min(1.0f, coverage);
  if (alpha <= 0.0f)
    return;
  uint8_t sa = static_cast<uint8_t>(alpha * 255.0f + 0.5f);
  blend_pixel_raw(img.pixel(static_cast<std::size_t>(ix), static_cast<std::size_t>(iy)), color.r,
                  color.g, color.b, sa);
}

// Xiaolin Wu style simplified: distance-to-segment coverage per pixel.
void draw_thick_segment(ImageBuffer &img, float x0, float y0, float x1, float y1, float width,
                        Color const &color) {
  float dx = x1 - x0;
  float dy = y1 - y0;
  float len = std::sqrt(dx * dx + dy * dy);
  if (len < 1e-6f) {
    plot_aa(img, x0, y0, color, 1.0f);
    return;
  }

  float nx = -dy / len;
  float ny = dx / len;
  float half = width * 0.5f;

  int steps = static_cast<int>(len * 2.0f) + 1;
  for (int i = 0; i <= steps; ++i) {
    float t = static_cast<float>(i) / static_cast<float>(steps);
    float cx = x0 + dx * t;
    float cy = y0 + dy * t;

    for (int o = -1; o <= 1; ++o) {
      for (int p = -1; p <= 1; ++p) {
        float ox = nx * half * static_cast<float>(o);
        float oy = ny * half * static_cast<float>(p);
        plot_aa(img, cx + ox, cy + oy, color, 0.85f);
      }
    }
  }
}

} // namespace

ImageBuffer::ImageBuffer() : width_(0), height_(0) {}

ImageBuffer::ImageBuffer(std::size_t width, std::size_t height)
    : width_(width), height_(height), data_(width * height * 4, 0) {}

uint8_t *ImageBuffer::pixel(std::size_t x, std::size_t y) {
  if (x >= width_ || y >= height_)
    return nullptr;
  return data_.data() + (y * width_ + x) * 4;
}

uint8_t const *ImageBuffer::pixel(std::size_t x, std::size_t y) const {
  if (x >= width_ || y >= height_)
    return nullptr;
  return data_.data() + (y * width_ + x) * 4;
}

void ImageBuffer::clear(Color const &color) {
  clear(color.r, color.g, color.b, color.a);
}

void ImageBuffer::clear(uint8_t r, uint8_t g, uint8_t b, uint8_t a) {
  for (std::size_t y = 0; y < height_; ++y) {
    for (std::size_t x = 0; x < width_; ++x) {
      auto *p = pixel(x, y);
      p[0] = r;
      p[1] = g;
      p[2] = b;
      p[3] = a;
    }
  }
}

void ImageBuffer::blend_pixel(std::size_t x, std::size_t y, uint8_t r, uint8_t g, uint8_t b,
                              float alpha) {
  if (x >= width_ || y >= height_ || alpha <= 0.0f)
    return;
  uint8_t sa = static_cast<uint8_t>(std::min(1.0f, alpha) * 255.0f + 0.5f);
  blend_pixel_raw(pixel(x, y), r, g, b, sa);
}

void ImageBuffer::fill_rect(float x, float y, float w, float h, Color const &color) {
  fill_rect(static_cast<int>(std::floor(x)), static_cast<int>(std::floor(y)),
            static_cast<int>(std::ceil(w)), static_cast<int>(std::ceil(h)), color);
}

void ImageBuffer::fill_rect(int x, int y, int w, int h, Color const &color) {
  if (w <= 0 || h <= 0)
    return;
  int x0 = std::max(0, x);
  int y0 = std::max(0, y);
  int x1 = std::min(static_cast<int>(width_), x + w);
  int y1 = std::min(static_cast<int>(height_), y + h);
  float fa = color.a / 255.0f;
  for (int py = y0; py < y1; ++py) {
    for (int px = x0; px < x1; ++px) {
      blend_pixel(static_cast<std::size_t>(px), static_cast<std::size_t>(py), color.r, color.g,
                  color.b, fa);
    }
  }
}

void ImageBuffer::fill_polygon(std::pair<float, float> const *points, std::size_t count,
                               Color const &color) {
  fill_polygon(points, count, color.r, color.g, color.b, color.a);
}

void ImageBuffer::fill_polygon(std::pair<float, float> const *points, std::size_t count, uint8_t r,
                               uint8_t g, uint8_t b, uint8_t a) {
  if (count < 3 || width_ == 0 || height_ == 0)
    return;

  float ymin = points[0].second, ymax = points[0].second;
  float xmin = points[0].first, xmax = points[0].first;
  for (std::size_t i = 1; i < count; ++i) {
    ymin = std::min(ymin, points[i].second);
    ymax = std::max(ymax, points[i].second);
    xmin = std::min(xmin, points[i].first);
    xmax = std::max(xmax, points[i].first);
  }

  int y0 = std::max(0, static_cast<int>(std::floor(ymin)));
  int y1 = std::min(static_cast<int>(height_ - 1), static_cast<int>(std::ceil(ymax)));
  int x0 = std::max(0, static_cast<int>(std::floor(xmin)));
  int x1 = std::min(static_cast<int>(width_ - 1), static_cast<int>(std::ceil(xmax)));

  float fa = a / 255.0f;

  for (int y = y0; y <= y1; ++y) {
    std::vector<int> crossings;
    for (std::size_t i = 0; i < count; ++i) {
      std::size_t j = (i + 1) % count;
      float yi = points[i].second;
      float yj = points[j].second;
      if (yi <= y && yj > y) {
        float x = points[i].first +
                  static_cast<float>(y - yi) / (yj - yi) * (points[j].first - points[i].first);
        crossings.push_back(static_cast<int>(std::floor(x)));
      } else if (yj <= y && yi > y) {
        float x = points[i].first +
                  static_cast<float>(y - yi) / (yj - yi) * (points[j].first - points[i].first);
        crossings.push_back(static_cast<int>(std::floor(x)));
      }
    }
    std::sort(crossings.begin(), crossings.end());
    for (std::size_t k = 0; k + 1 < crossings.size(); k += 2) {
      int xs = std::max(x0, crossings[k]);
      int xe = std::min(x1, crossings[k + 1]);
      for (int x = xs; x <= xe; ++x) {
        blend_pixel(static_cast<std::size_t>(x), static_cast<std::size_t>(y), r, g, b, fa);
      }
    }
  }
}

void ImageBuffer::draw_line(float x0, float y0, float x1, float y1, float width,
                            Color const &color) {
  if (width < 1.0f)
    width = 1.0f;
  draw_thick_segment(*this, x0, y0, x1, y1, width, color);
}

void ImageBuffer::draw_polyline(std::pair<float, float> const *points, std::size_t count,
                                float width, Color const &color) {
  if (count < 2)
    return;
  for (std::size_t i = 0; i + 1 < count; ++i) {
    draw_line(points[i].first, points[i].second, points[i + 1].first, points[i + 1].second, width,
              color);
  }
}

void ImageBuffer::draw_polygon_outline(std::pair<float, float> const *points, std::size_t count,
                                       float width, Color const &color) {
  if (count < 2)
    return;
  for (std::size_t i = 0; i < count; ++i) {
    std::size_t j = (i + 1) % count;
    draw_line(points[i].first, points[i].second, points[j].first, points[j].second, width, color);
  }
}

namespace {

// Draws the on/off runs of a dashed segment, advancing `running` (cumulative distance since the
// start of the dash cycle) so a caller can chain segments (polyline vertices) with a continuous
// pattern instead of restarting the cycle at each vertex.
void draw_dashed_segment(ImageBuffer &img, float x0, float y0, float x1, float y1, float width,
                         Color const &color, std::vector<float> const &dash, float total,
                         float &running) {
  float dx = x1 - x0;
  float dy = y1 - y0;
  float len = std::sqrt(dx * dx + dy * dy);
  if (len < 1e-6f)
    return;
  float ux = dx / len;
  float uy = dy / len;
  float pos = 0.0f;
  while (pos < len) {
    float m = std::fmod(running, total);
    if (m < 0.0f)
      m += total;
    float acc = 0.0f;
    bool on = true;
    float remaining = len - pos;
    for (float dlen : dash) {
      if (m < acc + dlen) {
        remaining = acc + dlen - m;
        break;
      }
      acc += dlen;
      on = !on;
    }
    float step = std::min(std::max(remaining, 1e-4f), len - pos);
    if (on) {
      float sx = x0 + ux * pos;
      float sy = y0 + uy * pos;
      float ex = x0 + ux * (pos + step);
      float ey = y0 + uy * (pos + step);
      img.draw_line(sx, sy, ex, ey, width, color);
    }
    pos += step;
    running += step;
  }
}

float dash_total_length(std::vector<float> const &dash) {
  float total = 0.0f;
  for (float d : dash)
    total += std::max(0.0f, d);
  return total;
}

} // namespace

void ImageBuffer::draw_dashed_line(float x0, float y0, float x1, float y1, float width,
                                   Color const &color, std::vector<float> const &dash) {
  float total = dash_total_length(dash);
  if (dash.empty() || total <= 1e-6f) {
    draw_line(x0, y0, x1, y1, width, color);
    return;
  }
  float running = 0.0f;
  draw_dashed_segment(*this, x0, y0, x1, y1, width, color, dash, total, running);
}

void ImageBuffer::draw_dashed_polyline(std::pair<float, float> const *points, std::size_t count,
                                       float width, Color const &color,
                                       std::vector<float> const &dash) {
  if (count < 2)
    return;
  float total = dash_total_length(dash);
  if (dash.empty() || total <= 1e-6f) {
    draw_polyline(points, count, width, color);
    return;
  }
  float running = 0.0f;
  for (std::size_t i = 0; i + 1 < count; ++i) {
    draw_dashed_segment(*this, points[i].first, points[i].second, points[i + 1].first,
                        points[i + 1].second, width, color, dash, total, running);
  }
}

namespace {

float smoothstep(float edge0, float edge1, float x) {
  if (edge0 == edge1)
    return x < edge0 ? 0.0f : 1.0f;
  float t = std::max(0.0f, std::min(1.0f, (x - edge0) / (edge1 - edge0)));
  return t * t * (3.0f - 2.0f * t);
}

// start/end are cumulative pie angles; atan2 is in [-pi, pi]. Compare relative span.
bool angle_in_sector(double ang, double start, double end) {
  double span = end - start;
  if (span <= 1e-12)
    return false;
  if (span > 2.0 * M_PI)
    span = 2.0 * M_PI;
  double rel = ang - start;
  while (rel < 0)
    rel += 2.0 * M_PI;
  while (rel >= 2.0 * M_PI)
    rel -= 2.0 * M_PI;
  return rel <= span + 1e-9;
}

} // namespace

void ImageBuffer::fill_circle(float cx, float cy, float radius, Color const &color) {
  if (radius <= 0.0f)
    return;
  constexpr float kAa = 1.0f;
  int x0 = std::max(0, static_cast<int>(std::floor(cx - radius - kAa)));
  int y0 = std::max(0, static_cast<int>(std::floor(cy - radius - kAa)));
  int x1 = std::min(static_cast<int>(width_) - 1, static_cast<int>(std::ceil(cx + radius + kAa)));
  int y1 = std::min(static_cast<int>(height_) - 1, static_cast<int>(std::ceil(cy + radius + kAa)));
  float fa = color.a / 255.0f;
  for (int y = y0; y <= y1; ++y) {
    for (int x = x0; x <= x1; ++x) {
      float cov = 0.0f;
      for (int sy = 0; sy < 4; ++sy) {
        for (int sx = 0; sx < 4; ++sx) {
          float px = static_cast<float>(x) + (static_cast<float>(sx) + 0.5f) / 4.0f;
          float py = static_cast<float>(y) + (static_cast<float>(sy) + 0.5f) / 4.0f;
          float dx = px - cx;
          float dy = py - cy;
          float dist = std::sqrt(dx * dx + dy * dy);
          if (dist > radius + kAa)
            continue;
          float c = (dist <= radius - kAa) ? 1.0f : smoothstep(radius + kAa, radius - kAa, dist);
          cov += c;
        }
      }
      cov /= 16.0f;
      if (cov > 0.0f) {
        blend_pixel(static_cast<std::size_t>(x), static_cast<std::size_t>(y), color.r, color.g,
                    color.b, fa * cov);
      }
    }
  }
}

void ImageBuffer::fill_sector(float cx, float cy, float radius, double start_rad, double end_rad,
                              Color const &color) {
  if (radius <= 0.0f)
    return;
  constexpr float kAa = 1.0f;
  int x0 = std::max(0, static_cast<int>(std::floor(cx - radius - kAa)));
  int y0 = std::max(0, static_cast<int>(std::floor(cy - radius - kAa)));
  int x1 = std::min(static_cast<int>(width_) - 1, static_cast<int>(std::ceil(cx + radius + kAa)));
  int y1 = std::min(static_cast<int>(height_) - 1, static_cast<int>(std::ceil(cy + radius + kAa)));
  float fa = color.a / 255.0f;

  for (int y = y0; y <= y1; ++y) {
    for (int x = x0; x <= x1; ++x) {
      float cov = 0.0f;
      for (int sy = 0; sy < 4; ++sy) {
        for (int sx = 0; sx < 4; ++sx) {
          float px = static_cast<float>(x) + (static_cast<float>(sx) + 0.5f) / 4.0f;
          float py = static_cast<float>(y) + (static_cast<float>(sy) + 0.5f) / 4.0f;
          float dx = px - cx;
          float dy = py - cy;
          float dist = std::sqrt(dx * dx + dy * dy);
          if (dist > radius + kAa)
            continue;
          double ang = std::atan2(static_cast<double>(dy), static_cast<double>(dx));
          if (!angle_in_sector(ang, start_rad, end_rad))
            continue;
          float c = (dist <= radius - kAa) ? 1.0f : smoothstep(radius + kAa, radius - kAa, dist);
          cov += c;
        }
      }
      cov /= 16.0f;
      if (cov > 0.0f) {
        blend_pixel(static_cast<std::size_t>(x), static_cast<std::size_t>(y), color.r, color.g,
                    color.b, fa * cov);
      }
    }
  }
}

void ImageBuffer::fill_rounded_rect(float x, float y, float w, float h, float radius,
                                    Color const &color) {
  if (w <= 0.0f || h <= 0.0f)
    return;
  radius = std::max(0.0f, std::min(radius, std::min(w, h) * 0.5f));
  if (radius <= 0.0f) {
    fill_rect(x, y, w, h, color);
    return;
  }
  constexpr float kAa = 1.0f;
  int x0 = std::max(0, static_cast<int>(std::floor(x - kAa)));
  int y0 = std::max(0, static_cast<int>(std::floor(y - kAa)));
  int x1 = std::min(static_cast<int>(width_) - 1, static_cast<int>(std::ceil(x + w + kAa)));
  int y1 = std::min(static_cast<int>(height_) - 1, static_cast<int>(std::ceil(y + h + kAa)));
  float cx = x + w * 0.5f;
  float cy = y + h * 0.5f;
  float hx = w * 0.5f - radius;
  float hy = h * 0.5f - radius;
  float fa = color.a / 255.0f;
  for (int py = y0; py <= y1; ++py) {
    for (int px = x0; px <= x1; ++px) {
      float cov = 0.0f;
      for (int sy = 0; sy < 4; ++sy) {
        for (int sx = 0; sx < 4; ++sx) {
          float sxp = static_cast<float>(px) + (static_cast<float>(sx) + 0.5f) / 4.0f;
          float syp = static_cast<float>(py) + (static_cast<float>(sy) + 0.5f) / 4.0f;
          // Rounded-box signed distance field: distance from the sample point to the
          // rounded-rect boundary (negative = inside).
          float qx = std::abs(sxp - cx) - hx;
          float qy = std::abs(syp - cy) - hy;
          float ax = std::max(qx, 0.0f);
          float ay = std::max(qy, 0.0f);
          float dist = std::sqrt(ax * ax + ay * ay) + std::min(std::max(qx, qy), 0.0f) - radius;
          if (dist > kAa)
            continue;
          float c = (dist <= -kAa) ? 1.0f : smoothstep(kAa, -kAa, dist);
          cov += c;
        }
      }
      cov /= 16.0f;
      if (cov > 0.0f) {
        blend_pixel(static_cast<std::size_t>(px), static_cast<std::size_t>(py), color.r, color.g,
                    color.b, fa * cov);
      }
    }
  }
}

ImageBuffer ImageBuffer::downsample(ImageBuffer const &src, int factor) {
  if (factor <= 1)
    return src;
  std::size_t dw = src.width() / static_cast<std::size_t>(factor);
  std::size_t dh = src.height() / static_cast<std::size_t>(factor);
  if (dw == 0 || dh == 0)
    return ImageBuffer();

  ImageBuffer out(dw, dh);
  int f2 = factor * factor;
  for (std::size_t y = 0; y < dh; ++y) {
    for (std::size_t x = 0; x < dw; ++x) {
      int sr = 0, sg = 0, sb = 0, sa = 0;
      for (int dy = 0; dy < factor; ++dy) {
        for (int dx = 0; dx < factor; ++dx) {
          auto const *p = src.pixel(x * factor + dx, y * factor + dy);
          sr += p[0];
          sg += p[1];
          sb += p[2];
          sa += p[3];
        }
      }
      auto *d = out.pixel(x, y);
      d[0] = static_cast<uint8_t>(sr / f2);
      d[1] = static_cast<uint8_t>(sg / f2);
      d[2] = static_cast<uint8_t>(sb / f2);
      d[3] = static_cast<uint8_t>(sa / f2);
    }
  }
  return out;
}

} // namespace insightdock::charts
