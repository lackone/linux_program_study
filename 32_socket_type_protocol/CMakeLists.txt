include_directories(.)

add_executable(32_socket_type_protocol main.cpp)

if(WIN32)
    target_link_libraries(32_socket_type_protocol ws2_32)
endif()