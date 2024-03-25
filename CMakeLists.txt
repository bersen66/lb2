cmake_minimum_required(VERSION 3.16)
project(lb2)

message("Building with CMake version: ${CMAKE_VERSION}")
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)


set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/lb2)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/lb2)
set(CMAKE_MODULE_PATH ${CMAKE_CURRENT_BINARY_DIR})

include(cmake/install_dependencies.cmake)
# App library
file(GLOB lb2_sources src/lb/*.cpp)
add_library(lb2 SHARED ${lb2_sources})
target_include_directories(lb2 PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src)
target_link_libraries(
    lb2 PRIVATE
    Boost::system
    Boost::thread
    Boost::program_options
    spdlog::spdlog
    yaml-cpp::yaml-cpp
    jemalloc::jemalloc
)

# Main app
add_executable(lb_app src/main.cpp)
target_link_libraries(lb_app lb2)

# Setting up unit-tests
enable_testing()
file(GLOB unit_tests_sources tests/*.cpp)
add_executable(lb_tests ${unit_tests_sources})
target_link_libraries(lb_tests PUBLIC lb2 GTest::gtest_main)