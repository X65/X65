cmake_minimum_required (VERSION 3.30)

project ("X65")

set(CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}/cmake" ${CMAKE_MODULE_PATH})

add_subdirectory ("book")

# Everything below is opt-in: `cmake --build build` still builds only the book.
# `emu`, `emu-wasm` and `examples` drive the submodules' own builds; `site`
# refreshes the web emulator and its ROMs inside the x65.zone checkout.
include(Submodules)
include(Site)
