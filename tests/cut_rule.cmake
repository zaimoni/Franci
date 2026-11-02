# GentzenW proof-session regression (cut-rule plan work item #7), run by CTest via
# cmake -P.  Inputs: GENTZENW (the built exe), SRC (source root), BIN (scratch root).
#
# The session runs from a *staged copy* of the exe: GentzenW looks for cfg/core.yaml
# beside itself, and staging cfg/ beside the build's own exe is deliberately not
# automated (it would overwrite the test_lines key in the Visual Studio debug build's
# staged copy).  The theorem file is written next to the input script, so the script is
# copied too and tests/ is never dirtied.  Stdin comes from an ASCII picks file through
# execute_process(INPUT_FILE), which has no BOM problem.

set(stage ${BIN}/cut_rule)
file(REMOVE_RECURSE ${stage})
file(MAKE_DIRECTORY ${stage}/cfg)

get_filename_component(exe_dir ${GENTZENW} DIRECTORY)
get_filename_component(exe_name ${GENTZENW} NAME)
file(GLOB runtime_dlls ${exe_dir}/*.dll)	# z_memory is SHARED; the post-build step stages it beside the exe
file(COPY ${GENTZENW} ${runtime_dlls} DESTINATION ${stage})
file(COPY ${SRC}/cfg/core.yaml DESTINATION ${stage}/cfg)
file(COPY ${SRC}/tests/cut_rule.yaml DESTINATION ${stage})

execute_process(
    COMMAND ${stage}/${exe_name} ${stage}/cut_rule.yaml
    INPUT_FILE ${SRC}/tests/cut_rule_picks.txt
    RESULT_VARIABLE rc
    OUTPUT_VARIABLE out
    ERROR_VARIABLE err)
if(NOT rc EQUAL 0)
    message(FATAL_ERROR "GentzenW exited ${rc}\n${out}\n${err}")
endif()

# fkYAML serializes deterministically, so a byte comparison modulo line endings is
# exact; a YAML-aware comparison would only be needed if the serializer changed
execute_process(
    COMMAND ${CMAKE_COMMAND} -E compare_files --ignore-eol
        ${stage}/cut_rule.theorem.yaml ${SRC}/tests/cut_rule.theorem.yaml
    RESULT_VARIABLE rc)
if(NOT rc EQUAL 0)
    file(READ ${stage}/cut_rule.theorem.yaml actual)
    message(FATAL_ERROR "theorem file differs from tests/cut_rule.theorem.yaml; actual:\n${actual}")
endif()
