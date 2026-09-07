cmake_minimum_required(VERSION 3.24)
project(PulseTrace VERSION 0.1.0 LANGUAGES CXX)

if(NOT CMAKE_SYSTEM_NAME STREQUAL "Linux")
    message(FATAL_ERROR "PulseTrace requires Linux")
endif()
if(NOT CMAKE_CXX_COMPILER_ID MATCHES "^(GNU|Clang)$")
    message(FATAL_ERROR "PulseTrace currently supports GCC and Clang")
endif()

option(PULSETRACE_WARNINGS_AS_ERRORS "Treat project warnings as errors" OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
configure_file(cmake/version.hpp.in generated/pulsetrace/version.hpp @ONLY)

add_executable(pulsetrace src/main.cpp)
target_compile_features(pulsetrace PRIVATE cxx_std_20)
set_target_properties(pulsetrace PROPERTIES CXX_EXTENSIONS OFF)
target_include_directories(pulsetrace PRIVATE "${CMAKE_CURRENT_BINARY_DIR}/generated")
target_compile_options(pulsetrace PRIVATE
    -Wall -Wextra -Wpedantic -Wconversion -Wshadow
    $<$<BOOL:${PULSETRACE_WARNINGS_AS_ERRORS}>:-Werror>)

include(CTest)
if(BUILD_TESTING)
    add_test(NAME cli_contract COMMAND "${CMAKE_COMMAND}"
        "-DPROGRAM=$<TARGET_FILE:pulsetrace>"
        "-DEXPECTED_VERSION=${PROJECT_VERSION}"
        -P "${CMAKE_CURRENT_SOURCE_DIR}/tests/cli_contract.cmake")
endif()
include(GNUInstallDirs)
install(TARGETS pulsetrace RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}")
