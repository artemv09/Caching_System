#ifndef CREAT_CACH_
#define CREAT_CACH_

#include <iostream>
#include <istream>
#include <limits>
#include <type_traits>
#include <cstdint>
#include <utility>
#include <stdexcept>
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
#include <string>

#include "lru_cach.hpp"
#include "lfu_cach.hpp"
#include "2Q_cach.hpp"
#include "arc_cach.hpp"
#include "lirs_cach.hpp"

#include "cach_type.hpp"


// Пределы ручного ввода и benchmark; вместимость шаблонных кешей ими не ограничена.
inline constexpr std::size_t MAX_REQUESTS = 1000000;
inline constexpr int MAX_PAGE_KEY = 1000000;

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

template <typename Key, typename Value>
using Cache_variant = std::variant<
    Lru_cach<Key, Value>,
    Lfu_cach<Key, Value>,
    Two_Q_Cach<Key, Value>,
    Arc_cach<Key, Value>,
    Lirs_cach<Key, Value>
>;

// Читает неотрицательное целое; проверяет весь токен и предел до size_t.
std::size_t read_size(std::istream& input, const std::string& label, std::size_t limit);

// Из config читает число уровней и имена, из input — capacity и HIR для LIRS.
std::vector<Cache_name_size> parsing_cach_parametr(std::istream& config, std::istream& input);

// Читает N ключей выбранного типа, не ожидая EOF после последнего.
// Для целочисленных Key действует диапазон 1..max_key; остальные читаются через >>.
template <typename Key>
std::vector<Key> read_requests(std::istream& input, int max_key = 10)
{
    // if constexpr выбирает ветку при компиляции: строкам числовой предел не нужен.
    if constexpr(std::is_integral_v<Key>)
    {
        if(max_key < 1 || max_key > MAX_PAGE_KEY)
        {
            throw std::invalid_argument("Недопустимый предел ключей");
        }
    }

    const auto count = read_size(input, "количество запросов", MAX_REQUESTS);

    std::vector<Key> keys;
    keys.reserve(count);

    for(std::size_t i = 0; i < count; i++)
    {
        Key key{};
        if constexpr(std::is_integral_v<Key>)
        {
            const auto number = read_size(input, "ключ", static_cast<std::size_t>(max_key));
            if(number == 0)
            {
                throw std::invalid_argument("Ключи начинаются с 1");
            }
            // До приведения проверяем тип Key, чтобы, например, 256 не превратилось в uint8_t(0).
            if(number > static_cast<std::uintmax_t>(std::numeric_limits<Key>::max()))
            {
                throw std::invalid_argument("Ключ не помещается в выбранный тип");
            }
            key = static_cast<Key>(number);
        }
        else if(!(input >> key))
        {
            throw std::invalid_argument("Не прочитан ключ");
        }
        // Переносим прочитанный ключ в вектор; строка не требует лишней копии.
        keys.push_back(std::move(key));
    }
    return keys;
}

Type_Cach cache_type(const std::string& name);

template <typename Key, typename Value>
using Cach_ptr = std::unique_ptr<Cache_variant<Key, Value>>;

template <typename Key, typename Value>
Cach_ptr<Key, Value> create_cache_one_ell(const Cache_name_size& parameter_ell) // создает один уровень кэша
{
    using Variant = Cache_variant<Key, Value>;

    switch (cache_type(parameter_ell.name_cach))
    {
        case (Type_Cach::LRU):
            return std::make_unique<Variant>(std::in_place_type<Lru_cach<Key, Value>>, parameter_ell.capacity);

        case (Type_Cach::LFU):
            return std::make_unique<Variant>(std::in_place_type<Lfu_cach<Key, Value>>, parameter_ell.capacity);

        case (Type_Cach::TWO_Q):
            return std::make_unique<Variant>(std::in_place_type<Two_Q_Cach<Key, Value>>, parameter_ell.capacity);

        case (Type_Cach::ARC):
            return std::make_unique<Variant>(std::in_place_type<Arc_cach<Key, Value>>, parameter_ell.capacity);

        case (Type_Cach::LIRS):
            return std::make_unique<Variant>(std::in_place_type<Lirs_cach<Key, Value>>, parameter_ell.capacity, parameter_ell.hir_capacity);
    }
    throw std::runtime_error("Unknown cache type");
}

template <typename Key, typename Value>
std::vector<Cach_ptr<Key, Value>> create_cach(const std::vector<Cache_name_size>& cach_list_name_size) // создает сам кэш полностью
{
    std::vector<Cach_ptr<Key, Value>> general_cach;
    general_cach.reserve(cach_list_name_size.size());

    for(const auto& cach_name_size : cach_list_name_size)
    {
        general_cach.push_back(create_cache_one_ell<Key, Value>(cach_name_size));
    }

    return general_cach;
}


#endif 