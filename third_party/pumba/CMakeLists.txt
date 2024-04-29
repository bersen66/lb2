add_library(pumba INTERFACE)

target_include_directories(
    pumba
    INTERFACE $<BUILD_INTERFACE: ${CMAKE_CURRENT_SOURCE_DIR}/include>
    
)
