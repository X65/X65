#
# project: X65 emulator
#
cmake_minimum_required(VERSION 3.21)
set(CMAKE_OSX_DEPLOYMENT_TARGET "10.13" CACHE STRING "")
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_C_STANDARD 11)
project([emulator])

# generate compile_commands.json for clangd
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# include the fips main cmake file
get_filename_component(FIPS_ROOT_DIR "../fips" ABSOLUTE)
include("${FIPS_ROOT_DIR}/cmake/fips.cmake")

include_directories(.)
fips_setup()

fips_ide_group(examples)
add_subdirectory(chips-test/examples)
include_directories(chips-test/examples/common chips-test/examples/roms)

add_subdirectory(apps)
