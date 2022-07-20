cmake_minimum_required(VERSION 3.16)
project (kissjson)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin/)

add_compile_options(-std=c++17)
add_compile_options(-Wall)
add_compile_options(-Werror)

add_subdirectory (tests)
