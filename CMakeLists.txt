cmake_minimum_required(VERSION 4.1)
project(NuLab LANGUAGES C VERSION 0.0.0)

option(NULIB_ENABLE_TESTING "Enable testing" YES)
option(NULIB_ENABLE_COVERAGE "Enable coverage" NO)

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

if(NULIB_ENABLE_TESTING)
    include(CTest)
endif()

add_subdirectory(libs/nulib)

add_subdirectory(apps/bclib)
add_subdirectory(apps/bcapp)

if(NULIB_ENABLE_TESTING)
    add_subdirectory(tests)
endif()

