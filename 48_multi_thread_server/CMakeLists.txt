include_directories(.)

add_executable(48_multi_thread_server main.cpp)

if (WIN32)
    target_link_libraries(48_multi_thread_server ws2_32)
endif ()