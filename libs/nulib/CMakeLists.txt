add_library(nu)
add_library(NuLab::NuLib ALIAS nu)

target_include_directories(
  nu PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
            $<INSTALL_INTERFACE:include>)

target_compile_options(nu PRIVATE 
    $<$<C_COMPILER_ID:GNU>:-std=c99>
    $<$<C_COMPILER_ID:Clang>:-std=c99>
    $<$<C_COMPILER_ID:MSVC>:/std:clatest>
)

target_compile_options(nu PRIVATE
    $<$<C_COMPILER_ID:GNU>:-Wall -Wextra -Wpedantic>
    $<$<C_COMPILER_ID:Clang>:-Wall -Wextra -Wpedantic>
    $<$<C_COMPILER_ID:MSVC>:/W4>

    $<$<AND:$<C_COMPILER_ID:GNU>,$<CONFIG:Release>>:-Werror>
    $<$<AND:$<C_COMPILER_ID:Clang>,$<CONFIG:Release>>:-Werror>
    $<$<AND:$<C_COMPILER_ID:MSVC>,$<CONFIG:Release>>:/WX>
)

if(NULIB_ENABLE_COVERAGE)
    target_compile_options(nu PRIVATE
        $<$<BOOL:${NULIB_ENABLE_TESTING}>:--coverage>
    )
    target_link_options(nu PRIVATE
        $<$<BOOL:${NULIB_ENABLE_TESTING}>:--coverage>
    )
endif()

target_sources(nu PRIVATE
    src/assert.c
    src/memory.c
    src/dca.c
)
