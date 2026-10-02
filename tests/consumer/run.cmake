# Installs the already-built tree to a scratch prefix, then configures, builds and runs the
# out-of-tree consumer against it. Cross-platform (no shell), run by CTest as
# `cmake -DBUILD_DIR=... -DSOURCE_DIR=... -DWORK_DIR=... -DCONFIG=... -P run.cmake`.
foreach(var BUILD_DIR SOURCE_DIR WORK_DIR)
  if(NOT ${var})
    message(FATAL_ERROR "${var} not set")
  endif()
endforeach()
# CONFIG is empty for single-config generators configured without CMAKE_BUILD_TYPE.
set(cfg_args "")
set(cfg_define "")
if(CONFIG)
  set(cfg_args --config "${CONFIG}")
  set(cfg_define "-DCMAKE_BUILD_TYPE=${CONFIG}")
endif()

set(prefix "${WORK_DIR}/prefix")
set(cbuild "${WORK_DIR}/build")
file(REMOVE_RECURSE "${WORK_DIR}")

function(run)
  execute_process(COMMAND ${ARGN} RESULT_VARIABLE rc)
  if(NOT rc EQUAL 0)
    message(FATAL_ERROR "failed (${rc}): ${ARGN}")
  endif()
endfunction()

run("${CMAKE_COMMAND}" --install "${BUILD_DIR}" --prefix "${prefix}" ${cfg_args})
# Forward the compiler and sanitizer flags so the consumer links against the same runtime.
run("${CMAKE_COMMAND}" -S "${SOURCE_DIR}" -B "${cbuild}" "-DCMAKE_PREFIX_PATH=${prefix}"
    "-DCMAKE_CXX_COMPILER=${CXX_COMPILER}" "-DCMAKE_CXX_FLAGS=${CXX_FLAGS}"
    "-DCMAKE_EXE_LINKER_FLAGS=${EXE_LINKER_FLAGS}" ${cfg_define})
run("${CMAKE_COMMAND}" --build "${cbuild}" ${cfg_args})

# Shared builds on Windows need the DLL next to the executable; elsewhere RPATH/LD path.
if(WIN32)
  file(GLOB dlls "${prefix}/bin/*.dll")
  foreach(d ${dlls})
    file(COPY "${d}" DESTINATION "${cbuild}/${CONFIG}")
    file(COPY "${d}" DESTINATION "${cbuild}")
  endforeach()
else()
  set(ENV{LD_LIBRARY_PATH} "${prefix}/lib:${prefix}/lib64:$ENV{LD_LIBRARY_PATH}")
  set(ENV{DYLD_LIBRARY_PATH} "${prefix}/lib:$ENV{DYLD_LIBRARY_PATH}")
endif()

find_program(consumer_exe consumer PATHS "${cbuild}" "${cbuild}/${CONFIG}" NO_DEFAULT_PATH)
if(NOT consumer_exe)
  message(FATAL_ERROR "consumer executable not found under ${cbuild}")
endif()
run("${consumer_exe}")
