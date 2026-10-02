// SPDX-License-Identifier: MIT
// Copyright (c) InsightDock contributors

#include "insightdock/export.h"

#ifdef INSIGHTDOCK_HAS_PIE
#include "insightdock/detail/svg_pie.h"
#include "insightdock/pie_chart.h"
#endif
#include "insightdock/png_writer.h"
#include "insightdock/svg_writer.h"

#ifdef INSIGHTDOCK_HAS_PODOFO
#include "insightdock/pdf/podofo_adapter.h"
#include "insightdock/pdf/podofo_vector.h"
#include <podofo/podofo.h>
#endif

#include <algorithm>
#include <memory>
#include <cctype>
#include <fstream>
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

bool save_svg_vector_page(Chart const &chart, std::string const &path) {
  if (path.empty())
    return false;
#ifdef INSIGHTDOCK_HAS_PIE
  if (auto const *pie = dynamic_cast<PieChart const *>(&chart)) {
    std::string xml = detail::build_pie_svg(*pie);
    if (xml.empty())
      return false;
    std::ofstream out(path, std::ios::binary);
    if (!out)
      return false;
    out << xml;
    return static_cast<bool>(out);
  }
#endif
  return write_svg(path, chart.render());
}

std::vector<uint8_t> encode_svg_vector_page(Chart const &chart) {
#ifdef INSIGHTDOCK_HAS_PIE
  if (auto const *pie = dynamic_cast<PieChart const *>(&chart)) {
    std::string xml = detail::build_pie_svg(*pie);
    return std::vector<uint8_t>(xml.begin(), xml.end());
  }
#endif
  return encode_svg(chart.render());
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
  if (o.width > ImageBuffer::kMaxPixels || o.height > ImageBuffer::kMaxPixels)
    return false;
  return ImageBuffer::dimensions_ok(o.width * ss, o.height * ss);
}

// Renders `chart` at the requested supersample factor. The caller's chart is never modified:
// supersampling renders a scaled clone. (Custom subclasses without clone() fall back to
// temporarily rewriting the original's options, which is not thread-safe.)
ImageBuffer render_scaled(Chart const &chart, ExportOptions const &opts) {
  if (!render_size_ok(chart, opts))
    return ImageBuffer();
  int ss = std::clamp(opts.supersample, 1, kMaxSupersample);
  if (ss <= 1)
    return chart.render();

  std::unique_ptr<Chart> copy = chart.clone();
  Chart &target = copy ? *copy : const_cast<Chart &>(chart);
  SupersampleScope scope(target, ss);
  ImageBuffer img = target.render();
  return ImageBuffer::downsample(img, ss);
}

#ifdef INSIGHTDOCK_HAS_PODOFO
bool save_pdf_page(ImageBuffer const &img, std::string const &path);
std::vector<uint8_t> encode_pdf_page(ImageBuffer const &img);
bool save_pdf_vector_page(Chart const &chart, std::string const &path);
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

bool write_buffer(std::string const &path, ImageBuffer const &img, ExportOptions const &opts) {
  switch (opts.format) {
  case OutputFormat::Png:
    return write_png(path, img);
  case OutputFormat::Jpeg: {
    int q = std::clamp(opts.jpeg_quality, 1, 100);
    return write_jpeg(path, img, q);
  }
  case OutputFormat::Svg:
    return write_svg(path, img);
  case OutputFormat::PdfRaster:
#ifdef INSIGHTDOCK_HAS_PODOFO
    return save_pdf_page(img, path);
#else
    (void)path;
    (void)img;
    return false;
#endif
  case OutputFormat::PdfVector:
    return false;
  }
  return false;
}

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

bool save_pdf_page(ImageBuffer const &img, std::string const &path) {
  if (img.width() == 0 || img.height() == 0 || path.empty())
    return false;

  PoDoFo::PdfMemDocument doc;
  PoDoFo::PdfPainter painter;
  auto &page = doc.GetPages().CreatePage(PoDoFo::PdfPageSize::A4);
  painter.SetCanvas(page);
  fit_chart_on_a4(img, doc, painter, page);
  painter.FinishDrawing();
  doc.Save(path);
  return true;
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

bool save_pdf_vector_page(Chart const &chart, std::string const &path) {
  if (path.empty() || chart.options().width == 0 || chart.options().height == 0)
    return false;

  PoDoFo::PdfMemDocument doc;
  PoDoFo::PdfPainter painter;
  auto &page = doc.GetPages().CreatePage(PoDoFo::PdfPageSize::A4);
  painter.SetCanvas(page);

  double x, y, w, h;
  double aspect =
      static_cast<double>(chart.options().width) / static_cast<double>(chart.options().height);
  fit_box_on_a4(aspect, x, y, w, h);
  podofo::draw_chart_vector(doc, painter, chart, x, y, w, h);

  painter.FinishDrawing();
  doc.Save(path);
  return true;
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
  podofo::draw_chart_vector(doc, painter, chart, x, y, w, h);

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

ImageBuffer ChartExporter::render(Chart &chart, ExportOptions const &opts) {
  return render_scaled(chart, opts);
}

ImageBuffer ChartExporter::render(Chart const &chart, ExportOptions const &opts) {
  return render_scaled(chart, opts);
}

ChartResult<void> ChartExporter::save(Chart &chart, std::string const &path, ExportOptions opts) {
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

#ifdef INSIGHTDOCK_HAS_PODOFO
  if (use_pdf_vector(opts)) {
    return save_pdf_vector_page(chart, path) ? R::success() : R::failure(ChartError::EncodeFailure);
  }
#else
  if (use_pdf_vector(opts))
    return R::failure(ChartError::PdfUnsupported);
#endif

  if (use_svg_vector(opts)) {
    return save_svg_vector_page(chart, path) ? R::success() : R::failure(ChartError::EncodeFailure);
  }

  if (!render_size_ok(chart, opts))
    return R::failure(ChartError::InvalidInput);
  ImageBuffer img = render_scaled(chart, opts);
  if (img.width() == 0 || img.height() == 0)
    return R::failure(ChartError::RenderFailure);

  return write_buffer(path, img, opts) ? R::success() : R::failure(ChartError::EncodeFailure);
}

ChartResult<std::vector<uint8_t>> ChartExporter::encode(Chart &chart, ExportOptions const &opts) {
  using R = ChartResult<std::vector<uint8_t>>;

#ifdef INSIGHTDOCK_HAS_PODOFO
  if (use_pdf_vector(opts)) {
    auto bytes = encode_pdf_vector_page(chart);
    return bytes.empty() ? R::failure(ChartError::EncodeFailure) : R::success(std::move(bytes));
  }
#else
  if (use_pdf_vector(opts))
    return R::failure(ChartError::PdfUnsupported);
#endif

  if (use_svg_vector(opts)) {
    auto bytes = encode_svg_vector_page(chart);
    return bytes.empty() ? R::failure(ChartError::EncodeFailure) : R::success(std::move(bytes));
  }

  if (!render_size_ok(chart, opts))
    return R::failure(ChartError::InvalidInput);
  ImageBuffer img = render_scaled(chart, opts);
  if (img.width() == 0 || img.height() == 0)
    return R::failure(ChartError::RenderFailure);

  auto bytes = encode_buffer(img, opts);
  return bytes.empty() ? R::failure(ChartError::EncodeFailure) : R::success(std::move(bytes));
}

#ifdef INSIGHTDOCK_HAS_PODOFO

void ChartExporter::draw_pdf_vector(PoDoFo::PdfDocument &doc, PoDoFo::PdfPainter &painter,
                                    Chart const &chart, double x, double y, double w, double h,
                                    ExportOptions const &) {
  podofo::draw_chart_vector(doc, painter, chart, x, y, w, h);
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
