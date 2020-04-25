cmake_minimum_required(VERSION 3.15)

add_library(z_memory STATIC memory.cpp)
target_compile_definitions(z_memory INTERFACE ZAIMONI_HAVE_ACCURATE_MSIZE)