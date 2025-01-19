include_directories(.)

add_compile_options(-l sqlite3)

add_executable(11_sqlite main.cpp sqlite3.h sqlite3.c)