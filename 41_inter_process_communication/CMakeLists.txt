include_directories(.)

add_executable(41_inter_process_communication main.cpp)

if (WIN32)
    target_link_libraries(41_inter_process_communication ws2_32)
endif ()