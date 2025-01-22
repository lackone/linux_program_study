include_directories(.)

add_executable(42_io_reuse main.cpp)

if (WIN32)
    target_link_libraries(42_io_reuse ws2_32)
endif ()