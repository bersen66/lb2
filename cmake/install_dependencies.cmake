find_package(
    Boost 1.74 REQUIRED
    system
    thread
    program_options
    stacktrace
    COMPONENTS
)

find_package(yaml-cpp REQUIRED)
find_package(spdlog REQUIRED)
find_package(jemalloc REQUIRED)
find_package(GTest REQUIRED)
find_package(ctre REQUIRED)
find_package(benchmark REQUIRED)

include_directories(${Boost_INCLUDE_DIRS})
include_directories(${yaml-cpp_INCLUDE_DIRS})

include_directories(third_party/pumba/include)