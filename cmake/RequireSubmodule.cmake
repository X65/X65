# Invoked with -P before driving a submodule's own build. A submodule that was
# never initialised configures with a confusing error from deep inside its own
# CMake (the emulator complains about missing SDL3/imgui rather than about the
# submodule), so check for a file that only exists once it is populated and say
# what to run instead.
if(NOT EXISTS "${MARKER}")
    message(FATAL_ERROR
        "${NAME} is not checked out.\n"
        "  git -C ${DIR} submodule update --init --recursive")
endif()
