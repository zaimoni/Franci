# TheoremTable regression, run by CTest via cmake -P.  Inputs: THEOREM_TABLE (the built
# exe), SRC (source root), BIN (scratch root).  Renders the expected theorem file and
# compares against the hand-written expected HTML, modulo line endings.

file(MAKE_DIRECTORY ${BIN})
execute_process(
    COMMAND ${THEOREM_TABLE} -o ${BIN}/cut_rule.html ${SRC}/tests/cut_rule.theorem.yaml
    RESULT_VARIABLE rc
    OUTPUT_VARIABLE out
    ERROR_VARIABLE err)
if(NOT rc EQUAL 0)
    message(FATAL_ERROR "TheoremTable exited ${rc}\n${out}\n${err}")
endif()

execute_process(
    COMMAND ${CMAKE_COMMAND} -E compare_files --ignore-eol
        ${BIN}/cut_rule.html ${SRC}/tests/cut_rule.html
    RESULT_VARIABLE rc)
if(NOT rc EQUAL 0)
    file(READ ${BIN}/cut_rule.html actual)
    message(FATAL_ERROR "HTML differs from tests/cut_rule.html; actual:\n${actual}")
endif()
