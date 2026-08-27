# Invoked with -P: splice a demo's data blocks into its assembled code.
#
# Some of the CGIA examples assemble to code only - loading one on its own gets
# you a program with no picture. `bins.c` writes the picture out as .xex blocks
# and xex-filter concatenates the two.
#
# A demo may also need its code moved first: `-a` hands each block of each input,
# in order, a new load address. mixed_modes uses that to lift its code and the
# shared font out of the memory the picture wants, which is why RELOCATE lists
# four addresses for three code blocks plus the font.
#
# Expects: XEX_FILTER CODE DATA OUT, optionally RELOCATE and WITH.
# DATA and WITH are '|'-separated, because a ';' would be split up on the way in
# from the -D argument that carries them.

get_filename_component(_out_dir "${OUT}" DIRECTORY)
file(MAKE_DIRECTORY "${_out_dir}")

string(REPLACE "|" ";" _data "${DATA}")
string(REPLACE "|" ";" _with "${WITH}")

set(_code "${CODE}")
if(RELOCATE)
    set(_moved "${OUT}.code")
    execute_process(COMMAND "${XEX_FILTER}" -o "${_moved}" -a "${RELOCATE}" "${CODE}" ${_with}
                    RESULT_VARIABLE _rc OUTPUT_QUIET)
    if(NOT _rc EQUAL 0)
        message(FATAL_ERROR "xex-filter failed to relocate ${CODE} to ${RELOCATE}")
    endif()
    set(_code "${_moved}")
endif()

execute_process(COMMAND "${XEX_FILTER}" -o "${OUT}" "${_code}" ${_data}
                RESULT_VARIABLE _rc OUTPUT_QUIET)
if(NOT _rc EQUAL 0)
    message(FATAL_ERROR "xex-filter failed to merge ${CODE} with its data blocks")
endif()

if(RELOCATE)
    file(REMOVE "${_moved}")
endif()
