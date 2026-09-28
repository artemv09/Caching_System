cmake_minimum_required(VERSION 3.16)
project(Caching_System LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CACHE_MODE "Inclusive" CACHE STRING "Inclusive or Exclusive")
set_property(CACHE CACHE_MODE PROPERTY STRINGS Inclusive Exclusive)

if(NOT CACHE_MODE STREQUAL "Inclusive" AND NOT CACHE_MODE STREQUAL "Exclusive")
    message(FATAL_ERROR "CACHE_MODE must be Inclusive or Exclusive")

endif()
# Раздельная сборка приложений, benchmark и unit-тестов
option(BUILD_CACHE_APP "Build the interactive cache application" ON)
option(BUILD_BENCHMARK "Build generated-request benchmarks" ON)
option(CACHE_SANITIZERS "Enable ASan and UBSan" OFF)
option(CACHE_DEBUG_ITERATORS "Enable libstdc++ iterator checks" OFF)
# Эти флаги применяются также к GoogleTest: ABI всех связанных файлов одинаков
if(CACHE_DEBUG_ITERATORS)
    add_compile_definitions(_GLIBCXX_DEBUG)
endif()
if(CACHE_SANITIZERS)
    add_compile_options(-fsanitize=address,undefined -fno-omit-frame-pointer -fno-pie)
    add_link_options(-fsanitize=address,undefined -no-pie)
endif()
add_library(cache_core creat_cach.cpp)
target_include_directories(cache_core PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_compile_options(cache_core PUBLIC -Wall -Wextra -Wpedantic)
string(TOUPPER "${CACHE_MODE}" CACHE_MODE_UPPER)
target_compile_definitions(cache_core PUBLIC CACHE_MODE_${CACHE_MODE_UPPER})
if(BUILD_CACHE_APP)
    add_executable(caching_system main.cpp)
    target_link_libraries(caching_system PRIVATE cache_core)
endif()
if(BUILD_BENCHMARK)
    add_executable(benchmark_test tests/benchmark_test.cpp)
    target_link_libraries(benchmark_test PRIVATE cache_core)
endif()

include(CTest)

if(BUILD_TESTING)
    # Сначала установленная библиотека
    if(NOT CACHE_DEBUG_ITERATORS AND NOT CACHE_SANITIZERS)
        find_package(GTest 1.10 QUIET)
    endif()
    if(NOT TARGET GTest::gtest_main AND NOT TARGET GTest::Main)
        include(FetchContent)
        if(POLICY CMP0135)
            cmake_policy(SET CMP0135 NEW)
        endif()
        FetchContent_Declare(googletest
            URL https://codeload.github.com/google/googletest/zip/f8d7d77c06936315286eb55f8de22cd23c188571)
        set(INSTALL_GTEST OFF CACHE BOOL "" FORCE)
        set(BUILD_GMOCK OFF CACHE BOOL "" FORCE)
        FetchContent_MakeAvailable(googletest)
    endif()
    if(TARGET GTest::gtest_main)
        set(CACHE_GTEST_MAIN GTest::gtest_main)
    else()
        set(CACHE_GTEST_MAIN GTest::Main)
    endif()
    # Один исполняемый файл содержит все политики, обе иерархии и свои примеры
    add_executable(unit_tests
        tests/unit/lru_test.cpp
        tests/unit/lfu_test.cpp
        tests/unit/two_q_test.cpp
        tests/unit/arc_test.cpp
        tests/unit/lirs_test.cpp
        tests/unit/opt_test.cpp
        tests/unit/input_test.cpp
        tests/hierarchy/inclusive_test.cpp
        tests/hierarchy/exclusive_test.cpp
        tests/my_test.cpp)
    target_link_libraries(unit_tests PRIVATE cache_core ${CACHE_GTEST_MAIN})
    add_test(NAME unit_tests COMMAND unit_tests)
endif()
