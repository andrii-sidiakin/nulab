add_library(bclib)

target_sources(bclib PRIVATE
    #bc_find_nonce.c
    bc_hex.c
    bc_json.c
    bc_merkle_root.c
    bc_utils.c
    sha256.c
)

target_include_directories(bclib PUBLIC
    "${CMAKE_CURRENT_SOURCE_DIR}"
)

target_compile_options(bclib PUBLIC
    $<$<C_COMPILER_ID:GNU>:-std=c23>
    $<$<C_COMPILER_ID:Clang>:-std=c23>
    $<$<C_COMPILER_ID:MSVC>:/std:clatest>

    $<$<COMPILE_LANG_AND_ID:C,Clang>:-Rpass=loop-vectorize>
    #$<$<COMPILE_LANG_AND_ID:C,Clang>:-Rpass-missed=loop-vectorize>
    #$<$<COMPILE_LANG_AND_ID:C,Clang>:-Rpass-analysis=loop-vectorize>

    $<$<CONFIG:Release>:-march=native>
    $<$<CONFIG:Release>:-ffast-math>
    $<$<CONFIG:Release>:-funroll-loops>
)
target_link_options(bclib PUBLIC
    $<$<CONFIG:Release>:-flto>
)

target_link_libraries(bclib PUBLIC NuLab::NuLib)

# tests

add_executable(test_sha256 test_sha256.c)
target_link_libraries(test_sha256 PRIVATE bclib)
add_test(NAME test_sha256 COMMAND test_sha256)

add_executable(test_merkle_root test_merkle_root.c)
target_link_libraries(test_merkle_root bclib)
add_test(NAME test_merkle_root COMMAND test_merkle_root)
