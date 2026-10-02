// SPDX-License-Identifier: MIT
// Copyright (c) InsightDock contributors

/**
 * @file export.h
 * @brief Unified chart export: render, save, and encode to PNG / JPEG / PDF.
 *
 * @ref ChartExporter is the primary API for output. Infer format from the file
 * extension or set @ref ExportOptions::format explicitly.
 *
 * PDF methods require building with `INSIGHTDOCK_WITH_PODOFO` and linking PoDoFo.
 */

#ifndef INSIGHTDOCK_CHARTS_EXPORT_H
#define INSIGHTDOCK_CHARTS_EXPORT_H

#include "insightdock/chart.h"
#include "insightdock/error.h"
#include "insightdock/export_macros.h"
#include "insightdock/image_buffer.h"
#include <cstdint>
#include <string>
#include <vector>

namespace PoDoFo {
class PdfDocument;
class PdfPainter;
} // namespace PoDoFo

namespace insightdock::charts {

/** @brief Output encoding format for @ref ChartExporter. */
enum class OutputFormat {
  Png,       ///< Portable Network Graphics (.png).
  Jpeg,      ///< JPEG (.jpg, .jpeg).
  Svg,       ///< SVG with chart rasterized as base64-inlined PNG.
  PdfRaster, ///< Single-page PDF with chart embedded as PNG image.
  PdfVector, ///< Single-page PDF with native vector graphics where supported (pie).
};

/** @brief PDF embedding strategy when writing or drawing PDF output. */
enum class PdfBackend {
  Raster, ///< Rasterize chart to PNG and embed (works for all chart types).
  Vector, ///< Native PDF paths for pie; bar/line fall back to raster.
};

/** @brief SVG drawing strategy when writing or encoding SVG output. */
enum class SvgBackend {
  Raster, ///< Rasterize chart to PNG and base64-inline it (works for all chart types).
  Vector, ///< Native SVG `<path>`/`<text>` for pie; other chart types fall back to raster.
};

/**
 * @brief Options controlling render quality and output format.
 *
 * @note @p supersample > 1 renders at higher resolution then downsamples for
 *       sharper PNG/JPEG and raster PDF output. Ignored for const @ref ChartExporter::render.
 */
struct ExportOptions {
  OutputFormat format =
      OutputFormat::Png; ///< Target format (overridden by file extension in save()).
  int jpeg_quality = 90; ///< JPEG quality 1–100 (ignored for PNG/PDF).
  int supersample = 1;   ///< Supersampling factor (1 = none, 2 = 2×, etc.).
  PdfBackend pdf_backend = PdfBackend::Raster; ///< Used by draw_pdf() and PDF save().
  SvgBackend svg_backend = SvgBackend::Raster; ///< Used by SVG save()/encode().
};

/**
 * @brief Infer @ref OutputFormat from a file path extension.
 * @param path File path (e.g. `"chart.png"`, `"out.pdf"`).
 * @return Detected format, or @ref OutputFormat::Png if extension is unknown.
 */
INSIGHTDOCK_API OutputFormat format_from_path(std::string const &path);

/**
 * @brief Unified chart export API — render once, save or encode.
 *
 * Typical usage:
 * @code
 * PieChart pie;
 * pie.add_slice("A", 40);
 * ChartExporter::save(pie, "chart.png");
 * @endcode
 *
 * @see ExportOptions, format_from_path
 */
class INSIGHTDOCK_API ChartExporter {
public:
  /**
   * @brief Render a chart to an RGBA buffer.
   * @param chart Chart to render (options may be read).
   * @param opts  Supersampling applied when @p opts.supersample > 1.
   * @return Rendered image buffer.
   */
  static ImageBuffer render(Chart &chart, ExportOptions const &opts = {});

  /**
   * @brief Render a const chart (no supersampling).
   * @param chart Chart to render.
   * @param opts  Ignored for supersampling on const charts.
   * @return Rendered image buffer.
   */
  static ImageBuffer render(Chart const &chart, ExportOptions const &opts = {});

  /**
   * @brief Write a chart to disk.
   * @param chart Chart to render and save.
   * @param path  Output file path; extension selects format if @p opts.format is default.
   * @param opts  Export options (quality, supersampling, PDF backend).
   * @return @ref ChartResult<void> — true on success; on failure, @ref ChartResult::error()
   *         distinguishes @ref ChartError::BadPath, @ref ChartError::RenderFailure,
   *         @ref ChartError::EncodeFailure, and @ref ChartError::PdfUnsupported.
   *         Contextually converts to `bool` so `if (!ChartExporter::save(...))` still compiles.
   */
  static ChartResult<void> save(Chart &chart, std::string const &path, ExportOptions opts = {});

  /**
   * @brief Encode a chart to memory.
   * @param chart Chart to render.
   * @param opts  Format and quality options.
   * @return @ref ChartResult holding the encoded bytes on success, or a
   *         @ref ChartError on failure (never returns an empty vector on success).
   */
  static ChartResult<std::vector<uint8_t>> encode(Chart &chart, ExportOptions const &opts = {});

#ifdef INSIGHTDOCK_HAS_PODOFO
  /**
   * @brief Embed a chart into an existing PDF page.
   *
   * Coordinates are in PDF user units (points). @p x,@p y is the lower-left corner
   * of the chart bounding box.
   *
   * @param doc    PoDoFo document (must outlive the draw call).
   * @param painter Active painter with canvas set to the target page.
   * @param chart  Chart to embed.
   * @param x      Left edge in PDF points.
   * @param y      Bottom edge in PDF points.
   * @param w      Drawn width in PDF points.
   * @param h      Drawn height in PDF points.
   * @param opts   Raster vs vector backend and supersampling.
   */
  static void draw_pdf(PoDoFo::PdfDocument &doc, PoDoFo::PdfPainter &painter, Chart const &chart,
                       double x, double y, double w, double h, ExportOptions const &opts = {});

  /**
   * @brief Embed a chart using native PDF vectors when supported.
   *
   * Pie charts render as vector paths; bar and line charts raster-fallback.
   *
   * @see draw_pdf
   */
  static void draw_pdf_vector(PoDoFo::PdfDocument &doc, PoDoFo::PdfPainter &painter,
                              Chart const &chart, double x, double y, double w, double h,
                              ExportOptions const &opts = {});
#endif
};

} // namespace insightdock::charts

#endif // INSIGHTDOCK_CHARTS_EXPORT_H
