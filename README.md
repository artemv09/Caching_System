# Caching System

`Caching System` — учебный проект на C++20 для моделирования и сравнения различных алгоритмов вытеснения в многоуровневой системе кэширования.

Проект поддерживает:

* несколько алгоритмов вытеснения;
* произвольное количество уровней кэша;
* Inclusive и Exclusive политики;
* выбор политики при сборке;
* автоматическое создание различных конфигураций уровней;
* собственные тесты и benchmark-сценарии;
* подсчёт попаданий по каждому уровню.

---

## Реализованные алгоритмы

В проекте реализованы:

| Алгоритм | Основная идея                                              |
| -------- | ---------------------------------------------------------- |
| LRU      | Вытесняется элемент, который дольше всего не использовался |
| LFU      | Вытесняется элемент с наименьшей частотой обращений        |
| 2Q       | Разделяет новые и часто используемые элементы              |
| ARC      | Адаптивно балансирует recent и frequent данные             |
| LIRS     | Использует информацию о повторном использовании элементов  |

Основные реализации находятся в отдельных файлах:

```text
lru_cach.hpp
lfu_cach.hpp
2Q_cach.hpp
arc_cach.hpp
lirs_cach.hpp
```

---

## Многоуровневый кэш

Главная часть проекта — класс:

```cpp
Multi_Level_Cach<Key, Value, Mode, Store_Data>
```

Он объединяет несколько отдельных кэшей в иерархию:

```text
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
```

Количество уровней задаётся конфигурацией.

Например:

```text
L1: LRU   capacity = 32
L2: ARC   capacity = 128
L3: LIRS  capacity = 512
```

Каждый уровень может использовать свой алгоритм.

---

## std::variant

Все алгоритмы имеют общий интерфейс и объединяются через `std::variant`.

Упрощённо:

```cpp
using Cache_variant = std::variant<
    Lru_cach<Key, Value>,
    Lfu_cach<Key, Value>,
    Two_Q_Cach<Key, Value>,
    Arc_cach<Key, Value>,
    Lirs_cach<Key, Value>
>;
```

Это позволяет хранить разные реализации кэшей в одной структуре многоуровневой системы.

Вызов метода конкретного алгоритма выполняется через:

```cpp
std::visit(...)
```

Благодаря этому `Multi_Level_Cach` не зависит от конкретного алгоритма отдельного уровня.

---

## Inclusive и Exclusive режимы

Проект поддерживает две политики:

```cpp
enum class Cach_Mode
{
    Inclusive,
    Exclusive
};
```

Режим выбирается на этапе сборки.

### Inclusive

В Inclusive-режиме данные верхнего уровня также присутствуют на нижних уровнях:

```text
L1 ⊆ L2 ⊆ L3
```

Если элемент вытесняется из нижнего уровня, он удаляется и из более высоких уровней.

### Exclusive

В Exclusive-режиме один resident-элемент хранится только на одном уровне.

При вытеснении из верхнего уровня элемент может быть передан ниже:

```text
L1 -> L2 -> L3
```

Это позволяет эффективнее использовать суммарную ёмкость всех уровней.

---

## Сборка проекта

Для сборки используется CMake.

Необходимы:

* CMake;
* компилятор с поддержкой C++20.

Например:

* GCC;
* Clang.

---

### Inclusive-сборка

Из корня проекта:

```bash
cmake -S . -B build-inclusive -DCACHE_MODE=Inclusive
cmake --build build-inclusive
```

Запуск основной программы:

```bash
./build-inclusive/caching_system
```

Запуск benchmark:

```bash
./build-inclusive/benchmark_test
```

---

### Exclusive-сборка

```bash
cmake -S . -B build-exclusive -DCACHE_MODE=Exclusive
cmake --build build-exclusive
```

Запуск основной программы:

```bash
./build-exclusive/caching_system
```

Запуск benchmark:

```bash
./build-exclusive/benchmark_test
```

---

## Работа с двумя режимами

Удобно использовать две независимые директории сборки:

```text
build-inclusive/
build-exclusive/
```

Таким образом можно одновременно иметь собранные Inclusive и Exclusive версии проекта.

После изменения исходного кода достаточно выполнить:

```bash
cmake --build build-inclusive
```

или:

```bash
cmake --build build-exclusive
```

Повторно выполнять конфигурацию CMake после каждого изменения исходных файлов обычно не требуется.

---


## Benchmark

Проект содержит отдельную программу для сравнения различных конфигураций кэша:

```text
tests/benchmark_test.cpp
```

Benchmark позволяет:

* генерировать последовательность запросов;
* создавать различные комбинации алгоритмов;
* запускать их на одинаковой последовательности запросов;
* считать попадания по каждому уровню;
* считать общее количество попаданий и промахов.

Например, для двух уровней можно сравнить:

```text
LRU + LRU
LRU + LFU
LRU + ARC
...
LIRS + ARC
LIRS + LIRS
```

Если доступно пять алгоритмов и используется `N` уровней, количество возможных комбинаций равно:

```text
5^N
```

Например:

```text
2 уровня -> 25 конфигураций
3 уровня -> 125 конфигураций
```

Все конфигурации получают одну и ту же последовательность запросов, что позволяет корректно сравнивать результаты.

---

## Генерация запросов

Для случайных тестов может использоваться генератор:

```cpp
std::mt19937 generator(42);
```

Фиксированный seed позволяет при каждом запуске получать одинаковую последовательность запросов.

Это удобно при сравнении разных алгоритмов, поскольку каждый из них получает одинаковую нагрузку.

---

## Добавление своего теста в CMake

Для нового теста можно добавить executable в `CMakeLists.txt`:

```cmake
add_executable(
    my_test
    tests/my_test.cpp
    creat_cach.cpp
)
```

После этого собрать проект:

```bash
cmake --build build-inclusive
```

И запустить тест:

```bash
./build-inclusive/my_test
```

Для Exclusive-сборки:

```bash
cmake --build build-exclusive
./build-exclusive/my_test
```

Тест будет использовать тот режим кэширования, с которым была создана соответствующая build-директория.

---

## Используемые возможности C++

Проект использует:

* templates;
* `std::variant`;
* `std::visit`;
* `std::optional`;
* `std::unordered_map`;
* `std::list`;
* `std::vector`;
* `if constexpr`;
* move semantics;
* RAII.

Стандарт языка:

```text
C++20
```

---

## Быстрый старт

Клонирование проекта:

```bash
git clone <repository-url>
cd Caching_System
```

### Inclusive

```bash
cmake -S . -B build-inclusive -DCACHE_MODE=Inclusive
cmake --build build-inclusive
./build-inclusive/caching_system
```

Benchmark:

```bash
./build-inclusive/benchmark_test
```

### Exclusive

```bash
cmake -S . -B build-exclusive -DCACHE_MODE=Exclusive
cmake --build build-exclusive
./build-exclusive/caching_system
```

Benchmark:

```bash
./build-exclusive/benchmark_test
```

---

## Цель проекта

Проект предназначен для изучения и сравнения алгоритмов кэширования в многоуровневых системах.

Он позволяет экспериментировать с:

* LRU, LFU, 2Q, ARC и LIRS;
* Inclusive и Exclusive политиками;
* различными размерами уровней;
* различными комбинациями алгоритмов;
* собственными последовательностями запросов;
* сравнением количества попаданий и промахов.
