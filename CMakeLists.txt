cmake_minimum_required(VERSION 3.16)
project (kissjson)

add_compile_options(-std=c++17)
add_compile_options(-Wall)
add_compile_options(-Werror)

add_subdirectory (tests)
