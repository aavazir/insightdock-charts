// SPDX-License-Identifier: MIT
// Copyright (c) InsightDock contributors

#include "insightdock/pdf/report.h"

#ifdef INSIGHTDOCK_HAS_PODOFO

#include <algorithm>
#include <cmath>
#include <sstream>

namespace insightdock::charts::pdf {

namespace {

double MX(double mm) {
  return mm * 72.0 / 25.4;
}

double MY(double mm_from_top) {
  return (PdfPageLayout::page_h_mm - mm_from_top) * 72.0 / 25.4;
}

int cell_line_count(std::string const &text) {
  if (text.empty())
    return 1;
  int lines = 1;
  for (char c : text) {
    if (c == '\n')
      ++lines;
  }
  return lines;
}

} // namespace

PoDoFo::PdfColor pdf_rgb(int r, int g, int b) {
  return PoDoFo::PdfColor(r / 255.0, g / 255.0, b / 255.0);
}

std::string multiline_cell(std::string const &text) {
  if (text.empty())
    return text;
  if (text.find(", ") != std::string::npos) {
    std::ostringstream out;
    std::size_t start = 0;
    bool first = true;
    while (start < text.size()) {
      auto pos = text.find(", ", start);
      std::string part =
          (pos == std::string::npos) ? text.substr(start) : text.substr(start, pos - start);
      while (!part.empty() && part.front() == ' ')
        part.erase(part.begin());
      while (!part.empty() && part.back() == ' ')
        part.pop_back();
      if (!part.empty()) {
        if (!first)
          out << '\n';
        out << part;
        first = false;
      }
      if (pos == std::string::npos)
        break;
      start = pos + 2;
    }
    return out.str();
  }
  if (text.find(' ') == std::string::npos)
    return text;
  std::ostringstream out;
  std::istringstream iss(text);
  std::string part;
  bool first = true;
  while (iss >> part) {
    if (!first)
      out << '\n';
    out << part;
    first = false;
  }
  return out.str();
}

PdfReportBuilder::PdfReportBuilder(PoDoFo::PdfMemDocument &doc, PoDoFo::PdfPainter &painter,
                                   PoDoFo::PdfFont &font, PoDoFo::PdfFont &font_bold)
    : doc_(doc), painter_(painter), font_(font), font_bold_(font_bold) {}

void PdfReportBuilder::set_font(double pt, bool bold) {
  painter_.TextState.SetFont(bold ? font_bold_ : font_, pt);
}

double PdfReportBuilder::text_width_mm(std::string const &text, bool bold) {
  if (text.empty())
    return 0;
  PoDoFo::PdfFont &f = bold ? font_bold_ : font_;
  return f.GetStringLength(text, painter_.TextState) * 25.4 / 72.0;
}

void PdfReportBuilder::draw_text_box(double x_mm, double y_mm, double box_w_mm,
                                     std::string const &text,
                                     PoDoFo::PdfHorizontalAlignment align) {
  if (text.empty() || box_w_mm <= 0)
    return;
  painter_.DrawTextAligned(text, MX(x_mm), MY(y_mm), MX(box_w_mm), align);
}

void PdfReportBuilder::draw_text_in_cell(double x_mm, double top_mm, double w_mm, double h_mm,
                                         std::string const &text,
                                         PoDoFo::PdfHorizontalAlignment align, double pad) {
  if (text.empty())
    return;
  PoDoFo::PdfDrawTextMultiLineParams params;
  params.HorizontalAlignment = align;
  params.VerticalAlignment = PoDoFo::PdfVerticalAlignment::Center;
  PoDoFo::Rect rect(MX(x_mm + pad), MY(top_mm + h_mm), MX(w_mm - 2 * pad), MX(h_mm));
  painter_.DrawTextMultiLine(text, rect, params);
}

double
PdfReportBuilder::render_inline_formatted(double x_mm, double y_mm, double max_w_mm,
                                          std::vector<std::pair<std::string, bool>> const &parts,
                                          double indent_mm) {
  struct Token {
    std::string text;
    bool bold;
  };
  std::vector<Token> tokens;
  for (auto const &part : parts) {
    std::istringstream iss(part.first);
    std::string word;
    while (iss >> word) {
      tokens.push_back({word, part.second});
    }
  }

  double x = x_mm + indent_mm;
  double y = y_mm;
  double line_h = 5.0;
  double start_x = x_mm + indent_mm;

  for (auto const &tok : tokens) {
    set_font(10, tok.bold);
    double word_w = text_width_mm(tok.text, tok.bold);

    bool punct = (tok.text == "." || tok.text == ",");
    bool first_on_line = (x <= start_x + 0.01);
    if (!first_on_line && !punct) {
      double space_w = text_width_mm(" ", false);
      if (x - x_mm + space_w + word_w > max_w_mm) {
        y += line_h;
        x = start_x;
      } else {
        x += space_w;
      }
    } else if (!first_on_line && x - x_mm + word_w > max_w_mm) {
      y += line_h;
      x = start_x;
    }

    painter_.DrawText(tok.text, MX(x), MY(y));
    x += word_w;
  }

  set_font(10, false);
  return y + line_h;
}

void PdfReportBuilder::fill_rect_top(double x_mm, double top_mm, double w_mm, double h_mm,
                                     PoDoFo::PdfColor const &c) {
  painter_.GraphicsState.SetNonStrokingColor(c);
  painter_.DrawRectangle(MX(x_mm), MY(top_mm + h_mm), MX(w_mm), MX(h_mm),
                         PoDoFo::PdfPathDrawMode::Fill);
}

void PdfReportBuilder::stroke_rect_top(double x_mm, double top_mm, double w_mm, double h_mm,
                                       PoDoFo::PdfColor const &stroke, double round) {
  painter_.GraphicsState.SetStrokingColor(stroke);
  painter_.GraphicsState.SetLineWidth(0.4);
  painter_.DrawRectangle(MX(x_mm), MY(top_mm + h_mm), MX(w_mm), MX(h_mm),
                         PoDoFo::PdfPathDrawMode::Stroke, round, round);
}

void PdfReportBuilder::hline(double y_mm, double x0, double x1) {
  painter_.GraphicsState.SetStrokingColor(pdf_rgb(200, 200, 200));
  painter_.GraphicsState.SetLineWidth(0.5);
  painter_.DrawLine(MX(x0), MY(y_mm), MX(x1), MY(y_mm));
}

double PdfReportBuilder::content_width_mm() const {
  return PdfPageLayout::page_w_mm - 2 * PdfPageLayout::margin_l_mm;
}

void PdfReportBuilder::section_title(std::string const &title) {
  static constexpr double kHeadingTopPad = 2.0;
  static constexpr double kHeadingUnderlineGap = 2.5;
  static constexpr double kHeadingContentGap = 9.0;

  y_mm_ += kHeadingTopPad;
  set_font(12, true);
  painter_.GraphicsState.SetNonStrokingColor(pdf_rgb(33, 37, 41));
  draw_text_box(PdfPageLayout::margin_l_mm, y_mm_, content_width_mm(), title,
                PoDoFo::PdfHorizontalAlignment::Left);
  hline(y_mm_ + kHeadingUnderlineGap);
  y_mm_ += kHeadingContentGap;
  set_font(8, false);
}

bool PdfReportBuilder::ensure_space(double need_mm) {
  if (y_mm_ + need_mm <= PdfPageLayout::page_h_mm - 12)
    return false;
  new_page();
  return true;
}

void PdfReportBuilder::new_page(double top_y_mm) {
  painter_.FinishDrawing();
  auto &page = doc_.GetPages().CreatePage(PoDoFo::PdfPageSize::A4);
  painter_.SetCanvas(page);
  y_mm_ = top_y_mm;
}

void PdfReportBuilder::draw_cell(double x, double top, double w, double h, bool is_label,
                                 std::string const &text, PoDoFo::PdfColor const &bg) {
  fill_rect_top(x, top, w, h, bg);
  painter_.GraphicsState.SetStrokingColor(pdf_rgb(180, 180, 180));
  painter_.GraphicsState.SetLineWidth(0.2);
  painter_.DrawRectangle(MX(x), MY(top + h), MX(w), MX(h), PoDoFo::PdfPathDrawMode::Stroke);
  set_font(8, is_label);
  painter_.GraphicsState.SetNonStrokingColor(is_label ? pdf_rgb(100, 100, 100) : pdf_rgb(0, 0, 0));
  draw_text_in_cell(x, top, w, h, text, PoDoFo::PdfHorizontalAlignment::Left);
}

void PdfReportBuilder::draw_header_bar(std::string const &title, std::string const &logo_path) {
  fill_rect_top(0, 0, PdfPageLayout::page_w_mm, 12, pdf_rgb(0, 0, 0));
  if (!logo_path.empty()) {
    try {
      auto img = doc_.CreateImage();
      img->Load(logo_path);
      double tw = MX(12);
      double th = MX(9);
      double sx = tw / static_cast<double>(img->GetWidth());
      double sy = th / static_cast<double>(img->GetHeight());
      painter_.DrawImage(*img, MX(15), MY(10), sx, sy);
      painter_.GraphicsState.SetStrokingColor(pdf_rgb(255, 255, 255));
      painter_.GraphicsState.SetLineWidth(0.5);
      painter_.DrawLine(MX(30), MY(1), MX(30), MY(10));
    } catch (...) {
      // logo optional
    }
  }
  set_font(16, false);
  painter_.GraphicsState.SetNonStrokingColor(pdf_rgb(255, 255, 255));
  draw_text_box(33, 8, PdfPageLayout::page_w_mm - 35, title, PoDoFo::PdfHorizontalAlignment::Left);
  y_mm_ = 20;
}

void PdfReportBuilder::draw_table(std::string const &title, std::vector<std::string> const &headers,
                                  std::vector<std::vector<std::string>> const &rows,
                                  std::vector<double> const &col_w_mm,
                                  std::vector<PoDoFo::PdfHorizontalAlignment> const &col_align) {
  ensure_space(28);
  set_font(10, true);
  painter_.GraphicsState.SetNonStrokingColor(pdf_rgb(33, 37, 41));
  draw_text_box(PdfPageLayout::margin_l_mm, y_mm_, content_width_mm(), title,
                PoDoFo::PdfHorizontalAlignment::Left);
  y_mm_ += 5;

  double header_h = 8;
  double base_row_h = 7.5;
  double line_h = 3.4;
  double x0 = PdfPageLayout::margin_l_mm;

  auto col_alignment = [&](std::size_t c) {
    if (c < col_align.size())
      return col_align[c];
    return PoDoFo::PdfHorizontalAlignment::Left;
  };

  auto draw_grid_line = [&](double x_mm, double y0_mm, double x1_mm, double y1_mm) {
    painter_.DrawLine(MX(x_mm), MY(y0_mm), MX(x1_mm), MY(y1_mm));
  };

  auto draw_row = [&](std::vector<std::string> const &cells, bool header, int row_idx) {
    double rh = header ? header_h : base_row_h;
    if (!header) {
      int max_lines = 1;
      for (auto const &c : cells)
        max_lines = std::max(max_lines, cell_line_count(c));
      rh = base_row_h + (max_lines - 1) * line_h;
      rh = std::min(rh, 30.0);
    }
    ensure_space(rh + 2);
    double x = x0;
    double top = y_mm_;

    PoDoFo::PdfColor bg =
        header ? pdf_rgb(233, 236, 239)
               : ((row_idx % 2 == 0) ? pdf_rgb(255, 255, 255) : pdf_rgb(248, 249, 250));
    double table_w = 0;
    for (double w : col_w_mm)
      table_w += w;
    fill_rect_top(x0, top, table_w, rh, bg);

    for (std::size_t c = 0; c < cells.size() && c < col_w_mm.size(); ++c) {
      set_font(8, header);
      painter_.GraphicsState.SetNonStrokingColor(header ? pdf_rgb(52, 58, 64)
                                                        : pdf_rgb(33, 37, 41));
      auto align = header ? PoDoFo::PdfHorizontalAlignment::Center : col_alignment(c);
      draw_text_in_cell(x, top, col_w_mm[c], rh, cells[c], align);
      x += col_w_mm[c];
    }

    painter_.GraphicsState.SetStrokingColor(pdf_rgb(222, 226, 230));
    painter_.GraphicsState.SetLineWidth(0.2);
    draw_grid_line(x0, top + rh, x0 + table_w, top + rh);

    y_mm_ += rh;
  };

  double table_w = 0;
  for (double w : col_w_mm)
    table_w += w;
  double table_top = y_mm_;

  painter_.GraphicsState.SetStrokingColor(pdf_rgb(173, 181, 189));
  painter_.GraphicsState.SetLineWidth(0.3);
  draw_grid_line(x0, table_top, x0 + table_w, table_top);

  draw_row(headers, true, 0);
  int ri = 0;
  for (auto const &row : rows)
    draw_row(row, false, ++ri);

  double table_bottom = y_mm_;
  painter_.GraphicsState.SetStrokingColor(pdf_rgb(206, 212, 218));
  painter_.GraphicsState.SetLineWidth(0.2);
  double cx = x0;
  for (std::size_t c = 0; c + 1 < col_w_mm.size(); ++c) {
    cx += col_w_mm[c];
    draw_grid_line(cx, table_top, cx, table_bottom);
  }
  painter_.GraphicsState.SetStrokingColor(pdf_rgb(173, 181, 189));
  painter_.GraphicsState.SetLineWidth(0.3);
  draw_grid_line(x0, table_top, x0, table_bottom);
  draw_grid_line(x0 + table_w, table_top, x0 + table_w, table_bottom);
  draw_grid_line(x0, table_bottom, x0 + table_w, table_bottom);

  y_mm_ += 12;
}

void PdfReportBuilder::draw_chart(double x_mm, double top_mm, double w_mm, double h_mm,
                                  Chart const &chart, ExportOptions const &opts) {
  double x = MX(x_mm);
  double y = MY(top_mm + h_mm);
  ChartExporter::draw_pdf(doc_, painter_, chart, x, y, MX(w_mm), MX(h_mm), opts);
}

void PdfReportBuilder::draw_chart_flow(double x_mm, double w_mm, double h_mm, Chart const &chart,
                                       std::string const &caption, ExportOptions const &opts) {
  double caption_h = caption.empty() ? 0.0 : 6.0;
  ensure_space(caption_h + h_mm + 4);

  if (!caption.empty()) {
    set_font(9, true);
    painter_.GraphicsState.SetNonStrokingColor(pdf_rgb(80, 80, 80));
    draw_text_box(x_mm, y_mm_, w_mm, caption, PoDoFo::PdfHorizontalAlignment::Center);
    y_mm_ += caption_h;
  }

  stroke_rect_top(x_mm, y_mm_, w_mm, h_mm, pdf_rgb(180, 180, 180), 3);
  draw_chart(x_mm, y_mm_, w_mm, h_mm, chart, opts);
  y_mm_ += h_mm + 4;
}

void PdfReportBuilder::draw_footers(std::string const &user_line, std::string const &center_line) {
  unsigned count = doc_.GetPages().GetCount();
  for (unsigned i = 0; i < count; ++i) {
    auto &page = doc_.GetPages().GetPageAt(i);
    painter_.SetCanvas(page);
    set_font(8, false);
    painter_.GraphicsState.SetNonStrokingColor(pdf_rgb(0, 0, 0));
    double footer_y = PdfPageLayout::page_h_mm - 6;
    draw_text_box(PdfPageLayout::margin_l_mm, footer_y, 70, "User: " + user_line,
                  PoDoFo::PdfHorizontalAlignment::Left);
    draw_text_box(PdfPageLayout::page_w_mm / 2 - 70, footer_y, 140, center_line,
                  PoDoFo::PdfHorizontalAlignment::Center);
    std::ostringstream pg;
    pg << "Page " << (i + 1) << " of " << count;
    draw_text_box(PdfPageLayout::page_w_mm - PdfPageLayout::margin_l_mm - 45, footer_y, 45,
                  pg.str(), PoDoFo::PdfHorizontalAlignment::Right);
    painter_.FinishDrawing();
  }
}

} // namespace insightdock::charts::pdf

#endif // INSIGHTDOCK_HAS_PODOFO
