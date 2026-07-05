// SPDX-License-Identifier: MIT
// Copyright (c) InsightDock contributors

#include "insightdock/pdf/podofo_vector.h"
#include "insightdock/detail/math_constants.h"

#include "insightdock/color.h"
#include "insightdock/pdf/podofo_adapter.h"
#include "insightdock/pie_chart.h"

#include <podofo/podofo.h>

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <vector>

namespace insightdock::charts::podofo {

namespace {

PoDoFo::PdfFontCreateParams standard_font_params() {
  PoDoFo::PdfFontCreateParams params;
  params.Encoding =
      PoDoFo::PdfEncoding(PoDoFo::PdfEncodingMapFactory::GetWinAnsiEncodingInstancePtr());
  return params;
}

PoDoFo::PdfFont &resolve_font(PoDoFo::PdfDocument &doc, PoDoFo::PdfFont *user,
                              PoDoFo::PdfStandard14FontType fallback) {
  if (user != nullptr)
    return *user;
  return doc.GetFonts().GetStandard14Font(fallback, standard_font_params());
}

PoDoFo::PdfColor to_pdf_color(int r, int g, int b) {
  return PoDoFo::PdfColor(r / 255.0, g / 255.0, b / 255.0);
}

PoDoFo::PdfColor slice_fill(std::size_t index, PaletteKind palette, double opacity) {
  Color c = palette_color_by_kind(palette, index);
  auto mix = [opacity](uint8_t v) { return (v * opacity + 255.0 * (1.0 - opacity)) / 255.0; };
  return PoDoFo::PdfColor(mix(c.r), mix(c.g), mix(c.b));
}

void draw_text_anchored(PoDoFo::PdfPainter &painter, PoDoFo::PdfFont &font, double size,
                        std::string const &text, double x, double y, bool left_anchor) {
  if (text.empty())
    return;
  painter.TextState.SetFont(font, size);
  if (left_anchor) {
    painter.DrawText(text, x, y);
  } else {
    double w = font.GetStringLength(text, painter.TextState);
    painter.DrawText(text, x - w, y);
  }
}

struct SliceDraw {
  std::string label;
  double value = 0.0;
  double frac = 0.0;
  double mid = 0.0;
  std::size_t color_index = 0;
};

struct PlacedLabel {
  double mid = 0.0;
  std::string line1;
  std::string line2;
  std::vector<std::string> label_lines;
  bool right = true;
  double sx = 0.0;
  double sy = 0.0;
  double kx = 0.0;
  double ky_rad = 0.0;
  double ky = 0.0;
  double tx = 0.0;
  double ex = 0.0;
};

double measure_text_pt(PoDoFo::PdfFont &font, PoDoFo::PdfPainter &painter,
                       std::string const &text) {
  if (text.empty())
    return 0.0;
  return font.GetStringLength(text, painter.TextState);
}

void append_wrapped_token_pt(PoDoFo::PdfFont &font, PoDoFo::PdfPainter &painter,
                             double max_width_pt, std::string const &token, std::string &cur,
                             std::vector<std::string> &out) {
  if (token.empty())
    return;
  if (cur.empty()) {
    for (char c : token) {
      std::string next = cur + c;
      if (!cur.empty() && measure_text_pt(font, painter, next) > max_width_pt) {
        out.push_back(cur);
        cur = std::string(1, c);
      } else {
        cur = next;
      }
    }
    return;
  }
  std::string spaced = cur + ' ' + token;
  if (measure_text_pt(font, painter, spaced) <= max_width_pt) {
    cur = spaced;
    return;
  }
  if (!cur.empty()) {
    out.push_back(cur);
    cur.clear();
  }
  append_wrapped_token_pt(font, painter, max_width_pt, token, cur, out);
}

std::vector<std::string> wrap_pie_label(PoDoFo::PdfFont &font, PoDoFo::PdfPainter &painter,
                                        double font_pt, std::string const &text,
                                        double max_width_pt) {
  if (text.empty())
    return {};
  painter.TextState.SetFont(font, font_pt);
  if (max_width_pt <= 1.0)
    return {text};

  std::vector<std::string> out;
  std::size_t para_start = 0;
  while (para_start <= text.size()) {
    auto para_end = text.find('\n', para_start);
    std::string para = (para_end == std::string::npos)
                           ? text.substr(para_start)
                           : text.substr(para_start, para_end - para_start);

    std::string cur;
    std::size_t i = 0;
    while (i < para.size()) {
      while (i < para.size() && para[i] == ' ')
        ++i;
      if (i >= para.size())
        break;
      auto j = para.find(' ', i);
      append_wrapped_token_pt(font, painter, max_width_pt,
                              j == std::string::npos ? para.substr(i) : para.substr(i, j - i), cur,
                              out);
      i = j == std::string::npos ? para.size() : j + 1;
    }
    if (!cur.empty())
      out.push_back(cur);
    if (para_end == std::string::npos)
      break;
    para_start = para_end + 1;
  }
  if (out.empty())
    return {text};
  return out;
}

double widest_line(PoDoFo::PdfFont &font, PoDoFo::PdfPainter &painter, double font_pt,
                   std::vector<std::string> const &lines) {
  double max_w = 0.0;
  painter.TextState.SetFont(font, font_pt);
  for (auto const &line : lines) {
    if (!line.empty())
      max_w = std::max(max_w, font.GetStringLength(line, painter.TextState));
  }
  return max_w;
}

double label_line_spacing_pt(PoDoFo::PdfFont &font, PoDoFo::PdfPainter &painter, double font_pt) {
  painter.TextState.SetFont(font, font_pt);
  return font.GetLineSpacing(painter.TextState);
}

double label_block_height(PieVectorOptions const &opts, PoDoFo::PdfFont &font,
                          PoDoFo::PdfPainter &painter, int label_lines) {
  double h = label_line_spacing_pt(font, painter, opts.label_font_pt) *
             static_cast<double>(std::max(1, label_lines));
  if (opts.show_percentage)
    return h + opts.pct_font_pt + 5.0;
  return h + 2.0;
}

void draw_label_lines_anchored(PoDoFo::PdfPainter &painter, PoDoFo::PdfFont &font, double font_pt,
                               std::vector<std::string> const &lines, double x, double top_y,
                               double line_spacing, bool left_anchor) {
  double y = top_y;
  for (auto const &line : lines) {
    if (!line.empty())
      draw_text_anchored(painter, font, font_pt, line, x, y, left_anchor);
    y -= line_spacing;
  }
}

void draw_internal_legend(PoDoFo::PdfPainter &painter, PoDoFo::PdfFont &font,
                          std::vector<SliceDraw> const &slices, PieVectorOptions const &opts,
                          double x, double y, double w) {
  if (slices.empty())
    return;

  std::size_t cols = slices.size() > 5 ? 2 : 1;
  std::size_t rows = (slices.size() + cols - 1) / cols;
  double row_h = 8.0;
  double sw = 4.5;
  double gap = 2.0;
  double font_pt = std::min(opts.label_font_pt, 6.5f);
  double col_w = w / static_cast<double>(cols);
  double legend_h = rows * row_h + 4.0;
  double ly = y + 2.0;

  painter.TextState.SetFont(font, font_pt);

  for (std::size_t i = 0; i < slices.size(); ++i) {
    std::size_t col = i % cols;
    std::size_t row = i / cols;
    double lx = x + col * col_w + 3.0;
    double cy = ly + row * row_h + row_h * 0.5;

    painter.GraphicsState.SetNonStrokingColor(slice_fill(slices[i].color_index, opts.palette, 1.0));
    painter.DrawRectangle(lx, cy - sw * 0.5, sw, sw, PoDoFo::PdfPathDrawMode::Fill);

    char buf[160];
    if (opts.show_percentage)
      std::snprintf(buf, sizeof(buf), "%s: %.0f (%.1f%%)", slices[i].label.c_str(), slices[i].value,
                    slices[i].frac * 100.0);
    else
      std::snprintf(buf, sizeof(buf), "%s: %.0f", slices[i].label.c_str(), slices[i].value);

    painter.GraphicsState.SetNonStrokingColor(to_pdf_color(40, 40, 40));
    painter.DrawText(buf, lx + sw + gap, cy - font_pt * 0.3);
  }

  (void)legend_h;
}

void resolve_side_collisions(std::vector<PlacedLabel> &labels, double y_min, double y_max,
                             double spacing) {
  if (labels.size() < 2)
    return;

  std::sort(labels.begin(), labels.end(),
            [](PlacedLabel const &a, PlacedLabel const &b) { return a.ky < b.ky; });

  for (int pass = 0; pass < 32; ++pass) {
    bool moved = false;

    for (std::size_t i = 1; i < labels.size(); ++i) {
      double need = labels[i - 1].ky + spacing;
      if (labels[i].ky < need) {
        labels[i].ky = need;
        moved = true;
      }
    }

    if (labels.back().ky > y_max) {
      double shift = labels.back().ky - y_max;
      for (auto &lab : labels)
        lab.ky -= shift;
      moved = true;
    }

    if (labels.front().ky < y_min) {
      double shift = y_min - labels.front().ky;
      for (auto &lab : labels)
        lab.ky += shift;
      moved = true;
    }

    for (std::size_t i = labels.size(); i-- > 1;) {
      double need = labels[i].ky - spacing;
      if (labels[i - 1].ky > need) {
        labels[i - 1].ky = need;
        moved = true;
      }
    }

    if (!moved)
      break;
  }
}

void draw_pie_impl(PoDoFo::PdfDocument &doc, PoDoFo::PdfPainter &painter,
                   std::vector<SliceDraw> const &slices, double x, double y, double w, double h,
                   PieVectorOptions const &opts, PdfVectorFonts fonts) {
  if (slices.empty() || w <= 0.0 || h <= 0.0)
    return;

  auto &font = resolve_font(doc, fonts.regular, PoDoFo::PdfStandard14FontType::Helvetica);

  bool use_legend = opts.show_legend;
  if (!use_legend && opts.auto_legend_min_slices > 0 &&
      slices.size() >= opts.auto_legend_min_slices)
    use_legend = true;

  std::size_t legend_cols = slices.size() > 5 ? 2 : 1;
  std::size_t legend_rows = use_legend ? (slices.size() + legend_cols - 1) / legend_cols : 0;
  double legend_h = use_legend ? legend_rows * 8.0 + 8.0 : 0.0;

  double cx = x + w / 2.0;
  double pie_area_h = h - legend_h;
  double cy = y + legend_h + pie_area_h / 2.0;

  double radius_frac = opts.radius_fraction;
  if (use_legend)
    radius_frac *= 0.92;
  else if (slices.size() >= 8)
    radius_frac *= 0.82;
  else if (slices.size() >= 6)
    radius_frac *= 0.88;
  double radius = std::min(w, pie_area_h) * radius_frac;

  double cum = 0.0;
  for (auto const &s : slices) {
    double a1 = M_PI / 2.0 - 2.0 * M_PI * cum;
    double a2 = M_PI / 2.0 - 2.0 * M_PI * (cum + s.frac);

    int steps = std::max(8, static_cast<int>(std::ceil(s.frac * 180.0)));
    PoDoFo::PdfPainterPath path;
    path.MoveTo(cx, cy);
    for (int k = 0; k <= steps; ++k) {
      double a = a1 + (a2 - a1) * (static_cast<double>(k) / steps);
      path.AddLineTo(cx + radius * std::cos(a), cy + radius * std::sin(a));
    }
    path.Close();

    painter.GraphicsState.SetNonStrokingColor(
        slice_fill(s.color_index, opts.palette, opts.fill_opacity));
    painter.GraphicsState.SetStrokingColor(to_pdf_color(255, 255, 255));
    painter.GraphicsState.SetLineWidth(opts.stroke_width_pt);
    painter.DrawPath(path, PoDoFo::PdfPathDrawMode::StrokeFill);

    cum += s.frac;
  }

  if (use_legend) {
    draw_internal_legend(painter, font, slices, opts, x, y, w);
    return;
  }

  if (!opts.show_labels)
    return;

  // Preserve the report's original jsPDF-like geometry: the old code used
  // millimeters and converted them to PDF points before drawing.
  constexpr double pt_per_mm = 72.0 / 25.4;
  double knee = 3.0 * pt_per_mm;
  double elbow = 3.0 * pt_per_mm;
  double gap = 1.0 * pt_per_mm;
  double left_bound = x + 1.5;
  double right_bound = x + w - 1.5;
  double y_min = y + 2.0;
  double y_max = y + h - 2.0;

  std::vector<PlacedLabel> placed;
  placed.reserve(slices.size());

  cum = 0.0;
  for (auto const &s : slices) {
    if (opts.min_label_frac > 0.0 && s.frac < opts.min_label_frac)
      continue;

    double mid = M_PI / 2.0 - 2.0 * M_PI * (cum + s.frac * 0.5);
    double ca = std::cos(mid);
    double sa = std::sin(mid);
    bool right = ca >= 0.0;

    char l1[160];
    std::snprintf(l1, sizeof(l1), "%s: %.0f", s.label.c_str(), s.value);

    std::string l2;
    if (opts.show_percentage) {
      char buf[32];
      std::snprintf(buf, sizeof(buf), "%.1f%%", s.frac * 100.0);
      l2 = buf;
    }

    double sx = cx + radius * ca;
    double sy = cy + radius * sa;
    double kx = cx + (radius + knee) * ca;
    double ky = cy + (radius + knee) * sa;

    constexpr double label_wrap_max_pt = 50.0;
    double max_label_w =
        right ? (right_bound - (kx + elbow + gap)) : ((kx - elbow - gap) - left_bound);
    max_label_w = std::max(max_label_w, 20.0);
    max_label_w = std::min(max_label_w, label_wrap_max_pt);
    auto wrapped = wrap_pie_label(font, painter, opts.label_font_pt, l1, max_label_w);
    double w1 = widest_line(font, painter, opts.label_font_pt, wrapped);

    double tx;
    if (right) {
      tx = kx + elbow + gap;
      if (opts.clamp_labels && tx + w1 > right_bound)
        tx = right_bound - w1;
      if (tx < kx + gap)
        tx = kx + gap;
    } else {
      tx = kx - elbow - gap;
      if (opts.clamp_labels && tx - w1 < left_bound)
        tx = left_bound + w1;
      if (tx > kx - gap)
        tx = kx - gap;
    }
    max_label_w = right ? (right_bound - tx) : (tx - left_bound);
    max_label_w = std::max(max_label_w, 20.0);
    max_label_w = std::min(max_label_w, label_wrap_max_pt);
    wrapped = wrap_pie_label(font, painter, opts.label_font_pt, l1, max_label_w);
    double ex = right ? tx - gap : tx + gap;

    placed.push_back({mid, l1, l2, std::move(wrapped), right, sx, sy, kx, ky, ky, tx, ex});
    cum += s.frac;
  }

  if (placed.empty())
    return;

  if (opts.resolve_label_overlap) {
    std::vector<PlacedLabel> right_labels;
    std::vector<PlacedLabel> left_labels;
    for (auto &lab : placed) {
      if (lab.right)
        right_labels.push_back(lab);
      else
        left_labels.push_back(lab);
    }

    double block_h = 0.0;
    for (auto const &lab : placed)
      block_h = std::max(
          block_h,
          label_block_height(opts, font, painter,
                             static_cast<int>(std::max<std::size_t>(1, lab.label_lines.size()))));
    double spacing = block_h + 1.5;
    resolve_side_collisions(right_labels, y_min, y_max, spacing);
    resolve_side_collisions(left_labels, y_min, y_max, spacing);

    placed.clear();
    placed.insert(placed.end(), right_labels.begin(), right_labels.end());
    placed.insert(placed.end(), left_labels.begin(), left_labels.end());
  }

  for (auto const &lab : placed) {
    painter.GraphicsState.SetStrokingColor(to_pdf_color(150, 150, 150));
    painter.GraphicsState.SetLineWidth(opts.leader_width_pt);
    painter.DrawLine(lab.sx, lab.sy, lab.kx, lab.ky_rad);
    if (std::abs(lab.ky - lab.ky_rad) > 0.4)
      painter.DrawLine(lab.kx, lab.ky_rad, lab.kx, lab.ky);
    painter.DrawLine(lab.kx, lab.ky, lab.ex, lab.ky);

    double line_spacing = label_line_spacing_pt(font, painter, opts.label_font_pt);
    auto const &lines =
        lab.label_lines.empty() ? std::vector<std::string>{lab.line1} : lab.label_lines;
    int n_lines = static_cast<int>(std::max<std::size_t>(1, lines.size()));

    // Anchor % to the elbow (jsPDF legacy: ky-1.8 mm). Extra wrapped lines grow upward.
    constexpr double label_to_pct_baseline_gap_pt = 2.4 * pt_per_mm;
    double pct_y = lab.ky - 1.8 * pt_per_mm;
    double last_label_y = pct_y + label_to_pct_baseline_gap_pt;
    double first_label_y = last_label_y + static_cast<double>(n_lines - 1) * line_spacing;

    painter.GraphicsState.SetNonStrokingColor(to_pdf_color(40, 40, 40));
    draw_label_lines_anchored(painter, font, opts.label_font_pt, lines, lab.tx, first_label_y,
                              line_spacing, lab.right);

    if (!lab.line2.empty()) {
      painter.GraphicsState.SetNonStrokingColor(to_pdf_color(120, 120, 120));
      draw_text_anchored(painter, font, opts.pct_font_pt, lab.line2, lab.tx, pct_y, lab.right);
    }
  }
}

std::vector<SliceDraw> collect_slices(std::vector<std::pair<std::string, double>> const &raw,
                                      PieVectorOptions const &opts) {
  std::vector<SliceDraw> out;
  double total = 0.0;
  for (auto const &s : raw) {
    if (opts.skip_zero && s.second <= 0.0)
      continue;
    total += s.second;
  }
  if (total <= 0.0)
    return out;

  std::size_t idx = 0;
  double cum = 0.0;
  for (auto const &s : raw) {
    if (opts.skip_zero && s.second <= 0.0)
      continue;
    double frac = s.second / total;
    double a1 = M_PI / 2.0 - 2.0 * M_PI * cum;
    double a2 = M_PI / 2.0 - 2.0 * M_PI * (cum + frac);
    out.push_back({s.first, s.second, frac, (a1 + a2) / 2.0, idx});
    cum += frac;
    ++idx;
  }
  return out;
}

} // namespace

void draw_pie_slices(PoDoFo::PdfDocument &doc, PoDoFo::PdfPainter &painter,
                     std::vector<std::pair<std::string, double>> const &slices, double x, double y,
                     double w, double h, PieVectorOptions const &opts, PdfVectorFonts fonts) {
  auto drawn = collect_slices(slices, opts);
  draw_pie_impl(doc, painter, drawn, x, y, w, h, opts, fonts);
}

void draw_pie(PoDoFo::PdfDocument &doc, PoDoFo::PdfPainter &painter, PieChart const &chart,
              double x, double y, double w, double h, PieVectorOptions const &opts_in,
              PdfVectorFonts fonts) {
  PieVectorOptions opts = opts_in;
  auto const &po = chart.pie_options();
  opts.show_labels = po.show_labels;
  opts.show_percentage = po.show_percentage;
  opts.show_legend = po.show_legend;
  opts.skip_zero = po.skip_zero;
  opts.palette = po.palette;
  opts.fill_opacity = po.fill_opacity;
  opts.label_font_pt = po.label_font_size;
  opts.stroke_width_pt = po.stroke_width;

  std::vector<std::pair<std::string, double>> raw;
  for (auto const &s : chart.slices())
    raw.push_back({s.label, s.value});

  draw_pie_slices(doc, painter, raw, x, y, w, h, opts, fonts);
}

void draw_chart_vector(PoDoFo::PdfDocument &doc, PoDoFo::PdfPainter &painter, Chart const &chart,
                       double x, double y, double w, double h, PieVectorOptions const &pie_opts,
                       PdfVectorFonts fonts) {
  if (auto const *pie = dynamic_cast<PieChart const *>(&chart)) {
    draw_pie(doc, painter, *pie, x, y, w, h, pie_opts, fonts);
    return;
  }

  ImageBuffer img = chart.render();
  draw_chart(doc, painter, img, x, y, w, h);
}

} // namespace insightdock::charts::podofo
