cmake_minimum_required(VERSION 3.16)
project(lb2)

message("Building with CMake version: ${CMAKE_VERSION}")
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/lb2)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/lb2)

include(cmake/install_dependencies.cmake)
add_compile_definitions(SPDLOG_ACTIVE_LEVEL=SPDLOG_LEVEL_TRACE)

# App library
file(GLOB lb2_sources
    src/lb/*.cpp
    src/lb/tcp/*.cpp
)
add_library(lb2 ${lb2_sources})
target_include_directories(lb2 PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src)
target_link_libraries(
    lb2 PUBLIC
    Boost::system
    Boost::thread
    Boost::program_options
    Boost::stacktrace
    Boost::stacktrace_backtrace
    spdlog::spdlog
    yaml-cpp::yaml-cpp
    jemalloc::jemalloc
    ctre::ctre
)

# Copying configs
add_custom_target(
	copy_configs ALL
	COMMAND ${CMAKE_COMMAND} -E copy_directory
	${PROJECT_SOURCE_DIR}/configs
	${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/configs
	COMMENT "Copying configs...."
)

# Main app
add_executable(lb_app src/main.cpp)
target_compile_definitions(lb_app PUBLIC LB_APP_LOGGING)
target_link_libraries(lb_app PUBLIC lb2)
add_dependencies(lb_app copy_configs)

# Setting up unit-tests
enable_testing()
include(GoogleTest)
file(GLOB unit_tests_sources tests/*.cpp)
add_executable(lb_tests ${unit_tests_sources})
target_link_libraries(
    lb_tests PUBLIC
    lb2
    GTest::gtest
    spdlog::spdlog
    yaml-cpp::yaml-cpp
    jemalloc::jemalloc
)
add_dependencies(lb_tests copy_configs)
gtest_discover_tests(
    lb_tests
    WORKING_DIRECTORY ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}
)

file(GLOB lb_benchmark_sources benchmarks/*.cpp)
add_executable(lb_benchmark ${lb_benchmark_sources})
target_link_libraries(lb_benchmark PUBLIC lb2 benchmark::benchmark_main)
add_dependencies(lb_benchmark copy_configs)