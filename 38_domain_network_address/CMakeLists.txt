include_directories(.)

add_executable(38_domain_network_address main.cpp)

if (WIN32)
    target_link_libraries(38_domain_network_address ws2_32)
endif ()