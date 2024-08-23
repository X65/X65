# Add cmake commands: rp6502_executable() and rp6502_asset()

# Package a CC65 executable target as an RP6502 ROM.
#
# RP6502 Executables
# ^^^^^^^^^^^^^^^^^^
#
#  rp6502_executable(<name> START [addr] RESET [addr] roms...)
#
# Packages executable target ``<name>`` into RP6502 ROM format.
# ``START <addr>`` defaults to 0x200.
# ``RESET <addr>`` defaults to 0x200.
# ``IRQ <addr>`` no default.
# ``NMI <addr>`` no default.
#
function(rp6502_executable name)
    # Parse args
    set(start_addr "0x200")
    set(reset_addr "0x200")
    set(irq_addr "none")
    set(nmi_addr "none")
    set(extra_roms)
    foreach(X IN LISTS ARGN)
        if (NOT start_addr)
            set(start_addr ${X})
        elseif (NOT reset_addr)
            set(reset_addr ${X})
        elseif (NOT irq_addr)
            set(irq_addr ${X})
        elseif (NOT nmi_addr)
            set(nmi_addr ${X})
        elseif (X STREQUAL "START")
            set(start_addr FALSE)
        elseif (X STREQUAL "RESET")
            set(reset_addr FALSE)
        elseif (X STREQUAL "IRQ")
            set(irq_addr FALSE)
        elseif (X STREQUAL "NMI")
            set(nmi_addr FALSE)
        else ()
            list(APPEND extra_roms ${X})
        endif ()
    endforeach()
    if (NOT start_addr)
        message (FATAL_ERROR "rp6502_executable START address missing")
    elseif (NOT reset_addr)
        message (FATAL_ERROR "rp6502_executable RESET address missing")
    endif ()
    # Remove old ROM
    add_custom_command(TARGET ${name} PRE_BUILD
        COMMAND ${CMAKE_COMMAND} -E remove
        "${CMAKE_CURRENT_BINARY_DIR}/${name}.rp6502"
    )
    # Create new ROM
    find_package(Python3 REQUIRED COMPONENTS Interpreter)
    set(tool_command "${Python3_EXECUTABLE}"
        "${CMAKE_CURRENT_SOURCE_DIR}/tools/rp6502.py"
    )
    if (NOT start_addr STREQUAL "none")
        list(APPEND tool_command
            -a "${start_addr}"
        )
    endif ()
    if (NOT reset_addr STREQUAL "none")
        list(APPEND tool_command
            -r "${reset_addr}"
        )
    endif ()
    if (NOT irq_addr STREQUAL "none")
        list(APPEND tool_command
            -i "${irq_addr}"
        )
    endif ()
    if (NOT nmi_addr STREQUAL "none")
        list(APPEND tool_command
            -n "${nmi_addr}"
        )
    endif ()
    list(APPEND tool_command
        -o "${CMAKE_CURRENT_BINARY_DIR}/${name}.rp6502"
        create "${CMAKE_CURRENT_BINARY_DIR}/${name}"
        -- ${extra_roms}
    )
    add_custom_command(TARGET ${name} POST_BUILD
        COMMAND ${tool_command}
        DEPENDS ${name})
endfunction()

# Package anything as an RP6502 ROM.
#
# RP6502 ROMs
# ^^^^^^^^^^^
#
#  rp6502_asset(<name> addr in_file {out_file})
#
# Packages the ``<in_file>`` into RP6502 ROM format.
# ``out_file`` defaults to in_file plus ``.rp6502``
#
function(rp6502_asset name addr in_file)
    # Parse optional args
    get_filename_component(out_file ${in_file} NAME)
    set(out_file "${out_file}.rp6502")
    set(custom_target_name "${name}.${addr}.${out_file}")
    set(extra_args ${ARGN})
    list(LENGTH extra_args extra_count)
    if (${extra_count} GREATER 0)
        list(GET ${ARGN} 0 out_file)
    endif ()
    add_custom_target(
        ${custom_target_name} ALL
        DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/${out_file}
    )
    find_package(Python3 REQUIRED COMPONENTS Interpreter)
    add_custom_command(
        OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/${out_file}
        DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/${in_file}
        COMMAND
            "${Python3_EXECUTABLE}"
            "${CMAKE_CURRENT_SOURCE_DIR}/tools/rp6502.py"
            -a "${addr}"
            -o "${CMAKE_CURRENT_BINARY_DIR}/${out_file}"
            create "${CMAKE_CURRENT_SOURCE_DIR}/${in_file}"
    )
    add_dependencies(${name} ${custom_target_name})
endfunction()
