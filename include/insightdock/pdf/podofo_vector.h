// SPDX-License-Identifier: MIT
// Copyright (c) InsightDock contributors

/**
 * @file podofo_vector.h
 * @brief Native vector PDF pie charts (paths, leader lines, labels).
 *
 * Renders sharp, zoomable pie charts directly with PoDoFo drawing primitives.
 * Used by @ref ChartExporter when @ref PdfBackend::Vector is selected, and by
 * multi-page report samples in `examples/`.
 */

#ifndef INSIGHTDOCK_PDF_PODOFO_VECTOR_H
#define INSIGHTDOCK_PDF_PODOFO_VECTOR_H

#include "insightdock/export_macros.h"
#include "insightdock/palette.h"
#include <string>
#include <utility>
#include <vector>

namespace PoDoFo {
class PdfDocument;
class PdfFont;
class PdfPainter;
} // namespace PoDoFo

namespace insightdock::charts {
class Chart;
class PieChart;
} // namespace insightdock::charts

namespace insightdock::charts::podofo {

/**
 * @brief Optional PoDoFo fonts for vector PDF text.
 *
 * When null, Standard14 Helvetica is used for labels.
 */
struct PdfVectorFonts {
  PoDoFo::PdfFont *regular = nullptr; ///< Regular label font.
  PoDoFo::PdfFont *bold = nullptr;    ///< Bold variant (percentages, emphasis).
};

/**
 * @brief Layout and styling for vector pie charts in PDF points.
 *
 * Coordinates passed to draw functions are in PDF user units (1/72 inch).
 * Set @p resolve_label_overlap and @p auto_legend_min_slices for dense pies;
 * some report layouts may disable these to preserve a fixed layout.
 */
struct PieVectorOptions {
  bool show_labels = true;     ///< Draw external callout labels.
  bool show_percentage = true; ///< Second line shows percentage.
  bool show_legend = false;    ///< Bottom legend instead of callouts.
  bool skip_zero = true;       ///< Omit zero-value slices.
  bool clamp_labels = true;    ///< Keep labels inside the chart box.

  PaletteKind palette = PaletteKind::Tableau10; ///< Slice fill colors.
  double fill_opacity = 0.6;                    ///< Slice fill alpha (0–1).
  double radius_fraction = 0.26;                ///< Pie radius as fraction of min(box w,h).

  float label_font_pt = 7.0f;   ///< Label font size in points.
  float pct_font_pt = 6.5f;     ///< Percentage line font size.
  float stroke_width_pt = 1.0f; ///< Slice outline width.
  float leader_width_pt = 0.3f; ///< Leader line width.

  /** @brief Hide labels for slices below this fraction of total (0 = show all). */
  double min_label_frac = 0.0;
  /** @brief Vertically separate overlapping labels on each side. */
  bool resolve_label_overlap = true;
  /**
   * @brief Switch to legend when slice count ≥ this value (0 = never auto-switch).
   */
  std::size_t auto_legend_min_slices = 0;
};

/**
 * @brief Draw a @ref PieChart as native PDF vector paths.
 *
 * @param doc    PoDoFo document.
 * @param painter Active painter (canvas must be set).
 * @param chart  Source pie chart (reads slices and options).
 * @param x      Lower-left X of bounding box (PDF points).
 * @param y      Lower-left Y of bounding box (PDF points).
 * @param w      Box width in PDF points.
 * @param h      Box height in PDF points.
 * @param opts   Vector styling and label behavior.
 * @param fonts  Optional PDF fonts for labels.
 */
INSIGHTDOCK_API void draw_pie(PoDoFo::PdfDocument &doc, PoDoFo::PdfPainter &painter,
                              PieChart const &chart, double x, double y, double w, double h,
                              PieVectorOptions const &opts = {}, PdfVectorFonts fonts = {});

/**
 * @brief Draw a pie from label/value pairs without constructing a @ref PieChart.
 *
 * Convenience for report generators that already have aggregated data.
 *
 * @param slices Vector of {label, value} pairs.
 * @see draw_pie
 */
INSIGHTDOCK_API void draw_pie_slices(PoDoFo::PdfDocument &doc, PoDoFo::PdfPainter &painter,
                                     std::vector<std::pair<std::string, double>> const &slices,
                                     double x, double y, double w, double h,
                                     PieVectorOptions const &opts = {}, PdfVectorFonts fonts = {});

/**
 * @brief Draw any @ref Chart using vectors when supported, else raster embed.
 *
 * Pie charts use vector paths; bar and line charts fall back to PNG embed via
 * @ref draw_chart.
 */
INSIGHTDOCK_API void draw_chart_vector(PoDoFo::PdfDocument &doc, PoDoFo::PdfPainter &painter,
                                       Chart const &chart, double x, double y, double w, double h,
                                       PieVectorOptions const &pie_opts = {},
                                       PdfVectorFonts fonts = {});

} // namespace insightdock::charts::podofo

#endif // INSIGHTDOCK_PDF_PODOFO_VECTOR_H
