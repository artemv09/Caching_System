#ifndef RETURN_TYPE_
#define RETURN_TYPE_

#include <iostream>
#include <optional>

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

// template <typename Key, typename Value> // тип для 
// struct Erase_ELL
// {
//     std::optional<Entry<Key, Value>> entry_erase;
// };

template<typename Key, typename Value>
using Erase_ELL = std::optional<Entry<Key, Value>>;

#endif
