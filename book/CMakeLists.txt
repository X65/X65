find_package(Doxygen REQUIRED)
find_package(Sphinx REQUIRED)

# Find all the public headers.
# Firmware is split into NORTH (CPU bus, system services) and SOUTH (CGIA, SGU,
# terminal); the root also contains the shared PIX bus header. We feed Doxygen
# the whole src tree and rely on EXCLUDE_PATTERNS in Doxyfile.in to filter out
# vendored subtrees (tinyusb, fatfs, littlefs, pico_hdmi).
set(SOURCE_HEADERS_DIR ${PROJECT_SOURCE_DIR}/firmware/src)
file(GLOB_RECURSE SOURCE_HEADERS ${SOURCE_HEADERS_DIR}/*.h)

set(DOXYGEN_INPUT_DIR ${SOURCE_HEADERS_DIR})
set(DOXYGEN_OUTPUT_DIR ${CMAKE_CURRENT_BINARY_DIR}/doxygen)
set(DOXYGEN_INDEX_FILE ${DOXYGEN_OUTPUT_DIR}/xml/index.xml)
set(DOXYFILE_IN ${CMAKE_CURRENT_SOURCE_DIR}/Doxyfile.in)
set(DOXYFILE_OUT ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile)

# Replace variables inside @@ with the current values
configure_file(${DOXYFILE_IN} ${DOXYFILE_OUT} @ONLY)

# Doxygen won't create this for us
file(MAKE_DIRECTORY ${DOXYGEN_OUTPUT_DIR})

# Only regenerate Doxygen when the Doxyfile or public headers change
add_custom_command(OUTPUT ${DOXYGEN_INDEX_FILE}
                   DEPENDS ${SOURCE_HEADERS}
                   COMMAND ${DOXYGEN_EXECUTABLE} ${DOXYFILE_OUT}
                   MAIN_DEPENDENCY ${DOXYFILE_OUT} ${DOXYFILE_IN}
                   COMMENT "Generating docs"
                   VERBATIM)

# Nice named target so we can run the job easily
add_custom_target(Doxygen ALL DEPENDS ${DOXYGEN_INDEX_FILE})

set(SPHINX_SOURCE ${CMAKE_CURRENT_SOURCE_DIR})
set(SPHINX_BUILD ${CMAKE_CURRENT_BINARY_DIR}/sphinx)
set(SPHINX_INDEX_FILE ${SPHINX_BUILD}/index.html)

# Every book source, not just index.md. The chapters are pulled in by glob
# `{toctree}` directives, so there is no explicit list anywhere to track; without
# this, editing a chapter and re-running the build silently renders nothing.
# CONFIGURE_DEPENDS re-globs at build time, so a newly added chapter triggers a
# re-configure instead of being invisible until someone runs cmake by hand.
file(GLOB_RECURSE SPHINX_SOURCES CONFIGURE_DEPENDS
     ${SPHINX_SOURCE}/*.md
     ${SPHINX_SOURCE}/_static/*
     ${SPHINX_SOURCE}/_templates/*)
# AGENTS.md is excluded from the build (see exclude_patterns in conf.py).
list(REMOVE_ITEM SPHINX_SOURCES ${SPHINX_SOURCE}/AGENTS.md)

# Only regenerate Sphinx when:
# - Doxygen has rerun
# - Any book source has been updated
# - The Sphinx config has been updated
add_custom_command(OUTPUT ${SPHINX_INDEX_FILE}
                   COMMAND ${SPHINX_EXECUTABLE} -b dirhtml
                   ${SPHINX_SOURCE} ${SPHINX_BUILD}
                   # Sphinx keeps its own doctree cache and leaves index.html
                   # untouched when nothing changed, which would leave the OUTPUT
                   # older than its dependencies and re-run this command forever.
                   COMMAND ${CMAKE_COMMAND} -E touch ${SPHINX_INDEX_FILE}
                   WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
                   DEPENDS
                   ${SPHINX_SOURCES}
                   ${DOXYGEN_INDEX_FILE}
                   MAIN_DEPENDENCY ${SPHINX_SOURCE}/conf.py
                   COMMENT "Generating documentation with Sphinx")

# Nice named target so we can run the job easily
add_custom_target(Sphinx ALL DEPENDS ${SPHINX_INDEX_FILE})
add_dependencies(Sphinx Doxygen)

# Add an install target to install the docs
include(GNUInstallDirs)
install(DIRECTORY ${SPHINX_BUILD}
DESTINATION ${CMAKE_INSTALL_DOCDIR})
