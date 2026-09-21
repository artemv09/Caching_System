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
#include "crutch.hpp"


enum class Cach_Mode
{
    Inclusive,
    Exclusive
};

#if defined(CACHE_MODE_INCLUSIVE)

inline constexpr Cach_Mode BUILD_CACHE_MODE =
    Cach_Mode::Inclusive;

#elif defined(CACHE_MODE_EXCLUSIVE)

inline constexpr Cach_Mode BUILD_CACHE_MODE =
    Cach_Mode::Exclusive;

#else


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

        // void push_down(std::size_t level, Entry<Key, Value> entry);

        // void invalidate_above(std::size_t level, const Key& key);

        void redistribution_inc_cach(const Value& value, std::size_t level_cach, const Key& key);

        void redistribution_ex_cach(const Value& value, const Key& key);

        const Value get_long_data(const Key& key)//TODO незнабю насколько нормально то что я возвращаю ссылку
        {
            auto find_val = big_data -> find(key);
            if(find_val == big_data -> end())
            {
                throw std::runtime_error("попытка найти не существующий ключ");
            }
            return big_data -> at(key);
        }

    public:

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

        Multi_Level_Cach(const std::vector<Cache_name_size>& parameters, const Big_Data& data); 

        Multi_Level_Cach(const Multi_Level_Cach&) = delete;
        Multi_Level_Cach& operator=(const Multi_Level_Cach&) = delete;
};

template<typename Key, typename Value, Cach_Mode Mode>
Multi_Level_Cach<Key, Value, Mode>::Multi_Level_Cach
    (const std::vector<Cache_name_size>& parameters, const Big_Data& data): 
    general_cach(create_cach<Key, Value>(parameters)), big_data(&data)
{
}

template<typename Key, typename Value, Cach_Mode Mode>
Public_Access_Result<Value> Multi_Level_Cach<Key, Value, Mode>::access_inclusive(const Key& key)
{
    std::size_t level_cach = 0;

    while(level_cach < general_cach.size())
    {
        Access_Result<Value> result_look_up = std::visit([&](auto& cache) -> Access_Result<Value>
                                {return cache.look_up(key);},
                                *general_cach.at(level_cach));
        if(result_look_up.hit)
        {
            redistribution_inc_cach(*(result_look_up.found_ell), level_cach, key);
            return {true, *(result_look_up.found_ell)};
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

    while(level_cach < general_cach.size())
    {
        Erase_ELL<Key, Value> result_find = std::visit([&](auto& cache) -> Erase_ELL<Key, Value>
                                {return cache.find_del(key);},
                                *general_cach.at(level_cach));
        if(result_find.key_erase)
        {
            redistribution_ex_cach(*(result_find.value_erase), key);
            return {true, *(result_find.value_erase)};
        }

        level_cach++;
    }

    Value value = get_long_data(key);
    redistribution_ex_cach(value, key);
    return {false, value};
}

template<typename Key, typename Value, Cach_Mode Mode>
void Multi_Level_Cach<Key, Value, Mode>::redistribution_inc_cach(const Value& value, std::size_t level_cach, const Key& key)
{
    while(level_cach > 0)
    {
        --level_cach;

        Erase_ELL<Key, Value> erased = std::visit(
                [&](auto& cache) -> Erase_ELL<Key, Value>
                {return cache.insert_value(key, value);},
                *general_cach.at(level_cach)
            );

        assert(erased.key_erase.has_value() == erased.value_erase.has_value());

        if(erased.key_erase)
        {
            std::size_t count = level_cach;

            while(count > 0)
            {
                --count;

                std::visit(
                    [&](auto& cache) -> bool
                    {return cache.erase_key(*erased.key_erase);},
                    *general_cach.at(count)
                );
            }
        }
    }
}

template<typename Key, typename Value, Cach_Mode Mode>
void Multi_Level_Cach<Key, Value, Mode>::redistribution_ex_cach(const Value& value, const Key& key)
{
    std::size_t count_cach_level = 0;
    std::size_t max_level = general_cach.size();

    Erase_ELL<Key, Value> erase_ell{key, value};;

    while(count_cach_level < max_level)
    {
        erase_ell = std::visit(
                [&](auto& cache) -> Erase_ELL<Key, Value>
                {return cache.insert_value(*erase_ell.key_erase, *erase_ell.value_erase);},
                *general_cach.at(count_cach_level)
            );

        if(!erase_ell.key_erase)
        {
            break;
        }
        count_cach_level++;
    }
}

template<typename Key, typename Value>
void general_fun(std::istream& input, std::ostream& output)
{
    std::unordered_map<int, int> data
    {
        {1, 100},
        {2, 200},
        {3, 300},
        {4, 400},
        {5, 500},
        {6, 600},
        {7, 700},
        {8, 800},
        {9, 900},
        {10, 1000}
    };

    FILE* config = std::fopen("config.txt", "r");

    std::vector<Cache_name_size> cach_name_size_v = parsing_cach_parametr(config, std::cin);

    Multi_Level_Cach<int, int, BUILD_CACHE_MODE> cach(cach_name_size_v ,data);

    std::fclose(config);

    std::size_t count_key = 0;
    input >> count_key;

    std::size_t count = 0;
    std::size_t count_hit = 0;
    
    while(count < count_key)
    {
        Key key = 0;
        input >> key;

        Public_Access_Result rezult = cach.access(key);
        Value value = rezult.sought_element;

        if(rezult.hit)
        {
            output << "============= " << count + 1 << "\n";
            count_hit++;
        }
        output << "Key " << key << " == " << value << "\n";
        count++;
    }
    output << "Колличество попаданий == " << count_hit << "\n";
}

#endif