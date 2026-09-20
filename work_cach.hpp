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

template<
    typename Key,
    typename Value,
    Cach_Mode Mode,
    bool Store_Data
>
class Multi_Level_Cach
{
    private:
        using General_Cach = std::vector<Cach_ptr<Key, Value>>;
        using Big_Data = std::unordered_map<Key, Value>;

        General_Cach general_cach;
        const Big_Data* big_data;

        Public_Access_Result<Value> access_inclusive(const Key& key);

        Public_Access_Result<Value> access_exclusive(const Key& key);

        void push_down(std::size_t level, Entry<Key, Value> entry);

        void invalidate_above(std::size_t level, const Key& key);

        void redistribution_cach_ell(const Value& value, std::size, const Key& key);

        const Value get_long_data(const Key& key)//TODO незнабю насколько нормально то что я возвращаю ссылку
        {
            return big_data -> at(key);
        }

    public:

        Public_Access_Result<Value> acess(const Key& key)
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

template<
    typename Key,
    typename Value,
    Cach_Mode Mode,
    bool Store_Data
>
Multi_Level_Cach<Key, Value, Mode, Store_Data>::Multi_Level_Cach
    (const std::vector<Cache_name_size>& parameters, const Big_Data& data): 
    general_cach(create_cach<Key, Value>(parameters)), big_data(&data)
{
}

template<
    typename Key,
    typename Value,
    Cach_Mode Mode,
    bool Store_Data
>
Public_Access_Result<Value> Multi_Level_Cach<Key, Value, Mode, Store_Data>::access_inclusive(const Key& key)
{
    //сначал проверим а вообще есть ои эллемент в кэше
    std::size_t level_cach = 0;
    while(level_cach != general_cach.size())
    {
        Access_Result<Value> result_look_up = std::visit([&](auto& cache) -> Access_Result<Value>
                                {return cache.look_up(key);},
                                *general_cach.at(level_cach));
        if(result_look_up.hit)
        {
            redistribution_cach_ell(*(result_look_up.found_ell), level_cach, key);
            return {true, *(result_look_up.found_ell)};
        }
        
        level_cach++;
    }

    Value value = get_long_data(key);
    redistribution_cach_ell(value, level_cach + 1, key);  
    return {false, value};
}

template<
    typename Key,
    typename Value,
    Cach_Mode Mode,
    bool Store_Data
>
void Multi_Level_Cach<Key, Value, Mode, Store_Data>::redistribution_cach_ell(const Value& value, std::size_t level_cach, const Key& key)
{
    while(level_cach > 0)
    {
        --level_cach;

        std::optional<Key> key_erase = std::visit(
                [&](auto& cache) -> std::optional<Key>
                {return cache.insert_value(key, value);},
                *general_cach.at(level_cach)
            );

        if(key_erase != std::nullopt)
        {
            std::size_t count = level_cach;

            while(count > 0)
            {
                --count;

                std::visit(
                    [&](auto& cache) -> bool
                    {return cache.erase_key(*key_erase);},
                    *general_cach.at(count)
                );
            }
        }
    }
}


#endif