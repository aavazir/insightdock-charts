// SPDX-License-Identifier: MIT
// Copyright (c) InsightDock contributors

#include "insightdock/export.h"

#include "insightdock/detail/scaled_target.h"
#include "insightdock/detail/svg_target.h"
#ifdef INSIGHTDOCK_HAS_PIE
#include "insightdock/pie_chart.h"
#endif
#include "insightdock/png_writer.h"
#include "insightdock/svg_writer.h"

#ifdef INSIGHTDOCK_HAS_PODOFO
#include "insightdock/pdf/podofo_adapter.h"
#include "insightdock/pdf/podofo_target.h"
#include <podofo/podofo.h>
#endif

#include <algorithm>
#include <cctype>
#include <exception>
#include <fstream>
#ifdef _WIN32
#include <filesystem>
#endif
#include <memory>
#include <stdexcept>
#include <string>

namespace insightdock::charts {

namespace {

std::string ext_lower(std::string const &path) {
  auto dot = path.rfind('.');
  if (dot == std::string::npos || dot + 1 >= path.size())
    return {};
  std::string ext = path.substr(dot + 1);
  for (char &c : ext)
    c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
  return ext;
}

// File/byte output: vector PDF is chosen by the format, or by the Vector backend for a PDF target.
// A Vector pdf_backend on a PNG/JPEG/SVG export is meaningless and must not turn it into a PDF.
bool use_pdf_vector(ExportOptions const &opts) {
  return opts.format == OutputFormat::PdfVector ||
         (opts.format == OutputFormat::PdfRaster && opts.pdf_backend == PdfBackend::Vector);
}

// Embedding into an existing PdfPainter (draw_pdf) has no output format; only the backend counts.
[[maybe_unused]] bool use_pdf_vector_embed(ExportOptions const &opts) {
  return opts.format == OutputFormat::PdfVector || opts.pdf_backend == PdfBackend::Vector;
}

bool use_svg_vector(ExportOptions const &opts) {
  return opts.format == OutputFormat::Svg && opts.svg_backend == SvgBackend::Vector;
}

class SupersampleScope {
public:
  SupersampleScope(Chart &chart, int factor) : chart_(chart), factor_(factor), active_(factor > 1) {
    if (!active_)
      return;

    saved_ = chart_.options();
    auto &o = chart_.options();
    o.width *= static_cast<std::size_t>(factor_);
    o.height *= static_cast<std::size_t>(factor_);
    o.font_size *= static_cast<float>(factor_);
    o.margins.top *= static_cast<float>(factor_);
    o.margins.right *= static_cast<float>(factor_);
    o.margins.bottom *= static_cast<float>(factor_);
    o.margins.left *= static_cast<float>(factor_);

#ifdef INSIGHTDOCK_HAS_PIE
    if (auto *pie = dynamic_cast<PieChart *>(&chart_)) {
      saved_pie_ = pie->pie_options();
      PieOptions po = saved_pie_;
      if (po.center_x != 0.0 || po.center_y != 0.0) {
        po.center_x *= factor_;
        po.center_y *= factor_;
      }
      if (po.radius > 0.0)
        po.radius *= factor_;
      po.label_font_size *= static_cast<float>(factor_);
      po.stroke_width *= static_cast<float>(factor_);
      pie->set_pie_options(po);
    }
#endif
  }

  ~SupersampleScope() {
    if (!active_)
      return;
    chart_.options() = saved_;
#ifdef INSIGHTDOCK_HAS_PIE
    if (auto *pie = dynamic_cast<PieChart *>(&chart_))
      pie->set_pie_options(saved_pie_);
#endif
  }

private:
  Chart &chart_;
  int factor_;
  bool active_;
  ChartOptions saved_;
#ifdef INSIGHTDOCK_HAS_PIE
  PieOptions saved_pie_{};
#endif
};

constexpr int kMaxSupersample = 8;

// True if rendering at the requested supersample factor stays within ImageBuffer's size cap.
bool render_size_ok(Chart const &chart, ExportOptions const &opts) {
  std::size_t ss = static_cast<std::size_t>(std::clamp(opts.supersample, 1, kMaxSupersample));
  auto const &o = chart.options();
  if (o.width == 0 || o.height == 0 || o.width > ImageBuffer::kMaxPixels ||
      o.height > ImageBuffer::kMaxPixels)
    return false;
  return ImageBuffer::dimensions_ok(o.width * ss, o.height * ss);
}

// Renders `chart` at the requested supersample factor. The caller's chart is never modified:
// supersampling renders a scaled clone. Custom subclasses without clone() cannot supersample.
ImageBuffer render_scaled(Chart const &chart, ExportOptions const &opts) {
  if (!render_size_ok(chart, opts))
    return ImageBuffer();
  int ss = std::clamp(opts.supersample, 1, kMaxSupersample);
  if (ss <= 1)
    return chart.render();

  std::unique_ptr<Chart> copy = chart.clone();
  if (!copy)
    return ImageBuffer(); // a const custom chart must never be mutated by export
  ImageBuffer high(chart.options().width * ss, chart.options().height * ss);
  RasterTarget raster(high);
  detail::ScaledTarget scaled(raster, static_cast<float>(ss));
  if (copy->draw(scaled))
    return ImageBuffer::downsample(high, ss);

  // Compatibility path for custom charts that implement only render(). Their clone
  // receives scaled common options; custom pixel styles remain their responsibility.
  high = ImageBuffer();
  Chart &target = *copy;
  SupersampleScope scope(target, ss);
  ImageBuffer img = target.render();
  return ImageBuffer::downsample(img, ss);
}

#ifdef INSIGHTDOCK_HAS_PODOFO
std::vector<uint8_t> encode_pdf_page(ImageBuffer const &img);
std::vector<uint8_t> encode_pdf_vector_page(Chart const &chart);

void fit_box_on_a4(double aspect, double &x, double &y, double &w, double &h) {
  auto page = PoDoFo::PdfPage::CreateStandardPageSize(PoDoFo::PdfPageSize::A4);
  double pw = page.Width;
  double ph = page.Height;
  double margin = 72.0;
  double max_w = pw - 2.0 * margin;
  double max_h = ph - 2.0 * margin;

  w = max_w;
  h = w / aspect;
  if (h > max_h) {
    h = max_h;
    w = h * aspect;
  }
  x = margin + (max_w - w) * 0.5;
  y = margin + (max_h - h) * 0.5;
}
#endif

std::vector<uint8_t> encode_buffer(ImageBuffer const &img, ExportOptions const &opts) {
  switch (opts.format) {
  case OutputFormat::Png:
    return encode_png(img);
  case OutputFormat::Jpeg: {
    int q = std::clamp(opts.jpeg_quality, 1, 100);
    return encode_jpeg(img, q);
  }
  case OutputFormat::Svg:
    return encode_svg(img);
  case OutputFormat::PdfRaster:
#ifdef INSIGHTDOCK_HAS_PODOFO
    return encode_pdf_page(img);
#else
    return {};
#endif
  case OutputFormat::PdfVector:
    return {};
  }
  return {};
}

#ifdef INSIGHTDOCK_HAS_PODOFO

void fit_chart_on_a4(ImageBuffer const &img, PoDoFo::PdfDocument &doc, PoDoFo::PdfPainter &painter,
                     PoDoFo::PdfPage &page) {
  auto box = page.GetMediaBox();
  double pw = box.Width;
  double ph = box.Height;
  double margin = 72.0;
  double max_w = pw - 2.0 * margin;
  double max_h = ph - 2.0 * margin;

  double aspect = static_cast<double>(img.width()) / static_cast<double>(img.height());
  double draw_w = max_w;
  double draw_h = draw_w / aspect;
  if (draw_h > max_h) {
    draw_h = max_h;
    draw_w = draw_h * aspect;
  }
  double x = margin + (max_w - draw_w) * 0.5;
  double y = margin + (max_h - draw_h) * 0.5;
  podofo::draw_chart(doc, painter, img, x, y, draw_w, draw_h);
}

std::vector<uint8_t> encode_pdf_page(ImageBuffer const &img) {
  if (img.width() == 0 || img.height() == 0)
    return {};

  PoDoFo::PdfMemDocument doc;
  PoDoFo::PdfPainter painter;
  auto &page = doc.GetPages().CreatePage(PoDoFo::PdfPageSize::A4);
  painter.SetCanvas(page);
  fit_chart_on_a4(img, doc, painter, page);
  painter.FinishDrawing();

  std::vector<char> buffer;
  PoDoFo::VectorStreamDevice device(buffer);
  doc.Save(device);
  return std::vector<uint8_t>(buffer.begin(), buffer.end());
}

std::vector<uint8_t> encode_pdf_vector_page(Chart const &chart) {
  if (chart.options().width == 0 || chart.options().height == 0)
    return {};

  PoDoFo::PdfMemDocument doc;
  PoDoFo::PdfPainter painter;
  auto &page = doc.GetPages().CreatePage(PoDoFo::PdfPageSize::A4);
  painter.SetCanvas(page);

  double x, y, w, h;
  double aspect =
      static_cast<double>(chart.options().width) / static_cast<double>(chart.options().height);
  fit_box_on_a4(aspect, x, y, w, h);
  bool drawn;
  {
    podofo::PdfTarget target(doc, painter, static_cast<float>(chart.options().width),
                             static_cast<float>(chart.options().height), x, y, w, h);
    drawn = chart.draw(target);
  }
  if (!drawn)
    return {}; // Caller chooses whether to retry as raster.

  painter.FinishDrawing();
  std::vector<char> buffer;
  PoDoFo::VectorStreamDevice device(buffer);
  doc.Save(device);
  return std::vector<uint8_t>(buffer.begin(), buffer.end());
}

#endif // INSIGHTDOCK_HAS_PODOFO

} // namespace

OutputFormat format_from_path(std::string const &path) {
  std::string ext = ext_lower(path);
  if (ext == "jpg" || ext == "jpeg")
    return OutputFormat::Jpeg;
  if (ext == "svg")
    return OutputFormat::Svg;
  if (ext == "pdf")
    return OutputFormat::PdfRaster;
  return OutputFormat::Png;
}

ExportCapabilities ChartExporter::capabilities(Chart const &chart) noexcept {
  ExportCapabilities result;
  result.vector = chart.vector_capability();
#ifdef INSIGHTDOCK_HAS_PODOFO
  result.pdf_available = true;
#endif
  return result;
}

ImageBuffer ChartExporter::render(Chart &chart, ExportOptions const &opts) {
  return render_scaled(chart, opts);
}

ImageBuffer ChartExporter::render(Chart const &chart, ExportOptions const &opts) {
  return render_scaled(chart, opts);
}

ChartResult<void> ChartExporter::save(Chart &chart, std::string const &path, ExportOptions opts) {
  return save(static_cast<Chart const &>(chart), path, opts);
}

ChartResult<void> ChartExporter::save(Chart const &chart, std::string const &path,
                                      ExportOptions opts) {
  using R = ChartResult<void>;
  if (path.empty())
    return R::failure(ChartError::BadPath);

  std::string ext = ext_lower(path);
  if (ext == "jpg" || ext == "jpeg")
    opts.format = OutputFormat::Jpeg;
  else if (ext == "svg")
    opts.format = OutputFormat::Svg;
  else if (ext == "pdf") {
    if (opts.format != OutputFormat::PdfVector)
      opts.format = OutputFormat::PdfRaster;
  } else if (ext == "png")
    opts.format = OutputFormat::Png;

  auto encoded = encode(chart, opts);
  if (!encoded)
    return R::failure(encoded.error());
  try {
#ifdef _WIN32
    std::ofstream out(std::filesystem::u8path(path), std::ios::binary);
#else
    std::ofstream out(path, std::ios::binary);
#endif
    if (!out)
      return R::failure(ChartError::BadPath);
    out.write(reinterpret_cast<char const *>(encoded.value.data()),
              static_cast<std::streamsize>(encoded.value.size()));
    out.close(); // include flush/close failures in the result
    return out ? R::success() : R::failure(ChartError::BadPath);
  } catch (std::exception const &) {
    return R::failure(ChartError::BadPath);
  }
}

ChartResult<std::vector<uint8_t>> ChartExporter::encode(Chart &chart, ExportOptions const &opts) {
  return encode(static_cast<Chart const &>(chart), opts);
}

ChartResult<std::vector<uint8_t>> ChartExporter::encode(Chart const &chart,
                                                        ExportOptions const &opts) {
  using R = ChartResult<std::vector<uint8_t>>;
  bool vector_output = use_pdf_vector(opts) || use_svg_vector(opts);
  ExportOptions size_opts = opts;
  if (vector_output)
    size_opts.supersample = 1;
  if (!render_size_ok(chart, size_opts))
    return R::failure(ChartError::InvalidInput);
#ifndef INSIGHTDOCK_HAS_PODOFO
  if (opts.format == OutputFormat::PdfRaster || opts.format == OutputFormat::PdfVector)
    return R::failure(ChartError::PdfUnsupported);
#endif

  try {
#ifdef INSIGHTDOCK_HAS_PODOFO
    if (use_pdf_vector(opts)) {
      auto bytes = encode_pdf_vector_page(chart);
      if (!bytes.empty())
        return R::success(std::move(bytes));
    }
#endif
    if (use_svg_vector(opts)) {
      std::string xml = detail::build_chart_svg(chart);
      if (!xml.empty())
        return R::success(std::vector<uint8_t>(xml.begin(), xml.end()));
    }
  } catch (std::exception const &) {
    return R::failure(ChartError::EncodeFailure);
  }

  if (vector_output && opts.vector_fallback == VectorFallback::Reject)
    return R::failure(ChartError::VectorUnsupported);

  // A vector request declined by draw() uses the same quality and size checks as an explicit
  // raster request. Discard any partial vector drawing before encoding the image.
  ExportOptions raster_opts = opts;
  if (use_pdf_vector(opts))
    raster_opts.format = OutputFormat::PdfRaster;
  if (!render_size_ok(chart, raster_opts))
    return R::failure(ChartError::InvalidInput);

  ImageBuffer img;
  try {
    img = render_scaled(chart, raster_opts);
  } catch (std::exception const &) {
    return R::failure(ChartError::RenderFailure);
  }
  if (img.width() == 0 || img.height() == 0)
    return R::failure(ChartError::RenderFailure);
  try {
    auto bytes = encode_buffer(img, raster_opts);
    return bytes.empty() ? R::failure(ChartError::EncodeFailure) : R::success(std::move(bytes));
  } catch (std::exception const &) {
    return R::failure(ChartError::EncodeFailure);
  }
}

#ifdef INSIGHTDOCK_HAS_PODOFO

void ChartExporter::draw_pdf_vector(PoDoFo::PdfDocument &doc, PoDoFo::PdfPainter &painter,
                                    Chart const &chart, double x, double y, double w, double h,
                                    ExportOptions const &opts) {
  {
    auto const &o = chart.options();
    podofo::PdfTarget target(doc, painter, static_cast<float>(o.width),
                             static_cast<float>(o.height), x, y, w, h);
    if (chart.draw(target))
      return;
  }
  if (opts.vector_fallback == VectorFallback::Reject)
    throw std::runtime_error(chart_error_message(ChartError::VectorUnsupported));
  podofo::draw_chart(doc, painter, render_scaled(chart, opts), x, y, w, h);
}

void ChartExporter::draw_pdf(PoDoFo::PdfDocument &doc, PoDoFo::PdfPainter &painter,
                             Chart const &chart, double x, double y, double w, double h,
                             ExportOptions const &opts) {
  if (use_pdf_vector_embed(opts)) {
    draw_pdf_vector(doc, painter, chart, x, y, w, h, opts);
    return;
  }

  ImageBuffer img = render_scaled(chart, opts);
  podofo::draw_chart(doc, painter, img, x, y, w, h);
}

#endif // INSIGHTDOCK_HAS_PODOFO

} // namespace insightdock::charts
