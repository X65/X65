# Invoked with -P: splice a tune into its player to make a loadable ROM.
#
# The SID/SAP players in `examples` assemble to a bare player - the tune itself
# is a separate file the player expects to find already in memory. Loading one
# without the other gets you a running player with nothing to play. Two
# xex-filter passes fix that: wrap the raw tune as an XEX block at the address
# the player was assembled against, then concatenate the two into one file.
#
# Expects: XEX_FILTER PLAYER TUNE ADDR OUT
get_filename_component(_out_dir "${OUT}" DIRECTORY)
file(MAKE_DIRECTORY "${_out_dir}")
set(_wrapped "${OUT}.tune")

execute_process(COMMAND "${XEX_FILTER}" -o "${_wrapped}" -a "${ADDR}" -b "${TUNE}"
                RESULT_VARIABLE _rc OUTPUT_QUIET)
if(NOT _rc EQUAL 0)
    message(FATAL_ERROR "xex-filter failed to wrap ${TUNE} at ${ADDR}")
endif()

execute_process(COMMAND "${XEX_FILTER}" -o "${OUT}" "${PLAYER}" "${_wrapped}"
                RESULT_VARIABLE _rc OUTPUT_QUIET)
if(NOT _rc EQUAL 0)
    message(FATAL_ERROR "xex-filter failed to merge ${PLAYER} with ${TUNE}")
endif()

file(REMOVE "${_wrapped}")
