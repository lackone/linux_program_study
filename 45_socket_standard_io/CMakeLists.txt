include_directories(.)

add_executable(45_socket_standard_io main.cpp)

if (WIN32)
    target_link_libraries(45_socket_standard_io ws2_32)
endif ()