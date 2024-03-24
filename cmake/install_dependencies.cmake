find_package(
    Boost 1.74 REQUIRED
    system
    thread
    program_options
    log
    log_setup
    COMPONENTS
)

find_package(yaml-cpp REQUIRED)
find_package(spdlog REQUIRED)
find_package(jemalloc REQUIRED)


include_directories(${Boost_INCLUDE_DIRS})
include_directories(${yaml-cpp_INCLUDE_DIRS})
