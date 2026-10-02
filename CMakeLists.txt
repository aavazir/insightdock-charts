cmake_minimum_required(VERSION 3.14)
project(InsightDockCharts VERSION 0.7.0 LANGUAGES CXX)

if(PROJECT_VERSION_MAJOR EQUAL 0)
  set(INSIGHTDOCK_SOVERSION "${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}")
else()
  set(INSIGHTDOCK_SOVERSION "${PROJECT_VERSION_MAJOR}")
endif()

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Export compile_commands.json for clangd and MrDocs (docs/mrdocs.yml)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

include(GNUInstallDirs)

option(INSIGHTDOCK_BUILD_EXAMPLES "Build generic example programs" ON)
option(INSIGHTDOCK_WITH_PODOFO "Build PoDoFo PDF adapter and PDF examples" OFF)
option(INSIGHTDOCK_BUILD_TESTS "Build and register CTest smoke tests" ON)
option(INSIGHTDOCK_BUILD_BENCHMARKS "Build the std::chrono render benchmark harness" OFF)
option(INSIGHTDOCK_FONT_LATIN "Embed a Latin-1/Latin-Extended-A font subset (~22 KB) instead of the ASCII-only one" OFF)
option(INSIGHTDOCK_WERROR "Treat warnings as errors" OFF)

set(INSIGHTDOCK_SOURCES
  src/image_buffer.cc
  src/palette.cc
  src/font.cc
  src/scale.cc
  src/decimate.cc
  src/tick_format.cc
  src/spline.cc
  src/marker.cc
  src/bar_shape.cc
  src/reference_lines.cc
  src/cartesian_chart.cc
  src/bar_chart.cc
  src/line_chart.cc
  src/area_chart.cc
  src/combo_chart.cc
  src/scatter_chart.cc
  src/pie_chart.cc
  src/png_writer.cc
  src/svg_writer.cc
  src/svg_pie.cc
  src/export.cc
)

add_library(InsightDockCharts ${INSIGHTDOCK_SOURCES})
add_library(InsightDock::Charts ALIAS InsightDockCharts)

# Output filename is libinsightdock; the CMake target name, InsightDock::Charts
# alias, and insightdock::charts namespace/headers are unaffected by this override.
# EXPORT_NAME makes the *installed* (find_package) target InsightDock::Charts too —
# without it, install(EXPORT ... NAMESPACE InsightDock::) would produce
# InsightDock::InsightDockCharts, which only matches the in-tree ALIAS by coincidence
# of naming and silently breaks every out-of-tree consumer following this exact target name.
set_target_properties(InsightDockCharts PROPERTIES
  OUTPUT_NAME insightdock
  EXPORT_NAME Charts
  VERSION ${PROJECT_VERSION}
  # ABI policy: source-compatible only across releases. SOVERSION tracks major.minor while the
  # major is 0, then the major alone from 1.0 (a new minor may change the ABI before 1.0).
  SOVERSION ${INSIGHTDOCK_SOVERSION}
  CXX_VISIBILITY_PRESET hidden
  VISIBILITY_INLINES_HIDDEN ON
)

# Public symbols are marked INSIGHTDOCK_API (include/insightdock/export_macros.h).
if(BUILD_SHARED_LIBS)
  target_compile_definitions(InsightDockCharts PRIVATE INSIGHTDOCK_BUILDING=1)
else()
  target_compile_definitions(InsightDockCharts PUBLIC INSIGHTDOCK_STATIC=1)
endif()

# pie_chart.cc is always in INSIGHTDOCK_SOURCES; tell export.cc it can use PieChart
target_compile_definitions(InsightDockCharts PUBLIC INSIGHTDOCK_HAS_PIE=1)

if(INSIGHTDOCK_FONT_LATIN)
  target_compile_definitions(InsightDockCharts PRIVATE INSIGHTDOCK_FONT_LATIN=1)
endif()

target_include_directories(InsightDockCharts PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)

target_include_directories(InsightDockCharts PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}/third_party
)

if(INSIGHTDOCK_WERROR)
  if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(InsightDockCharts PRIVATE -Wall -Wextra -Werror)
  endif()
else()
  if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(InsightDockCharts PRIVATE -Wall -Wextra)
  endif()
endif()

install(TARGETS InsightDockCharts EXPORT InsightDockChartsTargets
  ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
  LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
  RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

install(DIRECTORY include/insightdock DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})

install(EXPORT InsightDockChartsTargets
  FILE InsightDockChartsTargets.cmake
  NAMESPACE InsightDock::
  DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/InsightDock
)

include(CMakePackageConfigHelpers)
# Before 1.0 a new minor release may break source/ABI compatibility, so match on major.minor.
if(PROJECT_VERSION_MAJOR EQUAL 0)
  set(INSIGHTDOCK_VERSION_COMPAT SameMinorVersion)
else()
  set(INSIGHTDOCK_VERSION_COMPAT SameMajorVersion)
endif()
write_basic_package_version_file(
  "${CMAKE_CURRENT_BINARY_DIR}/InsightDockConfigVersion.cmake"
  VERSION ${PROJECT_VERSION}
  COMPATIBILITY ${INSIGHTDOCK_VERSION_COMPAT}
)

configure_package_config_file(
  "${CMAKE_CURRENT_SOURCE_DIR}/cmake/InsightDockConfig.cmake.in"
  "${CMAKE_CURRENT_BINARY_DIR}/InsightDockConfig.cmake"
  INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/InsightDock
)

install(FILES
  "${CMAKE_CURRENT_BINARY_DIR}/InsightDockConfig.cmake"
  "${CMAKE_CURRENT_BINARY_DIR}/InsightDockConfigVersion.cmake"
  DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/InsightDock
)

# pkg-config support for Meson / plain-Makefile consumers that don't use CMake's
# find_package(). CMAKE_INSTALL_FULL_* resolves to absolute paths regardless of
# whether CMAKE_INSTALL_PREFIX is set at configure time or overridden at install time.
configure_file(
  "${CMAKE_CURRENT_SOURCE_DIR}/cmake/insightdock.pc.in"
  "${CMAKE_CURRENT_BINARY_DIR}/insightdock.pc"
  @ONLY
)
install(FILES "${CMAKE_CURRENT_BINARY_DIR}/insightdock.pc"
  DESTINATION ${CMAKE_INSTALL_LIBDIR}/pkgconfig
)

if(INSIGHTDOCK_BUILD_EXAMPLES)
  add_executable(example_main examples/main.cc)
  target_link_libraries(example_main PRIVATE InsightDock::Charts)

  add_executable(bar_line_example examples/bar_line_example.cc)
  target_link_libraries(bar_line_example PRIVATE InsightDock::Charts)

  add_executable(pie_overlap_test examples/pie_overlap_test.cc)
  target_link_libraries(pie_overlap_test PRIVATE InsightDock::Charts)

  add_executable(scatter_bubble_example examples/scatter_bubble_example.cc)
  target_link_libraries(scatter_bubble_example PRIVATE InsightDock::Charts)

  add_executable(combo_chart_example examples/combo_chart_example.cc)
  target_link_libraries(combo_chart_example PRIVATE InsightDock::Charts)

  add_executable(time_series_line_example examples/time_series_line_example.cc)
  target_link_libraries(time_series_line_example PRIVATE InsightDock::Charts)

  add_executable(bar_polish_example examples/bar_polish_example.cc)
  target_link_libraries(bar_polish_example PRIVATE InsightDock::Charts)

  add_executable(pie_doughnut_example examples/pie_doughnut_example.cc)
  target_link_libraries(pie_doughnut_example PRIVATE InsightDock::Charts)

  add_executable(line_spline_dash_example examples/line_spline_dash_example.cc)
  target_link_libraries(line_spline_dash_example PRIVATE InsightDock::Charts)

  add_executable(combo_reference_lines_example examples/combo_reference_lines_example.cc)
  target_link_libraries(combo_reference_lines_example PRIVATE InsightDock::Charts)

  add_executable(bar_horizontal_example examples/bar_horizontal_example.cc)
  target_link_libraries(bar_horizontal_example PRIVATE InsightDock::Charts)

  add_executable(log_scale_example examples/log_scale_example.cc)
  target_link_libraries(log_scale_example PRIVATE InsightDock::Charts)
endif()

if(INSIGHTDOCK_WITH_PODOFO)
  set(INSIGHTDOCK_PODOFO_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../podofo"
      CACHE PATH "PoDoFo source tree or install prefix")

  if(NOT TARGET podofo::podofo)
    if(EXISTS "${INSIGHTDOCK_PODOFO_DIR}/CMakeLists.txt")
      message(STATUS "Building PoDoFo from ${INSIGHTDOCK_PODOFO_DIR}")
      set(PODOFO_BUILD_TEST OFF CACHE BOOL "" FORCE)
      set(PODOFO_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
      add_subdirectory("${INSIGHTDOCK_PODOFO_DIR}" "${CMAKE_BINARY_DIR}/podofo" EXCLUDE_FROM_ALL)
    else()
      find_package(podofo CONFIG REQUIRED)
    endif()
  endif()

  if(NOT TARGET podofo::podofo)
    message(FATAL_ERROR
      "PoDoFo not found. Set INSIGHTDOCK_PODOFO_DIR or install podofo (CMake 3.23+).")
  endif()

  target_sources(InsightDockCharts PRIVATE
    src/podofo_adapter.cc src/podofo_vector.cc src/pdf_report.cc)
  target_compile_definitions(InsightDockCharts PUBLIC INSIGHTDOCK_HAS_PODOFO=1)
  target_link_libraries(InsightDockCharts PUBLIC podofo::podofo)
  target_include_directories(InsightDockCharts PRIVATE
    "$<BUILD_INTERFACE:${CMAKE_BINARY_DIR}/podofo/src/podofo/podofo/auxiliary>"
    "$<BUILD_INTERFACE:${CMAKE_BINARY_DIR}/podofo/src/podofo/podofo/private>"
  )

  set(_insightdock_podofo_include_dirs
    "$<BUILD_INTERFACE:${CMAKE_BINARY_DIR}/podofo/src/podofo/podofo/auxiliary>"
    "$<BUILD_INTERFACE:${CMAKE_BINARY_DIR}/podofo/src/podofo/podofo/private>"
  )

  if(INSIGHTDOCK_BUILD_EXAMPLES)
    add_executable(podofo_report_example examples/podofo_report_example.cc)
    target_link_libraries(podofo_report_example PRIVATE InsightDock::Charts)
    target_include_directories(podofo_report_example PRIVATE ${_insightdock_podofo_include_dirs})

    add_executable(pie_overlap_vector_pdf examples/pie_overlap_vector_pdf.cc)
    target_link_libraries(pie_overlap_vector_pdf PRIVATE InsightDock::Charts)
    target_include_directories(pie_overlap_vector_pdf PRIVATE ${_insightdock_podofo_include_dirs})

    add_executable(all_charts_raster_pdf examples/all_charts_raster_pdf.cc)
    target_link_libraries(all_charts_raster_pdf PRIVATE InsightDock::Charts)
    target_include_directories(all_charts_raster_pdf PRIVATE ${_insightdock_podofo_include_dirs})
  endif()
endif()

if(INSIGHTDOCK_BUILD_TESTS)
  enable_testing()
  add_subdirectory(tests)
endif()

if(INSIGHTDOCK_BUILD_BENCHMARKS)
  add_executable(insightdock_bench benchmarks/bench_render.cc)
  target_link_libraries(insightdock_bench PRIVATE InsightDock::Charts)
endif()
