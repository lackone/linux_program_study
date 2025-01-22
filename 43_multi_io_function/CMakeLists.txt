include_directories(.)

add_executable(43_multi_io_function main.cpp)

if (WIN32)
    target_link_libraries(43_multi_io_function ws2_32)
endif ()