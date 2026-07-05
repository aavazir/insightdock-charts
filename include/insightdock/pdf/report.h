// SPDX-License-Identifier: MIT
// Copyright (c) InsightDock contributors

/**
 * @file report.h
 * @brief Multi-page PDF report layout: tables, sections, footers, pagination.
 *
 * Requires building with `INSIGHTDOCK_WITH_PODOFO=ON`.
 */

#ifndef INSIGHTDOCK_PDF_REPORT_H
#define INSIGHTDOCK_PDF_REPORT_H

#ifdef INSIGHTDOCK_HAS_PODOFO

#include "insightdock/chart.h"
#include "insightdock/export.h"

#include <podofo/podofo.h>

#include <string>
#include <utility>
#include <vector>

namespace insightdock::charts::pdf {

/** @brief A4 page layout constants (millimetres, top-left origin). */
struct PdfPageLayout {
  static constexpr double page_w_mm = 210.0;
  static constexpr double page_h_mm = 297.0;
  static constexpr double margin_l_mm = 10.0;
};

/**
 * @brief Split comma- or space-separated MAC/IP lists onto separate lines for table cells.
 */
std::string multiline_cell(std::string const &text);

/**
 * @brief Builder for paginated PDF reports with tables and section headings.
 *
 * Coordinates use millimetres from the top-left of the page (jsPDF-style).
 */
class PdfReportBuilder {
public:
  PdfReportBuilder(PoDoFo::PdfMemDocument &doc, PoDoFo::PdfPainter &painter, PoDoFo::PdfFont &font,
                   PoDoFo::PdfFont &font_bold);

  /** @brief Current vertical cursor (mm from top). */
  double y_mm() const {
    return y_mm_;
  }
  void set_y_mm(double y) {
    y_mm_ = y;
  }

  void set_font(double pt, bool bold = false);
  double text_width_mm(std::string const &text, bool bold = false);

  void draw_text_box(double x_mm, double y_mm, double box_w_mm, std::string const &text,
                     PoDoFo::PdfHorizontalAlignment align = PoDoFo::PdfHorizontalAlignment::Left);

  void
  draw_text_in_cell(double x_mm, double top_mm, double w_mm, double h_mm, std::string const &text,
                    PoDoFo::PdfHorizontalAlignment align = PoDoFo::PdfHorizontalAlignment::Left,
                    double pad = 2.0);

  double render_inline_formatted(double x_mm, double y_mm, double max_w_mm,
                                 std::vector<std::pair<std::string, bool>> const &parts,
                                 double indent_mm = 0);

  void fill_rect_top(double x_mm, double top_mm, double w_mm, double h_mm,
                     PoDoFo::PdfColor const &c);
  void stroke_rect_top(double x_mm, double top_mm, double w_mm, double h_mm,
                       PoDoFo::PdfColor const &stroke, double round = 0);
  void hline(double y_mm, double x0 = 10, double x1 = 200);

  double content_width_mm() const;

  void section_title(std::string const &title);

  /** @brief Start a new page when vertical space is exhausted. @return true if a page break
   * occurred. */
  bool ensure_space(double need_mm);

  void draw_cell(double x, double top, double w, double h, bool is_label, std::string const &text,
                 PoDoFo::PdfColor const &bg);

  void draw_header_bar(std::string const &title, std::string const &logo_path = {});

  void draw_table(std::string const &title, std::vector<std::string> const &headers,
                  std::vector<std::vector<std::string>> const &rows,
                  std::vector<double> const &col_w_mm,
                  std::vector<PoDoFo::PdfHorizontalAlignment> const &col_align = {});

  /**
   * @brief Embed a chart at mm coordinates (top-left origin), no pagination.
   * @param x_mm   Left edge, mm from page left.
   * @param top_mm Top edge, mm from page top.
   * @param w_mm   Drawn width, mm.
   * @param h_mm   Drawn height, mm.
   * @param chart  Chart to embed (rendered via @ref ChartExporter::draw_pdf).
   * @param opts   Raster vs vector backend and supersampling.
   */
  void draw_chart(double x_mm, double top_mm, double w_mm, double h_mm, Chart const &chart,
                  ExportOptions const &opts = {});

  /**
   * @brief Embed a chart in the vertical flow: ensures space, draws an optional
   * caption and border, then advances @ref y_mm() by @p h_mm plus a gap.
   * @param x_mm    Left edge, mm from page left.
   * @param w_mm    Drawn width, mm.
   * @param h_mm    Drawn height, mm.
   * @param chart   Chart to embed.
   * @param caption Optional caption drawn above the chart (empty = none).
   * @param opts    Raster vs vector backend and supersampling.
   */
  void draw_chart_flow(double x_mm, double w_mm, double h_mm, Chart const &chart,
                       std::string const &caption = {}, ExportOptions const &opts = {});

  void draw_footers(std::string const &user_line, std::string const &center_line);

  void new_page(double top_y_mm = 15.0);

private:
  PoDoFo::PdfMemDocument &doc_;
  PoDoFo::PdfPainter &painter_;
  PoDoFo::PdfFont &font_;
  PoDoFo::PdfFont &font_bold_;
  double y_mm_ = 0;
};

PoDoFo::PdfColor pdf_rgb(int r, int g, int b);

} // namespace insightdock::charts::pdf

#endif // INSIGHTDOCK_HAS_PODOFO

#endif // INSIGHTDOCK_PDF_REPORT_H
