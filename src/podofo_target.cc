// SPDX-License-Identifier: MIT
// Copyright (c) InsightDock contributors

#include "insightdock/pdf/podofo_target.h"

#include <podofo/podofo.h>

#include <algorithm>
#include <cmath>
#include <exception>

namespace insightdock::charts::podofo {

namespace {

constexpr double kKappa = 0.5522847498; // cubic-Bézier circle approximation

PoDoFo::PdfColor pdf_color(Color const &c) {
  return PoDoFo::PdfColor(c.r / 255.0, c.g / 255.0, c.b / 255.0);
}

bool finite(double v) {
  return std::isfinite(v) && std::fabs(v) < 1.0e7;
}

// Keeps Latin-1 (+ euro) which WinAnsi can encode and replaces everything else with '?', so a
// chart title with e.g. CJK text degrades visibly instead of throwing from the PDF encoder.
std::string winansi_safe(std::string const &s) {
  std::string out;
  std::size_t i = 0;
  while (i < s.size()) {
    unsigned char c = static_cast<unsigned char>(s[i]);
    int cp = c;
    std::size_t len = 1;
    auto cont = [&](std::size_t k) {
      return static_cast<int>(static_cast<unsigned char>(s[i + k]) & 0x3F);
    };
    if (c >= 0xF0 && i + 3 < s.size()) {
      cp = ((c & 0x07) << 18) | (cont(1) << 12) | (cont(2) << 6) | cont(3);
      len = 4;
    } else if (c >= 0xE0 && c < 0xF0 && i + 2 < s.size()) {
      cp = ((c & 0x0F) << 12) | (cont(1) << 6) | cont(2);
      len = 3;
    } else if (c >= 0xC0 && c < 0xE0 && i + 1 < s.size()) {
      cp = ((c & 0x1F) << 6) | cont(1);
      len = 2;
    } else if (c >= 0x80) {
      cp = 0xFFFD; // stray continuation / truncated sequence
    }
    if (cp == '\n' || (cp >= 0x20 && cp < 0x7F) || (cp >= 0xA0 && cp < 0x100) || cp == 0x20AC)
      out.append(s, i, len);
    else if (cp >= 0x20)
      out.push_back('?');
    // control characters are dropped
    i += len;
  }
  return out;
}

} // namespace

struct PdfTarget::Impl {
  PoDoFo::PdfDocument &doc;
  PoDoFo::PdfPainter &p;
  PoDoFo::PdfFont *font;
  double x0, y0, bh;
  double sx = 1.0, sy = 1.0, s = 1.0;
  std::map<int, std::unique_ptr<PoDoFo::PdfExtGState>> alpha_states;
  int cur_alpha = 255;

  Impl(PoDoFo::PdfDocument &d, PoDoFo::PdfPainter &painter, PoDoFo::PdfFont *f, double x, double y,
       double h)
      : doc(d), p(painter), font(f), x0(x), y0(y), bh(h) {}

  double X(double x) const {
    return x0 + x * sx;
  }
  double Y(double y) const {
    return y0 + bh - y * sy;
  }

  void apply_alpha(int a) {
    if (a == cur_alpha)
      return;
    auto &slot = alpha_states[a];
    if (!slot) {
      auto def = std::make_shared<PoDoFo::PdfExtGStateDefinition>();
      def->StrokingAlpha = a / 255.0;
      def->NonStrokingAlpha = a / 255.0;
      slot = doc.CreateExtGState(def);
    }
    p.GraphicsState.SetExtGState(*slot);
    cur_alpha = a;
  }

  void set_fill(Color const &c) {
    apply_alpha(c.a);
    p.GraphicsState.SetNonStrokingColor(pdf_color(c));
  }

  void set_stroke(Color const &c, float width, std::vector<float> const &dash) {
    apply_alpha(c.a);
    p.GraphicsState.SetStrokingColor(pdf_color(c));
    p.GraphicsState.SetLineWidth(std::max(0.05, static_cast<double>(width) * s));
    float sum = 0.0f;
    bool ok = !dash.empty();
    for (float d : dash) {
      if (!std::isfinite(d) || d < 0.0f)
        ok = false;
      sum += d;
    }
    if (ok && sum >= 0.25f) {
      std::vector<double> d;
      for (float v : dash)
        d.push_back(std::max(0.01, static_cast<double>(v) * s));
      p.SetStrokeStyle(PoDoFo::cspan<double>(d.data(), d.size()), 0.0);
    } else {
      p.SetStrokeStyle(PoDoFo::PdfStrokeStyle::Solid);
    }
  }

  bool path_from(PoDoFo::PdfPainterPath &path, DrawTarget::Point const *pts, std::size_t n,
                 bool close) {
    if (n < 2)
      return false;
    for (std::size_t i = 0; i < n; ++i) {
      if (!finite(pts[i].first) || !finite(pts[i].second))
        return false;
      if (i == 0)
        path.MoveTo(X(pts[i].first), Y(pts[i].second));
      else
        path.AddLineTo(X(pts[i].first), Y(pts[i].second));
    }
    if (close)
      path.Close();
    return true;
  }

  // Draws one text run (no newlines) with its alphabetic baseline at PDF (bx, by).
  void run(std::string const &line, double bx, double by, TextAlign align) {
    if (line.empty())
      return;
    try {
      double w = font->GetStringLength(line, p.TextState);
      if (align == TextAlign::Center)
        bx -= w * 0.5;
      else if (align == TextAlign::Right)
        bx -= w;
      p.DrawText(line, bx, by);
    } catch (std::exception const &) {
      // Unencodable text: leave the run out rather than abort the whole page.
    }
  }

  void text(Font const &f, std::string const &raw, Color const &c, double bx, double by,
            TextAlign align) {
    std::string t = winansi_safe(raw);
    if (t.empty())
      return;
    p.TextState.SetFont(*font, std::max(1.0, static_cast<double>(f.size()) * s));
    set_fill(c);
    double lh = static_cast<double>(f.line_height()) * s;
    std::size_t start = 0;
    int line = 0;
    while (start <= t.size()) {
      std::size_t end = t.find('\n', start);
      run(t.substr(start, end == std::string::npos ? std::string::npos : end - start), bx,
          by - lh * line, align);
      if (end == std::string::npos)
        break;
      start = end + 1;
      ++line;
    }
  }

  void rotated(Font const &f, double cx, double cy, std::string const &t, Color const &c,
               bool clockwise) {
    if (!finite(cx) || !finite(cy))
      return;
    int saved_alpha = cur_alpha;
    p.Save();
    // cm = [a b c d e f]; +90° reads bottom-to-top (ccw), -90° top-to-bottom (cw).
    double b = clockwise ? -1.0 : 1.0;
    double cc = clockwise ? 1.0 : -1.0;
    p.GraphicsState.ConcatenateTransformationMatrix(PoDoFo::Matrix(0.0, b, cc, 0.0, X(cx), Y(cy)));
    // The raster path centers the line box on the anchor; baseline lies (ascent - lh/2) below it.
    double base =
        -(static_cast<double>(f.ascent()) - static_cast<double>(f.line_height()) * 0.5) * s;
    text(f, t, c, 0.0, base, TextAlign::Center);
    p.Restore();
    cur_alpha = saved_alpha;
  }
};

PdfTarget::PdfTarget(PoDoFo::PdfDocument &doc, PoDoFo::PdfPainter &painter, float chart_w,
                     float chart_h, double x, double y, double w, double h, PoDoFo::PdfFont *font)
    : impl_(new Impl(doc, painter, font, x, y, h)), cw_(chart_w), ch_(chart_h) {
  if (cw_ <= 0.0f)
    cw_ = 1.0f;
  if (ch_ <= 0.0f)
    ch_ = 1.0f;
  impl_->sx = w / static_cast<double>(cw_);
  impl_->sy = h / static_cast<double>(ch_);
  impl_->s = std::min(impl_->sx, impl_->sy);
  if (!impl_->font) {
    PoDoFo::PdfFontCreateParams params;
    params.Encoding =
        PoDoFo::PdfEncoding(PoDoFo::PdfEncodingMapFactory::GetWinAnsiEncodingInstancePtr());
    impl_->font =
        &doc.GetFonts().GetStandard14Font(PoDoFo::PdfStandard14FontType::Helvetica, params);
  }
  painter.Save(); // everything the chart sets (colors, dashes, alpha) is undone in the dtor
}

PdfTarget::~PdfTarget() {
  try {
    impl_->p.Restore();
  } catch (...) {
  }
}

void PdfTarget::clear(Color const &color) {
  fill_rect(0.0f, 0.0f, cw_, ch_, color);
}

void PdfTarget::fill_rect(float x, float y, float w, float h, Color const &color) {
  if (!finite(x) || !finite(y) || !finite(w) || !finite(h) || w <= 0.0f || h <= 0.0f)
    return;
  impl_->set_fill(color);
  impl_->p.DrawRectangle(impl_->X(x), impl_->Y(y + h), w * impl_->sx, h * impl_->sy,
                         PoDoFo::PdfPathDrawMode::Fill);
}

void PdfTarget::fill_rounded_rect(float x, float y, float w, float h, float radius,
                                  Color const &color) {
  if (!finite(x) || !finite(y) || !finite(w) || !finite(h) || w <= 0.0f || h <= 0.0f)
    return;
  double r = std::max(0.0f, std::min(radius, std::min(w, h) * 0.5f));
  double rx = r * impl_->sx, ry = r * impl_->sy;
  double l = impl_->X(x), rt = impl_->X(x + w), t = impl_->Y(y), b = impl_->Y(y + h);
  double kx = rx * kKappa, ky = ry * kKappa;
  PoDoFo::PdfPainterPath path;
  path.MoveTo(l + rx, t);
  path.AddLineTo(rt - rx, t);
  path.AddCubicBezierTo(rt - rx + kx, t, rt, t - ry + ky, rt, t - ry);
  path.AddLineTo(rt, b + ry);
  path.AddCubicBezierTo(rt, b + ry - ky, rt - rx + kx, b, rt - rx, b);
  path.AddLineTo(l + rx, b);
  path.AddCubicBezierTo(l + rx - kx, b, l, b + ry - ky, l, b + ry);
  path.AddLineTo(l, t - ry);
  path.AddCubicBezierTo(l, t - ry + ky, l + rx - kx, t, l + rx, t);
  path.Close();
  impl_->set_fill(color);
  impl_->p.DrawPath(path, PoDoFo::PdfPathDrawMode::Fill);
}

void PdfTarget::fill_polygon(Point const *points, std::size_t count, Color const &color) {
  PoDoFo::PdfPainterPath path;
  if (count < 3 || !impl_->path_from(path, points, count, true))
    return;
  impl_->set_fill(color);
  // Non-zero winding (the default Fill) keeps concave area-chart polygons solid.
  impl_->p.DrawPath(path, PoDoFo::PdfPathDrawMode::Fill);
}

void PdfTarget::fill_circle(float cx, float cy, float radius, Color const &color) {
  if (!finite(cx) || !finite(cy) || !finite(radius) || radius <= 0.0f)
    return;
  PoDoFo::PdfPainterPath path;
  path.AddCircle(impl_->X(cx), impl_->Y(cy), radius * impl_->s);
  impl_->set_fill(color);
  impl_->p.DrawPath(path, PoDoFo::PdfPathDrawMode::Fill);
}

void PdfTarget::draw_line(float x0, float y0, float x1, float y1, float width, Color const &color) {
  draw_dashed_line(x0, y0, x1, y1, width, color, {});
}

void PdfTarget::draw_dashed_line(float x0, float y0, float x1, float y1, float width,
                                 Color const &color, std::vector<float> const &dash) {
  Point pts[2] = {{x0, y0}, {x1, y1}};
  draw_dashed_polyline(pts, 2, width, color, dash);
}

void PdfTarget::draw_polyline(Point const *points, std::size_t count, float width,
                              Color const &color) {
  draw_dashed_polyline(points, count, width, color, {});
}

void PdfTarget::draw_dashed_polyline(Point const *points, std::size_t count, float width,
                                     Color const &color, std::vector<float> const &dash) {
  PoDoFo::PdfPainterPath path;
  if (!impl_->path_from(path, points, count, false))
    return;
  impl_->set_stroke(color, width, dash);
  impl_->p.GraphicsState.SetLineJoinStyle(PoDoFo::PdfLineJoinStyle::Round);
  impl_->p.DrawPath(path, PoDoFo::PdfPathDrawMode::Stroke);
}

void PdfTarget::draw_polygon_outline(Point const *points, std::size_t count, float width,
                                     Color const &color) {
  PoDoFo::PdfPainterPath path;
  if (!impl_->path_from(path, points, count, true))
    return;
  impl_->set_stroke(color, width, {});
  impl_->p.GraphicsState.SetLineJoinStyle(PoDoFo::PdfLineJoinStyle::Miter);
  impl_->p.DrawPath(path, PoDoFo::PdfPathDrawMode::Stroke);
}

void PdfTarget::draw_text(Font const &font, float x, float y, std::string const &text,
                          Color const &color, TextAlign align, TextBaseline baseline) {
  if (!finite(x) || !finite(y))
    return;
  double by = static_cast<double>(y) + static_cast<double>(font.baseline_offset(baseline));
  impl_->text(font, text, color, impl_->X(x), impl_->Y(by), align);
}

void PdfTarget::draw_text_rotated_ccw(Font const &font, float cx, float cy, std::string const &text,
                                      Color const &color) {
  impl_->rotated(font, cx, cy, text, color, false);
}

void PdfTarget::draw_text_rotated_cw(Font const &font, float cx, float cy, std::string const &text,
                                     Color const &color) {
  impl_->rotated(font, cx, cy, text, color, true);
}

} // namespace insightdock::charts::podofo
