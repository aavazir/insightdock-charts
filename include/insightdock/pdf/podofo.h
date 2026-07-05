// SPDX-License-Identifier: MIT
// Copyright (c) InsightDock contributors

/**
 * @file podofo.h
 * @brief Umbrella header for PoDoFo PDF integration.
 *
 * Requires building InsightDock with `INSIGHTDOCK_WITH_PODOFO=ON` and linking PoDoFo.
 * Defines `INSIGHTDOCK_HAS_PODOFO` when PDF support is available.
 *
 * @namespace insightdock::charts::podofo
 * PDF embedding and native vector chart drawing.
 *
 * @example
 * @code
 * #include "insightdock/charts.h"
 * #include "insightdock/pdf/podofo.h"
 *
 * PieChart pie;
 * pie.add_slice("A", 60);
 * pie.add_slice("B", 40);
 *
 * ExportOptions opts;
 * opts.pdf_backend = PdfBackend::Vector;
 * ChartExporter::save(pie, "pie.pdf", opts);
 * @endcode
 */

#ifndef INSIGHTDOCK_PDF_PODOFO_H
#define INSIGHTDOCK_PDF_PODOFO_H

#include "insightdock/export.h"

#ifdef INSIGHTDOCK_HAS_PODOFO
#include "insightdock/pdf/podofo_adapter.h"
#include "insightdock/pdf/podofo_vector.h"
#include "insightdock/pdf/report.h"
#endif

#endif // INSIGHTDOCK_PDF_PODOFO_H
