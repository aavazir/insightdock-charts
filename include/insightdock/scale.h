// SPDX-License-Identifier: MIT
// Copyright (c) InsightDock contributors

/**
 * @file scale.h
 * @brief Scale mapping data values to pixel coordinates.
 *
 * Used internally by @ref CartesianChart for axis layout and tick generation.
 * @ref ScaleType controls whether the mapping is linear, logarithmic, or
 * date/time-based (DateTime support lands in a later phase; the API is
 * established here so callers can set the type now).
 */

#ifndef INSIGHTDOCK_CHARTS_SCALE_H
#define INSIGHTDOCK_CHARTS_SCALE_H

#include "insightdock/export_macros.h"
#include <vector>

namespace insightdock::charts {

/**
 * @brief Axis scale interpolation strategy.
 *
 * - **Linear** — proportional mapping; always available.
 * - **Log** — logarithmic Y axis; suitable for network traffic, latency,
 *   and financial data spanning multiple orders of magnitude.
 * - **DateTime** — Unix-timestamp / chrono-based X axis; tick labels are
 *   formatted automatically by @ref TickFormat::DateTime.
 *
 * Log uses base-10 mapping; DateTime treats domain values as Unix timestamps (seconds).
 */
enum class ScaleType {
  Linear,   ///< Default proportional mapping.
  Log,      ///< Base-10 logarithmic mapping.
  DateTime, ///< Unix-timestamp x-axis with automatic tick formatting.
};

/**
 * @brief Maps a data domain [min, max] to a pixel range.
 *
 * @ref ScaleType selects the interpolation mode. @ref linear_nice builds a
 * scale with a rounded ("nice") domain and returns it.
 */
class INSIGHTDOCK_API Scale {
public:
  Scale() = default;

  /**
   * @brief Construct a linear scale.
   * @param min_val   Data domain minimum.
   * @param max_val   Data domain maximum.
   * @param pixel_min Pixel coordinate for @p min_val.
   * @param pixel_max Pixel coordinate for @p max_val.
   */
  Scale(double min_val, double max_val, float pixel_min, float pixel_max);

  /** @brief Set data domain endpoints. */
  void set_domain(double min_val, double max_val);
  /** @brief Set pixel range endpoints. */
  void set_range(float pixel_min, float pixel_max);

  /** @brief Current domain minimum. */
  double min() const {
    return min_;
  }
  /** @brief Current domain maximum. */
  double max() const {
    return max_;
  }

  /** @brief Map a data value to pixel coordinate. */
  float to_pixel(double value) const;
  /** @brief Map a pixel coordinate back to data value. */
  double from_pixel(float pixel) const;

  /**
   * @brief Generate human-readable tick values for the current domain.
   * @param max_ticks Approximate maximum number of ticks.
   * @return Tick values in ascending order.
   */
  std::vector<double> nice_ticks(int max_ticks = 8) const;

  /**
   * @brief Build a scale with a "nice" rounded domain and tick set.
   * @param data_min  Smallest data value.
   * @param data_max  Largest data value.
   * @param pixel_min Pixel for domain min.
   * @param pixel_max Pixel for domain max.
   * @param max_ticks Maximum tick count hint.
   * @return Configured scale with expanded nice domain.
   */
  static Scale linear_nice(double data_min, double data_max, float pixel_min, float pixel_max,
                           int max_ticks = 8);

  /**
   * @brief Build a @ref ScaleType::Log scale with a "nice" (power-of-10 / 1-2-5 mantissa)
   * domain and tick set, mirroring @ref linear_nice for logarithmic axes.
   * @param data_min  Smallest data value; values <= 0 are clamped to a small positive floor,
   *                  since a logarithmic domain cannot include zero or negative numbers.
   * @param data_max  Largest data value.
   * @param pixel_min Pixel for domain min.
   * @param pixel_max Pixel for domain max.
   * @param max_ticks Maximum tick count hint.
   * @return Configured @ref ScaleType::Log scale with expanded nice domain.
   */
  static Scale log_nice(double data_min, double data_max, float pixel_min, float pixel_max,
                        int max_ticks = 8);

  /** @brief Set the interpolation mode (default: Linear). */
  void set_type(ScaleType type) {
    type_ = type;
  }
  /** @brief Current interpolation mode. */
  ScaleType type() const {
    return type_;
  }

private:
  double min_ = 0.0;
  double max_ = 1.0;
  float pixel_min_ = 0.0f;
  float pixel_max_ = 100.0f;
  ScaleType type_ = ScaleType::Linear;
};

} // namespace insightdock::charts

#endif // INSIGHTDOCK_CHARTS_SCALE_H
