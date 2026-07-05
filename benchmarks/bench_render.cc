// SPDX-License-Identifier: MIT
// Copyright (c) InsightDock contributors
//
// Simple std::chrono timing harness — no external dependency (no Google
// Benchmark). Build in Release mode (-O2/-O3) and run directly; numbers get
// recorded by hand in docs/BENCHMARKS.md.

#include "insightdock/charts.h"

#include <chrono>
#include <cmath>
#include <cstdio>
#include <string>
#include <vector>

#if defined(__unix__) || defined(__APPLE__)
#include <sys/resource.h>
#define INSIGHTDOCK_BENCH_HAS_RUSAGE 1
#endif

using namespace insightdock::charts;
using Clock = std::chrono::steady_clock;

namespace {

double elapsed_ms(Clock::time_point start, Clock::time_point end) {
  return std::chrono::duration<double, std::milli>(end - start).count();
}

long peak_rss_kb() {
#ifdef INSIGHTDOCK_BENCH_HAS_RUSAGE
  struct rusage ru {};
  getrusage(RUSAGE_SELF, &ru);
#if defined(__APPLE__)
  return ru.ru_maxrss / 1024; // macOS reports bytes, not KB
#else
  return ru.ru_maxrss; // Linux reports KB
#endif
#else
  return -1;
#endif
}

std::vector<std::string> make_categories(std::size_t n) {
  std::vector<std::string> cats(n);
  for (std::size_t i = 0; i < n; ++i)
    cats[i] = std::to_string(i);
  return cats;
}

std::vector<double> make_series(std::size_t n) {
  std::vector<double> v(n);
  for (std::size_t i = 0; i < n; ++i)
    v[i] = 50.0 + 40.0 * std::sin(static_cast<double>(i) * 0.01);
  return v;
}

void report(char const *name, double ms) {
  std::printf("%-32s %10.2f ms\n", name, ms);
}

void bench_line_render(std::size_t n) {
  LineChart chart;
  chart.set_categories(make_categories(n));
  chart.add_series("S", make_series(n));

  auto start = Clock::now();
  auto img = chart.render();
  auto end = Clock::now();
  (void)img;

  char name[64];
  std::snprintf(name, sizeof(name), "line render (%zu pts)", n);
  report(name, elapsed_ms(start, end));
}

void bench_bar_render(std::size_t categories, std::size_t series) {
  BarChart chart;
  chart.set_categories(make_categories(categories));
  for (std::size_t s = 0; s < series; ++s)
    chart.add_series("S" + std::to_string(s), make_series(categories));

  auto start = Clock::now();
  auto img = chart.render();
  auto end = Clock::now();
  (void)img;

  char name[64];
  std::snprintf(name, sizeof(name), "bar render (%zux%zu)", categories, series);
  report(name, elapsed_ms(start, end));
}

void bench_pie_render(std::size_t slices) {
  PieChart chart;
  for (std::size_t i = 0; i < slices; ++i)
    chart.add_slice("S" + std::to_string(i), 1.0 + static_cast<double>(i));

  auto start = Clock::now();
  auto img = chart.render();
  auto end = Clock::now();
  (void)img;

  char name[64];
  std::snprintf(name, sizeof(name), "pie render (%zu slices)", slices);
  report(name, elapsed_ms(start, end));
}

void bench_png_encode() {
  PieChart chart;
  chart.options().width = 640;
  chart.options().height = 400;
  chart.add_slice("A", 40);
  chart.add_slice("B", 35);
  chart.add_slice("C", 25);
  auto img = chart.render();

  auto start = Clock::now();
  auto bytes = encode_png(img);
  auto end = Clock::now();
  (void)bytes;

  report("PNG encode (640x400)", elapsed_ms(start, end));
}

#ifdef INSIGHTDOCK_HAS_PODOFO
void bench_pdf_export() {
  PieChart chart;
  chart.options().width = 640;
  chart.options().height = 400;
  chart.add_slice("A", 40);
  chart.add_slice("B", 35);
  chart.add_slice("C", 25);

  ExportOptions opts;
  opts.format = OutputFormat::PdfRaster;

  auto start = Clock::now();
  auto result = ChartExporter::encode(chart, opts);
  auto end = Clock::now();
  (void)result;

  report("PDF raster export (single chart)", elapsed_ms(start, end));
}
#endif

} // namespace

int main() {
  std::printf("InsightDock render benchmarks\n");
  std::printf("------------------------------------------------\n");

  bench_line_render(1000);
  bench_line_render(10000);
  bench_line_render(100000);
  bench_bar_render(50, 10);
  bench_pie_render(20);
  bench_png_encode();
#ifdef INSIGHTDOCK_HAS_PODOFO
  bench_pdf_export();
#endif

  long rss = peak_rss_kb();
  if (rss >= 0)
    std::printf("\npeak RSS: %ld KB\n", rss);

  return 0;
}
