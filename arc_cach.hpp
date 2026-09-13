#ifndef ARC
#define ARC

#include <iostream>
#include <vector>
#include <string>
#include <list>
#include <unordered_map>
#include <iterator>
#include <cstddef>

template <typename Key>//тип значения

class Arc_cach
{
    private:
        using Key_List = std::list<Key>;
        using Iterator = typename Key_List::iterator;

        enum class Type_list_save// в каком списке храниться эллемент
        {
            T1,
            T2,
            B1,
            B2
        };

        struct Node_hash
        {
            Type_list_save type_list;
            Iterator position;
        };

        using Directory = std::unordered_map<Key, Entry>;
        using Directory_Iterator = typename Directory::iterator;
        
        std::size_t capacity_;
        std::size_t size_;

        Key_List T1;
        Key_List T2;
        Key_List B1;
        Key_List B2;

        Directory general_hash_table;

    public:
}

#endif