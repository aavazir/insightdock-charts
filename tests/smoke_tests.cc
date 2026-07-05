// SPDX-License-Identifier: MIT
// Copyright (c) InsightDock contributors

#include "insightdock/charts.h"

#include <cstdint>
#include <fstream>
#include <iostream>
#include <string>

namespace {

bool file_starts_with(std::string const &path, std::initializer_list<uint8_t> magic) {
  std::ifstream in(path, std::ios::binary);
  if (!in)
    return false;
  for (uint8_t b : magic) {
    char c = 0;
    if (!in.get(c) || static_cast<uint8_t>(c) != b)
      return false;
  }
  return true;
}

} // namespace

int main(int argc, char **argv) {
  if (argc < 2) {
    std::cerr << "usage: insightdock_smoke_tests <output.png>\n";
    return 1;
  }

  insightdock::charts::PieChart pie;
  pie.options().width = 320;
  pie.options().height = 240;
  pie.options().title = "Smoke Test";
  pie.add_slice("A", 40);
  pie.add_slice("B", 35);
  pie.add_slice("C", 25);

  auto img = insightdock::charts::ChartExporter::render(pie);
  if (img.width() == 0 || img.height() == 0 || img.data().empty()) {
    std::cerr << "render failed\n";
    return 2;
  }

  std::string out = argv[1];
  if (!insightdock::charts::ChartExporter::save(pie, out)) {
    std::cerr << "save failed\n";
    return 3;
  }

  if (!file_starts_with(out, {0x89, 'P', 'N', 'G', '\r', '\n', 0x1a, '\n'})) {
    std::cerr << "png magic mismatch\n";
    return 4;
  }

  insightdock::charts::ExportOptions jpg;
  jpg.format = insightdock::charts::OutputFormat::Jpeg;
  std::string jpg_path = out + ".jpg";
  if (!insightdock::charts::ChartExporter::save(pie, jpg_path, jpg)) {
    std::cerr << "jpeg save failed\n";
    return 5;
  }

  if (!file_starts_with(jpg_path, {0xFF, 0xD8, 0xFF})) {
    std::cerr << "jpeg magic mismatch\n";
    return 6;
  }

  std::cout << "smoke tests passed\n";
  return 0;
}
