// SPDX-License-Identifier: MIT
// Copyright (c) InsightDock contributors

/**
 * @file error.h
 * @brief Structured error codes and result type for chart export operations.
 *
 * @ref ChartResult<T> replaces raw `bool` and empty-vector returns in
 * @ref ChartExporter so callers can distinguish failure modes:
 *
 * @code
 * auto res = ChartExporter::save(chart, "out.png");
 * if (!res)
 *   fprintf(stderr, "save failed: %s\n", chart_error_message(res.error()));
 *
 * auto enc = ChartExporter::encode(chart, opts);
 * if (enc)
 *   upload(enc.value.data(), enc.value.size());
 * @endcode
 */

#ifndef INSIGHTDOCK_CHARTS_ERROR_H
#define INSIGHTDOCK_CHARTS_ERROR_H

namespace insightdock::charts {

/** @brief Reason a chart export operation failed. */
enum class ChartError {
  None = 0,
  BadPath,        ///< Empty or unwritable file path.
  RenderFailure,  ///< Chart rendered to a zero-size buffer (no data, zero dimensions).
  EncodeFailure,  ///< PNG / JPEG / PDF encoder returned an error.
  PdfUnsupported, ///< PDF output requested but library built without INSIGHTDOCK_WITH_PODOFO.
  InvalidInput,   ///< Malformed input (no slices, mismatched series lengths, etc.).
};

/** @brief Human-readable description of a @ref ChartError code. */
inline char const *chart_error_message(ChartError e) noexcept {
  switch (e) {
  case ChartError::None:
    return "ok";
  case ChartError::BadPath:
    return "empty or unwritable file path";
  case ChartError::RenderFailure:
    return "chart render produced empty buffer";
  case ChartError::EncodeFailure:
    return "image encode failed";
  case ChartError::PdfUnsupported:
    return "PDF output requires INSIGHTDOCK_WITH_PODOFO build";
  case ChartError::InvalidInput:
    return "invalid chart input (no data or bad dimensions)";
  }
  return "unknown error";
}

/**
 * @brief Result of a chart export operation — either a value or a @ref ChartError.
 *
 * Contextually converts to `bool` (true = success), so existing code using
 * `if (!ChartExporter::save(...))` continues to work without changes.
 *
 * @tparam T Value type on success. Use the `void` specialisation for operations
 *           that return no value (e.g. @ref ChartExporter::save).
 */
template <typename T> struct ChartResult {
  bool ok = false;                          ///< True when the operation succeeded.
  T value = {};                             ///< Result value (valid only when ok).
  ChartError error_code = ChartError::None; ///< Error code (valid only when !ok).

  /** @brief Contextual bool conversion — true on success. */
  explicit operator bool() const noexcept {
    return ok;
  }

  /** @brief Error code; @ref ChartError::None when ok. */
  ChartError error() const noexcept {
    return error_code;
  }

  /** @brief Construct a successful result holding @p v. */
  static ChartResult success(T v) {
    ChartResult r;
    r.ok = true;
    r.value = static_cast<T &&>(v);
    return r;
  }

  /** @brief Construct a failed result with error @p e. */
  static ChartResult failure(ChartError e) {
    ChartResult r;
    r.error_code = e;
    return r;
  }
};

/** @brief @ref ChartResult specialisation for operations with no return value. */
template <> struct ChartResult<void> {
  bool ok = false;
  ChartError error_code = ChartError::None;

  explicit operator bool() const noexcept {
    return ok;
  }
  ChartError error() const noexcept {
    return error_code;
  }

  static ChartResult success() {
    ChartResult r;
    r.ok = true;
    return r;
  }

  static ChartResult failure(ChartError e) {
    ChartResult r;
    r.error_code = e;
    return r;
  }
};

} // namespace insightdock::charts

#endif // INSIGHTDOCK_CHARTS_ERROR_H
