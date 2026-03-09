add_library(nu)
add_library(NuLab::NuLib ALIAS nu)

target_include_directories(
  nu PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
            $<INSTALL_INTERFACE:include>)

target_compile_options(nu PRIVATE -std=c99)

target_compile_options(nu PRIVATE
    $<$<C_COMPILER_ID:GNU>:-Wall -Wextra -Wpedantic>
    $<$<C_COMPILER_ID:Clang>:-Wall -Wextra -Wpedantic>

    $<$<CONFIG:Release>:-Werror>
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
