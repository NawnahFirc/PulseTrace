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
set(help "Usage: pulsetrace [--help | --version]\nLinux process monitoring and diagnostics.\nMilestone 0: build scaffold; monitoring is not implemented yet.\n")
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
