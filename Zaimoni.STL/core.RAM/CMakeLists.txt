cmake_minimum_required(VERSION 3.15)

add_library(z_memory SHARED memory.cpp)
target_compile_definitions(z_memory INTERFACE ZAIMONI_HAVE_ACCURATE_MSIZE)
target_compile_definitions(z_memory INTERFACE _HAS_ITERATOR_DEBUGGING=0)	# for MSVC++ STL
