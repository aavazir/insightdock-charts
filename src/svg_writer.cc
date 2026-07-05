// SPDX-License-Identifier: MIT
// Copyright (c) InsightDock contributors

#include "insightdock/svg_writer.h"

#include "insightdock/png_writer.h"

#include <cstdio>
#include <fstream>
#include <string>

namespace insightdock::charts {

namespace {

std::string base64_encode(std::vector<uint8_t> const &data) {
  static char const *kAlphabet = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
  std::string out;
  out.reserve((data.size() + 2) / 3 * 4);
  std::size_t i = 0;
  while (i + 2 < data.size()) {
    uint32_t n = (static_cast<uint32_t>(data[i]) << 16) |
                 (static_cast<uint32_t>(data[i + 1]) << 8) | static_cast<uint32_t>(data[i + 2]);
    out.push_back(kAlphabet[(n >> 18) & 63]);
    out.push_back(kAlphabet[(n >> 12) & 63]);
    out.push_back(kAlphabet[(n >> 6) & 63]);
    out.push_back(kAlphabet[n & 63]);
    i += 3;
  }
  if (i < data.size()) {
    uint32_t n = static_cast<uint32_t>(data[i]) << 16;
    if (i + 1 < data.size())
      n |= static_cast<uint32_t>(data[i + 1]) << 8;
    out.push_back(kAlphabet[(n >> 18) & 63]);
    out.push_back(kAlphabet[(n >> 12) & 63]);
    if (i + 1 < data.size()) {
      out.push_back(kAlphabet[(n >> 6) & 63]);
      out.push_back('=');
    } else {
      out.push_back('=');
      out.push_back('=');
    }
  }
  return out;
}

std::string build_svg_xml(ImageBuffer const &img) {
  if (img.width() == 0 || img.height() == 0)
    return {};

  auto png = encode_png(img);
  if (png.empty())
    return {};

  std::string const b64 = base64_encode(png);
  std::string w = std::to_string(img.width());
  std::string h = std::to_string(img.height());

  std::string xml;
  xml.reserve(256 + b64.size());
  xml += "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n";
  xml += "<svg xmlns=\"http://www.w3.org/2000/svg\" ";
  xml += "xmlns:xlink=\"http://www.w3.org/1999/xlink\" ";
  xml += "width=\"" + w + "\" height=\"" + h + "\">\n";
  xml += "  <image width=\"" + w + "\" height=\"" + h + "\" ";
  xml += "xlink:href=\"data:image/png;base64," + b64 + "\"/>\n";
  xml += "</svg>\n";
  return xml;
}

} // namespace

bool write_svg(std::string const &path, ImageBuffer const &img) {
  std::string xml = build_svg_xml(img);
  if (xml.empty() || path.empty())
    return false;
  std::ofstream out(path, std::ios::binary);
  if (!out)
    return false;
  out << xml;
  return static_cast<bool>(out);
}

std::vector<uint8_t> encode_svg(ImageBuffer const &img) {
  std::string xml = build_svg_xml(img);
  return std::vector<uint8_t>(xml.begin(), xml.end());
}

} // namespace insightdock::charts
