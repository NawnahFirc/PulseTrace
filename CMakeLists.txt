cmake_minimum_required(VERSION 3.24)
project(PulseTrace VERSION 0.2.0 LANGUAGES CXX)

if(NOT CMAKE_SYSTEM_NAME STREQUAL "Linux")
    message(FATAL_ERROR "PulseTrace requires Linux")
endif()
if(NOT CMAKE_CXX_COMPILER_ID MATCHES "^(GNU|Clang)$")
    message(FATAL_ERROR "PulseTrace currently supports GCC and Clang")
endif()

option(PULSETRACE_WARNINGS_AS_ERRORS "Treat project warnings as errors" OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
configure_file(cmake/version.hpp.in generated/pulsetrace/version.hpp @ONLY)

option(PULSETRACE_SANITIZERS "Enable AddressSanitizer and UndefinedBehaviorSanitizer" OFF)
function(pulsetrace_target_defaults target)
    target_compile_features(${target} PUBLIC cxx_std_20)
    set_target_properties(${target} PROPERTIES CXX_EXTENSIONS OFF)
    target_compile_options(${target} PRIVATE
        -Wall -Wextra -Wpedantic -Wconversion -Wshadow
        $<$<BOOL:${PULSETRACE_WARNINGS_AS_ERRORS}>:-Werror>)
    if(PULSETRACE_SANITIZERS)
        target_compile_options(${target} PRIVATE -fsanitize=address,undefined -fno-omit-frame-pointer)
        target_link_options(${target} PRIVATE -fsanitize=address,undefined)
    endif()
endfunction()

add_library(pulsetrace_core STATIC src/linux/process_discovery.cpp)
target_include_directories(pulsetrace_core PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include")
pulsetrace_target_defaults(pulsetrace_core)
add_executable(pulsetrace src/main.cpp)
target_link_libraries(pulsetrace PRIVATE pulsetrace_core)
target_include_directories(pulsetrace PRIVATE "${CMAKE_CURRENT_BINARY_DIR}/generated")
pulsetrace_target_defaults(pulsetrace)

include(CTest)
if(BUILD_TESTING)
    include(cmake/Testing.cmake)
    add_test(NAME cli_contract COMMAND "${CMAKE_COMMAND}"
        "-DPROGRAM=$<TARGET_FILE:pulsetrace>"
        "-DEXPECTED_VERSION=${PROJECT_VERSION}"
        -P "${CMAKE_CURRENT_SOURCE_DIR}/tests/cli_contract.cmake")
endif()
include(GNUInstallDirs)
install(TARGETS pulsetrace RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}")
