cmake_minimum_required(VERSION 3.15)

add_library(z_csvtable STATIC CSVTable.cpp)
# add_library(z_csvtable_iso STATIC CSVTable.cpp)	# doesn't build yet
add_library(z_kuroda STATIC FormalWord.cpp LexNode.cpp)
add_library(z_langconf STATIC LangConf.cpp)
add_library(z_langconf_iso STATIC LangConf.cpp)

target_compile_definitions(z_csvtable PUBLIC ZAIMONI_HAVE_ACCURATE_MSIZE)
target_compile_definitions(z_csvtable PRIVATE _HAS_ITERATOR_DEBUGGING=0)	# for MSVC++ STL
# target_compile_definitions(z_csvtable_iso PUBLIC ZAIMONI_FORCE_ISO)
target_compile_definitions(z_kuroda PUBLIC ZAIMONI_HAVE_ACCURATE_MSIZE)
target_compile_definitions(z_kuroda PRIVATE _HAS_ITERATOR_DEBUGGING=0)	# for MSVC++ STL
target_compile_definitions(z_langconf PUBLIC ZAIMONI_HAVE_ACCURATE_MSIZE)
target_compile_definitions(z_langconf PRIVATE _HAS_ITERATOR_DEBUGGING=0)	# for MSVC++ STL
target_compile_definitions(z_langconf_iso PUBLIC ZAIMONI_FORCE_ISO)