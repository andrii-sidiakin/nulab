
function(nulib_add_test NAME)
    cmake_parse_arguments(arg "" "" "SOURCES" ${ARGN})

    add_executable(${NAME} ${arg_SOURCES})
    target_link_libraries(${NAME} PRIVATE NuLab::NuLib)
    add_test(NAME ${NAME} COMMAND ${NAME})

endfunction()

nulib_add_test(test_sanity_check SOURCES sanity_check.c)
nulib_add_test(test_dca SOURCES dca.c)

