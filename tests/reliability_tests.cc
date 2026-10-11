// SPDX-License-Identifier: MIT
// Copyright (c) InsightDock contributors

#include "insightdock/charts.h"
#include "insightdock/detail/svg_target.h"
#include "insightdock/detail/text_wrap.h"
#include "insightdock/detail/tick_format.h"

#include <algorithm>
#include <clocale>
#include <cmath>
#include <cstdio>
#include <ctime>
#include <filesystem>
#include <fstream>
#include <limits>
#include <locale>
#include <regex>
#include <stdexcept>
#include <string>
#include <typeinfo>

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

std::string svg(Chart &chart) {
  ExportOptions options;
  options.format = OutputFormat::Svg;
  options.svg_backend = SvgBackend::Vector;
  auto result = ChartExporter::encode(chart, options);
  CHECK(static_cast<bool>(result));
  return std::string(result.value.begin(), result.value.end());
}

void formatting() {
  CHECK(detail::format_tick_value(1.2, TickFormat::Raw, {}) == "1.2");
  CHECK(detail::format_tick_value(0.001, TickFormat::Raw, {}) == "0.001");
  CHECK(detail::format_tick_value(1e-10, TickFormat::Raw, {}) != "0");
  CHECK(detail::format_tick_value(-0.0, TickFormat::Raw, {}) == "0");
  CHECK(detail::format_axis_tick(1.25, 0.25, TickFormat::Raw, {}) == "1.25");
  CHECK(detail::format_axis_tick(0.3, 0.1, TickFormat::Raw, {}) == "0.3");
  CHECK(detail::format_axis_tick(1e15, 1.0, TickFormat::Raw, {}) !=
        detail::format_axis_tick(1e15 + 1.0, 1.0, TickFormat::Raw, {}));
  CHECK(detail::format_label_value(3.14159, TickFormat::Raw, 1000000).size() < 30);
  auto exact = [](double v) { return v == 1e-10 ? "small" : "changed"; };
  CHECK(detail::format_tick_value(1e-10, TickFormat::Raw, exact) == "small");

  // These expectations hold in every CTest timezone, including DST-observing zones.
  CHECK(detail::format_tick_value(0, TickFormat::DateTime, {}) == "1970-01-01 00:00");
  CHECK(detail::format_tick_value(1774137600, TickFormat::DateTime, {}) == "2026-03-22 00:00");
  for (double invalid : {std::nan(""), std::numeric_limits<double>::infinity(), 1e300, -1e300,
                         std::ldexp(1.0, std::numeric_limits<std::time_t>::digits)}) {
    CHECK(detail::format_tick_value(invalid, TickFormat::DateTime, {}).empty());
  }

  struct Comma : std::numpunct<char> {
    char do_decimal_point() const override {
      return ',';
    }
  };
  std::locale original = std::locale();
  std::locale::global(std::locale(original, new Comma));
  CHECK(detail::format_tick_value(1.2, TickFormat::Raw, {}) == "1.2");
  CHECK(detail::format_tick_value(1.2, TickFormat::Currency, {}) == "$1.20");
  CHECK(detail::format_label_value(1.2, TickFormat::Percent, 2) == "1.20%");
  std::locale::global(original);

  LineChart line;
  line.set_x_values({1774137600.0, 1774310400.0});
  line.add_series("time", {1.2, 1.4});
  line.cartesian_options().x_scale_type = ScaleType::DateTime;
  line.cartesian_options().custom_x_formatter = [](double) { return "custom time"; };
  CHECK(svg(line).find(">custom time</text>") != std::string::npos);
}

bool valid_utf8(std::string const &text) {
  for (std::size_t i = 0; i < text.size();) {
    unsigned char c = static_cast<unsigned char>(text[i++]);
    int remaining = c < 0x80             ? 0
                    : (c & 0xE0) == 0xC0 ? 1
                    : (c & 0xF0) == 0xE0 ? 2
                    : (c & 0xF8) == 0xF0 ? 3
                                         : -1;
    if (remaining < 0 || i + remaining > text.size())
      return false;
    while (remaining-- > 0)
      if ((static_cast<unsigned char>(text[i++]) & 0xC0) != 0x80)
        return false;
  }
  return true;
}

void pie_layout() {
  Font font;
  font.set_size(11.0f);
  std::string token;
  for (int i = 0; i < 32; ++i)
    token += "\xC3\xA9\xE4\xB8\xAD\xF0\x9F\x98\x80"; // 2-, 3-, 4-byte characters
  auto lines = detail::wrap_text(token, 30.0,
                                 [&](std::string const &text) { return font.measure_width(text); });
  CHECK(lines.size() > 1);
  std::string joined;
  for (auto const &line : lines) {
    CHECK(valid_utf8(line));
    CHECK(font.measure_width(line) <= 30.0f);
    joined += line;
  }
  CHECK(joined == token);
  auto paragraphs =
      detail::wrap_text("first\n\nlast\n", 1000.0, [](std::string const &) { return 1.0; });
  CHECK(paragraphs == std::vector<std::string>({"first", "", "last", ""}));

  PieChart pie;
  pie.options().width = 500;
  pie.options().height = 400;
  pie.options().title = "Shared pie";
  auto options = pie.pie_options();
  options.show_legend = false;
  options.show_percentage = false;
  options.fill_opacity = 0.5f;
  options.stroke_color = Color::rgb(102, 153, 204);
  options.stroke_width = 3.5f;
  options.data_labels.enabled = true;
  options.data_labels.decimals = 2;
  pie.set_pie_options(options);
  Color fill = Color::rgb(204, 51, 102);
  fill.a = 128;
  pie.add_slice(token + " LABEL_END", 60.0, fill);
  pie.add_slice("Other", 40.0, Color::rgb(17, 34, 51));
  auto xml = svg(pie);
  CHECK(valid_utf8(xml));
  CHECK(xml.find("LABEL_END</text>") != std::string::npos);
  CHECK(xml.find("fill=\"#cc3366\" fill-opacity=\"0.25\"") != std::string::npos);
  CHECK(xml.find("stroke=\"#6699cc\" stroke-width=\"3.5\"") != std::string::npos);
  CHECK(xml.find(">60.00</text>") != std::string::npos);
  CHECK(xml.find(">40.00</text>") != std::string::npos);
  CHECK(xml.find("base64") == std::string::npos);

  // Both font measurement and SVG coordinates must ignore C++ locale punctuation/grouping.
  struct Comma : std::numpunct<char> {
    char do_decimal_point() const override {
      return ',';
    }
    char do_thousands_sep() const override {
      return '_';
    }
    std::string do_grouping() const override {
      return "\3";
    }
  };
  auto original = std::locale();
  std::locale::global(std::locale(original, new Comma));
  CHECK(svg(pie) == xml);
  detail::SvgTarget target(250.5f, 100.25f);
  target.fill_circle(12.5f, 17.25f, 3.5f, fill);
  auto geometry = target.str();
  std::locale::global(original);
  CHECK(geometry.find("width=\"250.5\"") != std::string::npos);
  CHECK(geometry.find("cx=\"12.5\" cy=\"17.25\"") != std::string::npos);

  // snprintf follows LC_NUMERIC, a separate global setting from an unnamed C++ locale.
  // Exercise decimal-comma locales where installed; CI platforms may only ship the C locale.
  std::string numeric_locale = std::setlocale(LC_NUMERIC, nullptr);
  for (auto name : {"en_DK.UTF-8", "de_DE.UTF-8", "fr_FR.UTF-8", "Danish_Denmark.1252"}) {
    if (std::setlocale(LC_NUMERIC, name)) {
      CHECK(svg(pie) == xml);
      break;
    }
  }
  std::setlocale(LC_NUMERIC, numeric_locale.c_str());

  // Finite values whose naive sum overflows still give two proportional sectors.
  PieChart huge;
  huge.add_slice("large A", std::numeric_limits<double>::max());
  huge.add_slice("large B", std::numeric_limits<double>::max());
  auto large_xml = svg(huge);
  CHECK(large_xml.find(">50.0%</text>") != std::string::npos);
  CHECK(large_xml.find("<path") != std::string::npos);
  huge.add_slice("negative", -1.0);
  CHECK(huge.slices().back().value == 0.0);

  PieChart rounding;
  rounding.add_slice("Organic", 45.0);
  rounding.add_slice("Paid", 25.0);
  rounding.add_slice("Referral", 18.0);
  rounding.add_slice("Social", 12.0);
  CHECK(svg(rounding).find(">Organic (45%)</text>") != std::string::npos);
}

void layout() {
  BarChart bar;
  bar.set_categories({"a", "b"});
  bar.add_series("legend sentinel", {1.0, 2.0});
  bar.options().title = "Title";
  bar.options().legend.position = LegendOptions::Position::Top;
  std::string top = svg(bar);
  auto label = top.find(">legend sentinel</text>");
  CHECK(label != std::string::npos);
  if (label != std::string::npos) {
    auto element = top.rfind("<text", label);
    auto y_attr = top.find(" y=\"", element);
    CHECK(y_attr != std::string::npos && y_attr < label);
    if (y_attr != std::string::npos)
      CHECK(std::stod(top.substr(y_attr + 4)) < 100.0);
  }
  bar.cartesian_options().x_label_mode = CartesianOptions::XLabelMode::Truncate;
  bar.options().width = 180;
  bar.options().legend.enabled = false;
  std::string accented, cjk;
  for (int i = 0; i < 20; ++i) {
    accented += "\xC3\xA9";
    cjk += "\xE4\xB8\xAD";
  }
  bar.set_categories({accented, cjk});
  std::string truncated = svg(bar);
  CHECK(truncated.find("...") != std::string::npos);
  CHECK(valid_utf8(truncated));
}

// Inspect exported positions rather than duplicating the layout algorithm. Legend text must
// remain on the canvas and text rectangles must not intersect, including wrapped continuations.
void check_legend_bounds(std::string const &xml, float width, float height) {
  Font font;
  font.set_size(12);
  struct Box {
    float x, y, w, h;
  };
  std::vector<Box> boxes;
  std::regex text(R"re(<text x="([^"]+)" y="([^"]+)"[^>]*>([^<]*)</text>)re");
  for (auto it = std::sregex_iterator(xml.begin(), xml.end(), text); it != std::sregex_iterator();
       ++it) {
    auto label = (*it)[3].str();
    if (label.empty() || (label.front() != 'L' && label.front() != 'W' && label.front() != '+' &&
                          static_cast<unsigned char>(label.front()) < 128))
      continue;
    float x = std::stof((*it)[1]), y = std::stof((*it)[2]);
    float w = font.measure_width(label), h = font.line_height();
    CHECK(x >= 0 && x + w <= width + 0.1f);
    CHECK(y >= 0 && y + h <= height + 0.1f);
    for (auto const &b : boxes)
      CHECK(x + w <= b.x + 0.1f || b.x + b.w <= x + 0.1f || y + h <= b.y + 0.1f ||
            b.y + b.h <= y + 0.1f);
    boxes.push_back({x, y, w, h});
  }
  CHECK(!boxes.empty());
  CHECK(valid_utf8(xml));
}

// Measure text emitted by the shared renderer, including title/legend intersections. SVG stores
// alphabetic baselines; recover the top of each font's line box before comparing rectangles.
void check_pie_text_bounds(std::string const &xml, float width, float height) {
  struct Box {
    float x, y, w, h;
  };
  std::vector<Box> boxes;
  std::regex text(
      R"re(<text x="([^"]+)" y="([^"]+)" font-size="([^"]+)" text-anchor="([^"]+)"[^>]*>([^<]*)</text>)re");
  for (auto it = std::sregex_iterator(xml.begin(), xml.end(), text); it != std::sregex_iterator();
       ++it) {
    if ((*it)[5].str().empty())
      continue;
    Font font;
    font.set_size(std::stof((*it)[3]));
    float w = font.measure_width((*it)[5]), h = font.line_height();
    float x = std::stof((*it)[1]);
    float y = std::stof((*it)[2]) - font.baseline_offset(TextBaseline::Top);
    if ((*it)[4] == "end")
      x -= w;
    else if ((*it)[4] == "middle")
      x -= w * 0.5f;
    CHECK(x >= -0.1f && x + w <= width + 0.1f);
    CHECK(y >= -0.1f && y + h <= height + 0.1f);
    for (auto const &b : boxes)
      CHECK(x + w <= b.x + 0.1f || b.x + b.w <= x + 0.1f || y + h <= b.y + 0.1f ||
            b.y + b.h <= y + 0.1f);
    boxes.push_back({x, y, w, h});
  }
  CHECK(!boxes.empty());
}

void check_pie_leaders(std::string const &xml, float cx, float cy, float radius) {
  struct Segment {
    float x1, y1, x2, y2;
  };
  std::vector<Segment> segments;
  std::regex line(R"re(<line x1="([^"]+)" y1="([^"]+)" x2="([^"]+)" y2="([^"]+)"[^>]*/>)re");
  for (auto it = std::sregex_iterator(xml.begin(), xml.end(), line); it != std::sregex_iterator();
       ++it) {
    Segment s{std::stof((*it)[1]), std::stof((*it)[2]), std::stof((*it)[3]), std::stof((*it)[4])};
    float dx = s.x2 - s.x1, dy = s.y2 - s.y1;
    float length_sq = dx * dx + dy * dy;
    float t = length_sq > 0
                  ? std::clamp(((cx - s.x1) * dx + (cy - s.y1) * dy) / length_sq, 0.0f, 1.0f)
                  : 0;
    float distance = std::hypot(s.x1 + t * dx - cx, s.y1 + t * dy - cy);
    CHECK(distance >= radius - 0.1f);
    auto side = [](Segment const &a, float x, float y) {
      return (a.x2 - a.x1) * (y - a.y1) - (a.y2 - a.y1) * (x - a.x1);
    };
    for (auto const &other : segments) {
      // Shared elbows are allowed; proper segment crossings are not.
      bool crosses = side(s, other.x1, other.y1) * side(s, other.x2, other.y2) < -0.01f &&
                     side(other, s.x1, s.y1) * side(other, s.x2, s.y2) < -0.01f;
      CHECK(!crosses);
    }
    segments.push_back(s);
  }
  CHECK(!segments.empty());
}

void pie_callouts(std::filesystem::path const &work_dir) {
  std::filesystem::create_directories(work_dir);
  for (auto position : {PieLegendPosition::Bottom, PieLegendPosition::Right}) {
    for (bool percentage : {false, true}) {
      PieChart pie;
      pie.options().width = 500;
      pie.options().height = 400;
      pie.options().title = "Crowded pie";
      auto options = pie.pie_options();
      options.legend_position = position;
      options.show_percentage = percentage;
      pie.set_pie_options(options);
      // Both sides contain nearly coincident slice midpoints, long tokens and explicit lines.
      for (int i = 0; i < 18; ++i)
        pie.add_slice("Device" + std::to_string(i) + "\nlong-device-token", 1);
      pie.add_slice("Major", 82);
      auto xml = svg(pie);
      check_pie_text_bounds(xml, 500, 400);
      CHECK(xml.find(">Major</text>") != std::string::npos);
      CHECK(xml == svg(pie));
      CHECK(ChartExporter::save(
          pie, (work_dir / ("callouts-" + std::to_string(static_cast<int>(position)) +
                            (percentage ? "-pct.png" : ".png")))
                   .string()));
      // Resize after assigning options: layout must recompute available label and legend space.
      pie.options().width = 240;
      pie.options().height = 200;
      check_pie_text_bounds(svg(pie), 240, 200);
    }
  }
  PieChart pie;
  pie.options().width = 600;
  pie.options().height = 400;
  auto options = pie.pie_options();
  options.show_legend = false;
  options.show_percentage = true;
  options.center_x = 300;
  options.center_y = 200;
  options.radius = 100;
  pie.set_pie_options(options);
  pie.add_slice("First\n\nLast", 50);
  pie.add_slice("Other", 50);
  auto xml = svg(pie);
  check_pie_text_bounds(xml, 600, 400);
  CHECK(xml.find(">First</text>") != std::string::npos);
  CHECK(xml.find(">Last</text>") != std::string::npos);
  pie.clear_slices();
  for (int side = 0; side < 2; ++side) {
    for (int i = 0; i < 18; ++i)
      pie.add_slice("Cluster" + std::to_string(side) + std::to_string(i), 0.5);
    pie.add_slice("Large" + std::to_string(side), 41);
  }
  xml = svg(pie);
  check_pie_text_bounds(xml, 600, 400);
  check_pie_leaders(xml, 300, 200, 100);
  CHECK(xml.find(">Large0</text>") != std::string::npos);
  CHECK(xml.find(">Large1</text>") != std::string::npos);
  CHECK(xml.find(">Cluster017</text>") == std::string::npos);
  // Oversized label blocks are omitted as a whole rather than clipped or truncated.
  pie.clear_slices();
  pie.add_slice(std::string(2000, 'W'), 50);
  pie.add_slice("Retained", 50);
  xml = svg(pie);
  CHECK(xml.find(">Retained</text>") != std::string::npos);
  CHECK(xml.find(">W") == std::string::npos);
  check_pie_text_bounds(xml, 600, 400);
  // An explicit radius may leave no horizontal room. It remains explicit and labels are hidden.
  options.radius = 195;
  options.center_x = 200;
  options.center_y = 200;
  pie.options().width = 400;
  pie.set_pie_options(options);
  CHECK(svg(pie).find("<text") == std::string::npos);
}

void legends(std::filesystem::path const &work_dir) {
  std::filesystem::create_directories(work_dir);
  for (auto position : {LegendOptions::Position::Top, LegendOptions::Position::Bottom,
                        LegendOptions::Position::Right}) {
    BarChart bar;
    bar.options().width = 360;
    bar.options().height = 360;
    bar.options().legend.position = position;
    bar.cartesian_options().x_axis_label = "Axis title";
    bar.set_categories({"a", "b"});
    for (int i = 0; i < 6; ++i)
      bar.add_series("Legend" + std::to_string(i), {1.0 + i, 2.0 + i});
    auto xml = svg(bar);
    check_legend_bounds(xml, 360, 360);
    if (position != LegendOptions::Position::Right)
      CHECK(xml.find(">Legend5</text>") != std::string::npos);
    CHECK(ChartExporter::save(
        bar,
        (work_dir / ("legend-" + std::to_string(static_cast<int>(position)) + ".png")).string()));
    for (int i = 6; i < 40; ++i)
      bar.add_series("Legend" + std::to_string(i), {1.0, 2.0});
    xml = svg(bar);
    check_legend_bounds(xml, 360, 360);
    CHECK(xml.find(" more</text>") != std::string::npos);
    CHECK(xml.find(">Legend39</text>") == std::string::npos);
  }
  for (auto position : {PieLegendPosition::Bottom, PieLegendPosition::Right}) {
    PieChart pie;
    pie.options().width = 360;
    pie.options().height = 360;
    auto options = pie.pie_options();
    options.show_labels = false;
    options.show_percentage = false;
    options.legend_position = position;
    pie.set_pie_options(options);
    // Automatic geometry remains automatic when options are assigned and the canvas changes.
    CHECK(pie.pie_options().radius == 0 && pie.pie_options().center_x == 0);
    pie.add_slice(std::string(24, 'W'), 3);
    pie.add_slice("Legend\nLegendNext", 2);
    auto xml = svg(pie);
    check_legend_bounds(xml, 360, 360);
    CHECK(xml.find(">LegendNext</text>") != std::string::npos);
    CHECK(ChartExporter::save(
        pie, (work_dir / ("pie-legend-" + std::to_string(static_cast<int>(position)) + ".png"))
                 .string()));
    for (int i = 0; i < 40; ++i)
      pie.add_slice("Legend" + std::to_string(i), 1);
    xml = svg(pie);
    check_legend_bounds(xml, 360, 360);
    CHECK(xml.find(" more</text>") != std::string::npos);
  }
  ScatterChart scatter;
  scatter.options().width = 360;
  scatter.options().height = 360;
  scatter.options().legend.position = LegendOptions::Position::Top;
  scatter.add_series("Legend scatter", {{1, 2}, {2, 3}}, Color::rgb(255, 0, 255));
  auto count_swatch = [](ImageBuffer const &image) {
    int count = 0;
    for (std::size_t y = 0; y < 64; ++y)
      for (std::size_t x = 0; x < image.width(); ++x) {
        auto i = (y * image.width() + x) * 4;
        auto const &p = image.data();
        if (p[i] == 255 && p[i + 1] == 0 && p[i + 2] == 255)
          ++count;
      }
    return count;
  };
  CHECK(count_swatch(scatter.render()) >= 90);
  scatter.options().legend.enabled = false;
  CHECK(count_swatch(scatter.render()) == 0);
  scatter.options().legend.enabled = true;
  for (int i = 0; i < 40; ++i)
    scatter.add_series("Legend scatter " + std::to_string(i), {{1, 2}, {2, 3}});
  CHECK(ChartExporter::save(scatter, (work_dir / "scatter-legend-overflow.png").string()));
  BarChart unicode;
  unicode.options().legend.position = LegendOptions::Position::Right;
  unicode.set_categories({"a"});
  std::string label;
  for (int i = 0; i < 35; ++i)
    label += "\xC3\xA9";
  unicode.add_series(label, {1});
  check_legend_bounds(svg(unicode), 640, 400);
}

class ThrowingChart : public Chart {
public:
  ImageBuffer render() const override {
    throw std::runtime_error("render failed");
  }
};

class ThrowingVectorChart : public ThrowingChart {
public:
  bool draw(DrawTarget &) const override {
    throw std::runtime_error("vector failed");
  }
};

class UnclonableChart : public Chart {
public:
  ImageBuffer render() const override {
    return ImageBuffer(options_.width, options_.height);
  }
};

// Compare final-pixel ink coverage, so thinner marks or smaller labels cannot hide
// behind a whole-image difference dominated by the white background.
double color_ink(ImageBuffer const &image, int channel) {
  double sum = 0;
  for (std::size_t y = 0; y < image.height(); ++y)
    for (std::size_t x = 0; x < image.width(); ++x) {
      auto p = image.pixel(x, y);
      sum += std::max(0, int(p[channel]) -
                             std::max(int(p[(channel + 1) % 3]), int(p[(channel + 2) % 3]))) /
             255.0;
    }
  return sum;
}

void sampling_coverage(Chart &chart) {
  chart.options().width = 480;
  chart.options().height = 320;
  chart.options().legend.enabled = false;
  chart.options().show_grid = false;
  chart.options().axis_color = Color::white();
  chart.options().text_color = Color::white();
  // Exercise an explicitly supplied shared font as well as independently sized labels.
  Font font;
  chart.options().font = font.data();
  auto before = chart.options();
  auto base = chart.render();
  for (int factor : {2, 4}) {
    ExportOptions options;
    options.supersample = factor;
    auto image = ChartExporter::render(static_cast<Chart const &>(chart), options);
    CHECK(image.width() == base.width() && image.height() == base.height());
    if (image.width() != base.width() || image.height() != base.height())
      continue;
    for (int channel : {0, 1}) {
      double reference = color_ink(base, channel);
      CHECK(reference > 10);
      double actual = color_ink(image, channel);
      if (std::fabs(actual - reference) / reference >= 0.15)
        std::fprintf(stderr, "%s factor %d channel %d ink %.2f -> %.2f\n", typeid(chart).name(),
                     factor, channel, reference, actual);
      CHECK(std::fabs(actual - reference) / reference < 0.15);
    }
  }
  CHECK(chart.options().width == before.width && chart.options().height == before.height);
  CHECK(chart.options().font_size == before.font_size && chart.options().font == before.font);
}

class StrokeChart final : public Chart {
public:
  StrokeChart() {
    options_.width = 240;
    options_.height = 120;
  }
  std::unique_ptr<Chart> clone() const override {
    return std::make_unique<StrokeChart>(*this);
  }
  ImageBuffer render() const override {
    ImageBuffer image(options_.width, options_.height);
    RasterTarget target(image);
    draw(target);
    return image;
  }
  bool draw(DrawTarget &target) const override {
    target.clear(Color::white());
    target.draw_line(20, 20, 200, 20, 12, Color::rgba(255, 0, 0, 128));
    target.draw_dashed_line(20, 50, 200, 50, 4, Color::rgb(255, 0, 0), {12, 8});
    DrawTarget::Point points[] = {{20, 80}, {87, 80}, {200, 80}};
    target.draw_dashed_polyline(points, 3, 4, Color::rgb(255, 0, 0), {12, 8});
    return true;
  }
};

void stroke_sampling() {
  StrokeChart chart;
  for (int factor : {1, 2, 4, 8}) {
    ExportOptions options;
    options.supersample = factor;
    auto image = ChartExporter::render(chart, options);
    CHECK(image.width() == 240 && image.height() == 120);
    if (image.width() != 240 || image.height() != 120)
      continue;
    // Thick translucent strokes have a fully filled interior, blended once.
    for (int y = 16; y < 24; ++y)
      for (int x = 25; x < 195; ++x) {
        auto pixel = image.pixel(x, y);
        CHECK(pixel[0] == 255 && pixel[1] >= 126 && pixel[1] <= 128);
      }
    // Check dash lengths and continuity across the polyline's intermediate vertex.
    for (int y : {50, 80})
      for (int cycle = 0; cycle < 8; ++cycle) {
        CHECK(image.pixel(25 + 20 * cycle, y)[1] < 10);
        CHECK(image.pixel(36 + 20 * cycle, y)[1] > 245);
      }
  }
}

void supersampling() {
  Color red = Color::rgb(255, 0, 0);
  DataLabelOptions labels;
  labels.enabled = true;
  labels.color = Color::rgb(0, 255, 0);
  labels.font_size = 18;

  LineChart line;
  line.set_categories({"A", "B", "C"});
  SeriesStyle style;
  style.line_width = 5;
  style.dash = DashPattern{12, 8};
  style.marker = PointMarkerShape::Triangle;
  line.add_series("line", {2, 6, 4}, red, style);
  auto &lo = line.line_options();
  lo.line_width = 5;
  lo.point_radius = 9;
  lo.dash = {12, 8};
  lo.point_shape = PointMarkerShape::Triangle;
  lo.data_labels = labels;
  sampling_coverage(line);
  CHECK(lo.line_width == 5 && lo.point_radius == 9 && lo.dash == DashPattern({12, 8}));
  CHECK(lo.data_labels.font_size == 18);

  BarChart bar;
  bar.set_categories({"A", "B", "C"});
  bar.add_series("bars", {2, 6, 4}, red);
  bar.bar_options().corner_radius = 8;
  bar.bar_options().border_width = 4;
  bar.bar_options().border_color = red;
  bar.bar_options().data_labels = labels;
  sampling_coverage(bar);

  AreaChart area;
  area.set_categories({"A", "B", "C"});
  area.add_series("area", {2, 6, 4}, red);
  area.area_options().data_labels = labels;
  sampling_coverage(area);

  ComboChart combo;
  combo.set_categories({"A", "B", "C"});
  combo.add_bar_series("bar", {2, 6, 4}, red);
  combo.add_line_series("line", {4, 3, 5}, red);
  combo.line_options().line_width = 5;
  combo.line_options().point_radius = 9;
  combo.line_options().dash = {12, 8};
  combo.line_options().data_labels = labels;
  combo.bar_options().data_labels = labels;
  sampling_coverage(combo);

  PieChart pie;
  pie.add_slice("A", 2, red);
  pie.add_slice("B", 6, red);
  pie.add_slice("C", 4, red);
  auto po = pie.pie_options();
  po.show_labels = po.show_legend = false;
  po.radius = 100;
  po.inner_radius_ratio = 0.35;
  po.stroke_width = 5;
  po.data_labels = labels;
  pie.set_pie_options(po);
  sampling_coverage(pie);
  CHECK(pie.pie_options().radius == 100 && pie.pie_options().stroke_width == 5);

  ScatterChart scatter;
  scatter.add_series("points", {{1, 2}, {2, 6}, {3, 4}}, red);
  scatter.scatter_options().point_radius = 12;
  scatter.scatter_options().data_labels = labels;
  sampling_coverage(scatter);
  CHECK(scatter.scatter_options().point_radius == 12);
  auto native = svg(scatter);
  CHECK(native.find("<image") == std::string::npos);
  CHECK(native.find("<circle") != std::string::npos);
  scatter.clear_series();
  scatter.add_series("bubbles", {{1, 2, 1}, {2, 6, 2}, {3, 4, 3}}, red);
  scatter.scatter_options().min_bubble_radius = 9;
  scatter.scatter_options().max_bubble_radius = 22;
  sampling_coverage(scatter);
#ifdef INSIGHTDOCK_HAS_PODOFO
  ExportOptions pdf;
  pdf.format = OutputFormat::PdfVector;
  auto encoded = ChartExporter::encode(scatter, pdf);
  CHECK(static_cast<bool>(encoded));
  std::string bytes(encoded.value.begin(), encoded.value.end());
  CHECK(bytes.find("/Subtype /Image") == std::string::npos);
#endif
}

void export_errors(std::filesystem::path const &work_dir) {
  BarChart bar;
  bar.set_categories({"a", "b"});
  bar.add_series("s", {1, 2});
  // Writing a file over the working directory fails portably, including Windows.
  CHECK(ChartExporter::save(bar, ".").error() == ChartError::BadPath);
  CHECK(ChartExporter::save(bar, "").error() == ChartError::BadPath);
  ThrowingChart throwing;
  CHECK(ChartExporter::encode(throwing).error() == ChartError::RenderFailure);
  ThrowingVectorChart vector_throwing;
  ExportOptions vector;
  vector.format = OutputFormat::Svg;
  vector.svg_backend = SvgBackend::Vector;
  CHECK(ChartExporter::encode(vector_throwing, vector).error() == ChartError::EncodeFailure);
  bar.options().width = ImageBuffer::kMaxPixels;
  CHECK(ChartExporter::encode(bar, vector).error() == ChartError::InvalidInput);
  bar.options().width = 0;
  CHECK(ChartExporter::encode(bar, vector).error() == ChartError::InvalidInput);
  bar.options().width = 640;
#ifndef INSIGHTDOCK_HAS_PODOFO
  for (auto format : {OutputFormat::PdfRaster, OutputFormat::PdfVector}) {
    ExportOptions pdf;
    pdf.format = format;
    CHECK(ChartExporter::encode(bar, pdf).error() == ChartError::PdfUnsupported);
  }
#endif
  UnclonableChart const custom;
  ExportOptions supersampled;
  supersampled.supersample = 2;
  CHECK(ChartExporter::render(custom, supersampled).width() == 0);
  CHECK(custom.options().width == 640 && custom.options().font_size == 12.0f);

  // A sparse oversized font file must be rejected before allocating/reading its contents.
  std::filesystem::create_directories(work_dir);
  std::filesystem::path path = work_dir / "oversized-font-reliability.tmp";
  {
    std::ofstream file(path, std::ios::binary);
    file.seekp((std::streamoff{64} << 20));
    file.put('x');
    CHECK(static_cast<bool>(file));
  }
  CHECK(load_font_data(path.string().c_str()) == nullptr);
  std::filesystem::remove(path);
}
} // namespace

int main(int argc, char **argv) {
  formatting();
  layout();
  pie_layout();
  pie_callouts(argc > 1 ? argv[1] : "reliability-output");
  legends(argc > 1 ? argv[1] : "reliability-output");
  stroke_sampling();
  supersampling();
  export_errors(argc > 1 ? argv[1] : "reliability-output");
  return failures ? 1 : 0;
}
