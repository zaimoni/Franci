cmake_minimum_required(VERSION 3.15)
project(Franci C CXX)
set(CMAKE_CXX_STANDARD 23)

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

add_executable(Franci WIN32 LenName.cxx Related.cxx MetaCon2.cxx __GCF.cxx __IntegerNumeral.cxx Integer1.cxx GCF.cxx SymConst.cxx TVal.cxx Class.cxx SeriesOp.cxx Combin1.cxx StdAdd.cxx AddMult.cxx AddEqual.cxx StdMult.cxx EqualMult.cxx Equal.cxx ConEqual.cxx MetaCon3.cxx QState.cxx Quantify.cxx Variable.cxx AntiIdem.cxx MetaCon4.cxx LowRel.cxx MetaCon1.cxx AddInter.cxx Interval.cxx Digraph.cxx SrchTree.cxx main.cxx OS.cxx VConsole.cxx InParse.cxx CmdShell.cxx LexParse.cxx FlatFile.cxx PhraseN.cxx Phrase1.cxx LangCon_.cxx Unparsed.cxx ParseNode.cxx NoReturn.cxx)
target_link_libraries(Franci z_console z_csvtable z_langconf z_logging z_log_adapter z_format_util z_stdio_c z_memory)
target_link_options(Franci PRIVATE /FORCE:MULTIPLE)
# add_dependencies(... AutoDetect)

add_executable(Franci_console LenName.cxx Related.cxx MetaCon2.cxx __GCF.cxx __IntegerNumeral.cxx Integer1.cxx GCF.cxx SymConst.cxx TVal.cxx Class.cxx SeriesOp.cxx Combin1.cxx StdAdd.cxx AddMult.cxx AddEqual.cxx StdMult.cxx EqualMult.cxx Equal.cxx ConEqual.cxx MetaCon3.cxx QState.cxx Quantify.cxx Variable.cxx AntiIdem.cxx MetaCon4.cxx LowRel.cxx MetaCon1.cxx AddInter.cxx Interval.cxx Digraph.cxx SrchTree.cxx main.cxx OS.cxx VConsole.cxx InParse.cxx CmdShell.cxx LexParse.cxx FlatFile.cxx PhraseN.cxx Phrase1.cxx LangCon_.cxx Unparsed.cxx ParseNode.cxx NoReturn.cxx)
target_link_libraries(Franci_console z_cstdio_console z_csvtable z_langconf z_logging z_log_adapter z_format_util z_stdio_c z_memory)
target_link_options(Franci_console PRIVATE /FORCE:MULTIPLE)
target_compile_definitions(Franci_console PUBLIC FRANCI_CLI)
# add_dependencies(... AutoDetect)

add_executable(LogicC Logic.cpp)
target_link_libraries(LogicC z_log_adapter z_stdio_log z_memory)
target_link_options(LogicC PRIVATE /FORCE:MULTIPLE)
target_compile_definitions(LogicC PUBLIC LOGIC_DRIVER)

add_executable(Gentzen Gentzen.cpp HTMLtag.cpp)
target_link_libraries(Gentzen z_kuroda z_log_adapter z_stdio_log z_memory)
target_link_options(Gentzen PRIVATE /FORCE:MULTIPLE)
target_compile_definitions(Gentzen PUBLIC GENTZEN_DRIVER)

add_executable(GentzenW GentzenW.cpp HTMLtag.cpp)
target_link_libraries(GentzenW z_kuroda z_log_adapter z_stdio_log z_memory)
target_link_options(GentzenW PRIVATE /FORCE:MULTIPLE)
target_compile_definitions(GentzenW PUBLIC GENTZEN_DRIVER)

add_custom_command(TARGET Franci POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        $<TARGET_FILE:z_memory>
        $<TARGET_FILE_DIR:Franci>)
add_custom_command(TARGET Franci_console POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        $<TARGET_FILE:z_memory>
        $<TARGET_FILE_DIR:Franci_console>)

# EXCLUDE_FROM_ALL means "can't test"
