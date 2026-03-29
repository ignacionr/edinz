# This script is run at build time to regenerate version.hpp with the current git hash.
# It is invoked via add_custom_target in the main CMakeLists.txt.

find_package(Git QUIET)

set(EDINZ_GIT_HASH "unknown")
set(EDINZ_GIT_DIRTY 0)

if(Git_FOUND)
    execute_process(
        COMMAND "${GIT_EXECUTABLE}" rev-parse --short HEAD
        WORKING_DIRECTORY "${SOURCE_DIR}"
        OUTPUT_VARIABLE EDINZ_GIT_HASH
        OUTPUT_STRIP_TRAILING_WHITESPACE
        ERROR_QUIET
        RESULT_VARIABLE git_result
    )
    if(NOT git_result EQUAL 0)
        set(EDINZ_GIT_HASH "unknown")
    endif()

    execute_process(
        COMMAND "${GIT_EXECUTABLE}" diff --quiet HEAD
        WORKING_DIRECTORY "${SOURCE_DIR}"
        RESULT_VARIABLE git_dirty_result
    )
    if(NOT git_dirty_result EQUAL 0)
        set(EDINZ_GIT_DIRTY 1)
    endif()
endif()

configure_file(
    "${SOURCE_DIR}/src/version.hpp.in"
    "${BINARY_DIR}/generated/version.hpp"
    @ONLY
)
