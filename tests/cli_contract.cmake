function(check_cli expected_exit expected_stdout expected_stderr)
    execute_process(COMMAND "${PROGRAM}" ${ARGN}
        RESULT_VARIABLE actual_exit OUTPUT_VARIABLE actual_stdout
        ERROR_VARIABLE actual_stderr TIMEOUT 5)
    if(NOT "${actual_exit}" STREQUAL "${expected_exit}" OR
       NOT "${actual_stdout}" STREQUAL "${expected_stdout}" OR
       NOT "${actual_stderr}" STREQUAL "${expected_stderr}")
        message(FATAL_ERROR "CLI contract failed for [${ARGN}]: exit=${actual_exit}, stdout=[${actual_stdout}], stderr=[${actual_stderr}]")
    endif()
endfunction()
set(help "Usage: pulsetrace [--help | --version | --list-pids [--proc-root PATH]]\nLinux process monitoring and diagnostics.\nMilestone 1: process discovery; metadata and metrics are not implemented yet.\n")
check_cli(0 "PulseTrace ${EXPECTED_VERSION}\n${help}" "")
check_cli(0 "${help}" "" --help)
check_cli(0 "PulseTrace ${EXPECTED_VERSION}\n" "" --version)
check_cli(2 "" "Invalid arguments. Use pulsetrace --help.\n" --unknown)
check_cli(2 "" "Invalid arguments. Use pulsetrace --help.\n" --help extra)
execute_process(COMMAND "${PROGRAM}" --version OUTPUT_FILE /dev/full
    ERROR_VARIABLE errors RESULT_VARIABLE result TIMEOUT 5)
if(NOT "${result}" STREQUAL "1")
    message(FATAL_ERROR "Expected exit 1 on output failure, got ${result}: ${errors}")
endif()

# Controlled root tests exercise the CLI-to-filesystem boundary.
set(root "${CMAKE_CURRENT_BINARY_DIR}/cli-proc-fixture")
file(REMOVE_RECURSE "${root}")
file(MAKE_DIRECTORY "${root}/9" "${root}/2" "${root}/self")
file(WRITE "${root}/5" "regular file")
check_cli(0 "2\n9\n" "" --list-pids --proc-root "${root}")
check_cli(2 "" "Invalid arguments. Use pulsetrace --help.\n" --list-pids --proc-root)
execute_process(COMMAND "${PROGRAM}" --list-pids --proc-root "${root}/missing"
    RESULT_VARIABLE result OUTPUT_VARIABLE output ERROR_VARIABLE errors TIMEOUT 5)
if(NOT "${result}" STREQUAL "1" OR NOT "${output}" STREQUAL "" OR
   NOT errors MATCHES "Cannot open process root:")
    message(FATAL_ERROR "Missing CLI root must fail explicitly: ${result}, ${output}, ${errors}")
endif()
file(REMOVE_RECURSE "${root}")
