#ifndef WORK_CACH_
#define WORK_CACH

#include <iostream>
#include <algorithm>
#include <cassert>
#include <cstddef>
#include <iterator>
#include <list>
#include <stdexcept>
#include <unordered_map>

#include "creat_cach.hpp"


enum class Cach_Mode
{
    Inclusive,
    Exclusive
};

template<typename Value>
struct Access_Result
{
    bool hit;
    Value* found_ell;//будет {} если не нашли и будет на список если нашли
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

        Access_Result<Value> access_inclusive(const Key& key);

        Access_Result<Value> access_exclusive(const Key& key);

        void push_down(std::size_t level, Entry<Key, Value> entry);

        void invalidate_above(std::size_t level, const Key& key);

        const Value& get_long_data(const Key& key)//TODO незнабю насколько нормально то что я возвращаю ссылку
        {
            return big_data -> at(key);
        }

    public:

        Access_Result<Value> acess(const Key& key)
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
        Multi_Level_Cach& operator=(const Multi_Level_Cache&) = delete;
};

template<
    typename Key,
    typename Value,
    Cach_Mode Mode,
    bool Store_Data
>
Multi_Level_Cach<Key, Value, Mode, Store_Data>::Multi_Level_Cach
    (const std::vector<Cache_name_size>& parameters, const Big_Data& data): 
    general_cach(create_cach<Key>(parameters)), big_data(&data)
{
}

template<
    typename Key,
    typename Value,
    Cach_Mode Mode,
    bool Store_Data
>
Access_Result<Value> Multi_Level_Cach<Key, Value, Mode, Store_Data>::access_inclusive(const Key& key)
{
    //сначал проверим а вообще есть ои эллемент в кэше
    std::size_t level_cach = 0;
    while(level_cach != general_cach.size())
    {
        Access_Result<Value> result = std::visit([&](auto& cache) -> Access_Result<Value>
                                {return cache.look_up(key);},
                                *general_cach.at(level_cach));
    }
}


#endif