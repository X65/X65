# `site`: refresh the web emulator and its ROMs inside the x65.zone checkout.
#
# x65.zone is a Jekyll site that serves a WASM build of the emulator from
# `emu/`, and the ROMs it offers from `emu/roms/`. Both are committed artifacts
# there, not build outputs of that repo, so they go stale unless something
# rebuilds and copies them - this target is that something.
#
# It copies and stops. x65.zone deploys from `master` through GitHub Pages, so
# committing is left to a human who has looked at `git status` first.

include(${CMAKE_CURRENT_LIST_DIR}/SiteRoms.cmake)

set(SITE_SOURCE_DIR ${PROJECT_SOURCE_DIR}/x65.zone)
set(SITE_EMU_DIR    ${SITE_SOURCE_DIR}/emu)
set(SITE_ROMS_DIR   ${SITE_EMU_DIR}/roms)
set(SITE_STAGE_DIR  ${PROJECT_BINARY_DIR}/site-roms)

find_program(XEX_FILTER_EXECUTABLE xex-filter.pl)
find_program(SGU_TRACKER_EXECUTABLE sgu-tracker)

set(_site_commands
    COMMAND ${CMAKE_COMMAND}
            -DNAME=x65.zone -DDIR=${SITE_SOURCE_DIR}
            -DMARKER=${SITE_EMU_DIR}/index.html
            -P ${PROJECT_SOURCE_DIR}/cmake/RequireSubmodule.cmake)

# The Emscripten build emits the page, its loader and the module next to each
# other; the site serves all three from emu/.
foreach(_artifact emu.html emu.js emu.wasm)
    list(APPEND _site_commands
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
                ${EMU_WASM_BINARY_DIR}/${_artifact} ${SITE_EMU_DIR}/${_artifact})
endforeach()

# copy_if_different throughout: an unchanged ROM should not show up as a
# modification in the site's working tree.
if(SITE_ROMS)
    list(LENGTH SITE_ROMS _n)
    math(EXPR _last "${_n} / 2 - 1")
    foreach(_i RANGE ${_last})
        math(EXPR _t "${_i} * 2")
        math(EXPR _p "${_t} + 1")
        list(GET SITE_ROMS ${_t} _target)
        list(GET SITE_ROMS ${_p} _published)
        list(APPEND _site_commands
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
                    ${EXAMPLES_BINARY_DIR}/src/${_target}.xex
                    ${SITE_ROMS_DIR}/${_published}.xex)
    endforeach()
endif()

if(NOT XEX_FILTER_EXECUTABLE)
    message(STATUS "xex-filter.pl not found: 'site' will skip the merged ROMs "
                   "(Driller, Mystery_Cannon, Pokeymania, SOTB, mixed_modes)")
elseif(SITE_TUNE_ROMS)
    list(LENGTH SITE_TUNE_ROMS _n)
    math(EXPR _last "${_n} / 4 - 1")
    foreach(_i RANGE ${_last})
        math(EXPR _t "${_i} * 4")
        math(EXPR _p "${_t} + 1")
        math(EXPR _u "${_t} + 2")
        math(EXPR _a "${_t} + 3")
        list(GET SITE_TUNE_ROMS ${_t} _target)
        list(GET SITE_TUNE_ROMS ${_p} _published)
        list(GET SITE_TUNE_ROMS ${_u} _tune)
        list(GET SITE_TUNE_ROMS ${_a} _addr)
        list(APPEND _site_commands
            COMMAND ${CMAKE_COMMAND}
                    -DXEX_FILTER=${XEX_FILTER_EXECUTABLE}
                    -DPLAYER=${EXAMPLES_BINARY_DIR}/src/${_target}.xex
                    -DTUNE=${EXAMPLES_SOURCE_DIR}/${_tune}
                    -DADDR=${_addr}
                    -DOUT=${SITE_STAGE_DIR}/${_published}.xex
                    -P ${PROJECT_SOURCE_DIR}/cmake/MergeTuneXex.cmake
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
                    ${SITE_STAGE_DIR}/${_published}.xex
                    ${SITE_ROMS_DIR}/${_published}.xex)
    endforeach()
endif()

# Code-plus-picture ROMs. The data blocks come from the `example-data` target;
# everything about which blocks and in what order is in SiteRoms.cmake.
if(XEX_FILTER_EXECUTABLE AND SITE_DATA_ROMS)
    foreach(_rom IN LISTS SITE_DATA_ROMS)
        set(_published ${SITE_DATA_ROM_${_rom}_PUBLISHED})

        # '|' rather than ';': a ';' inside a -D argument would be split back
        # into separate arguments before the script ever saw it.
        set(_data "")
        foreach(_block IN LISTS SITE_DATA_ROM_${_rom}_DATA)
            string(APPEND _data "${EXAMPLE_DATA_BINARY_DIR}/${_block}|")
        endforeach()
        set(_with "")
        foreach(_extra IN LISTS SITE_DATA_ROM_${_rom}_WITH)
            string(APPEND _with "${_extra}|")
        endforeach()

        list(APPEND _site_commands
            COMMAND ${CMAKE_COMMAND}
                    -DXEX_FILTER=${XEX_FILTER_EXECUTABLE}
                    -DCODE=${EXAMPLES_BINARY_DIR}/src/${_rom}.xex
                    -DRELOCATE=${SITE_DATA_ROM_${_rom}_RELOCATE}
                    -DWITH=${_with}
                    -DDATA=${_data}
                    -DOUT=${SITE_STAGE_DIR}/${_published}.xex
                    -P ${PROJECT_SOURCE_DIR}/cmake/MergeDataXex.cmake
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
                    ${SITE_STAGE_DIR}/${_published}.xex
                    ${SITE_ROMS_DIR}/${_published}.xex)
    endforeach()
endif()

# Tunes imported straight from an original module by `sgu-tracker` - no
# examples target, no merging. `--compact` drops what the import left unused
# before packing, and the .sgm the exporter writes next to the .xex lands in the
# staging directory rather than in the site's tree.
if(SITE_SGM_ROMS)
    if(NOT SGU_TRACKER_EXECUTABLE)
        message(STATUS "sgu-tracker not found: 'site' will skip the imported "
                       "tune ROMs (Draconus, alloyrun, e1m1, fm-troni, "
                       "pinball_illusions)")
    else()
        # sgu-tracker writes the .xex (and the .sgm beside it) itself and
        # will not create a missing directory to do it.
        list(APPEND _site_commands
            COMMAND ${CMAKE_COMMAND} -E make_directory ${SITE_STAGE_DIR})
        list(LENGTH SITE_SGM_ROMS _n)
        math(EXPR _last "${_n} / 2 - 1")
        foreach(_i RANGE ${_last})
            math(EXPR _m "${_i} * 2")
            math(EXPR _p "${_m} + 1")
            list(GET SITE_SGM_ROMS ${_m} _module)
            list(GET SITE_SGM_ROMS ${_p} _published)
            list(APPEND _site_commands
                COMMAND ${SGU_TRACKER_EXECUTABLE}
                        import ${SITE_ROMS_DIR}/${_module}
                        sgm --xex --compact
                        ${SITE_SGM_ROM_${_published}_OPTIONS}
                        -o ${SITE_STAGE_DIR}/${_published}.xex
                        --exit
                COMMAND ${CMAKE_COMMAND} -E copy_if_different
                        ${SITE_STAGE_DIR}/${_published}.xex
                        ${SITE_ROMS_DIR}/${_published}.xex)
        endforeach()
    endif()
endif()

# VERBATIM: without it the recipe shell eats the `$` in a hex load address
# ($0F82 arrives as F82, and $0882 silently arrives as decimal 882) and in the
# relocation list the data ROMs pass through.
add_custom_target(site
    ${_site_commands}
    USES_TERMINAL VERBATIM
    COMMENT "Refreshing the web emulator and ROMs in ${SITE_EMU_DIR}")
add_dependencies(site emu-wasm examples example-data)
