find_package(
    Boost 1.74 REQUIRED
    system
    thread
    program_options
    COMPONENTS
)

find_package(yaml-cpp REQUIRED)

include_directories(${Boost_INCLUDE_DIRS})
include_directories(${yaml-cpp_INCLUDE_DIRS})
