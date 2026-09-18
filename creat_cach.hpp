#ifndef CREAT_CACH
#define CREAT_CACH

#include <iostream>
#include <vector>
#include <string>
#include <list>
#include <unordered_map>
#include <iterator>
#include <cstddef>
#include <algorithm>
#include <cassert>
#include <variant>
#include <memory>

#include "lru_cach.hpp"
#include "lfu_cach.hpp"
#include "2Q_cach.hpp"
#include "arc_cach.hpp"
#include "lirs_cach.hpp"

struct Cache_name_size
{
    std::string name_cach;
    std::size_t capacity;
    std::size_t hir_capacity;

};

enum class Type_Cach
{
    LRU,
    LFU,
    TWO_Q,
    ARC,
    LIRS
};

template <typename Key>
using Cache_variant = std::variant
<
    Lru_cach<Key>,
    Lfu_cach<Key>,
    Two_Q_Cach<Key>,
    Arc_cach<Key>,
    Lirs_cach<Key>
>;

std::vector<Cache_name_size> parsing_cach_parametr(FILE* config, std::istream& input);

Type_Cach cache_type(const std::string& name);

template <typename Key>
using Cach_ptr = std::unique_ptr<Cache_variant<Key>>;

template <typename Key>
Cach_ptr<Key> create_cache_one_ell(const Cache_name_size& parameter_ell)
{
    using Variant = Cache_variant<Key>;

    switch (cache_type(parameter_ell.name_cach))
    {
        case (Type_Cach::LRU):
            return std::make_unique<Variant>(std::in_place_type<Lru_cach<Key>>, parameter_ell.capacity);

        case (Type_Cach::LFU):
            return std::make_unique<Variant>(std::in_place_type<Lfu_cach<Key>>, parameter_ell.capacity);

        case (Type_Cach::TWO_Q):
            return std::make_unique<Variant>(std::in_place_type<Two_Q_Cach<Key>>, parameter_ell.capacity);

        case (Type_Cach::ARC):
            return std::make_unique<Variant>(std::in_place_type<Arc_cach<Key>>, parameter_ell.capacity);

        case (Type_Cach::LIRS):
            return std::make_unique<Variant>(std::in_place_type<Lirs_cach<Key>>, parameter_ell.capacity, parameter_ell.hir_capacity);
    }
    throw std::runtime_error("Unknown cache type");
}

template <typename Key>
std::vector<Cach_ptr<Key>> create_cach(const std::vector<Cache_name_size>& cach_list_name_size)
{
    std::vector<Cach_ptr<Key>> general_cach;
    general_cach.reserve(cach_list_name_size.size());

    for(const auto& cach_name_size : cach_list_name_size)
    {
        general_cach.push_back(create_cache_one_ell<Key>(cach_name_size));
    }

    return general_cach;
}



#endif 