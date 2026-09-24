Caching System

Caching System — учебный проект на C++20 для моделирования и сравнения различных алгоритмов вытеснения в многоуровневой системе кэширования.

Проект поддерживает:

несколько алгоритмов вытеснения;
произвольное количество уровней кэша;
Inclusive и Exclusive политики;
выбор политики при сборке;
автоматическое создание различных конфигураций уровней;
собственные тесты и benchmark-сценарии;
подсчёт попаданий по каждому уровню.
Реализованные алгоритмы

В проекте реализованы:

Алгоритм	Основная идея
LRU	вытесняется элемент, который дольше всего не использовался
LFU	вытесняется элемент с наименьшей частотой обращений
2Q	разделяет новые и часто используемые элементы
ARC	адаптивно балансирует recent и frequent данные
LIRS	использует информацию о повторном использовании элементов

Основные реализации находятся в отдельных файлах:

lru_cach.hpp
lfu_cach.hpp
2Q_cach.hpp
arc_cach.hpp
lirs_cach.hpp
Многоуровневый кэш

Главная часть проекта — класс:

Multi_Level_Cach<Key, Value, Mode, Store_Data>

Он объединяет несколько отдельных кэшей в иерархию:

Request
   |
   v
  L1
   |
   v
  L2
   |
   v
  L3
   |
   v
Big Data

Количество уровней задаётся конфигурацией.

Например:

L1: LRU   capacity = 32
L2: ARC   capacity = 128
L3: LIRS  capacity = 512

Каждый уровень может использовать свой алгоритм.

std::variant

Все алгоритмы имеют общий интерфейс и объединяются через std::variant.

Упрощённо:

using Cache_variant = std::variant<
    Lru_cach<Key, Value>,
    Lfu_cach<Key, Value>,
    Two_Q_Cach<Key, Value>,
    Arc_cach<Key, Value>,
    Lirs_cach<Key, Value>
>;

Это позволяет хранить разные реализации кэшей в одной структуре многоуровневой системы.

Вызов нужного метода выполняется через:

std::visit(...)

Благодаря этому Multi_Level_Cach не зависит от конкретного алгоритма отдельного уровня.

Inclusive и Exclusive режимы

Проект поддерживает две политики:

enum class Cach_Mode
{
    Inclusive,
    Exclusive
};

Режим выбирается на этапе сборки.

Inclusive

В Inclusive-режиме данные верхнего уровня также должны присутствовать на нижних уровнях:

L1 ⊆ L2 ⊆ L3

Если элемент вытесняется из нижнего уровня, он удаляется и из более высоких уровней.

Такой подход может хранить один элемент сразу на нескольких уровнях.

Exclusive

В Exclusive-режиме один resident-элемент хранится только на одном уровне.

При вытеснении из верхнего уровня элемент может быть передан ниже:

L1 -> L2 -> L3

Это позволяет эффективнее использовать суммарную ёмкость всех уровней.

Сборка проекта

Для сборки используется CMake.

Необходимы:

CMake
C++20 compatible compiler

Например:

GCC 11+
Clang
Inclusive-сборка

Из корня проекта:

cmake -S . -B build-inclusive -DCACHE_MODE=Inclusive
cmake --build build-inclusive

Запуск основной программы:

./build-inclusive/caching_system

Запуск benchmark:

./build-inclusive/benchmark_test
Exclusive-сборка
cmake -S . -B build-exclusive -DCACHE_MODE=Exclusive
cmake --build build-exclusive

Запуск:

./build-exclusive/caching_system

Benchmark:

./build-exclusive/benchmark_test
Работа с двумя режимами

Удобно использовать две независимые директории:

build-inclusive/
build-exclusive/

Таким образом можно одновременно иметь собранные Inclusive и Exclusive версии проекта.

После изменения исходного кода достаточно выполнить:

cmake --build build-inclusive

или:

cmake --build build-exclusive

Повторно выполнять полную конфигурацию CMake обычно не требуется.

Конфигурация уровней

Каждый уровень описывается структурой конфигурации.

Например:

struct Cache_name_size
{
    std::string name_cach;
    std::size_t capacity;
    std::size_t hir_capacity;
};

Можно создать, например:

std::vector<Cache_name_size> configuration = {
    {"LRU", 32, 0},
    {"ARC", 128, 0},
    {"LIRS", 512, 64}
};

Поле hir_capacity используется алгоритмами, которым требуется отдельный размер HIR-части, прежде всего LIRS.

Benchmark

Проект содержит отдельную программу для сравнения различных конфигураций кэша.

tests/benchmark_test.cpp

Benchmark может:

генерировать последовательность запросов;
создавать разные комбинации алгоритмов;
запускать их на одинаковой последовательности запросов;
считать попадания по каждому уровню;
считать общее количество попаданий и промахов.

Например, для двух уровней можно сравнить:

LRU + LRU
LRU + LFU
LRU + ARC
...
LIRS + ARC
LIRS + LIRS

Если доступно пять алгоритмов и используется N уровней, число возможных комбинаций:

5^N

Для двух уровней:

25 конфигураций

Для трёх:

125 конфигураций

Все конфигурации должны получать одну и ту же последовательность запросов, чтобы результаты можно было корректно сравнивать.

Генерация запросов

Для случайных тестов может использоваться:

std::mt19937 generator(42);

Фиксированный seed позволяет при каждом запуске получать одну и ту же последовательность запросов.

Это удобно при сравнении разных алгоритмов.

Также можно использовать случайный seed, если нужна новая последовательность при каждом запуске.

Собственные тесты

Можно создавать собственные тестовые сценарии.

Например:

tests/my_test.cpp

Внутри можно самостоятельно задать данные:

std::unordered_map<int, int> data = {
    {1, 100},
    {2, 200},
    {3, 300},
    {4, 400}
};

Конфигурацию:

std::vector<Cache_name_size> configuration = {
    {"LRU", 2, 0},
    {"ARC", 4, 0}
};

И последовательность запросов:

std::vector<int> requests = {
    1, 2, 3, 1, 4, 2, 1, 3
};

После этого ключи можно передавать в:

cache.access(key);

Такой подход удобен для проверки конкретных случаев работы алгоритмов.

Добавление своего теста в CMake

Для нового теста можно добавить executable:

add_executable(
    my_test
    tests/my_test.cpp
    creat_cach.cpp
)

После этого:

cmake --build build-inclusive

Запуск:

./build-inclusive/my_test

Для Exclusive:

cmake --build build-exclusive
./build-exclusive/my_test

Тест будет использовать тот режим кэширования, с которым была создана соответствующая build-директория.

Что можно менять в тестах

Можно самостоятельно менять:

количество уровней;
алгоритм каждого уровня;
capacity;
HIR capacity для LIRS;
размер основного хранилища;
количество запросов;
последовательность запросов;
распределение случайных запросов;
Inclusive / Exclusive режим;
набор тестируемых конфигураций.

Например, можно проверить повторяющиеся обращения:

std::vector<int> requests = {
    1, 2, 3,
    1, 2, 3,
    1, 2, 3,
    4, 5, 6
};

Или почти полностью последовательный поток:

std::vector<int> requests = {
    1, 2, 3, 4, 5,
    6, 7, 8, 9, 10
};
Используемые возможности C++

Проект использует:

templates
std::variant
std::visit
std::optional
std::unordered_map
std::list
std::vector
if constexpr
move semantics
RAII

Стандарт языка:

C++20
Быстрый старт

Inclusive:

git clone <repository-url>
cd Caching_System

cmake -S . -B build-inclusive -DCACHE_MODE=Inclusive
cmake --build build-inclusive

./build-inclusive/caching_system

Benchmark:

./build-inclusive/benchmark_test

Exclusive:

cmake -S . -B build-exclusive -DCACHE_MODE=Exclusive
cmake --build build-exclusive

./build-exclusive/caching_system

Benchmark:

./build-exclusive/benchmark_test
Цель проекта

Проект предназначен для изучения и сравнения алгоритмов кэширования в многоуровневых системах.

Он позволяет экспериментировать с:

LRU, LFU, 2Q, ARC и LIRS;
Inclusive и Exclusive политиками;
различными размерами уровней;
различными комбинациями алгоритмов;
собственными последовательностями запросов;
сравнением количества попаданий и промахов.