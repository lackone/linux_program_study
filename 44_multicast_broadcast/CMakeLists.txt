include_directories(.)

add_executable(44_multicast_broadcast main.cpp)

if (WIN32)
    target_link_libraries(44_multicast_broadcast ws2_32)
endif ()