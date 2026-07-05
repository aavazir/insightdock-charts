cmake_minimum_required(VERSION 3.14)

# --- Smoke tests: render PNG/JPEG and check magic bytes ---
add_executable(insightdock_smoke_tests smoke_tests.cc)
target_link_libraries(insightdock_smoke_tests PRIVATE InsightDock::Charting)

add_test(NAME smoke_render_png COMMAND insightdock_smoke_tests ${CMAKE_BINARY_DIR}/smoke_output.png)
set_tests_properties(smoke_render_png PROPERTIES WORKING_DIRECTORY ${CMAKE_BINARY_DIR})

# --- Unit tests: Scale, Palette, ImageBuffer, ChartResult, theme ---
add_executable(insightdock_unit_tests unit_tests.cc)
target_link_libraries(insightdock_unit_tests PRIVATE InsightDock::Charting)

add_test(NAME unit_tests COMMAND insightdock_unit_tests)
set_tests_properties(unit_tests PROPERTIES WORKING_DIRECTORY ${CMAKE_BINARY_DIR})

add_executable(insightdock_gallery_regression gallery_regression.cc)
target_link_libraries(insightdock_gallery_regression PRIVATE InsightDock::Charting)

add_test(NAME gallery_regression COMMAND insightdock_gallery_regression
  ${CMAKE_CURRENT_SOURCE_DIR}/../docs/gallery)
set_tests_properties(gallery_regression PROPERTIES
  WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/..)
