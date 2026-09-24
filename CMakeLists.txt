cmake_minimum_required(VERSION 3.16)

project(Caching_System LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)


# ==========================================
# Выбор режима Multi-Level Cache
# ==========================================

set(
    CACHE_MODE
    "Inclusive"
    CACHE STRING
    "Cache mode: Inclusive or Exclusive"
)

set_property(
    CACHE CACHE_MODE
    PROPERTY STRINGS Inclusive Exclusive
)


# ==========================================
# Основная программа
# ==========================================

add_executable(
    caching_system

    main.cpp
    creat_cach.cpp
)

target_include_directories(
    caching_system
    PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
)


# ==========================================
# Benchmark
# ==========================================

add_executable(
    benchmark_test

    tests/benchmark_test.cpp
    creat_cach.cpp
)

target_include_directories(
    benchmark_test
    PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
)


# ==========================================
# Выбор Inclusive / Exclusive
# ==========================================

if(CACHE_MODE STREQUAL "Inclusive")

    target_compile_definitions(
        caching_system
        PRIVATE
        CACHE_MODE_INCLUSIVE
    )

    target_compile_definitions(
        benchmark_test
        PRIVATE
        CACHE_MODE_INCLUSIVE
    )

elseif(CACHE_MODE STREQUAL "Exclusive")

    target_compile_definitions(
        caching_system
        PRIVATE
        CACHE_MODE_EXCLUSIVE
    )

    target_compile_definitions(
        benchmark_test
        PRIVATE
        CACHE_MODE_EXCLUSIVE
    )

else()

    message(
        FATAL_ERROR
        "CACHE_MODE must be Inclusive or Exclusive"
    )

endif()


# ==========================================
# Предупреждения компилятора
# ==========================================

target_compile_options(
    caching_system
    PRIVATE
    -Wall
    -Wextra
    -Wpedantic
)

target_compile_options(
    benchmark_test
    PRIVATE
    -Wall
    -Wextra
    -Wpedantic
)