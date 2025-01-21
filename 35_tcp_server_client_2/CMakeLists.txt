include_directories(.)

add_executable(35_tcp_server_client_2 main.cpp)

if (WIN32)
    target_link_libraries(35_tcp_server_client_2 ws2_32)
endif ()