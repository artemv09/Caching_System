#ifndef RETURN_TYPE_
#define RETURN_TYPE_

#include <iostream>
#include <optional>
#include <vector>
#include <string>
#include <utility>
#include <cstddef>

template<typename Value> // тип для возврата из главной функции
struct Public_Access_Result
{
    bool hit;
    Value  sought_element; 
};

template<typename Key, typename Value> // тип который храниться в кэше
struct Entry
{
    Key key;
    Value value;
};

template<typename Key, typename Value>
using Erase_ELL = std::optional<Entry<Key, Value>>;

// Копия состояния для тестов: первый ключ каждой очереди — её начало/MRU.
template<typename Key, typename Value>
struct Cache_Snapshot
{
    std::vector<Entry<Key, Value>> resident;
    std::vector<std::pair<std::string, std::vector<Key>>> queues;
    std::size_t target = 0;
    std::size_t lir_count = 0;
    std::size_t hir_count = 0;
    bool consistent = true;
};

#endif
