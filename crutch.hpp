#ifndef C
#define C

#include <iostream>
#include <optional>

template<typename Value>
struct Access_Result
{
    bool hit;
    Value* found_ell; //будет {} если не нашли и будет на список если нашли
};

template<typename Value>
struct Public_Access_Result
{
    bool hit;
    Value  sought_element; //будет {} если не нашли и будет на список если нашли
};

template<typename Key, typename Value>
struct Entry
{
    Key key;
    Value value;
};

template <typename Key, typename Value>
struct Erase_ELL
{
    std::optional<Key> key_erase;
    std::optional<Value> value_erase;
};

#endif
