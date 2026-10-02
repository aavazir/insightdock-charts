// SPDX-License-Identifier: MIT
// Vector PDF check for multi-line pie labels + percentage spacing.

#include "insightdock/pdf/podofo_vector.h"

#include <podofo/podofo.h>
#include <iostream>

int main() {
  PoDoFo::PdfMemDocument doc;
  PoDoFo::PdfPainter painter;
  auto &page = doc.GetPages().CreatePage(PoDoFo::PdfPageSize::A4);
  painter.SetCanvas(page);

  PoDoFo::PdfFont &font =
      doc.GetFonts().GetStandard14Font(PoDoFo::PdfStandard14FontType::Helvetica);

  std::vector<std::pair<std::string, double>> slices = {
      {"40-AE-30-69-BD-51111", 1111},
      {"maisam", 1111},
  };

  insightdock::charts::podofo::PieVectorOptions opts;
  opts.show_legend = false;
  opts.clamp_labels = true;
  opts.resolve_label_overlap = false;
  insightdock::charts::podofo::PdfVectorFonts fonts{&font, &font};

  // Chart box of 92 mm x 82 mm.
  constexpr double pt_per_mm = 72.0 / 25.4;
  constexpr double chart_w_mm = 92.0;
  constexpr double chart_h_mm = 82.0;
  double cx = 72.0;
  double cy = 400.0;
  insightdock::charts::podofo::draw_pie_slices(doc, painter, slices, cx, cy, chart_w_mm * pt_per_mm,
                                               chart_h_mm * pt_per_mm, opts, fonts);

  painter.FinishDrawing();
  doc.Save("pie_overlap_vector.pdf");
  std::cout << "Wrote pie_overlap_vector.pdf\n";
  return 0;
}
