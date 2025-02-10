include_directories(.)

#声明头文件路径
set(INC_DIR ./include)
#声明链接库路径
set(LINK_DIR ./lib)

#引入头文件
include_directories(${INC_DIR})
#引入库文件
link_directories(${LINK_DIR})

add_executable(82_libevent main.cpp)

target_link_libraries(82_libevent event)