// SPDX-License-Identifier: MIT
// Copyright (c) InsightDock contributors

/**
 * @file png_writer.h
 * @brief Encode @ref ImageBuffer to PNG or JPEG files and memory buffers.
 *
 * Prefer @ref ChartExporter::save for chart output; use these functions when
 * you already have a raw @ref ImageBuffer.
 */

#ifndef INSIGHTDOCK_CHARTS_PNG_WRITER_H
#define INSIGHTDOCK_CHARTS_PNG_WRITER_H

#include "insightdock/image_buffer.h"
#include <cstdint>
#include <string>
#include <vector>

namespace insightdock::charts {

/**
 * @brief Write an RGBA buffer to a PNG file.
 * @param path Output file path.
 * @param img  Source buffer.
 * @return `true` on success.
 */
bool write_png(std::string const &path, ImageBuffer const &img);

/**
 * @brief Encode an RGBA buffer to PNG bytes in memory.
 * @param img Source buffer.
 * @return PNG file bytes, or empty vector on failure.
 */
std::vector<uint8_t> encode_png(ImageBuffer const &img);

/**
 * @brief Write an RGBA buffer to a JPEG file.
 * @param path    Output file path.
 * @param img     Source buffer (alpha composited onto white).
 * @param quality JPEG quality 1–100 (default 90).
 * @return `true` on success.
 */
bool write_jpeg(std::string const &path, ImageBuffer const &img, int quality = 90);

/**
 * @brief Encode an RGBA buffer to JPEG bytes in memory.
 * @param img     Source buffer.
 * @param quality JPEG quality 1–100.
 * @return JPEG bytes, or empty on failure.
 */
std::vector<uint8_t> encode_jpeg(ImageBuffer const &img, int quality = 90);

} // namespace insightdock::charts

#endif // INSIGHTDOCK_CHARTS_PNG_WRITER_H
