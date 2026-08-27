# Generate the data blocks three of the CGIA examples are merged with.
#
# `sotb`, `mixed_modes` and `four_byte_burger` assemble to code only. Their
# bitmaps, tile maps and sprite frames live as C arrays in
# `examples/src/cgia/data/*.h`, and `bins.c` - a host program, not part of the
# examples build - writes them out as .xex blocks that `xex-filter.pl` later
# splices into the assembled code. The offsets in those headers are the demo's
# memory map: they have to agree with the constants mirrored in its .asm, and
# nothing checks that they do.
#
# bins.c writes its output into the current directory, so it is run with the
# staging directory as its working directory and the submodule tree stays clean.

set(EXAMPLE_DATA_SOURCE_DIR ${EXAMPLES_SOURCE_DIR}/src/cgia/data)
set(EXAMPLE_DATA_BINARY_DIR ${PROJECT_BINARY_DIR}/example-data)

set(_example_data_commands
    COMMAND ${CMAKE_COMMAND} -E make_directory ${EXAMPLE_DATA_BINARY_DIR}
    COMMAND ${CMAKE_C_COMPILER} -o ${EXAMPLE_DATA_BINARY_DIR}/bins
            ${EXAMPLE_DATA_SOURCE_DIR}/bins.c
    COMMAND ${CMAKE_COMMAND} -E chdir ${EXAMPLE_DATA_BINARY_DIR}
            ${EXAMPLE_DATA_BINARY_DIR}/bins)

add_custom_target(example-data
    ${_example_data_commands}
    USES_TERMINAL VERBATIM
    COMMENT "Generating example data blocks -> ${EXAMPLE_DATA_BINARY_DIR}/*.xex")
