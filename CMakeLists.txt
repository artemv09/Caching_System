cmake_minimum_required(VERSION 3.16)

project(Caching_System LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)


# ============================================================
# Выбор политики multi-level cache
# ============================================================

set(
    CACHE_MODE
    "Inclusive"
    CACHE STRING
    "Cache hierarchy mode: Inclusive or Exclusive"
)

set_property(
    CACHE CACHE_MODE
    PROPERTY STRINGS
    Inclusive
    Exclusive
)


# ============================================================
# Основная программа
# ============================================================

add_executable(
    caching_system

    main.cpp
    creat_cach.cpp
)

target_include_directories(
    caching_system PRIVATE

    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_compile_options(
    caching_system PRIVATE

    -Wall
    -Wextra
    -Wpedantic
)


# ============================================================
# Тесты
# ============================================================

add_executable(
    multi_level_test

    tests/multi_level_test.cpp
    creat_cach.cpp
)

target_include_directories(
    multi_level_test PRIVATE

    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_compile_options(
    multi_level_test PRIVATE

    -Wall
    -Wextra
    -Wpedantic

    -fsanitize=address,undefined
    -fno-omit-frame-pointer
)

target_link_options(
    multi_level_test PRIVATE

    -fsanitize=address,undefined
)


# ============================================================
# Передаём выбранную политику программе
# ============================================================

if(CACHE_MODE STREQUAL "Inclusive")

    target_compile_definitions(
        caching_system PRIVATE
        CACHE_MODE_INCLUSIVE
    )

    target_compile_definitions(
        multi_level_test PRIVATE
        CACHE_MODE_INCLUSIVE
    )

    message(STATUS "Cache mode: Inclusive")


elseif(CACHE_MODE STREQUAL "Exclusive")

    target_compile_definitions(
        caching_system PRIVATE
        CACHE_MODE_EXCLUSIVE
    )

    target_compile_definitions(
        multi_level_test PRIVATE
        CACHE_MODE_EXCLUSIVE
    )

    message(STATUS "Cache mode: Exclusive")


else()

    message(
        FATAL_ERROR
        "Unknown CACHE_MODE='${CACHE_MODE}'. "
        "Use Inclusive or Exclusive."
    )

endif()