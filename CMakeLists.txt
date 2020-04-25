cmake_minimum_required(VERSION 3.15)
project(Franci C CXX)
set(CMAKE_CXX_STANDARD 17)

if (NOT DEFINED $CACHE{ZSTL_CMAKE_SUFFIX})
	string(FIND ${CMAKE_INSTALL_PREFIX} "/" TMP_INDEX REVERSE)
	string(SUBSTRING ${CMAKE_INSTALL_PREFIX} ${TMP_INDEX} -1 TMP_CMAKE_SUFFIX)
	set(ZSTL_CMAKE_SUFFIX ${TMP_CMAKE_SUFFIX} CACHE STRING "platform suffix")
endif()

if (NOT DEFINED $CACHE{ZSTL_COMPILER})
	string(FIND ${CMAKE_C_COMPILER} "/" TMP_INDEX REVERSE)
	math(EXPR TMP_INDEX "${TMP_INDEX}+1")
	string(SUBSTRING ${CMAKE_C_COMPILER} ${TMP_INDEX} -1 TMP_COMPILER_STEM)
	string(FIND ${TMP_COMPILER_STEM} "." TMP_INDEX REVERSE)
	string(SUBSTRING ${TMP_COMPILER_STEM} 0 ${TMP_INDEX} TMP_COMPILER_STEM)
	set(ZSTL_COMPILER ${TMP_COMPILER_STEM} CACHE STRING "compiler")
endif()

add_subdirectory(Zaimoni.STL)

if ($CACHE{ZSTL_USLEEP_CODE})
	add_compile_definitions(ZSTL_USLEEP_CODE=$CACHE{ZSTL_USLEEP_CODE})
endif()

# might actually need this?
include_directories(${CMAKE_BINARY_DIR})

# add_executable(... ...)
# target_link_libraries(... ...)
# add_dependencies(... AutoDetect)

# EXCLUDE_FROM_ALL means "can't test"
