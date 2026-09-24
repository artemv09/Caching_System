#include <optional>
#include <iostream>
#include <algorithm>
#include <cassert>
#include <cstddef>
#include <iterator>
#include <list>
#include <stdexcept>
#include <unordered_map>
#include <random>
#include <vector>
#include <string>

#include "multi_level_cache.hpp"

using Key = int;
using Value = int;
using Cach_Configs = std::vector<std::vector<Cache_name_size>>;

struct Level_size
{
    std::size_t capacity;
    std::size_t hir_capacity;
};

void parsing_parameters(
    std::vector<std::string>& variant_names_level, 
    std::vector<Level_size>& cach_level_size, 
    std::size_t count_levels, 
    std::size_t unique_cach_types);

Cach_Configs creat_all_configurations(std::vector<std::string>& variant_names_level, std::vector<Level_size>& cach_level_size);

std::unordered_map<Key, Value> create_big_data();

std::vector<Key> generate_requests(std::size_t count_requests);

void print_result(
    std::size_t test_number,
    const std::vector<Cache_name_size>& configuration,
    const std::vector<std::size_t>& hits_level,
    std::size_t total_hits,
    std::size_t count_requests
);

int main()
{
    std::unordered_map<Key, Value> data = create_big_data();

    std::size_t count_levels;
    std::cout << "Колличество уровней кэша levels = ";
    std::cin >> count_levels;

    std::size_t unique_cach_types;
    std::cout << "Колличество уникальный типов кэша unique_cach_types = ";
    std::cin >> unique_cach_types;

    std::size_t number_requests;
    std::cout << "колличество запросов number_requests = ";
    std::cin >> number_requests;

    std::vector<std::string> variant_names_level;
    std::vector<Level_size> cach_level_size;

    parsing_parameters(variant_names_level, cach_level_size, count_levels, unique_cach_types);

    Cach_Configs configurations = creat_all_configurations(variant_names_level, cach_level_size);


    for(const auto& configuration : configurations)
    {
        Multi_Level_Cach<Key, Value, BUILD_CACHE_MODE> cache(configuration, data);

        std::size_t total_hits = 0;
        std::size_t test_number = 1;

        for(const Key& key : generate_requests(number_requests))
        {
            auto result = cache.access(key);

            if(result.hit)
            {
                total_hits++;
            }
        }

        print_result(test_number, configuration, cache.hits_level, total_hits, number_requests);

        test_number++;
    }
    return 0;
}

void parsing_parameters(
    std::vector<std::string>& variant_names_level, 
    std::vector<Level_size>& cach_level_size, 
    std::size_t count_levels, 
    std::size_t unique_cach_types)
{
    std::cout << "введите в любом порядке названия кэшей, которые буду использоваться: ";
    for(std::size_t i = 0; i < unique_cach_types; i++)
    {
        std::string type_cach;
        std::cin >> type_cach;

        variant_names_level.push_back(type_cach);
    }

    if(std::find(variant_names_level.begin(), variant_names_level.end(), "LIRS") != variant_names_level.end())
    {
        for(std::size_t i = 0; i < count_levels; i++)
        {
            std::size_t capacity;
            std::size_t hir_capacity;

            std::cout << "\nL" << i + 1 << ": capacity = ";
            std::cin >> capacity;
            std::cout <<" hir_capacity = ";
            std::cin >> hir_capacity;

            cach_level_size.push_back({capacity, hir_capacity});
        }
    }
    else
    {
        for(std::size_t i = 0; i < count_levels; i++)
        {
            std::size_t capacity;

            std::cout << "\nL" << i + 1 << ": capacity = ";
            std::cin >> capacity;

            cach_level_size.push_back({capacity, 0});
        }
    }
}

Cach_Configs creat_all_configurations(std::vector<std::string>& variant_names_level, std::vector<Level_size>& cach_level_size)
{
    Cach_Configs general_name_size;

    std::size_t count_levels = cach_level_size.size();
    std::size_t unique_cach_types = variant_names_level.size();

    std::size_t number_unique_configurations = 1;

    for(std::size_t i = 0; i < count_levels; i++)
    {
        number_unique_configurations *= unique_cach_types;
    }

    general_name_size.reserve(number_unique_configurations);

    for(std::size_t i = 0; i < number_unique_configurations; i++)
    {
        std::vector<Cache_name_size> current_configuration;
        current_configuration.reserve(count_levels);

        std::size_t number = i;

        for(std::size_t level = 0; level < count_levels; ++level)
        {
            std::size_t type_cach = number % unique_cach_types;

            current_configuration.push_back(
                Cache_name_size{
                    variant_names_level.at(type_cach),
                    cach_level_size.at(level).capacity,
                    cach_level_size.at(level).hir_capacity
                }
            );

            number /= unique_cach_types;
        }

        general_name_size.push_back(std::move(current_configuration));
    }

    return general_name_size;

}


std::unordered_map<Key, Value> create_big_data()
{
    constexpr int DATA_SIZE = 1000000;

    std::unordered_map<Key, Value> data;

    data.reserve(DATA_SIZE);

    for(int key = 1; key <= DATA_SIZE; ++key)
    {
        data.emplace(key, key);
    }

    return data;
}

std::vector<Key> generate_requests(std::size_t count_requests)
{
    std::vector<Key> requests;

    requests.reserve(count_requests);

    std::mt19937 generator(42); // после начала использования изменить

    std::uniform_int_distribution<Key> distribution(1, 1000);

    for(std::size_t i = 0; i < count_requests; ++i)
    {
        requests.push_back(distribution(generator));
    }

    return requests;
}

void print_result(
    std::size_t test_number,
    const std::vector<Cache_name_size>& configuration,
    const std::vector<std::size_t>& hits_level,
    std::size_t total_hits,
    std::size_t count_requests
)
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

    std::cout << "Hit rate: " << 100.0 * total_hits / count_requests << "%\n";

    std::cout << "====================================\n";
}

