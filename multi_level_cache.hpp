#ifndef WORK_CACH_
#define WORK_CACH_

#include <iostream>
#include <algorithm>
#include <cassert>
#include <cstddef>
#include <iterator>
#include <list>
#include <stdexcept>
#include <unordered_map>

#include "creat_cach.hpp"
#include "cach_type.hpp"
#include <limits>
#include <fstream>
#include "opt_cach.hpp"


enum class Cach_Mode
{
    Inclusive,
    Exclusive
};

#if defined(CACHE_MODE_INCLUSIVE)
inline constexpr Cach_Mode BUILD_CACHE_MODE = Cach_Mode::Inclusive;
#elif defined(CACHE_MODE_EXCLUSIVE)
inline constexpr Cach_Mode BUILD_CACHE_MODE = Cach_Mode::Exclusive;
#else
#error "Define CACHE_MODE_INCLUSIVE or CACHE_MODE_EXCLUSIVE"
#endif

template<typename Key, typename Value, Cach_Mode Mode>
class Multi_Level_Cach
{
    private:
        using General_Cach = std::vector<Cach_ptr<Key, Value>>;
        using Big_Data = std::unordered_map<Key, Value>;

        General_Cach general_cach;
        const Big_Data* big_data;

        Public_Access_Result<Value> access_inclusive(const Key& key);

        Public_Access_Result<Value> access_exclusive(const Key& key);

        void redistribution_inc_cach(const Value& value, std::size_t level_cach, const Key& key); // перестройка кэша по правилам инклюзивного

        void redistribution_ex_cach(Erase_ELL<Key, Value> erase_ell); // перестройка кэша по правилам эксклюзивного

        const Value get_long_data(const Key& key) // типа мы обращаемся к долгой памеяе
        {
            auto find_val = big_data -> find(key);
            if(find_val == big_data -> end())
            {
                throw std::runtime_error("попытка найти не существующий ключ");
            }
            return big_data -> at(key);
        }

    public:
        std::vector<std::size_t> hits_level;// здесь будет записывать колличество попаданий на каждом уровне

        Public_Access_Result<Value> access(const Key& key)
        {
            if constexpr (Mode == Cach_Mode::Inclusive)
            {
               return access_inclusive(key);
            }
            else
            {
               return access_exclusive(key);
            }
        }

        std::size_t total_capacity() const
        {
            std::size_t total = 0;

            for(const auto& level : general_cach)
            {
                const auto capacity = std::visit(
                    [](const auto& cache)
                    {
                        return cache.capacity();
                    },
                    *level
                );

                if(capacity > std::numeric_limits<std::size_t>::max() - total)
                {
                    throw std::overflow_error("Переполнение суммы вместимостей");
                }

                total += capacity;
            }

            return total;
        }

        // Возвращает копии состояний уровней, не вызывая look_up.
        std::vector<Cache_Snapshot<Key, Value>> snapshot() const
        {
            std::vector<Cache_Snapshot<Key, Value>> result;

            for(const auto& level : general_cach)
            {
                result.push_back(std::visit([](const auto& cache)
                {
                    return cache.snapshot();
                }, 
                *level));
            }
            return result;
        }

        // Плоский OPT сравниваем с тем же числом доступных уникальных страниц.
        std::size_t reference_capacity() const
        {
            if constexpr(Mode == Cach_Mode::Inclusive)
            {
                return std::visit([](const auto& cache) { return cache.capacity(); },
                                  *general_cach.back());
            }
            return total_capacity();
        }

        Multi_Level_Cach(const std::vector<Cache_name_size>& parameters, const Big_Data& data); 

        Multi_Level_Cach(const Multi_Level_Cach&) = delete;
        Multi_Level_Cach& operator=(const Multi_Level_Cach&) = delete;
};

template<typename Key, typename Value, Cach_Mode Mode>
Multi_Level_Cach<Key, Value, Mode>::Multi_Level_Cach
    (const std::vector<Cache_name_size>& parameters, const Big_Data& data): 
    general_cach(),
    big_data(&data),
    hits_level(parameters.size(), 0)
{
    if(parameters.empty())
    {
        throw std::invalid_argument("Нужен хотя бы один уровень");
    }
    for(const auto& parameter : parameters)
    {
        if constexpr(Mode == Cach_Mode::Inclusive)
        {
            if(parameter.capacity == 0)
            {
                throw std::invalid_argument("Inclusive требует положительных вместимостей");
            }
        }
    }
    general_cach = create_cach<Key, Value>(parameters);
}

template<typename Key, typename Value, Cach_Mode Mode>
Public_Access_Result<Value> Multi_Level_Cach<Key, Value, Mode>::access_inclusive(const Key& key)
{
    std::size_t level_cach = 0;
    std::size_t general_size = general_cach.size();

    while(level_cach < general_size)
    {
        Value* result_look_up = std::visit([&](auto& cache) ->  Value*
                                {return cache.look_up(key);},
                                *general_cach.at(level_cach));
        if(result_look_up != nullptr) // обработка попадания в кэш
        {
            hits_level[level_cach]++;

            redistribution_inc_cach(*result_look_up, level_cach, key);
            return {true, *result_look_up};
        }

        level_cach++;
    }

    Value value = get_long_data(key);
    redistribution_inc_cach(value, level_cach, key);
    return {false, value};
}

template<typename Key, typename Value, Cach_Mode Mode>
Public_Access_Result<Value> Multi_Level_Cach<Key, Value, Mode>::access_exclusive(const Key& key)
{
    std::size_t level_cach = 0;
    std::size_t general_size = general_cach.size();

    // первая проверка при попадании не должна вообще ничего удалять или вставлять
    Value* result_look_up = std::visit([&](auto& cache) ->  Value*
                                {return cache.look_up(key);},
                                *general_cach.at(level_cach));
    if(result_look_up != nullptr) // обработка попадания в кэш
    {
        hits_level[level_cach]++;
        return {true, *result_look_up};
    }

    level_cach++;

    while(level_cach < general_size)
    {
        Erase_ELL<Key, Value> result_find = std::visit([&](auto& cache) -> Erase_ELL<Key, Value>
                                {return cache.extract_entry(key);},
                                *general_cach.at(level_cach));
        if(result_find) // обработка попадания в кэш
        {
            hits_level[level_cach]++;

            redistribution_ex_cach(result_find);
            return {true, result_find -> value};
        }

        level_cach++;
    }

    Erase_ELL<Key, Value> result_find{Entry<Key, Value>{key, get_long_data(key)}};
    redistribution_ex_cach(result_find);
    return {false, result_find -> value};
}

template<typename Key, typename Value, Cach_Mode Mode>
void Multi_Level_Cach<Key, Value, Mode>::redistribution_inc_cach(const Value& value, std::size_t level_cach, const Key& key)
{
    while(level_cach > 0)
    {
        level_cach--;

        Erase_ELL<Key, Value> erased = std::visit(
                [&](auto& cache) -> Erase_ELL<Key, Value>
                {return cache.insert_value(key, value);},
                *general_cach.at(level_cach)
            );

        if(erased) // если нам пришлось удалить эллемент обрабатываем его удаления из всего кэша
        {
            std::size_t count_cach_level = level_cach;

            while(count_cach_level > 0)
            {
                count_cach_level--;

                std::visit(
                    [&](auto& cache) -> bool
                    {return cache.erase_key(erased -> key);},
                    *general_cach.at(count_cach_level)
                );
            }
        }
    }
}

template<typename Key, typename Value, Cach_Mode Mode>
void Multi_Level_Cach<Key, Value, Mode>::redistribution_ex_cach(Erase_ELL<Key, Value> erase_ell)
{
    std::size_t count_cach_level = 0;
    std::size_t max_level = general_cach.size();

    while(count_cach_level < max_level)
    {
        erase_ell = std::visit(
                [&](auto& cache) -> Erase_ELL<Key, Value>
                {return cache.insert_value(erase_ell -> key, erase_ell -> value);},
                *general_cach.at(count_cach_level)
            );

        if(!erase_ell)
        {
            break;
        }
        count_cach_level++;
    }
}

// Один ручной прогон: учебные данные, ответы, попадания уровней и плоский OPT.
template<typename Key, typename Value>
void general_fun(std::istream& input, std::ostream& output)
{
    const std::unordered_map<Key, Value> data
    {
        {1, 100}, {2, 200}, {3, 300}, {4, 400}, {5, 500},
        {6, 600}, {7, 700}, {8, 800}, {9, 900}, {10, 1000}
    };

    std::ifstream config("config.txt");

    if(!config)
    {
        throw std::runtime_error("Не открыт config.txt в текущем каталоге");
    }

    const auto parameters = parsing_cach_parametr(config, input);

    Multi_Level_Cach<Key, Value, BUILD_CACHE_MODE> cach(parameters, data);

    const auto list_key_requests = read_requests<Key>(input);
    std::size_t count_hit = 0;

    for(const Key& key : list_key_requests)
    {
        const auto result = cach.access(key);
        count_hit += result.hit;
        output << "Key " << key << " == " << result.sought_element << "\n";
    }

    output << "\n========== Cache hit statistics ==========\n\n";
    output << "Колличество попаданий моего кэша == " << count_hit << "\n";
    for(std::size_t i = 0; i < cach.hits_level.size(); ++i)
    {
        output << "Колличество попаданий в L" << i + 1 << " == " << cach.hits_level[i] << "\n";
    }
    output << "\n==========================================\n\n";

    OptCache<Key, Value> opt_cach(cach.reference_capacity(), list_key_requests, data);
    for(const Key& key : list_key_requests)
    {
        opt_cach.access(key);
    }
    opt_cach.print_statistics(output);
}

#endif
