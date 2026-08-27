# Build triggers for the submodules that have a build of their own.
#
# These are separate CMake projects with their own toolchains - `examples` runs
# cc65, `emu-wasm` runs Emscripten - so they cannot be `add_subdirectory`'d into
# this one. Each target drives that project's own build instead, which keeps its
# dependency tracking and leaves the submodule working tree clean by building
# into this build tree rather than into `<submodule>/build`.
#
# None of them is in ALL, deliberately: `cmake --build build` still builds only
# the book, which is all CI is asked to do.

set(EMU_SOURCE_DIR      ${PROJECT_SOURCE_DIR}/emulator)
set(EXAMPLES_SOURCE_DIR ${PROJECT_SOURCE_DIR}/examples)
set(EMU_BINARY_DIR      ${PROJECT_BINARY_DIR}/emulator)
set(EMU_WASM_BINARY_DIR ${PROJECT_BINARY_DIR}/emulator-wasm)
set(EXAMPLES_BINARY_DIR ${PROJECT_BINARY_DIR}/examples)

# The emulator needs its own nested submodules (ext/SDL, ext/imgui, ext/sgu-1...)
# which a top-level `git submodule update --init` does not pull.
set(_require_emu_submodule
    ${CMAKE_COMMAND}
    -DNAME=emulator -DDIR=${EMU_SOURCE_DIR}
    -DMARKER=${EMU_SOURCE_DIR}/ext/SDL/CMakeLists.txt
    -P ${PROJECT_SOURCE_DIR}/cmake/RequireSubmodule.cmake)

add_custom_target(emu
    COMMAND ${_require_emu_submodule}
    COMMAND ${CMAKE_COMMAND} -S ${EMU_SOURCE_DIR} -B ${EMU_BINARY_DIR}
            -DCMAKE_BUILD_TYPE=Release
    COMMAND ${CMAKE_COMMAND} --build ${EMU_BINARY_DIR} --parallel
    USES_TERMINAL VERBATIM
    COMMENT "Building the emulator -> ${EMU_BINARY_DIR}/emu")

# `emcmake` is resolved at build time rather than baked in, so sourcing emsdk
# after configuring is enough - no need to re-run cmake.
find_program(EMCMAKE_EXECUTABLE emcmake)
if(NOT EMCMAKE_EXECUTABLE)
    message(STATUS "emcmake not found: the 'emu-wasm' and 'site' targets need "
                   "the Emscripten SDK on PATH")
endif()

add_custom_target(emu-wasm
    COMMAND ${_require_emu_submodule}
    COMMAND emcmake ${CMAKE_COMMAND} -S ${EMU_SOURCE_DIR} -B ${EMU_WASM_BINARY_DIR}
            -DCMAKE_BUILD_TYPE=Release
    COMMAND ${CMAKE_COMMAND} --build ${EMU_WASM_BINARY_DIR} --parallel
    USES_TERMINAL VERBATIM
    COMMENT "Building the emulator for the web -> ${EMU_WASM_BINARY_DIR}/emu.html")

add_custom_target(examples
    COMMAND ${CMAKE_COMMAND} -S ${EXAMPLES_SOURCE_DIR} -B ${EXAMPLES_BINARY_DIR}
    COMMAND ${CMAKE_COMMAND} --build ${EXAMPLES_BINARY_DIR} --parallel
    USES_TERMINAL VERBATIM
    COMMENT "Building the example ROMs -> ${EXAMPLES_BINARY_DIR}/src/*.xex")
