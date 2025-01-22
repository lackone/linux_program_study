include_directories(.)

add_executable(39_socket_optional main.cpp)

if (WIN32)
    target_link_libraries(39_socket_optional ws2_32)
endif ()