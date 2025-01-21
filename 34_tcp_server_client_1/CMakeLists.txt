include_directories(.)

add_executable(34_tcp_server_client_1 main.cpp)

if (WIN32)
    target_link_libraries(34_tcp_server_client_1 ws2_32)
endif ()