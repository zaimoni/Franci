cmake_minimum_required(VERSION 3.15)

add_subdirectory(Pure.C)
add_subdirectory(OS)
add_subdirectory(core.RAM)
# add_subdirectory(LexParse)	# doesn't build yet

# Z.C++ uses C stdio logging
# target_link_libraries(z_langconf INTERFACE z_memory)
target_link_libraries(z_log_adapter INTERFACE z_format_util INTERFACE z_stdio_log)
target_link_libraries(z_console INTERFACE z_logging INTERFACE z_memory)
target_link_libraries(z_cstdio_console INTERFACE z_logging INTERFACE z_memory)
