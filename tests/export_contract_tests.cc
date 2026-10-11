// SPDX-License-Identifier: MIT
// Copyright (c) InsightDock contributors

#include "insightdock/charts.h"

#ifdef INSIGHTDOCK_HAS_PODOFO
#include <podofo/podofo.h>
#endif

#include <cstdio>
#include <filesystem>
#include <fstream>
#include <future>
#include <stdexcept>
#include <string>

using namespace insightdock::charts;

namespace {
int failures = 0;
#define CHECK(condition)                                                                           \
  do {                                                                                             \
    if (!(condition)) {                                                                            \
      std::fprintf(stderr, "FAIL: %s (%s:%d)\n", #condition, __FILE__, __LINE__);                  \
      ++failures;                                                                                  \
    }                                                                                              \
  } while (0)

class RasterChart : public Chart {
public:
  mutable int renders = 0;
  ImageBuffer render() const override {
    ++renders;
    ImageBuffer image(options_.width, options_.height);
    image.clear(Color::rgba(20, 40, 60, 255));
    return image;
  }
};

class CloneChart : public RasterChart {
public:
  std::unique_ptr<Chart> clone() const override {
    return std::make_unique<CloneChart>(*this);
  }
};

// Existing custom charts overriding only draw() keep working in strict mode.
class LegacyVectorChart : public RasterChart {
public:
  bool draw(DrawTarget &target) const override {
    target.fill_rect(1, 2, 3, 4, Color::rgba(11, 22, 33, 255));
    return true;
  }
};

class DecliningChart : public RasterChart {
public:
  VectorCapability vector_capability() const noexcept override {
    return VectorCapability::Native;
  }
  bool draw(DrawTarget &target) const override {
    target.fill_rect(1, 2, 3, 4, Color::rgba(11, 22, 33, 255));
    return false;
  }
};

class RasterOnlyChart : public RasterChart {
public:
  VectorCapability vector_capability() const noexcept override {
    return VectorCapability::RasterOnly;
  }
};

class ThrowingChart : public RasterChart {
public:
  ImageBuffer render() const override {
    throw std::runtime_error("render");
  }
};

class ThrowingVectorChart : public RasterChart {
public:
  bool draw(DrawTarget &) const override {
    throw std::runtime_error("draw");
  }
};

class NonstandardChart : public RasterChart {
public:
  ImageBuffer render() const override {
    throw 42;
  }
};

std::string bytes(ChartResult<std::vector<uint8_t>> const &result) {
  CHECK(static_cast<bool>(result));
  return {result.value.begin(), result.value.end()};
}

bool contains_image(std::string const &document, bool pdf) {
#ifdef INSIGHTDOCK_HAS_PODOFO
  if (pdf) {
    PoDoFo::PdfMemDocument parsed;
    parsed.LoadFromBuffer(PoDoFo::bufferview(document.data(), document.size()));
    CHECK(parsed.GetPages().GetCount() == 1);
    for (auto const *object : parsed.GetObjects()) {
      if (!object->IsDictionary())
        continue;
      auto const *subtype = object->GetDictionary().GetKey("Subtype");
      if (subtype && subtype->IsName() && subtype->GetName() == PoDoFo::PdfName("Image"))
        return true;
    }
    return false;
  }
#else
  (void)pdf;
#endif
  return document.find("<image") != std::string::npos;
}

void capability_queries() {
  BarChart bar;
  LineChart line;
  AreaChart area;
  ComboChart combo;
  PieChart pie;
  ScatterChart scatter;
  bar.set_categories({"A", "B"});
  bar.add_series("bars", {1, 2});
  line.set_categories({"A", "B"});
  line.add_series("line", {1, 2});
  area.set_categories({"A", "B"});
  area.add_series("area", {1, 2});
  combo.set_categories({"A", "B"});
  combo.add_bar_series("bars", {1, 2});
  combo.add_line_series("line", {2, 1});
  pie.add_slice("A", 1);
  pie.add_slice("B", 2);
  scatter.add_series("points", {{1, 2}, {2, 1}});
  for (Chart const *chart :
       {static_cast<Chart *>(&bar), static_cast<Chart *>(&line), static_cast<Chart *>(&area),
        static_cast<Chart *>(&combo), static_cast<Chart *>(&pie), static_cast<Chart *>(&scatter)}) {
    auto caps = ChartExporter::capabilities(*chart);
    CHECK(caps.vector == VectorCapability::Native);
    ExportOptions strict;
    strict.format = OutputFormat::Svg;
    strict.svg_backend = SvgBackend::Vector;
    strict.vector_fallback = VectorFallback::Reject;
    CHECK(!contains_image(bytes(ChartExporter::encode(*chart, strict)), false));
#ifdef INSIGHTDOCK_HAS_PODOFO
    CHECK(caps.pdf_available);
    strict.format = OutputFormat::PdfVector;
    CHECK(!contains_image(bytes(ChartExporter::encode(*chart, strict)), true));
#else
    CHECK(!caps.pdf_available);
#endif
  }
  RasterChart custom;
  RasterOnlyChart declared;
  ThrowingVectorChart throwing;
  CHECK(ChartExporter::capabilities(custom).vector == VectorCapability::Unknown);
  CHECK(ChartExporter::capabilities(declared).vector == VectorCapability::RasterOnly);
  CHECK(ChartExporter::capabilities(throwing).vector == VectorCapability::Unknown);
  CHECK(custom.renders == 0 && declared.renders == 0 && throwing.renders == 0);
}

void vector_policy(OutputFormat format) {
  ExportOptions options;
  options.format = format;
  options.svg_backend = SvgBackend::Vector;
  options.vector_fallback = VectorFallback::Reject;
  bool pdf = format != OutputFormat::Svg;
  if (format == OutputFormat::PdfRaster)
    options.pdf_backend = PdfBackend::Vector;
  RasterChart const custom;
#ifndef INSIGHTDOCK_HAS_PODOFO
  if (pdf) {
    CHECK(ChartExporter::encode(custom, options).error() == ChartError::PdfUnsupported);
    CHECK(custom.renders == 0);
    return;
  }
#endif
  CHECK(ChartExporter::encode(custom, options).error() == ChartError::VectorUnsupported);
  CHECK(custom.renders == 0);

  DecliningChart const declining;
  CHECK(ChartExporter::encode(declining, options).error() == ChartError::VectorUnsupported);
  CHECK(declining.renders == 0);
  LegacyVectorChart const legacy;
  auto native = bytes(ChartExporter::encode(legacy, options));
  CHECK(!contains_image(native, pdf));
  CHECK(legacy.renders == 0);
  // Native output ignores supersampling even when a raster at that resolution exceeds the cap.
  LegacyVectorChart large;
  large.options().width = large.options().height = 2000;
  options.supersample = 8;
  CHECK(static_cast<bool>(ChartExporter::encode(large, options)));
  options.supersample = 1;

  ThrowingVectorChart const vector_throwing;
  CHECK(ChartExporter::encode(vector_throwing, options).error() == ChartError::EncodeFailure);
  options.vector_fallback = VectorFallback::AllowRaster;
  auto fallback = bytes(ChartExporter::encode(custom, options));
  CHECK(contains_image(fallback, pdf));
  CHECK(custom.renders == 1);
  auto partial = bytes(ChartExporter::encode(declining, options));
  CHECK(contains_image(partial, pdf));
  if (!pdf)
    CHECK(partial.find("#0b1621") == std::string::npos);
  ThrowingChart const throwing;
  CHECK(ChartExporter::encode(throwing, options).error() == ChartError::RenderFailure);
  options.supersample = 2;
  CHECK(ChartExporter::encode(custom, options).error() == ChartError::RenderFailure);
  CloneChart const cloned;
  CHECK(static_cast<bool>(ChartExporter::encode(cloned, options)));
  CHECK(cloned.renders == 0 && cloned.options().width == 640);
  CloneChart oversized;
  oversized.options().width = oversized.options().height = 2000;
  options.supersample = 8;
  CHECK(ChartExporter::encode(oversized, options).error() == ChartError::InvalidInput);
  CHECK(oversized.renders == 0);
}

void const_exports(std::filesystem::path const &directory) {
  std::filesystem::create_directories(directory);
  BarChart bar;
  bar.set_categories({"A", "B"});
  bar.add_series("series", {1, 2});
  Chart const &chart = bar;
  ExportOptions vector;
  vector.format = OutputFormat::Svg;
  vector.svg_backend = SvgBackend::Vector;
  vector.vector_fallback = VectorFallback::Reject;
  auto expected = ChartExporter::encode(chart, vector);
  CHECK(static_cast<bool>(expected));
  auto path = directory / "const.svg";
  CHECK(static_cast<bool>(ChartExporter::save(chart, path.string(), vector)));
  std::ifstream file(path, std::ios::binary);
  std::string saved((std::istreambuf_iterator<char>(file)), {});
  CHECK(saved == bytes(expected));
  RasterChart const custom;
  CHECK(ChartExporter::save(custom, path.string(), vector).error() ==
        ChartError::VectorUnsupported);
  std::ifstream unchanged(path, std::ios::binary);
  CHECK(std::string((std::istreambuf_iterator<char>(unchanged)), {}) == saved);
  CHECK(custom.renders == 0);
  ExportOptions raster;
  raster.supersample = 2;
  raster.vector_fallback = VectorFallback::Reject; // Applies only to native vector requests.
  auto png = ChartExporter::encode(chart, raster);
  CHECK(static_cast<bool>(png));
  CHECK(static_cast<bool>(ChartExporter::save(chart, (directory / "const.png").string(), raster)));
  auto task = [&chart, raster] { return ChartExporter::encode(chart, raster); };
  auto first = std::async(std::launch::async, task);
  auto second = std::async(std::launch::async, task);
  auto a = first.get();
  auto b = second.get();
  CHECK(a && b && a.value == png.value && b.value == png.value);
  CHECK(bar.options().width == 640 && bar.options().font_size == 12);
  CHECK(ChartExporter::save(chart, "").error() == ChartError::BadPath);
  CHECK(ChartExporter::save(chart, directory.string()).error() == ChartError::BadPath);
  NonstandardChart const unusual;
  bool propagated = false;
  try {
    ChartExporter::encode(unusual);
  } catch (int code) {
    propagated = code == 42;
  }
  CHECK(propagated);
}

#ifdef INSIGHTDOCK_HAS_PODOFO
void embedding() {
  PoDoFo::PdfMemDocument document;
  PoDoFo::PdfPainter painter;
  auto &page = document.GetPages().CreatePage(PoDoFo::PdfPageSize::A4);
  painter.SetCanvas(page);
  ExportOptions options;
  options.pdf_backend = PdfBackend::Vector;
  options.vector_fallback = VectorFallback::Reject;
  RasterChart const custom;
  for (bool direct : {false, true}) {
    bool rejected = false;
    try {
      if (direct)
        ChartExporter::draw_pdf_vector(document, painter, custom, 0, 0, 200, 100, options);
      else
        ChartExporter::draw_pdf(document, painter, custom, 0, 0, 200, 100, options);
    } catch (std::runtime_error const &error) {
      rejected = std::string(error.what()) == chart_error_message(ChartError::VectorUnsupported);
    }
    CHECK(rejected);
  }
  CHECK(custom.renders == 0);
  LegacyVectorChart const legacy;
  ChartExporter::draw_pdf(document, painter, legacy, 0, 0, 200, 100, options);
  options.vector_fallback = VectorFallback::AllowRaster;
  ChartExporter::draw_pdf_vector(document, painter, custom, 0, 100, 200, 100, options);
  CHECK(custom.renders == 1);
  painter.FinishDrawing();
  std::vector<char> buffer;
  PoDoFo::VectorStreamDevice device(buffer);
  document.Save(device);
  CHECK(!buffer.empty());
}
#endif
} // namespace

int main(int argc, char **argv) {
  capability_queries();
  for (auto format : {OutputFormat::Svg, OutputFormat::PdfVector, OutputFormat::PdfRaster})
    vector_policy(format);
  const_exports(argc > 1 ? argv[1] : "export-contract-output");
#ifdef INSIGHTDOCK_HAS_PODOFO
  embedding();
#endif
  return failures ? 1 : 0;
}
