// SPDX-License-Identifier: MIT
// Copyright (c) InsightDock contributors

#include "insightdock/png_writer.h"

// On Windows, treat `path` as UTF-8 (stb converts to wide chars) instead of the ANSI code page.
#define STBIW_WINDOWS_UTF8
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "../third_party/stb_image_write.h"

#include <algorithm>
#include <fstream>

namespace insightdock::charts {

namespace {

void png_write_callback(void *context, void *data, int size) {
  auto *vec = static_cast<std::vector<uint8_t> *>(context);
  auto *bytes = static_cast<unsigned char *>(data);
  vec->insert(vec->end(), bytes, bytes + size);
}

} // namespace

bool write_png(std::string const &path, ImageBuffer const &img) {
  if (img.width() == 0 || img.height() == 0)
    return false;
  int stride = static_cast<int>(img.width() * 4);
  return stbi_write_png(path.c_str(), static_cast<int>(img.width()), static_cast<int>(img.height()),
                        4, img.data().data(), stride) != 0;
}

std::vector<uint8_t> encode_png(ImageBuffer const &img) {
  std::vector<uint8_t> out;
  if (img.width() == 0 || img.height() == 0)
    return out;

  int stride = static_cast<int>(img.width() * 4);
  stbi_write_png_to_func(png_write_callback, &out, static_cast<int>(img.width()),
                         static_cast<int>(img.height()), 4, img.data().data(), stride);
  return out;
}

bool write_jpeg(std::string const &path, ImageBuffer const &img, int quality) {
#ifdef INSIGHTDOCK_NO_JPEG
  (void)path;
  (void)img;
  (void)quality;
  return false;
#else
  if (img.width() == 0 || img.height() == 0)
    return false;
  int q = std::clamp(quality, 1, 100);
  return stbi_write_jpg(path.c_str(), static_cast<int>(img.width()), static_cast<int>(img.height()),
                        4, img.data().data(), q) != 0;
#endif
}

std::vector<uint8_t> encode_jpeg(ImageBuffer const &img, int quality) {
#ifdef INSIGHTDOCK_NO_JPEG
  (void)img;
  (void)quality;
  return {};
#else
  std::vector<uint8_t> out;
  if (img.width() == 0 || img.height() == 0)
    return out;
  int q = std::clamp(quality, 1, 100);
  stbi_write_jpg_to_func(png_write_callback, &out, static_cast<int>(img.width()),
                         static_cast<int>(img.height()), 4, img.data().data(), q);
  return out;
#endif
}

} // namespace insightdock::charts
