include_directories(.)

add_executable(46_io_stream_separate main.cpp)

if (WIN32)
    target_link_libraries(46_io_stream_separate ws2_32)
endif ()