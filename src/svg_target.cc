// SPDX-License-Identifier: MIT
// Copyright (c) InsightDock contributors

#include "insightdock/detail/svg_target.h"

#include <algorithm>
#include <cmath>
#include <cstdio>

namespace insightdock::charts::detail {

namespace {

std::string num(float v) {
  if (!std::isfinite(v))
    v = 0.0f;
  v = std::max(-1.0e7f, std::min(1.0e7f, v));
  char buf[32];
  std::snprintf(buf, sizeof(buf), "%.2f", static_cast<double>(v));
  std::string s(buf);
  // Trim trailing zeros ("12.50" -> "12.5", "3.00" -> "3").
  if (s.find('.') != std::string::npos) {
    while (s.back() == '0')
      s.pop_back();
    if (s.back() == '.')
      s.pop_back();
  }
  if (s == "-0")
    s = "0";
  return s;
}

std::string rgb(Color const &c) {
  char buf[16];
  std::snprintf(buf, sizeof(buf), "#%02x%02x%02x", c.r, c.g, c.b);
  return buf;
}

std::string opacity_attr(char const *name, Color const &c) {
  if (c.a == 255)
    return {};
  return std::string(" ") + name + "=\"" + num(static_cast<float>(c.a) / 255.0f) + "\"";
}

std::string fill_attrs(Color const &c) {
  return " fill=\"" + rgb(c) + "\"" + opacity_attr("fill-opacity", c);
}

std::string stroke_attrs(Color const &c, float width) {
  return " fill=\"none\" stroke=\"" + rgb(c) + "\"" + opacity_attr("stroke-opacity", c) +
         " stroke-width=\"" + num(width) + "\"";
}

std::string escape(std::string const &s) {
  std::string out;
  out.reserve(s.size());
  for (char ch : s) {
    switch (ch) {
    case '&':
      out += "&amp;";
      break;
    case '<':
      out += "&lt;";
      break;
    case '>':
      out += "&gt;";
      break;
    case '"':
      out += "&quot;";
      break;
    default:
      // Control characters other than tab/newline are illegal in XML 1.0.
      if (static_cast<unsigned char>(ch) >= 0x20 || ch == '\t' || ch == '\n')
        out.push_back(ch);
    }
  }
  return out;
}

// Dash cycles shorter than a quarter pixel render solid in the raster path; match that.
bool is_solid(std::vector<float> const &dash) {
  if (dash.empty())
    return true;
  float sum = 0.0f;
  for (float d : dash) {
    if (!std::isfinite(d) || d < 0.0f)
      return true;
    sum += d;
  }
  return sum < 0.25f;
}

std::string dash_attr(std::vector<float> const &dash) {
  if (is_solid(dash))
    return {};
  std::string a = " stroke-dasharray=\"";
  // SVG repeats an odd-length list to make it even, as the raster dasher does.
  for (std::size_t i = 0; i < dash.size(); ++i) {
    if (i)
      a += ' ';
    a += num(dash[i]);
  }
  return a + "\"";
}

std::string points_attr(DrawTarget::Point const *pts, std::size_t n) {
  std::string s;
  for (std::size_t i = 0; i < n; ++i) {
    if (i)
      s += ' ';
    s += num(pts[i].first) + "," + num(pts[i].second);
  }
  return s;
}

} // namespace

SvgTarget::SvgTarget(float width, float height) : w_(width), h_(height) {}

void SvgTarget::clear(Color const &color) {
  fill_rect(0.0f, 0.0f, w_, h_, color);
}

void SvgTarget::fill_rect(float x, float y, float w, float h, Color const &color) {
  body_ += "<rect x=\"" + num(x) + "\" y=\"" + num(y) + "\" width=\"" + num(w) + "\" height=\"" +
           num(h) + "\"" + fill_attrs(color) + "/>\n";
}

void SvgTarget::fill_rounded_rect(float x, float y, float w, float h, float radius,
                                  Color const &color) {
  float r = std::max(0.0f, std::min(radius, std::min(w, h) * 0.5f));
  body_ += "<rect x=\"" + num(x) + "\" y=\"" + num(y) + "\" width=\"" + num(w) + "\" height=\"" +
           num(h) + "\" rx=\"" + num(r) + "\"" + fill_attrs(color) + "/>\n";
}

void SvgTarget::fill_polygon(Point const *points, std::size_t count, Color const &color) {
  if (count < 3)
    return;
  body_ += "<polygon points=\"" + points_attr(points, count) + "\"" + fill_attrs(color) + "/>\n";
}

void SvgTarget::fill_circle(float cx, float cy, float radius, Color const &color) {
  body_ += "<circle cx=\"" + num(cx) + "\" cy=\"" + num(cy) + "\" r=\"" + num(radius) + "\"" +
           fill_attrs(color) + "/>\n";
}

void SvgTarget::draw_line(float x0, float y0, float x1, float y1, float width, Color const &color) {
  draw_dashed_line(x0, y0, x1, y1, width, color, {});
}

void SvgTarget::draw_dashed_line(float x0, float y0, float x1, float y1, float width,
                                 Color const &color, std::vector<float> const &dash) {
  body_ += "<line x1=\"" + num(x0) + "\" y1=\"" + num(y0) + "\" x2=\"" + num(x1) + "\" y2=\"" +
           num(y1) + "\"" + stroke_attrs(color, width) + dash_attr(dash) + "/>\n";
}

void SvgTarget::draw_polyline(Point const *points, std::size_t count, float width,
                              Color const &color) {
  draw_dashed_polyline(points, count, width, color, {});
}

void SvgTarget::draw_dashed_polyline(Point const *points, std::size_t count, float width,
                                     Color const &color, std::vector<float> const &dash) {
  if (count < 2)
    return;
  body_ += "<polyline points=\"" + points_attr(points, count) + "\"" + stroke_attrs(color, width) +
           " stroke-linejoin=\"round\" stroke-linecap=\"butt\"" + dash_attr(dash) + "/>\n";
}

void SvgTarget::draw_polygon_outline(Point const *points, std::size_t count, float width,
                                     Color const &color) {
  if (count < 2)
    return;
  body_ += "<polygon points=\"" + points_attr(points, count) + "\"" + stroke_attrs(color, width) +
           " stroke-linejoin=\"miter\"/>\n";
}

void SvgTarget::text_element(Font const &font, std::string const &text, Color const &color,
                             char const *anchor, float x, float y, char const *transform) {
  // One <text> per line; '\n' advances by the font's line height, as the raster path does.
  float lh = font.line_height();
  std::size_t start = 0;
  int line = 0;
  while (start <= text.size()) {
    std::size_t end = text.find('\n', start);
    std::string seg =
        text.substr(start, end == std::string::npos ? std::string::npos : end - start);
    if (!seg.empty()) {
      body_ += "<text";
      if (transform)
        body_ += std::string(" transform=\"") + transform + "\"";
      body_ += " x=\"" + num(x) + "\" y=\"" + num(y + lh * static_cast<float>(line)) +
               "\" font-size=\"" + num(font.size()) + "\" text-anchor=\"" + anchor + "\"" +
               fill_attrs(color) + ">" + escape(seg) + "</text>\n";
    }
    if (end == std::string::npos)
      break;
    start = end + 1;
    ++line;
  }
}

void SvgTarget::draw_text(Font const &font, float x, float y, std::string const &text,
                          Color const &color, TextAlign align, TextBaseline baseline) {
  if (text.empty())
    return;
  char const *anchor = align == TextAlign::Center  ? "middle"
                       : align == TextAlign::Right ? "end"
                                                   : "start";
  text_element(font, text, color, anchor, x, y + font.baseline_offset(baseline), nullptr);
}

void SvgTarget::draw_text_rotated_ccw(Font const &font, float cx, float cy, std::string const &text,
                                      Color const &color) {
  if (text.empty())
    return;
  std::string tf = "translate(" + num(cx) + " " + num(cy) + ") rotate(-90)";
  // The raster path centers the line box on (cx, cy); the baseline sits ascent - lh/2 below it.
  text_element(font, text, color, "middle", 0.0f, font.ascent() - font.line_height() * 0.5f,
               tf.c_str());
}

void SvgTarget::draw_text_rotated_cw(Font const &font, float cx, float cy, std::string const &text,
                                     Color const &color) {
  if (text.empty())
    return;
  std::string tf = "translate(" + num(cx) + " " + num(cy) + ") rotate(90)";
  text_element(font, text, color, "middle", 0.0f, font.ascent() - font.line_height() * 0.5f,
               tf.c_str());
}

std::string SvgTarget::str() const {
  std::string xml;
  xml.reserve(body_.size() + 256);
  xml += "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n";
  xml += "<svg xmlns=\"http://www.w3.org/2000/svg\" width=\"" + num(w_) + "\" height=\"" + num(h_) +
         "\" viewBox=\"0 0 " + num(w_) + " " + num(h_) + "\">\n";
  xml += "<g font-family=\"DejaVu Sans, Verdana, Arial, sans-serif\">\n";
  xml += body_;
  xml += "</g>\n</svg>\n";
  return xml;
}

std::string build_chart_svg(Chart const &chart) {
  auto const &o = chart.options();
  if (o.width <= 0 || o.height <= 0)
    return {};
  SvgTarget target(static_cast<float>(o.width), static_cast<float>(o.height));
  if (!chart.draw(target))
    return {};
  return target.str();
}

} // namespace insightdock::charts::detail
