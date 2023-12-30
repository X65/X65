#
# project: X65 emulator
#
cmake_minimum_required(VERSION 3.21)
set(CMAKE_OSX_DEPLOYMENT_TARGET "10.13" CACHE STRING "")
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_C_STANDARD 11)
project([emulator])

# include the fips main cmake file
get_filename_component(FIPS_ROOT_DIR "../fips" ABSOLUTE)
include("${FIPS_ROOT_DIR}/cmake/fips.cmake")

fips_setup()

fips_ide_group(examples)
include_directories(ext/chips-test/examples/common ext/chips-test/examples/roms)
add_subdirectory(ext/chips-test/examples/common)
fips_ide_group(roms)
add_subdirectory(ext/chips-test/examples/roms)

fips_ide_group(apps)
include_directories(.)
add_subdirectory(apps)
