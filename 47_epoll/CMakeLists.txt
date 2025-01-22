include_directories(.)

add_executable(47_epoll main.cpp)

if (WIN32)
    target_link_libraries(47_epoll ws2_32)
endif ()