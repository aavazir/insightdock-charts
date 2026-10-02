// SPDX-License-Identifier: MIT
// Copyright (c) InsightDock contributors

/**
 * @file podofo_target.h
 * @brief @ref DrawTarget that emits native PDF vector operators through a PoDoFo painter.
 */

#ifndef INSIGHTDOCK_PDF_PODOFO_TARGET_H
#define INSIGHTDOCK_PDF_PODOFO_TARGET_H

#include "insightdock/draw_target.h"
#include "insightdock/export_macros.h"

#include <map>
#include <memory>

namespace PoDoFo {
class PdfDocument;
class PdfExtGState;
class PdfFont;
class PdfPainter;
} // namespace PoDoFo

namespace insightdock::charts::podofo {

/**
 * @brief Draws chart-pixel geometry into a box on a PDF page.
 *
 * The chart's `width × height` pixel space is mapped onto the lower-left-origin PDF box
 * `(x, y, w, h)` in points (y flipped). Text is laid out with the embedded font's metrics but
 * drawn with a PDF font (Standard14 Helvetica unless one is supplied), so it stays selectable
 * and sharp at any zoom. Translucent colors use ExtGState opacity.
 */
class INSIGHTDOCK_API PdfTarget final : public DrawTarget {
public:
  PdfTarget(PoDoFo::PdfDocument &doc, PoDoFo::PdfPainter &painter, float chart_w, float chart_h,
            double x, double y, double w, double h, PoDoFo::PdfFont *font = nullptr);
  ~PdfTarget() override;

  float width() const override {
    return cw_;
  }
  float height() const override {
    return ch_;
  }
  void clear(Color const &color) override;
  void fill_rect(float x, float y, float w, float h, Color const &color) override;
  void fill_rounded_rect(float x, float y, float w, float h, float radius,
                         Color const &color) override;
  void fill_polygon(Point const *points, std::size_t count, Color const &color) override;
  void fill_circle(float cx, float cy, float radius, Color const &color) override;
  void draw_line(float x0, float y0, float x1, float y1, float width, Color const &color) override;
  void draw_polyline(Point const *points, std::size_t count, float width,
                     Color const &color) override;
  void draw_polygon_outline(Point const *points, std::size_t count, float width,
                            Color const &color) override;
  void draw_dashed_line(float x0, float y0, float x1, float y1, float width, Color const &color,
                        std::vector<float> const &dash) override;
  void draw_dashed_polyline(Point const *points, std::size_t count, float width, Color const &color,
                            std::vector<float> const &dash) override;
  void draw_text(Font const &font, float x, float y, std::string const &text, Color const &color,
                 TextAlign align, TextBaseline baseline) override;
  void draw_text_rotated_ccw(Font const &font, float cx, float cy, std::string const &text,
                             Color const &color) override;
  void draw_text_rotated_cw(Font const &font, float cx, float cy, std::string const &text,
                            Color const &color) override;

private:
  struct Impl;
  std::unique_ptr<Impl> impl_;
  float cw_, ch_;
};

} // namespace insightdock::charts::podofo

#endif // INSIGHTDOCK_PDF_PODOFO_TARGET_H
