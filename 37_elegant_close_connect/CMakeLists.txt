include_directories(.)

add_executable(37_elegant_close_connect main.cpp)

if (WIN32)
    target_link_libraries(37_elegant_close_connect ws2_32)
endif ()