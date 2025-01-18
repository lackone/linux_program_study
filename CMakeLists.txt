cmake_minimum_required(VERSION 3.26.5)
project(linux_program_study)

set(CMAKE_CXX_STANDARD 20)

add_executable(linux_program_study main.cpp)

add_subdirectory(01_file_dir)
add_subdirectory(02_io)
add_subdirectory(03_process)
add_subdirectory(04_thread)
add_subdirectory(05_ipc)
add_subdirectory(06_system_v_ipc)
add_subdirectory(07_network_overview)
add_subdirectory(08_tcp)
add_subdirectory(09_server_model)