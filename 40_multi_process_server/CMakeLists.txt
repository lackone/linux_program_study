include_directories(.)

add_executable(40_multi_process_server main.cpp)

if (WIN32)
    target_link_libraries(40_multi_process_server ws2_32)
endif ()