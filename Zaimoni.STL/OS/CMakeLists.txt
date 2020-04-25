cmake_minimum_required(VERSION 3.15)

add_library(z_console STATIC console.cpp)
add_library(z_cstdio_console STATIC cstdio_console.cpp)
add_library(z_log_adapter STATIC log_adapter_inc_inform.cpp log_adapter_inform.cpp log_adapter_log.cpp)

target_link_libraries(z_console INTERFACE z_logging INTERFACE z_memory)
target_link_libraries(z_cstdio_console INTERFACE z_logging INTERFACE z_memory)

target_compile_definitions(z_console PRIVATE ZAIMONI_HAVE_ACCURATE_MSIZE)
target_compile_definitions(z_cstdio_console PRIVATE ZAIMONI_HAVE_ACCURATE_MSIZE)

if (NOT DEFINED $CACHE{ZSTL_WINDOWS_H} OR $CACHE{ZSTL_WINDOWS_H})
	add_subdirectory(Win32)
endif()
