cmake_minimum_required(VERSION 3.14)

# --- Smoke tests: render PNG/JPEG and check magic bytes ---
add_executable(insightdock_smoke_tests smoke_tests.cc)
target_link_libraries(insightdock_smoke_tests PRIVATE InsightDock::Charts)

add_test(NAME smoke_render_png COMMAND insightdock_smoke_tests ${CMAKE_BINARY_DIR}/smoke_output.png)
set_tests_properties(smoke_render_png PROPERTIES WORKING_DIRECTORY ${CMAKE_BINARY_DIR})

# --- Unit tests: Scale, Palette, ImageBuffer, ChartResult, theme ---
add_executable(insightdock_unit_tests unit_tests.cc)
find_package(Threads REQUIRED)
target_link_libraries(insightdock_unit_tests PRIVATE InsightDock::Charts Threads::Threads)
if(INSIGHTDOCK_WITH_PODOFO)
  # PoDoFo's private config headers, for the vector-PDF tests that load the result back.
  target_include_directories(insightdock_unit_tests PRIVATE ${_insightdock_podofo_include_dirs})
endif()
if(INSIGHTDOCK_FONT_LATIN)
  target_compile_definitions(insightdock_unit_tests PRIVATE INSIGHTDOCK_FONT_LATIN=1)
endif()

add_test(NAME unit_tests COMMAND insightdock_unit_tests)
set_tests_properties(unit_tests PROPERTIES WORKING_DIRECTORY ${CMAKE_BINARY_DIR})

add_executable(insightdock_gallery_regression gallery_regression.cc)
target_link_libraries(insightdock_gallery_regression PRIVATE InsightDock::Charts)
target_include_directories(insightdock_gallery_regression PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

add_test(NAME gallery_regression COMMAND insightdock_gallery_regression
  ${CMAKE_CURRENT_SOURCE_DIR}/../docs/gallery)
set_tests_properties(gallery_regression PROPERTIES
  WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/..)

# --- find_package consumer: install to a scratch prefix, build + run an out-of-tree program ---
add_test(NAME find_package_consumer
  COMMAND ${CMAKE_COMMAND}
    -DBUILD_DIR=${CMAKE_BINARY_DIR}
    -DSOURCE_DIR=${CMAKE_CURRENT_SOURCE_DIR}/consumer
    -DWORK_DIR=${CMAKE_BINARY_DIR}/consumer-test
    -DCONFIG=$<CONFIG>
    "-DCXX_COMPILER=${CMAKE_CXX_COMPILER}"
    "-DCXX_FLAGS=${CMAKE_CXX_FLAGS}"
    "-DEXE_LINKER_FLAGS=${CMAKE_EXE_LINKER_FLAGS}"
    -P ${CMAKE_CURRENT_SOURCE_DIR}/consumer/run.cmake)
