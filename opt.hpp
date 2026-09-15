#ifndef OPT_CACH
#define OPT_CACH

#include <iostream>
#include <vector>
#include <string>
#include <list>
#include <unordered_map>
#include <iterator>
#include <cstddef>
#include <algorithm>
#include <cassert>


template <typename Key>
class Opt_cach
{
    private:
        using List_Key = std::list<Key>;
        using Iterator = typename Key_List::iterator;

        std::size_t capacity_;
        std::size_t size_;

        using Directory = std::unordered_map<Key, Iterator>;
        using Directory_Iterator = typename Directory::iterator;

        const std::vector<Key>& full_data;
        Directory hash_table;
        List_Key list_cach;


        Iterator choose_who_delete();
        void insert_new(const Key& key);
    
    public:
        explicit Opt_cach();
        explicit Opt_cach(std::size_t capacity, const std::vector<Key>& full_data);

        Opt_cach(const Opt_cach&) = delete;
        Opt_cach& operator=(const Opt_cach&) = delete;

        bool access(const Key& key);//функция для обединения всего в одну систему

        std::size_t size() const noexcept
        {
            return list_cach.size();
        }

        std::size_t capacity() const noexcept
        {
            return capacity_;
        }
};

template <typename Key>
bool Opt_cach<Key>::access(const Key& key)
{
    if(capacity_ == false)
    {
        return false;
    }

    auto found = general_hash_table.find(key);

    if(found == general_hash_table.end())
    {
        insert_new(key);
        return false;
    }

    return true;
}

template <typename Key>
void Opt_cach<Key>::insert_new(const Key& key)
{
    if(size() != capacity_)
    {
        list_cach.push_front(key);
        hash_table.emplace(key, list_cach.begin());
        return;
    }

}

#endif