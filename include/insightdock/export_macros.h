// SPDX-License-Identifier: MIT
// Copyright (c) InsightDock contributors

/**
 * @file export_macros.h
 * @brief `INSIGHTDOCK_API` — symbol visibility for shared-library builds.
 *
 * The library is built with hidden default visibility; every public class and free function is
 * marked `INSIGHTDOCK_API`. Static builds define `INSIGHTDOCK_STATIC` (done by CMake), which makes
 * the macro empty on Windows.
 */

#ifndef INSIGHTDOCK_CHARTS_EXPORT_MACROS_H
#define INSIGHTDOCK_CHARTS_EXPORT_MACROS_H

#if defined(_WIN32) || defined(__CYGWIN__)
#if defined(INSIGHTDOCK_STATIC)
#define INSIGHTDOCK_API
#elif defined(INSIGHTDOCK_BUILDING)
#define INSIGHTDOCK_API __declspec(dllexport)
#else
#define INSIGHTDOCK_API __declspec(dllimport)
#endif
#elif defined(__GNUC__) || defined(__clang__)
#define INSIGHTDOCK_API __attribute__((visibility("default")))
#else
#define INSIGHTDOCK_API
#endif

#endif // INSIGHTDOCK_CHARTS_EXPORT_MACROS_H
