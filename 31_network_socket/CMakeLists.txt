include_directories(.)

add_executable(31_network_socket main.cpp)

if (WIN32)
    target_link_libraries(31_network_socket ws2_32)
endif ()

