#include <algorithm>
#include <cstddef>
#include <iostream>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>
#include <unistd.h>

#include "creat_cach.hpp"
#include "multi_level_cache.hpp"
#include "opt_cach.hpp"
#include "request_patterns.hpp"

using Key = int;
using Value = int;
using Cach_Configs = std::vector<std::vector<Cache_name_size>>;

struct Level_size
{
    std::size_t capacity;
    std::size_t hir_capacity;
};

// Читает названия, затем размеры уровней; HIR нужен на каждом уровне, если выбран LIRS.
void parsing_parameters(
    std::vector<std::string>& variant_names_level,
    std::vector<Level_size>& cach_level_size,
    std::size_t count_levels,
    std::size_t unique_cach_types)
{
    std::cout << "введите в любом порядке названия кэшей, которые буду использоваться: ";

    for(std::size_t i = 0; i < unique_cach_types; ++i)
    {
        std::string type_cach;

        if(!(std::cin >> type_cach))
        {
            throw std::invalid_argument("Не хватает названий алгоритмов");
        }

        cache_type(type_cach);

        if(std::find(variant_names_level.begin(), variant_names_level.end(), type_cach) !=
           variant_names_level.end())
        {
            throw std::invalid_argument("Названия алгоритмов не должны повторяться");
        }

        variant_names_level.push_back(type_cach);
    }

    const bool has_lirs = std::find(variant_names_level.begin(), variant_names_level.end(), "LIRS") !=
                          variant_names_level.end();
    for(std::size_t i = 0; i < count_levels; ++i)
    {
        std::cout << "\nL" << i + 1 << ": capacity = ";
        const auto capacity = read_size(std::cin, "capacity", MAX_PAGE_KEY);
        std::size_t hir_capacity = 0;
        if(has_lirs)
        {
            std::cout << " hir_capacity = ";
            hir_capacity = read_size(std::cin, "hir_capacity", MAX_PAGE_KEY);
            // Правила HIR проверяет сам LIRS, включая особые случаи capacity 0 и 1.
            Lirs_cach<Key, Value> validate(capacity, hir_capacity);
        }
        cach_level_size.push_back({capacity, hir_capacity});
    }
}

// Перебирает K^L размещений; младшая цифра номера задаёт L1, как в старом benchmark.
Cach_Configs creat_all_configurations(
    const std::vector<std::string>& variant_names_level,
    const std::vector<Level_size>& cach_level_size)
{
    constexpr std::size_t MAX_CONFIGURATIONS = 10000;
    std::size_t number_unique_configurations = 1;
    for(std::size_t i = 0; i < cach_level_size.size(); ++i)
    {
        if(number_unique_configurations > MAX_CONFIGURATIONS / variant_names_level.size())
        {
            throw std::invalid_argument("Слишком много конфигураций: допустимо не более 10000");
        }
        number_unique_configurations *= variant_names_level.size();
    }

    Cach_Configs configurations;
    configurations.reserve(number_unique_configurations);
    for(std::size_t i = 0; i < number_unique_configurations; ++i)
    {
        std::vector<Cache_name_size> configuration;
        configuration.reserve(cach_level_size.size());
        std::size_t number = i;
        for(const auto& size : cach_level_size)
        {
            const auto& name = variant_names_level[number % variant_names_level.size()];
            configuration.push_back({name, size.capacity, name == "LIRS" ? size.hir_capacity : 0});
            number /= variant_names_level.size();
        }
        configurations.push_back(std::move(configuration));
    }
    return configurations;
}

// Создаёт прежние данные benchmark: для ключей 1..1000000 значение равно ключу.
std::unordered_map<Key, Value> create_big_data()
{
    std::unordered_map<Key, Value> data;
    data.reserve(MAX_PAGE_KEY);
    for(int key = 1; key <= MAX_PAGE_KEY; ++key)
    {
        data.emplace(key, key);
    }
    return data;
}

// Печатает прежний блок статистики; пустой поток имеет 0% попаданий.
void print_result(
    std::size_t test_number,
    const std::vector<Cache_name_size>& configuration,
    const std::vector<std::size_t>& hits_level,
    std::size_t total_hits,
    std::size_t count_requests)
{
    std::cout << "\n========== Configuration " << test_number << " ==========\n";
    for(std::size_t level = 0; level < configuration.size(); ++level)
    {
        std::cout << "L" << level + 1 << ": " << configuration[level].name_cach
                  << " capacity = " << configuration[level].capacity << "\n\n";
    }
    for(std::size_t level = 0; level < hits_level.size(); ++level)
    {
        std::cout << "Hits L" << level + 1 << ": " << hits_level[level] << "\n";
    }

    std::cout << "Total hits: " << total_hits << "\n";
    std::cout << "Total misses: " << count_requests - total_hits << "\n";

    const double hit_rate = count_requests == 0 ? 0.0 : 100.0 * total_hits / count_requests;
    std::cout << "Hit rate: " << hit_rate << "%\n";
    std::cout << "======================================\n";
}

// Прогоняет готовые запросы в одном режиме: каждый кеш пустой, OPT общий для паттерна.
template<Cach_Mode Mode>
void run_patterns(
    const std::vector<request_patterns::Pattern>& patterns,
    const Cach_Configs& configurations,
    const std::unordered_map<Key, Value>& data)
{
    Multi_Level_Cach<Key, Value, Mode> reference(configurations.front(), data);
    const auto opt_capacity = reference.reference_capacity();

    for(const auto& pattern : patterns)
    {
        std::cout << "\nPattern: " << pattern.name;
        std::size_t test_number = 1;

        for(const auto& configuration : configurations)
        {
            Multi_Level_Cach<Key, Value, Mode> cache(configuration, data);
            std::size_t total_hits = 0;

            for(const Key& key : pattern.keys)
            {
                if(cache.access(key).hit)
                {
                    ++total_hits;
                }
            }

            print_result(test_number++, configuration, cache.hits_level, total_hits,
                         pattern.keys.size());
        }

        OptCache<Key, Value> opt_cach(opt_capacity, pattern.keys, data);

        for(const Key& key : pattern.keys)
        {
            opt_cach.access(key);
        }
        opt_cach.print_statistics(std::cout);
    }
}

int main()
{
    try
    {
        std::cout << "Колличество уровней кэша levels = ";
        const auto count_levels = read_size(std::cin, "число уровней", 64);

        std::cout << "Колличество уникальный типов кэша unique_cach_types = ";
        const auto unique_cach_types = read_size(std::cin, "число типов", 5);

        std::cout << "колличество запросов number_requests = ";
        const auto number_requests = read_size(std::cin, "число запросов", MAX_REQUESTS);

        if(count_levels == 0 || unique_cach_types == 0)
        {
            throw std::invalid_argument("Нужен хотя бы один уровень и один тип кеша");
        }

        std::vector<std::string> variant_names_level;
        std::vector<Level_size> cach_level_size;

        parsing_parameters(variant_names_level, cach_level_size, count_levels, unique_cach_types);

        const auto configurations = creat_all_configurations(variant_names_level, cach_level_size);
        const auto data = create_big_data();
        // Проверяет вместимости по правилам выбранного режима до запуска паттернов.
        Multi_Level_Cach<Key, Value, BUILD_CACHE_MODE> validate(configurations.front(), data);

        const auto patterns = request_patterns::make_patterns(
            number_requests, cach_level_size.front().capacity, cach_level_size.back().capacity,
            validate.total_capacity());

        run_patterns<BUILD_CACHE_MODE>(patterns, configurations, data);
        return 0;
    }
    catch(const std::exception& error)
    {
        std::cerr << "Ошибка: " << error.what() << '\n';
        return 1;
    }
}