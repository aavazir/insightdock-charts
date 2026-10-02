// SPDX-License-Identifier: MIT
// Copyright (c) InsightDock contributors

/**
 * @file svg_writer.h
 * @brief Encode @ref ImageBuffer to SVG (raster PNG base64-inlined).
 */

#ifndef INSIGHTDOCK_CHARTS_SVG_WRITER_H
#define INSIGHTDOCK_CHARTS_SVG_WRITER_H

#include "insightdock/export_macros.h"
#include "insightdock/image_buffer.h"
#include <cstdint>
#include <string>
#include <vector>

namespace insightdock::charts {

INSIGHTDOCK_API bool write_svg(std::string const &path, ImageBuffer const &img);
INSIGHTDOCK_API std::vector<uint8_t> encode_svg(ImageBuffer const &img);

} // namespace insightdock::charts

#endif // INSIGHTDOCK_CHARTS_SVG_WRITER_H
