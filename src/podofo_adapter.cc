// SPDX-License-Identifier: MIT
// Copyright (c) InsightDock contributors

#include "insightdock/pdf/podofo_adapter.h"
#include "insightdock/png_writer.h"

#include <podofo/podofo.h>

namespace insightdock::charts::podofo {

void draw_chart(PoDoFo::PdfDocument &doc, PoDoFo::PdfPainter &painter, ImageBuffer const &img,
                double x, double y, double w, double h) {
  if (img.width() == 0 || img.height() == 0)
    return;

  std::vector<uint8_t> png = encode_png(img);
  if (png.empty())
    return;

  auto image = doc.CreateImage();
  PoDoFo::bufferview view(reinterpret_cast<char const *>(png.data()), png.size());
  image->LoadFromBuffer(view);

  double scale_x = w / static_cast<double>(img.width());
  double scale_y = h / static_cast<double>(img.height());
  painter.DrawImage(*image, x, y, scale_x, scale_y);
}

} // namespace insightdock::charts::podofo
