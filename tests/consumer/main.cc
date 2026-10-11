// SPDX-License-Identifier: MIT
// Copyright (c) InsightDock contributors

#include <insightdock/charts.h>

#include <cstdio>

int main() {
  using namespace insightdock::charts;
  BarChart chart;
  chart.set_categories({"A", "B"});
  chart.add_series("S", {1.0, 2.0});
  Chart const &constant = chart;
  if (ChartExporter::capabilities(constant).vector != VectorCapability::Native)
    return 1;
  ExportOptions svg;
  svg.format = OutputFormat::Svg;
  svg.svg_backend = SvgBackend::Vector;
  svg.vector_fallback = VectorFallback::Reject;
  if (!ChartExporter::encode(constant, svg) || !ChartExporter::save(constant, "consumer.svg", svg))
    return 1;
  auto png = encode_png(chart.render());
  if (png.size() < 8 || png[1] != 'P' || png[2] != 'N' || png[3] != 'G') {
    std::fprintf(stderr, "consumer: bad PNG output\n");
    return 1;
  }
#ifdef INSIGHTDOCK_HAS_PODOFO
  ExportOptions pdf;
  pdf.format = OutputFormat::PdfVector;
  pdf.vector_fallback = VectorFallback::Reject;
  auto result = ChartExporter::encode(constant, pdf);
  if (!result || result.value.size() < 5 || result.value[0] != '%') {
    std::fprintf(stderr, "consumer: bad PDF output\n");
    return 1;
  }
#endif
  return 0;
}
