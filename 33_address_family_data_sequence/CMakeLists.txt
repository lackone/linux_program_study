include_directories(.)

add_executable(33_address_family_data_sequence main.cpp)

if (WIN32)
    target_link_libraries(33_address_family_data_sequence ws2_32)
endif ()