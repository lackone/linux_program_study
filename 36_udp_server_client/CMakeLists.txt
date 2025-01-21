include_directories(.)

add_executable(36_udp_server_client main.cpp)

if (WIN32)
    target_link_libraries(36_udp_server_client ws2_32)
endif ()