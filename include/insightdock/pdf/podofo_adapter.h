// SPDX-License-Identifier: MIT
// Copyright (c) InsightDock contributors

/**
 * @file podofo_adapter.h
 * @brief Raster PDF embedding — draw an @ref ImageBuffer as PNG inside a PDF page.
 *
 * Part of the PoDoFo optional backend. No temporary files are written; PNG bytes
 * are encoded in memory and passed to PoDoFo.
 */

#ifndef INSIGHTDOCK_PDF_PODOFO_ADAPTER_H
#define INSIGHTDOCK_PDF_PODOFO_ADAPTER_H

#include "insightdock/export_macros.h"
#include "insightdock/image_buffer.h"

namespace PoDoFo {
class PdfDocument;
class PdfPainter;
} // namespace PoDoFo

namespace insightdock::charts::podofo {

/**
 * @brief Embed a raster chart image into the current PDF page.
 *
 * Encodes @p img as PNG in memory and draws it scaled to (@p w × @p h) PDF points.
 *
 * @param doc     PoDoFo document (used to create the embedded image XObject).
 * @param painter Active painter with canvas set.
 * @param img     RGBA chart buffer to embed.
 * @param x       Lower-left X in PDF user units (points).
 * @param y       Lower-left Y in PDF user units (points).
 * @param w       Drawn width in points.
 * @param h       Drawn height in points.
 *
 * @see ChartExporter::draw_pdf
 */
INSIGHTDOCK_API void draw_chart(PoDoFo::PdfDocument &doc, PoDoFo::PdfPainter &painter,
                                ImageBuffer const &img, double x, double y, double w, double h);

} // namespace insightdock::charts::podofo

#endif // INSIGHTDOCK_PDF_PODOFO_ADAPTER_H
