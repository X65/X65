find_package(Sphinx REQUIRED)

set(SPHINX_SOURCE ${CMAKE_CURRENT_SOURCE_DIR})
set(SPHINX_BUILD ${CMAKE_CURRENT_BINARY_DIR}/sphinx)
# Success stamp rather than index.html. Sphinx writes its output and *then*
# exits non-zero under -W, so index.html would look up to date after a failed
# build and the next `make` would silently succeed with the defect still there.
# The stamp is only touched when sphinx-build actually returns 0. It lives
# inside the output tree so `rm -rf build/book/sphinx` still forces a rebuild.
set(SPHINX_STAMP ${SPHINX_BUILD}/.sphinx-stamp)

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
# - Any book source has been updated
# - The Sphinx config has been updated
# -W turns every Sphinx warning into a build failure. The book is warning-free,
# and warnings here mean real defects: broken cross-references, missing toctree
# entries, unknown directives. If a Sphinx upgrade ever starts warning about
# something unrelated, fix it or drop this flag - do not add a baseline back.
#
# -E is what makes -W trustworthy. Sphinx only re-resolves documents it considers
# out of date, so with a warm doctree cache a warning is reported once and then
# silently disappears on the next run, leaving a broken build that "passes" on
# retry. -E re-reads every source each time. Costs nothing here: a full re-read
# of the whole book measures the same as an incremental one (~3.5s).
add_custom_command(OUTPUT ${SPHINX_STAMP}
                   COMMAND ${SPHINX_EXECUTABLE} -b dirhtml -W -E
                   ${SPHINX_SOURCE} ${SPHINX_BUILD}
                   # Only reached when sphinx-build succeeded. Also covers the case
                   # where Sphinx's doctree cache says nothing changed and it leaves
                   # the output untouched, which would otherwise re-run this forever.
                   COMMAND ${CMAKE_COMMAND} -E touch ${SPHINX_STAMP}
                   WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
                   DEPENDS
                   ${SPHINX_SOURCES}
                   MAIN_DEPENDENCY ${SPHINX_SOURCE}/conf.py
                   COMMENT "Generating documentation with Sphinx")

# Nice named target so we can run the job easily
add_custom_target(Sphinx ALL DEPENDS ${SPHINX_STAMP})

# Add an install target to install the docs
include(GNUInstallDirs)
install(DIRECTORY ${SPHINX_BUILD}
DESTINATION ${CMAKE_INSTALL_DOCDIR})
